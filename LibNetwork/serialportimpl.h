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
//////////////////////////////////
#include <asio.hpp>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <queue>
#include <chrono>
#include <algorithm>
#include <cstdint>
#include "serialport.h"

 // 前向声明
namespace serial {
    struct ProtocolConfig;
    class CRCVerifier;
    class FuncCodeMatcher;
}

// =============== 协议配置结构 ===============
namespace serial {

    struct ProtocolConfig {
        // === 通用协议配置 ===
        bool enable_crc_check = true;
        bool enable_func_code_match = true;
        bool strict_response_match = false;

        // === Modbus RTU 特定 ===
        uint8_t slave_address = 1;
        int crc_start_offset = 0;
        int crc_length = -1;

        // === 自定义协议 ===
        std::string frame_header;
        size_t frame_header_len = 0;
        size_t min_frame_size = 5;

        // === 自动重连配置 ===
        bool enable_auto_reconnect = true;
        int max_reconnect_attempts = 5;
        std::chrono::milliseconds reconnect_interval{ 2000 };
        bool exponential_backoff = true;

        // === 超时重发配置 ===
        bool enable_timeout_retry = true;
        int max_retry_count = 3;
        std::chrono::milliseconds retry_interval{ 500 };
        bool retry_on_crc_error = true;
        bool retry_on_func_mismatch = true;

        // === 超时设置 ===
        std::chrono::milliseconds read_timeout{ 5000 };
        std::chrono::milliseconds write_timeout{ 2000 };
    };

    // =============== CRC 校验器 ===============
    class CRCVerifier {
    public:
        explicit CRCVerifier(const ProtocolConfig& cfg) : config_(cfg) {}

        bool VerifyCRC(const std::vector<char>& frame) const {
            if (!config_.enable_crc_check || frame.size() < 2) return true;

            size_t crc_start = static_cast<size_t>(config_.crc_start_offset);
            size_t crc_end = frame.size() - 2;
            if (config_.crc_length > 0) {
                crc_end = crc_start + static_cast<size_t>(config_.crc_length);
            }

            uint16_t expected = (static_cast<uint8_t>(frame[frame.size() - 2]) |
                (static_cast<uint8_t>(frame[frame.size() - 1]) << 8));
            uint16_t calculated = crc16_modbus(frame, crc_start, crc_end - crc_start);
            return calculated == expected;
        }

    private:
        static uint16_t crc16_modbus(const std::vector<char>& data, size_t start, size_t len) {
            uint16_t crc = 0xFFFF;
            for (size_t i = start; i < start + len; ++i) {
                crc ^= static_cast<uint8_t>(data[i]);
                for (int j = 0; j < 8; ++j) {
                    if (crc & 1) crc = (crc >> 1) ^ 0xA001;
                    else crc >>= 1;
                }
            }
            return crc;
        }

        ProtocolConfig config_;
    };

    // =============== 功能码匹配器 ===============
    class FuncCodeMatcher {
    public:
        explicit FuncCodeMatcher(const ProtocolConfig& cfg) : config_(cfg) {}

        bool MatchRequestResponse(const std::vector<char>& req, const std::vector<char>& resp) const {
            if (req.size() < 2 || resp.size() < 2) return false;

            // 地址匹配
            if (resp[0] != req[0]) return false;

            // 功能码匹配
            if (!config_.enable_func_code_match) return true;

            uint8_t r = static_cast<uint8_t>(req[1]);
            uint8_t s = static_cast<uint8_t>(resp[1]);

            return (s == r) || ((s & 0x7F) == r && (s & 0x80));
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

    private:
        ProtocolConfig config_;
    };

} // namespace serial

// =============== 请求重试上下文 ===============
struct RequestRetryContext {
    std::shared_ptr<std::vector<char>> request;
    std::shared_ptr<std::vector<char>> response;
    std::promise<int> prom;

    int attempt_count = 0;
    int max_attempts;
    std::chrono::milliseconds retry_interval;

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
};

// =============== 主实现类 ===============
class AsioSerialPortImpl : public ISerialPort {
public:
    AsioSerialPortImpl()
        : io_ctx_(),
        serial_(io_ctx_),
        logger_(spdlog::get("serial")),
        protocol_config_(),
        crc_verifier_(protocol_config_),
        func_matcher_(protocol_config_),
        connected_(false),
        reconnecting_(false),
        reconnect_attempt_(0),
        current_request_id_(0) {
        // 设置默认配置
        protocol_config_.enable_crc_check = true;
        protocol_config_.enable_func_code_match = true;
        protocol_config_.enable_auto_reconnect = true;
        protocol_config_.enable_timeout_retry = true;
        protocol_config_.max_retry_count = 3;
        protocol_config_.retry_interval = std::chrono::milliseconds(500);
        protocol_config_.max_reconnect_attempts = 5;
        protocol_config_.reconnect_interval = std::chrono::milliseconds(2000);
        protocol_config_.exponential_backoff = true;
        protocol_config_.read_timeout = std::chrono::milliseconds(5000);
        protocol_config_.write_timeout = std::chrono::milliseconds(2000);

        crc_verifier_ = serial::CRCVerifier(protocol_config_);
        func_matcher_ = serial::FuncCodeMatcher(protocol_config_);
    }

    ~AsioSerialPortImpl() override {
        Disconnect();
    }

    // ISerialPort 接口实现
    int RegisterHandler(ISerialPortHandlerFunction handler_fun) override {
        std::lock_guard<std::mutex> lock(mtx_);
        handler_fun_ = handler_fun;
        return 0;
    }

    int RegisterHandler(ISerialPortHandler* handler) override {
        std::lock_guard<std::mutex> lock(mtx_);
        handler_ = handler;
        return 0;
    }

    int Connect(const std::string& portname, int baudrate = 115200) override {
        std::lock_guard<std::mutex> lock(mtx_);
        if (connected_) return static_cast<int>(serial::SerialError::SUCCESS);

        try {
            serial_.open(portname);
            serial_.set_option(asio::serial_port_base::baud_rate(baudrate));
            serial_.set_option(asio::serial_port_base::character_size(8));
            serial_.set_option(asio::serial_port_base::flow_control(asio::serial_port_base::flow_control::none));
            serial_.set_option(asio::serial_port_base::parity(asio::serial_port_base::parity::none));
            serial_.set_option(asio::serial_port_base::stop_bits(asio::serial_port_base::stop_bits::one));

            connected_ = true;
            reconnect_attempt_ = 0;
            reconnecting_ = false;

            log(2, "Connected to {}", portname);

            // 启动读取循环
            start_async_read();

            // 通知连接成功
            invoke_on_connect(asio::error_code());

            return static_cast<int>(serial::SerialError::SUCCESS);
        }
        catch (const std::exception& e) {
            log(4, "Connect failed: {}", e.what());
            connected_ = false;
            invoke_on_connect(asio::error::make_error_code(asio::error::connection_refused));
            return static_cast<int>(serial::SerialError::ERROR_CONNECT_FAILED);
        }
    }

    int AsyncConnect(const std::string& portname, int baudrate = 115200) override {
        // 实际中可异步处理，此处简化为同步
        return Connect(portname, baudrate);
    }

    int Disconnect() override {
        std::lock_guard<std::mutex> lock(mtx_);
        if (!connected_) return 0;

        try {
            serial_.close();
            connected_ = false;
            reconnecting_ = false;

            log(2, "Disconnected");

            invoke_on_disconnect(asio::error_code());
        }
        catch (...) {
            log(4, "Disconnect exception");
        }
        return 0;
    }

    int Write(const std::vector<char>& data) override {
        std::lock_guard<std::mutex> lock(mtx_);
        if (!connected_) return static_cast<int>(serial::SerialError::ERROR_SERIAL_CLOSED);

        try {
            asio::write(serial_, asio::buffer(data));
            log(3, "Write {} bytes", data.size());
            invoke_on_write(data, asio::error_code());
            return static_cast<int>(serial::SerialError::SUCCESS);
        }
        catch (const std::exception& e) {
            log(4, "Write failed: {}", e.what());
            invoke_on_write(data, asio::error::make_error_code(asio::error::broken_pipe));
            return static_cast<int>(serial::SerialError::ERROR_WRITE_FAILED);
        }
    }

    int Write(const std::vector<char>& data, std::vector<char>& response_data, int timeout_ms) override {
        std::lock_guard<std::mutex> lock(mtx_);
        if (!connected_) {
            return static_cast<int>(serial::SerialError::ERROR_SERIAL_CLOSED);
        }

        auto cfg = protocol_config_; // 复制当前配置（避免锁中长期持有）

        // 如果禁用重试，走单次发送逻辑
        if (!cfg.enable_timeout_retry) {
            return write_single_attempt(data, response_data, timeout_ms);
        }

        // 创建重试上下文
        auto ctx = std::make_shared<RequestRetryContext>(
            data,
            cfg.max_retry_count,
            cfg.retry_interval,
            [this](int attempt, const std::vector<char>& req) {
                log(2, "Retry #{} for request", attempt);
                if (handler_) handler_->OnWrite(req, {});
                else if (handler_fun_.onwritefun) handler_fun_.onwritefun(req, {});
            });

        int req_id = ++current_request_id_;
        {
            std::lock_guard<std::mutex> lock(req_mtx_);
            active_requests_[req_id] = ctx;
        }

        // 首次发送
        int send_res = Write(data);
        if (send_res != static_cast<int>(serial::SerialError::SUCCESS)) {
            ctx->prom.set_value(send_res);
            return send_res;
        }

        // 启动超时定时器
        ctx->timeout_timer = std::make_shared<asio::steady_timer>(io_ctx_);
        ctx->timeout_timer->expires_after(std::chrono::milliseconds(timeout_ms));
        ctx->timeout_timer->async_wait([this, ctx, req_id](const std::error_code& ec) {
            if (ec == asio::error::operation_aborted) return;

            std::lock_guard<std::mutex> lock(req_mtx_);
            auto it = active_requests_.find(req_id);
            if (it == active_requests_.end() || it->second.expired()) return;

            // 超时处理
            if (ctx->attempt_count < ctx->max_attempts) {
                ctx->attempt_count++;
                log(3, "Timeout on attempt #{}, retrying...", ctx->attempt_count);

                if (ctx->on_retry) {
                    ctx->on_retry(ctx->attempt_count, *ctx->request);
                }

                // 重新发送（在 io_context 线程中）
                asio::post(io_ctx_, [this, ctx]() {
                    Write(*ctx->request);
                    });

                // 重启超时定时器（带重试间隔）
                ctx->timeout_timer->expires_after(ctx->retry_interval);
                ctx->timeout_timer->async_wait([this, ctx, req_id](const std::error_code& ec) {
                    if (ec == asio::error::operation_aborted) return;
                    std::lock_guard<std::mutex> lock(req_mtx_);
                    auto it = active_requests_.find(req_id);
                    if (it != active_requests_.end() && !it->second.expired()) {
                        ctx->prom.set_value(static_cast<int>(serial::SerialError::ERROR_TIME_OUT));
                    }
                    });
            }
            else {
                log(3, "Max retries exceeded for request ({} attempts)", ctx->max_attempts);
                ctx->prom.set_value(static_cast<int>(serial::SerialError::ERROR_TIME_OUT));
            }
            });

        // 等待结果（注意：不能在 io_context 线程中调用 get()！）
        auto fut = ctx->prom.get_future();
        auto status = fut.wait_for(std::chrono::milliseconds(timeout_ms));
        if (status == std::future_status::timeout) {
            return static_cast<int>(serial::SerialError::ERROR_TIME_OUT);
        }

        try {
            int res = fut.get();
            if (res == static_cast<int>(serial::SerialError::SUCCESS)) {
                response_data = *ctx->response;
            }
            return res;
        }
        catch (...) {
            return static_cast<int>(serial::SerialError::ERROR_UNKNOWN);
        }
    }

    int AsyncWrite(const std::vector<char>& data) override {
        return Write(data);
    }

    bool IsConnected() const override {
        std::lock_guard<std::mutex> lock(mtx_);
        return connected_;
    }

    // 设置协议配置（运行时开关）
    void SetProtocolConfig(const serial::ProtocolConfig& cfg) {
        std::lock_guard<std::mutex> lock(mtx_);
        protocol_config_ = cfg;
        crc_verifier_ = serial::CRCVerifier(cfg);
        func_matcher_ = serial::FuncCodeMatcher(cfg);
        log(2, "Protocol config updated: CRC={}, FuncCode={}, AutoReconnect={}, TimeoutRetry={}",
            cfg.enable_crc_check, cfg.enable_func_code_match,
            cfg.enable_auto_reconnect, cfg.enable_timeout_retry);
    }

    const serial::ProtocolConfig& GetProtocolConfig() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return protocol_config_;
    }

private:
    // 单次发送逻辑（用于 disable_retry 时）
    int write_single_attempt(const std::vector<char>& data, std::vector<char>& response_data, int timeout_ms) {
        auto ctx = std::make_shared<RequestRetryContext>(
            data, 0, std::chrono::milliseconds(0)); // 不重试

        std::lock_guard<std::mutex> lock(req_mtx_);
        active_requests_[++current_request_id_] = ctx;

        // 发送
        int send_res = Write(data);
        if (send_res != static_cast<int>(serial::SerialError::SUCCESS)) {
            ctx->prom.set_value(send_res);
            return send_res;
        }

        // 启动超时
        ctx->timeout_timer = std::make_shared<asio::steady_timer>(io_ctx_);
        ctx->timeout_timer->expires_after(std::chrono::milliseconds(timeout_ms));
        ctx->timeout_timer->async_wait([this, ctx](const std::error_code& ec) {
            if (ec == asio::error::operation_aborted) return;
            std::lock_guard<std::mutex> lock(req_mtx_);
            ctx->prom.set_value(static_cast<int>(serial::SerialError::ERROR_TIME_OUT));
            });

        auto fut = ctx->prom.get_future();
        auto status = fut.wait_for(std::chrono::milliseconds(timeout_ms));
        if (status == std::future_status::timeout) {
            return static_cast<int>(serial::SerialError::ERROR_TIME_OUT);
        }

        try {
            int res = fut.get();
            if (res == static_cast<int>(serial::SerialError::SUCCESS)) {
                response_data = *ctx->response;
            }
            return res;
        }
        catch (...) {
            return static_cast<int>(serial::SerialError::ERROR_UNKNOWN);
        }
    }

    void start_async_read() {
        if (!connected_) return;

        auto buffer = std::make_shared<std::vector<char>>(1024);
        serial_.async_read_some(
            asio::buffer(*buffer),
            [this, buffer](const std::error_code& ec, std::size_t bytes_transferred) {
                if (ec) {
                    if (ec == asio::error::operation_aborted) return;
                    log(4, "Read error: {}", ec.message());
                    if (protocol_config_.enable_auto_reconnect) {
                        start_reconnect();
                    }
                    invoke_on_error(ec);
                    return;
                }

                if (bytes_transferred == 0) return;

                std::vector<char> data(buffer->begin(), buffer->begin() + bytes_transferred);
                log(3, "Received {} bytes", data.size());

                // 尝试匹配请求-应答
                bool handled_by_request = false;
                {
                    std::lock_guard<std::mutex> lock(req_mtx_);
                    if (!pending_requests_.empty()) {
                        auto ctx = pending_requests_.front();
                        if (!ctx->expired && func_matcher_.IsValidResponse(data) &&
                            func_matcher_.MatchRequestResponse(*ctx->request, data)) {

                            // CRC 校验
                            bool crc_ok = !protocol_config_.enable_crc_check ||
                                crc_verifier_.VerifyCRC(data);

                            if (crc_ok) {
                                *ctx->response = std::move(data);
                                ctx->prom.set_value(static_cast<int>(serial::SerialError::SUCCESS));
                                pending_requests_.pop();
                                handled_by_request = true;
                            }
                            else if (protocol_config_.retry_on_crc_error &&
                                ctx->attempt_count < ctx->max_attempts) {
                                log(3, "CRC error, will retry (attempt {})", ctx->attempt_count + 1);
                                // 不设置 prom，让超时机制触发重试
                            }
                            else {
                                ctx->prom.set_value(static_cast<int>(serial::SerialError::ERROR_READ_FAILED));
                            }
                        }
                        else if (protocol_config_.retry_on_func_mismatch &&
                            ctx->attempt_count < ctx->max_attempts) {
                            log(3, "Func code mismatch, will retry (attempt {})", ctx->attempt_count + 1);
                            // 让超时机制处理
                        }
                    }
                }

                if (!handled_by_request) {
                    invoke_on_read(data, ec);
                }

                // 继续读取
                start_async_read();
            });
    }

    void start_reconnect() {
        if (reconnecting_ || !protocol_config_.enable_auto_reconnect) return;

        reconnecting_ = true;
        log(3, "Starting reconnect (attempt #{})", ++reconnect_attempt_);

        if (protocol_config_.max_reconnect_attempts > 0 &&
            reconnect_attempt_ > protocol_config_.max_reconnect_attempts) {
            log(4, "Max reconnect attempts exceeded");
            reconnecting_ = false;
            invoke_on_error(asio::error::make_error_code(asio::error::connection_refused));
            return;
        }

        auto timer = std::make_shared<asio::steady_timer>(io_ctx_);
        auto delay = protocol_config_.reconnect_interval;
        if (protocol_config_.exponential_backoff) {
            delay = std::min(
                delay * std::pow(2, reconnect_attempt_ - 1),
                std::chrono::milliseconds(30000));
        }

        timer->expires_after(delay);
        timer->async_wait([this, timer](const std::error_code& ec) {
            if (ec) return;
            int ret = Connect(config_.port_name, config_.baud_rate);
            if (ret != static_cast<int>(serial::SerialError::SUCCESS)) {
                start_reconnect();
            }
            else {
                reconnecting_ = false;
                reconnect_attempt_ = 0;
            }
            });
    }

    void invoke_on_connect(const std::error_code& ec) {
        if (handler_) handler_->OnConnect(ec);
        else if (handler_fun_.onconnectfun) handler_fun_.onconnectfun(ec);
    }

    void invoke_on_disconnect(const std::error_code& ec) {
        if (handler_) handler_->OnDisconnect(ec);
        else if (handler_fun_.ondisconnectfun) handler_fun_.ondisconnectfun(ec);
    }

    void invoke_on_read(const std::vector<char>& data, const std::error_code& ec) {
        if (handler_) handler_->OnRead(data, ec);
        else if (handler_fun_.onreadfun) handler_fun_.onreadfun(data, ec);
    }

    void invoke_on_write(const std::vector<char>& data, const std::error_code& ec) {
        if (handler_) handler_->OnWrite(data, ec);
        else if (handler_fun_.onwritefun) handler_fun_.onwritefun(data, ec);
    }

    void invoke_on_error(const std::error_code& ec) {
        if (handler_) handler_->OnError(ec);
        else if (handler_fun_.onerrorfun) handler_fun_.onerrorfun(ec);
    }

    void log(int level, const std::string& fmt, ...) {
        if (!logger_) return;
        va_list args;
        va_start(args, fmt);
        logger_->log(static_cast<spdlog::level::level_enum>(level), fmt, args);
        va_end(args);
    }

    // 成员变量
    asio::io_context io_ctx_;
    asio::serial_port serial_;
    std::shared_ptr<spdlog::logger> logger_;

    mutable std::mutex mtx_;
    ISerialPortHandler* handler_ = nullptr;
    ISerialPortHandlerFunction handler_fun_;

    struct Config {
        std::string port_name;
        int baud_rate = 115200;
    } config_;

    bool connected_;
    bool reconnecting_;
    int reconnect_attempt_;

    serial::ProtocolConfig protocol_config_;
    serial::CRCVerifier crc_verifier_;
    serial::FuncCodeMatcher func_matcher_;

    // 请求队列（用于应答匹配）
    std::queue<std::shared_ptr<RequestRetryContext>> pending_requests_;
    mutable std::mutex req_mtx_;
    std::map<int, std::weak_ptr<RequestRetryContext>> active_requests_;
    int current_request_id_;
};

// =============== 工厂函数 ===============
//inline ISerialPort* NewSerialPort() {
//    return new AsioSerialPortImpl();
//}