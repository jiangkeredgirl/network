/*********************************************************************
 * \file   SerialPortImpl.h
 * \brief  工业级串口类.
 * \author 蒋珂
 * \date   2026.03.04
 *********************************************************************/
#pragma once
#include "serialport.h"
#include "serialerror.h"
#include <asio.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <memory>
#include <thread>
#include <queue>
#include <optional>
#include <algorithm>




//
//#pragma once
#include "serialport.h"
#include <asio.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <map>
#include <chrono>
#include <functional>
#include <memory>
#include <vector>
#include <algorithm>
#include <cctype>
#include <atomic>

//namespace asio {
//    class io_context;
//    class steady_timer;
//    class serial_port;
//}

namespace serial {

    // ============ 协议配置 ============
    struct ProtocolConfig {
        bool enable_crc_check = false;
        bool enable_func_code_match = false;
        bool strict_response_match = false;

        uint8_t slave_address = 1;
        int crc_start_offset = 0;
        int crc_length = -1;

        std::string frame_header;
        size_t frame_header_len = 0;
        size_t min_frame_size = 5;

        bool enable_auto_reconnect = true;
        int max_reconnect_attempts = 5;
        std::chrono::milliseconds reconnect_interval{ 2000 };
        bool exponential_backoff = true;

        bool enable_timeout_retry = true;
        int max_retry_count = 3;
        std::chrono::milliseconds retry_interval{ 500 };
        bool retry_on_crc_error = true;
        bool retry_on_func_mismatch = true;

        std::chrono::milliseconds read_timeout{ 5000 };
        std::chrono::milliseconds write_timeout{ 2000 };
    };

    // ============ CRC 校验器 ============
    inline uint16_t crc16_modbus(const std::vector<char>& data, size_t start = 0, size_t len = 0) {
        if (len == 0) len = data.size() - start;
        uint16_t crc = 0xFFFF;
        for (size_t i = start; i < start + len; ++i) {
            crc ^= static_cast<uint8_t>(data[i]);
            for (int j = 0; j < 8; ++j) {
                if (crc & 1) {
                    crc = (crc >> 1) ^ 0xA001;
                }
                else {
                    crc >>= 1;
                }
            }
        }
        return crc;
    }

    class CRCVerifier {
        ProtocolConfig config_;
    public:
        explicit CRCVerifier(const ProtocolConfig& cfg) : config_(cfg) {}
        bool VerifyCRC(const std::vector<char>& frame) const {
            if (!config_.enable_crc_check || frame.size() < 2) return true;
            size_t crc_start = config_.crc_start_offset >= 0 ?
                static_cast<size_t>(config_.crc_start_offset) : 0;
            size_t crc_end = frame.size() - 2;
            uint16_t expected = (static_cast<uint8_t>(frame[frame.size() - 2]) |
                (static_cast<uint8_t>(frame[frame.size() - 1]) << 8));
            uint16_t calculated = crc16_modbus(frame, crc_start, crc_end - crc_start);
            return calculated == expected;
        }
    };

    // ============ 功能码匹配器 ============
    class FuncCodeMatcher {
        ProtocolConfig config_;
    public:
        explicit FuncCodeMatcher(const ProtocolConfig& cfg) : config_(cfg) {}

        bool MatchRequestResponse(const std::vector<char>& req, const std::vector<char>& resp) const {
            if (req.size() < 2 || resp.size() < 2) return false;
            if (resp[0] != req[0]) return false;
            uint8_t rfunc = static_cast<uint8_t>(req[1]);
            uint8_t rfunc_resp = static_cast<uint8_t>(resp[1]);
            return (rfunc_resp == rfunc) || ((rfunc_resp & 0x7F) == rfunc && (rfunc_resp & 0x80));
        }

        bool IsValidResponse(const std::vector<char>& frame) const {
            if (frame.size() < config_.min_frame_size) return false;
            if (!config_.frame_header.empty()) {
                if (frame.size() < config_.frame_header_len) return false;
                for (size_t i = 0; i < config_.frame_header_len; ++i) {
                    if (static_cast<unsigned char>(frame[i]) !=
                        static_cast<unsigned char>(config_.frame_header[i])) {
                        return false;
                    }
                }
            }
            return true;
        }
    };

    // ============ 请求重试上下文（线程安全修正版） ============
    struct RequestRetryContext {
        std::shared_ptr<std::vector<char>> request;
        std::shared_ptr<std::vector<char>> response;
        std::promise<int> prom;

        int attempt_count = 0;
        int max_attempts;
        std::chrono::milliseconds retry_interval;

        // ✅ 线程安全标记：使用 atomic<bool>
        std::atomic<bool> expired{ false };  // 初始化为 false

        std::shared_ptr<asio::steady_timer> timeout_timer;
        std::shared_ptr<asio::steady_timer> retry_timer;

        std::function<void(int, const std::vector<char>&)> on_retry;

        RequestRetryContext(
            const std::vector<char>& req,
            int max_attempts_,
            std::chrono::milliseconds interval,
            std::function<void(int, const std::vector<char>&)> retry_cb = nullptr)
            : request(std::make_shared<std::vector<char>>(req)),
            response(std::make_shared<std::vector<char>>()),
            max_attempts(max_attempts_),
            retry_interval(interval),
            on_retry(retry_cb) {}

        // 安全检查
        bool IsExpired() const { return expired.load(std::memory_order_acquire); }
        void MarkExpired() { expired.store(true, std::memory_order_release); }
    };

    // ============ 主实现类 ============
    class AsioSerialPortImpl : public ISerialPort {
    private:
        asio::io_context io_ctx_;
        asio::serial_port serial_;
        std::thread io_thread_;
        bool running_ = false;
        bool connected_ = false;
        bool reconnecting_ = false;
        int reconnect_attempt_ = 0;

        ProtocolConfig protocol_config_;
        CRCVerifier crc_verifier_;
        FuncCodeMatcher func_matcher_;

        ISerialPortHandler* handler_ = nullptr;
        ISerialPortHandlerFunction handler_fun_;
        std::mutex handler_mtx_;

        std::queue<std::shared_ptr<RequestRetryContext>> pending_requests_;
        std::mutex req_mtx_;
        std::map<int, std::weak_ptr<RequestRetryContext>> active_requests_;
        int current_request_id_ = 0;

        struct SerialConfig {
            std::string port_name;
            int baud_rate = 115200;
        } config_;

        std::shared_ptr<spdlog::logger> logger_;

        template<typename... Args>
        void log(int level, const std::string& fmt, Args&&... args) {
            if (logger_) {
                logger_->log(static_cast<spdlog::level::level_enum>(level), fmt, std::forward<Args>(args)...);
            }
        }

        void invoke_on_connect(std::error_code ec) {
            std::lock_guard<std::mutex> lock(handler_mtx_);
            if (handler_) handler_->OnConnect(ec);
            else if (handler_fun_.onconnectfun) handler_fun_.onconnectfun(ec);
        }

        void invoke_on_disconnect(std::error_code ec) {
            std::lock_guard<std::mutex> lock(handler_mtx_);
            if (handler_) handler_->OnDisconnect(ec);
            else if (handler_fun_.ondisconnectfun) handler_fun_.ondisconnectfun(ec);
        }

        void invoke_on_read(const std::vector<char>& data, std::error_code ec) {
            std::lock_guard<std::mutex> lock(handler_mtx_);
            if (handler_) handler_->OnRead(data, ec);
            else if (handler_fun_.onreadfun) handler_fun_.onreadfun(data, ec);
        }

        void invoke_on_write(const std::vector<char>& data, std::error_code ec) {
            std::lock_guard<std::mutex> lock(handler_mtx_);
            if (handler_) handler_->OnWrite(data, ec);
            else if (handler_fun_.onwritefun) handler_fun_.onwritefun(data, ec);
        }

        void invoke_on_error(std::error_code ec) {
            std::lock_guard<std::mutex> lock(handler_mtx_);
            if (handler_) handler_->OnError(ec);
            else if (handler_fun_.onerrorfun) handler_fun_.onerrorfun(ec);
        }

        void start_io_context() {
            if (!running_) {
                running_ = true;
                io_thread_ = std::thread([this]() {
                    //asio::executor_work_guard<asio::io_context::executor_type> guard(io_ctx_.get_executor());
                    asio::executor_work_guard<asio::io_context::executor_type> guard(io_ctx_.get_executor());
                    io_ctx_.run();
                    });
            }
        }

        void stop_io_context() {
            if (running_) {
                io_ctx_.stop();
                if (io_thread_.joinable()) {
                    io_thread_.join();
                }
                running_ = false;
            }
        }

        void start_async_read();
        void start_reconnect();

        int write_single_attempt(const std::vector<char>& data, std::vector<char>& response_data, int timeout_ms);

        friend class ISerialPort;

    public:
        AsioSerialPortImpl()
            : serial_(io_ctx_),
            logger_(spdlog::get("serial")),
            crc_verifier_(protocol_config_),
            func_matcher_(protocol_config_) {
            if (!logger_) {
                logger_ = spdlog::stdout_logger_mt("serial");
            }
        }

        ~AsioSerialPortImpl() {
            Disconnect();
            stop_io_context();
        }

        int RegisterHandler(ISerialPortHandlerFunction handler_fun) override {
            std::lock_guard<std::mutex> lock(handler_mtx_);
            handler_fun_ = handler_fun;
            return 0;
        }

        int RegisterHandler(ISerialPortHandler* serialport_handler) override {
            std::lock_guard<std::mutex> lock(handler_mtx_);
            handler_ = serialport_handler;
            return 0;
        }

        int Connect(const std::string& portname, int baudrate = 115200) override {
            std::lock_guard<std::mutex> lock(req_mtx_);
            if (connected_) return static_cast<int>(SerialError::SUCCESS);

            config_.port_name = portname;
            config_.baud_rate = baudrate;

            try {
                if (serial_.is_open()) serial_.close();
                serial_.open(portname);
                serial_.set_option(asio::serial_port_base::baud_rate(baudrate));
                serial_.set_option(asio::serial_port_base::character_size(8));
                serial_.set_option(asio::serial_port_base::parity(asio::serial_port_base::parity::none));
                serial_.set_option(asio::serial_port_base::flow_control(asio::serial_port_base::flow_control::none));
                serial_.set_option(asio::serial_port_base::stop_bits(asio::serial_port_base::stop_bits::one));

                connected_ = true;
                reconnect_attempt_ = 0;
                reconnecting_ = false;
                start_async_read();
                invoke_on_connect({});
                return static_cast<int>(SerialError::SUCCESS);
            }
            catch (const std::exception& e) {
                log(4, "Connect failed: {}", e.what());
                connected_ = false;
                invoke_on_connect(asio::error::connection_refused);
                return static_cast<int>(SerialError::ERROR_CONNECT_FAILED);
            }
        }

        int AsyncConnect(const std::string& portname, int baudrate = 115200) override {
            return Connect(portname, baudrate);
        }

        int Disconnect() override {
            std::lock_guard<std::mutex> lock(req_mtx_);
            if (!connected_ && !reconnecting_) return static_cast<int>(SerialError::SUCCESS);

            try {
                // ✅ 清理所有待处理请求（关键！）
                while (!pending_requests_.empty()) {
                    auto ctx = pending_requests_.front();
                    ctx->MarkExpired();  // ✅ 安全标记过期
                    ctx->prom.set_value(static_cast<int>(SerialError::ERROR_SERIAL_CLOSED));
                    pending_requests_.pop();
                }

                if (serial_.is_open()) serial_.close();
                connected_ = false;
                reconnecting_ = false;
                invoke_on_disconnect({});
                return static_cast<int>(SerialError::SUCCESS);
            }
            catch (...) {
                return static_cast<int>(SerialError::ERROR_DISCONNECT_FAILED);
            }
        }

        int Write(const std::vector<char>& data) override {
            if (!connected_) return static_cast<int>(SerialError::ERROR_SERIAL_CLOSED);
            try {
                asio::write(serial_, asio::buffer(data));
                invoke_on_write(data, {});
                return static_cast<int>(SerialError::SUCCESS);
            }
            catch (const std::exception& e) {
                log(4, "Write failed: {}", e.what());
                return static_cast<int>(SerialError::ERROR_WRITE_FAILED);
            }
        }

        int Write(const std::vector<char>& data, std::vector<char>& response_data, int timeout_ms) override {
            if (!connected_) return static_cast<int>(SerialError::ERROR_SERIAL_CLOSED);

            auto cfg = protocol_config_;
            if (!cfg.enable_timeout_retry) {
                return write_single_attempt(data, response_data, timeout_ms);
            }

            auto ctx = std::make_shared<RequestRetryContext>(
                data, cfg.max_retry_count, cfg.retry_interval,
                [this](int attempt, const std::vector<char>& req) {
                    log(2, "Retry #{} for request", attempt);
                    invoke_on_write(req, {});
                });

            int req_id = ++current_request_id_;
            {
                std::lock_guard<std::mutex> lock(req_mtx_);
                active_requests_[req_id] = ctx;
                pending_requests_.push(ctx);
            }

            int send_res = Write(data);
            if (send_res != static_cast<int>(SerialError::SUCCESS)) {
                ctx->MarkExpired();  // ✅ 发送失败即标记过期
                ctx->prom.set_value(send_res);
                return send_res;
            }

            ctx->timeout_timer = std::make_shared<asio::steady_timer>(io_ctx_);
            ctx->timeout_timer->expires_after(std::chrono::milliseconds(timeout_ms));
            ctx->timeout_timer->async_wait([this, ctx, req_id](const std::error_code& ec) {
                if (ec == asio::error::operation_aborted) return;

                std::lock_guard<std::mutex> lock(req_mtx_);
                auto it = active_requests_.find(req_id);
                if (it == active_requests_.end() || it->second.expired()) return;

                if (ctx->attempt_count < ctx->max_attempts) {
                    ctx->attempt_count++;
                    log(3, "Timeout on attempt #{}, retrying...", ctx->attempt_count);

                    if (ctx->on_retry) ctx->on_retry(ctx->attempt_count, *ctx->request);

                    asio::post(io_ctx_, [this, ctx]() {
                        Write(*ctx->request);
                        });

                    ctx->timeout_timer->expires_after(ctx->retry_interval);
                    ctx->timeout_timer->async_wait([this, ctx, req_id](const std::error_code& ec) {
                        if (ec == asio::error::operation_aborted) return;
                        std::lock_guard<std::mutex> lock(req_mtx_);
                        auto it = active_requests_.find(req_id);
                        if (it != active_requests_.end() && !it->second.expired()) {
                            ctx->MarkExpired();  // ✅ 超时终止前标记
                            ctx->prom.set_value(static_cast<int>(SerialError::ERROR_TIME_OUT));
                        }
                        });
                }
                else {
                    log(3, "Max retries exceeded for request");
                    ctx->MarkExpired();  // ✅ 终止请求
                    ctx->prom.set_value(static_cast<int>(SerialError::ERROR_TIME_OUT));
                }
                });

            auto fut = ctx->prom.get_future();
            auto status = fut.wait_for(std::chrono::milliseconds(timeout_ms));
            if (status == std::future_status::timeout) {
                ctx->MarkExpired();
                return static_cast<int>(SerialError::ERROR_TIME_OUT);
            }

            try {
                int res = fut.get();
                if (res == static_cast<int>(SerialError::SUCCESS)) {
                    response_data = *ctx->response;
                }
                return res;
            }
            catch (...) {
                return static_cast<int>(SerialError::ERROR_UNKNOWN);
            }
        }

        int AsyncWrite(const std::vector<char>& data) override {
            return Write(data);
        }

        bool IsConnected() const override {
            return connected_;
        }

        void SetProtocolConfig(const ProtocolConfig& cfg) {
            std::lock_guard<std::mutex> lock(req_mtx_);
            protocol_config_ = cfg;
            crc_verifier_ = CRCVerifier(cfg);
            func_matcher_ = FuncCodeMatcher(cfg);
        }

        const ProtocolConfig& GetProtocolConfig() {
            std::lock_guard<std::mutex> lock(req_mtx_);
            return protocol_config_;
        }
    };

    void AsioSerialPortImpl::start_async_read() {
        if (!connected_) return;

        auto temp_buf = std::make_shared<std::array<char, 1024>>();
        serial_.async_read_some(
            asio::buffer(*temp_buf),
            [this, temp_buf](const std::error_code& ec, std::size_t n) {
				if (ec) { /* 处理错误 */ return; }

				std::vector<char> packet(temp_buf->begin(), temp_buf->begin() + n);
				// 处理 packet...
				invoke_on_read(packet, {});
				// 注意：剩余数据需缓存（需额外 ring buffer）
				start_async_read();
            });
    }

    void AsioSerialPortImpl::start_reconnect() {
        if (reconnecting_ || !protocol_config_.enable_auto_reconnect) {
            return;
        }

        // 防止递归调用
        if (reconnect_attempt_ > 0 && reconnecting_) {
            return;
        }

        reconnecting_ = true;
        log(3, "Starting reconnect (attempt #{})", ++reconnect_attempt_);

        // 检查是否超限
        if (protocol_config_.max_reconnect_attempts > 0 &&
            reconnect_attempt_ > protocol_config_.max_reconnect_attempts) {
            log(4, "Max reconnect attempts ({}) exceeded", protocol_config_.max_reconnect_attempts);
            reconnecting_ = false;
            invoke_on_error(asio::error::operation_aborted);
            return;
        }

        // ✅ 修正：用整数位移计算指数退避（避免 std::pow + duration 乘法错误）
        std::chrono::milliseconds base_delay = protocol_config_.reconnect_interval;
        long long ms = base_delay.count();

        // exp = max(0, reconnect_attempt_ - 1) → 2^(exp)
        int exp = std::max(0, reconnect_attempt_ - 1);
        if (exp > 60) { // 防溢出（2^60 ≈ 1e18 ms > 30000s，已足够大）
            ms = 30000;
        }
        else {
            ms *= (1LL << exp);  // 整数左移，精确无误差
        }
        ms = std::min(ms, 30000LL); // capped at 30s
        auto delay = std::chrono::milliseconds(ms);

        log(3, "Reconnect delay: {} ms", ms);

        // ✅ 安全：在 io_context 中 post 启动 timer，避免跨线程构造
        asio::post(io_ctx_, [this, delay]() {
            auto timer = std::make_shared<asio::steady_timer>(io_ctx_);
            timer->expires_after(delay);
            timer->async_wait([this, timer](const std::error_code& ec) {
                if (ec == asio::error::operation_aborted) {
                    return;
                }

                int ret = Connect(config_.port_name, config_.baud_rate);
                if (ret != static_cast<int>(SerialError::SUCCESS)) {
                    // ❗ 注意：此处不能直接调用 start_reconnect()（会栈溢出！）
                    // 改为 post 回 io_context 继续重试
                    asio::post(io_ctx_, [this]() {
                        start_reconnect();  // 递归需在 io_context 中安全调用
                        });
                }
                else {
                    reconnecting_ = false;
                    reconnect_attempt_ = 0;
                    log(2, "Reconnect successful");
                }
                });
            });
    }

    int AsioSerialPortImpl::write_single_attempt(
        const std::vector<char>& data,
        std::vector<char>& response_data,
        int timeout_ms) {

        auto ctx = std::make_shared<RequestRetryContext>(
            data,
            protocol_config_.max_retry_count,
            protocol_config_.retry_interval,
            [this](int attempt, const std::vector<char>& req) {
                log(2, "Retry #{} for request", attempt);
                invoke_on_write(req, {});
            });

        int req_id = ++current_request_id_;
        {
            std::lock_guard<std::mutex> lock(req_mtx_);
            active_requests_[req_id] = ctx;
            pending_requests_.push(ctx);
        }

        int send_res = Write(data);
        if (send_res != static_cast<int>(SerialError::SUCCESS)) {
            ctx->MarkExpired();
            ctx->prom.set_value(send_res);
            return send_res;
        }

        ctx->timeout_timer = std::make_shared<asio::steady_timer>(io_ctx_);
        ctx->timeout_timer->expires_after(std::chrono::milliseconds(timeout_ms));
        ctx->timeout_timer->async_wait([this, ctx, req_id](const std::error_code& ec) {
            if (ec == asio::error::operation_aborted) return;
            std::lock_guard<std::mutex> lock(req_mtx_);
            auto it = active_requests_.find(req_id);
            if (it != active_requests_.end() && !it->second.expired()) {
                ctx->MarkExpired();
                ctx->prom.set_value(static_cast<int>(SerialError::ERROR_TIME_OUT));
            }
            });

        auto fut = ctx->prom.get_future();
        if (fut.wait_for(std::chrono::milliseconds(timeout_ms)) == std::future_status::timeout) {
            ctx->MarkExpired();
            return static_cast<int>(SerialError::ERROR_TIME_OUT);
        }

        try {
            int res = fut.get();
            if (res == static_cast<int>(SerialError::SUCCESS)) {
                response_data = *ctx->response;
            }
            return res;
        }
        catch (...) {
            return static_cast<int>(SerialError::ERROR_UNKNOWN);
        }
    }

    //LIBSERIALPORT_API ISerialPort* NewSerialPort() {
    //    try {
    //        return new AsioSerialPortImpl();
    //    }
    //    catch (...) {
    //        return nullptr;
    //    }
    //}

    //LIBSERIALPORT_API void DeleteSerialPort(ISerialPort* serial_port) {
    //    delete serial_port;
    //}

} // namespace serial