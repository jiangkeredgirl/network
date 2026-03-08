/*********************************************************************
 * \file   SerialPortImpl.cpp
 * \brief  工业级串口类.
 * \author 蒋珂
 * \date   2026.03.04
 *********************************************************************/
#include "SerialPortImpl.h"


CSerialPortImpl::CSerialPortImpl()
    : port_(io_)
{
}

CSerialPortImpl::~CSerialPortImpl()
{
    Disconnect();
}

int CSerialPortImpl::RegisterHandler(ISerialPortHandlerFunction handler_fun)
{
    handler_fun_ = handler_fun;
    return 0;
}

int CSerialPortImpl::RegisterHandler(ISerialPortHandler* handler)
{
    handler_interface_ = handler;
    return 0;
}

bool CSerialPortImpl::IsConnected() const
{
    return connected_;
}

int CSerialPortImpl::Connect(const string& portname, int baudrate)
{
    port_name_ = portname;
    config_.baudrate = baudrate;

    std::error_code ec;

    port_.open(portname, ec);

    if (ec)
    {
        SerialLogger::Error("Open port failed");

        return -1;
    }

    port_.set_option(asio::serial_port_base::baud_rate(baudrate));

    connected_ = true;

    SerialLogger::Info("Serial connected");

    StartRead();

    io_thread_ = std::thread([this]()
        {
            io_.run();
        });

    return 0;
}

int CSerialPortImpl::AsyncConnect(const string& portname, int baudrate)
{
    return Connect(portname, baudrate);
}

int CSerialPortImpl::Disconnect()
{
    stopping_ = true;

    if (port_.is_open())
    {
        std::error_code ec;

        port_.cancel(ec);

        port_.close(ec);
    }

    io_.stop();

    if (io_thread_.joinable())
        io_thread_.join();

    connected_ = false;

    return 0;
}

void CSerialPortImpl::StartRead()
{
    port_.async_read_some(

        asio::buffer(temp_read_, sizeof(temp_read_)),

        [this](const std::error_code& ec, size_t bytes)
        {
            HandleRead(ec, bytes);
        });
}

void CSerialPortImpl::HandleRead(const std::error_code& ec, size_t bytes)
{
    if (ec)
    {
        SerialLogger::Error("Read error");

        connected_ = false;

        if (config_.auto_reconnect)
            DoReconnect();

        return;
    }

    vector<char> data(temp_read_, temp_read_ + bytes);

    SerialLogger::Debug("Read bytes=" + std::to_string(bytes));

    if (handler_interface_)
        handler_interface_->OnRead(data, ec);

    if (handler_fun_.onreadfun)
        handler_fun_.onreadfun(data, ec);

    CompleteRequest(data);

    StartRead();
}

int CSerialPortImpl::Write(const vector<char>& data)
{
    if (!port_.is_open())
    {
        SerialLogger::Error("Serial port not open");
        return -1;
    }

    std::error_code ec;

    size_t bytes = asio::write(port_, asio::buffer(data), ec);

    if (ec)
    {
        SerialLogger::log(spdlog::level::err, "Serial write failed: {}", ec.message());

        if (handler_fun_.onerrorfun)
            handler_fun_.onerrorfun(ec);

        return -2;
    }

    SerialLogger::log(spdlog::level::info, "Serial write {} bytes", bytes);

    if (handler_fun_.onwritefun)
        handler_fun_.onwritefun(data, ec);

    return 0;
}

int CSerialPortImpl::AsyncWrite(const vector<char>& data)
{
    if (!port_.is_open())
    {
        SerialLogger::Error("Serial port not open");
        return -1;
    }

    auto write_buffer = std::make_shared<vector<char>>(data);

    asio::async_write(
        port_,
        asio::buffer(*write_buffer),
        [this, write_buffer](const std::error_code& ec, size_t bytes_transferred)
        {
            if (ec)
            {
                SerialLogger::log(spdlog::level::err, "Async write failed: {}", ec.message());

                if (handler_fun_.onerrorfun)
                    handler_fun_.onerrorfun(ec);

                return;
            }

            SerialLogger::log(spdlog::level::info, "Async write {} bytes", bytes_transferred);

            if (handler_fun_.onwritefun)
                handler_fun_.onwritefun(*write_buffer, ec);
        });

    return 0;
}

int CSerialPortImpl::Write(const vector<char>& data,
    vector<char>& response,
    int timeout_ms)
{
    auto req = std::make_shared<SerialRequest>();

    req->request = data;

    req->timeout_ms = timeout_ms;
    
    int try_counting = 0;

	do
	{
		auto future = req->promise.get_future();

		{
			std::lock_guard<std::mutex> lock(write_mutex_);

			request_queue_.push_back(req);
		}

		asio::post(io_,
			[this, req]()
			{
				asio::async_write(port_,

					asio::buffer(req->request),

					[this, req](const std::error_code& ec, size_t)
					{
						if (ec)
						{
							req->promise.set_value(-1);
                            SerialLogger::log(spdlog::level::err, "Async write failed: {}", ec.message());
							return;
						}
					});
			});

		if (future.wait_for(std::chrono::milliseconds(timeout_ms)) ==
			std::future_status::timeout)
		{
            req = request_queue_.back();
            request_queue_.pop_back();
            req->promise = std::promise<int>();  // 新的 promise
            SerialLogger::log(spdlog::level::err, "Skipped expired request");
			//return -1;
            continue;
		}
        break;

	} while (try_counting++ < config_.request_retry);

    if (try_counting > config_.request_retry)
    {
        SerialLogger::log(spdlog::level::err, "stry {} times, expired request", config_.request_retry);
    }

    response = req->response;

    return response.size();
}

void CSerialPortImpl::CompleteRequest(const vector<char>& data)
{
    std::lock_guard<std::mutex> lock(write_mutex_);

    if (request_queue_.empty())
        return;

    auto req = request_queue_.front();

    request_queue_.pop_front();

    req->response = data;

    req->promise.set_value(data.size());
}

void CSerialPortImpl::DoReconnect()
{
    SerialLogger::Info("Try reconnect");

    reconnect_timer_ = std::make_unique<asio::steady_timer>(
        io_,
        std::chrono::milliseconds(config_.reconnect_interval_ms));

    reconnect_timer_->async_wait(

        [this](const std::error_code&)
        {
            std::error_code ec;

            port_.open(port_name_, ec);

            if (!ec)
            {
                connected_ = true;

                SerialLogger::Info("Reconnect success");

                StartRead();
            }
            else
            {
                DoReconnect();
            }
        });
}





LIBSERIALPORT_API ISerialPort* NewSerialPort(void)
{
    return new CSerialPortImpl();
}

LIBSERIALPORT_API void DeleteSerialPort(ISerialPort* serial_port)
{
    if (serial_port)
    {
        delete dynamic_cast<CSerialPortImpl*>(serial_port);
        serial_port = nullptr;
    }
}
