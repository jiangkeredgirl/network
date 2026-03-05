/*********************************************************************
 * \file   SerialPortImpl.h
 * \brief  工业级串口类.
 * \author 蒋珂
 * \date   2026.03.04
 *********************************************************************/
#pragma once
#include "serialport.h"
#define ASIO_STANDALONE
#include <asio.hpp>
#include <thread>
#include <mutex>
#include <chrono>
#include <queue>

//using namespace xserial;
using namespace std;
using namespace asio;
using asio::serial_port;

//namespace xserial {

    class SerialPortImpl : public ISerialPort
    {
    public:
        explicit SerialPortImpl()
            : port_(io_ctx_), connected_(false), handler_func_(), handler_ptr_(nullptr)
        {
        }

        ~SerialPortImpl() override
        {
            Disconnect();
        }

        int RegisterHandler(ISerialPortHandlerFunction handler_fun) override
        {
            std::lock_guard<std::mutex> lock(mutex_);
            handler_func_ = handler_fun;
            handler_ptr_ = nullptr;
            return 0;
        }

        int RegisterHandler(ISerialPortHandler* serialport_handler) override
        {
            std::lock_guard<std::mutex> lock(mutex_);
            handler_ptr_ = serialport_handler;
            handler_func_ = {};
            return 0;
        }

        int Connect(const std::string& portname, int baudrate) override
        {
            std::error_code ec;
            port_.open(portname, ec);
            if (ec)
            {
                invokeOnError(ec);
                return -1;
            }

            port_.set_option(serial_port::baud_rate(baudrate), ec);
            if (ec) { port_.close(); invokeOnError(ec); return -1; }

            port_.set_option(serial_port::character_size(8), ec);
            if (ec) { port_.close(); invokeOnError(ec); return -1; }

            port_.set_option(serial_port::parity(serial_port::parity::none), ec);
            if (ec) { port_.close(); invokeOnError(ec); return -1; }

            port_.set_option(serial_port::stop_bits(serial_port::stop_bits::one), ec);
            if (ec) { port_.close(); invokeOnError(ec); return -1; }

            port_.set_option(serial_port::flow_control(serial_port::flow_control::none), ec);
            if (ec) { port_.close(); invokeOnError(ec); return -1; }

            connected_ = true;

            io_ctx_thread_.reset(new std::thread([this]()
                {
                    io_ctx_.run();
                }));

            // 启动异步读取
            startAsyncRead();

            // 调用连接成功回调
            std::error_code ok_ec;
            invokeOnConnect(ok_ec);

            return 0;
        }

        int AsyncConnect(const std::string& portname, int baudrate) override
        {
            // 使用 post 将 connect 操作放入 io_context 线程中执行
            asio::post(io_ctx_, [this, portname, baudrate]() {
                Connect(portname, baudrate);
                });
            return 0;
        }

        int Disconnect() override
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!connected_)
                return 0;

            std::error_code ec;
            port_.cancel(ec);
            port_.close(ec);

            connected_ = false;

            io_ctx_.stop();

            if (io_ctx_thread_ && io_ctx_thread_->joinable())
                io_ctx_thread_->join();

            // 调用断开回调
            std::error_code ok_ec;
            invokeOnDisconnect(ok_ec);

            return 0;
        }

        int Write(const std::vector<char>& data) override
        {
            if (!connected_)
                return -1;

            std::error_code ec;
            size_t written = asio::write(port_, asio::buffer(data), ec);
            if (ec)
            {
                invokeOnError(ec);
                return -1;
            }

            // 调用写入回调
            invokeOnWrite(data, std::error_code());

            return static_cast<int>(written);
        }

        int Write(const std::vector<char>& data, std::vector<char>& response_data, int timeout_ms) override
        {
            if (!connected_)
                return -1;

            std::promise<int> result_promise;
            std::future<int> result_future = result_promise.get_future();

            // 异步写入 + 读取响应（含超时）
            asio::steady_timer timer(io_ctx_, std::chrono::milliseconds(timeout_ms));
            auto expiry_callback = [this, &result_promise](const std::error_code& ec) {
                if (!ec) {
                    // 超时，取消读取
                    port_.cancel();
                    result_promise.set_value(-2); // timeout
                }
                };

            timer.async_wait(expiry_callback);

            auto write_handler = [this, &result_promise, &response_data, timeout_ms](
                const std::error_code& ec, std::size_t bytes_transferred) mutable {
                    if (ec)
                    {
                        result_promise.set_value(-1);
                        return;
                    }

                    // 写成功，开始读
                    std::vector<char> buf(1024);
                    port_.async_read_some(asio::buffer(buf),
                        [this, &result_promise, &buf, &response_data](const std::error_code& ec2, std::size_t n) {
                            if (ec2)
                            {
                                result_promise.set_value(-1);
                                return;
                            }
                            response_data.assign(buf.begin(), buf.begin() + n);
                            result_promise.set_value(static_cast<int>(n));
                        });
                };

            asio::async_write(port_, asio::buffer(data), write_handler);

            // 阻塞等待结果（注意：此调用会阻塞当前线程！如果调用线程是 io_context 线程，请勿使用同步 Write 带 timeout）
            try {
                int res = result_future.get();
                if (res > 0) {
                    invokeOnWrite(data, std::error_code());
                    invokeOnRead(response_data, std::error_code());
                }
                return res;
            }
            catch (...) {
                return -1;
            }
        }

        int AsyncWrite(const std::vector<char>& data) override
        {
            if (!connected_)
                return -1;

            auto write_handler = [this, data](const std::error_code& ec, std::size_t bytes_transferred) {
                if (ec) {
                    invokeOnError(ec);
                }
                else {
                    invokeOnWrite(data, std::error_code());
                }
                };

            asio::async_write(port_, asio::buffer(data), write_handler);
            return 0;
        }

        bool IsConnected() const override
        {
            std::lock_guard<std::mutex> lock(mutex_);
            return connected_;
        }

    private:
        void startAsyncRead()
        {
            if (!connected_) return;

            read_buffer_.resize(1024);
            port_.async_read_some(asio::buffer(read_buffer_),
                [this](const std::error_code& ec, std::size_t bytes_transferred) {
                    if (ec)
                    {
                        if (ec != asio::error::operation_aborted)
                        {
                            invokeOnError(ec);
                            Disconnect();
                        }
                        return;
                    }

                    if (bytes_transferred > 0)
                    {
                        std::vector<char> data(read_buffer_.begin(), read_buffer_.begin() + bytes_transferred);
                        invokeOnRead(data, std::error_code());
                    }

                    // 继续读
                    startAsyncRead();
                });
        }

        void invokeOnConnect(std::error_code ec)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (handler_ptr_)
                handler_ptr_->OnConnect(ec);
            else if (handler_func_.onconnectfun)
                handler_func_.onconnectfun(ec);
        }

        void invokeOnDisconnect(std::error_code ec)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (handler_ptr_)
                handler_ptr_->OnDisconnect(ec);
            else if (handler_func_.ondisconnectfun)
                handler_func_.ondisconnectfun(ec);
        }

        void invokeOnRead(const std::vector<char>& data, std::error_code ec)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (handler_ptr_)
                handler_ptr_->OnRead(data, ec);
            else if (handler_func_.onreadfun)
                handler_func_.onreadfun(data, ec);
        }

        void invokeOnWrite(const std::vector<char>& data, std::error_code ec)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (handler_ptr_)
                handler_ptr_->OnWrite(data, ec);
            else if (handler_func_.onwritefun)
                handler_func_.onwritefun(data, ec);
        }

        void invokeOnError(std::error_code ec)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (handler_ptr_)
                handler_ptr_->OnError(ec);
            else if (handler_func_.onerrorfun)
                handler_func_.onerrorfun(ec);
        }

    private:
        std::unique_ptr<std::thread> io_ctx_thread_;
        asio::io_context io_ctx_;
        serial_port port_;
        bool connected_;
        std::vector<char> read_buffer_;

        ISerialPortHandlerFunction handler_func_;
        ISerialPortHandler* handler_ptr_;

        mutable std::mutex mutex_;
    };

    // 工厂函数
    //std::unique_ptr<ISerialPort> create_serial_port(asio::io_context& io_ctx)
    //{
    //    return std::make_unique<SerialPortImpl>(io_ctx);
    //}

//} // namespace xserial