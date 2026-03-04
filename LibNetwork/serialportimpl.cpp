/*********************************************************************
 * \file   SerialPortImpl.cpp
 * \brief  串口类.
 * \author 蒋珂
 * \date   2024.08.29
 *********************************************************************/

#include "serialportimpl.h"

#include <cstring>
#include <algorithm>

CSerialPortImpl::CSerialPortImpl()
    : serial_(io_),
    connected_(false),
    manual_close_(false),
    auto_reconnect_(true),
    reconnect_delay_(1000),
    handler_(nullptr)
{
    io_thread_ = std::thread([this]()
        {
            io_.run();
        });
}

CSerialPortImpl::~CSerialPortImpl()
{
    Disconnect();
    io_.stop();
    if (io_thread_.joinable())
        io_thread_.join();
}

int CSerialPortImpl::RegisterHandler(ISerialPortHandler* handler)
{
    handler_ = handler;
    return 0;
}

int CSerialPortImpl::Connect(const string& portname, int baudrate)
{
    portname_ = portname;
    baudrate_ = baudrate;
    manual_close_ = false;

    asio::post(io_, [this]() { DoOpen(); });
    return 0;
}

int CSerialPortImpl::AsyncConnect(const string& portname, int baudrate)
{
    return Connect(portname, baudrate);
}

void CSerialPortImpl::DoOpen()
{
    asio::error_code ec;
    serial_.open(portname_, ec);

    if (ec)
    {
        if (handler_)
            handler_->OnSerialPortConnect(ec.value(), ec.message());

        StartReconnect();
        return;
    }

    serial_.set_option(asio::serial_port_base::baud_rate(baudrate_));
    serial_.set_option(asio::serial_port_base::character_size(8));
    serial_.set_option(asio::serial_port_base::parity(asio::serial_port_base::parity::none));
    serial_.set_option(asio::serial_port_base::stop_bits(asio::serial_port_base::stop_bits::one));

    connected_ = true;
    reconnect_delay_ = 1000;

    if (handler_)
        handler_->OnSerialPortConnect(0, "success");

    StartRead();
}

void CSerialPortImpl::StartReconnect()
{
    if (manual_close_ || !auto_reconnect_)
        return;

    reconnect_timer_ = make_unique<asio::steady_timer>(io_);
    reconnect_timer_->expires_after(std::chrono::milliseconds(reconnect_delay_));

    reconnect_timer_->async_wait([this](const asio::error_code& ec)
        {
            if (!ec)
            {
                reconnect_delay_ = std::min(reconnect_delay_ * 2, 16000);
                DoOpen();
            }
        });
}

void CSerialPortImpl::StartRead()
{
    serial_.async_read_some(
        asio::buffer(read_buffer_, READ_BUFFER_SIZE),
        [this](const asio::error_code& ec, size_t bytes)
        {
            HandleRead(ec, bytes);
        });
}

void CSerialPortImpl::HandleRead(const asio::error_code& ec, size_t bytes)
{
    if (ec)
    {
        HandleError(ec);
        return;
    }

    if (handler_)
        handler_->OnSerialPortRead(read_buffer_, bytes, 0, "success");

    StartRead();
}

int CSerialPortImpl::AsyncWrite(const char* data, size_t size)
{
    if (!connected_)
        return -1;

    std::lock_guard<std::mutex> lock(write_mutex_);

    if (write_queue_.size() > 100)
        return -2;

    write_queue_.emplace(data, data + size);

    if (write_queue_.size() == 1)
        asio::post(io_, [this]() { DoWrite(); });

    return 0;
}

void CSerialPortImpl::DoWrite()
{
    if (write_queue_.empty())
        return;

    asio::async_write(serial_,
        asio::buffer(write_queue_.front()),
        [this](const asio::error_code& ec, size_t bytes)
        {
            HandleWrite(ec, bytes);
        });
}

void CSerialPortImpl::HandleWrite(const asio::error_code& ec, size_t bytes)
{
    std::lock_guard<std::mutex> lock(write_mutex_);

    auto data = write_queue_.front();
    write_queue_.pop();

    if (handler_)
        handler_->OnSerialPortWrite(
            data.data(),
            bytes,
            ec ? ec.value() : 0,
            ec ? ec.message() : "success");

    if (ec)
    {
        HandleError(ec);
        return;
    }

    if (!write_queue_.empty())
        DoWrite();
}

int CSerialPortImpl::Write(const char* data, size_t size)
{
    if (!connected_)
        return -1;

    asio::error_code ec;
    asio::write(serial_, asio::buffer(data, size), ec);

    if (handler_)
        handler_->OnSerialPortWrite(
            data,
            size,
            ec ? ec.value() : 0,
            ec ? ec.message() : "success");

    return ec ? -2 : 0;
}

int CSerialPortImpl::Write(const char* data, size_t size,
    char** response_data,
    size_t& response_data_size,
    int timeout_ms)
{
    if (!connected_)
        return -1;

    asio::error_code ec;
    asio::write(serial_, asio::buffer(data, size), ec);
    if (ec)
        return -2;

    asio::steady_timer timer(io_);
    bool timeout = false;

    timer.expires_after(std::chrono::milliseconds(timeout_ms));
    timer.async_wait([&](auto)
        {
            timeout = true;
            serial_.cancel();
        });

    size_t bytes = serial_.read_some(
        asio::buffer(read_buffer_, READ_BUFFER_SIZE), ec);

    if (timeout || ec)
        return -3;

    *response_data = new char[bytes];
    memcpy(*response_data, read_buffer_, bytes);
    response_data_size = bytes;

    return 0;
}

void CSerialPortImpl::HandleError(const asio::error_code& ec)
{
    connected_ = false;

    if (handler_)
    {
        handler_->OnSerialPortError(ec.value(), ec.message());
        handler_->OnSerialPortDisconnect(ec.value(), ec.message());
    }

    if (serial_.is_open())
        serial_.close();

    StartReconnect();
}

int CSerialPortImpl::Disconnect()
{
    manual_close_ = true;
    connected_ = false;

    asio::post(io_, [this]()
        {
            if (serial_.is_open())
                serial_.close();
        });

    if (handler_)
        handler_->OnSerialPortDisconnect(0, "manual disconnect");

    return 0;
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
