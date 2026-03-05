/*********************************************************************
 * \file   SerialPortImpl.cpp
 * \brief  工业级串口类.
 * \author 蒋珂
 * \date   2026.03.04
 *********************************************************************/
#include "SerialPortImpl.h"


CSerialPortImpl::CSerialPortImpl()
    : serial_(io_), strand_(asio::make_strand(io_)),
    connected_(false), handler_(nullptr), running_(true)
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
    handler_ = handler;
    return 0;
}

int CSerialPortImpl::Connect(const std::string& portname, int baudrate)
{
    portname_ = portname;
    baudrate_ = baudrate;
    try
    {
        serial_.open(portname_);
        serial_.set_option(asio::serial_port_base::baud_rate(baudrate_));
        connected_ = true;

        io_thread_ = std::make_unique<std::thread>([this]() {
            io_.run();
            });

        if (handler_) handler_->OnConnect({});
        DoRead();
        return 0;
    }
    catch (std::exception& e)
    {
        if (handler_) handler_->OnConnect(make_error_code(SerialErrc::NotConnected));
        return -1;
    }
}

void CSerialPortImpl::DoRead()
{
    serial_.async_read_some(
        asio::buffer(recv_cache_, 1024),
        asio::bind_executor(strand_, [this](std::error_code ec, std::size_t len)
            {
                if (ec)
                {
                    SerialLogger::Info("Read Error");
                    HandleReconnect();
                    return;
                }

                ringbuf_.push(recv_cache_.data(), len);
                ProcessRawData();
                DoRead();
            })
    );
}

void CSerialPortImpl::ProcessRawData()
{
    std::vector<char> frame;
    while (ringbuf_.size() > 0)
    {
        std::vector<char> tmp;
        ringbuf_.pop(tmp, ringbuf_.size());
        if (SerialFrame::Parse(tmp, frame, { config_.enable_frame, config_.frame_header, config_.frame_tail }))
        {
            if (config_.enable_crc)
            {
                // CRC 校验略，可调用 CRC16::Calc()
            }
            if (handler_) handler_->OnRead(frame, {});
        }
    }
}

int CSerialPortImpl::Write(const std::vector<char>& data)
{
    asio::post(strand_, [this, data]()
        {
            write_queue_.push_back(data);
            if (write_queue_.size() == 1) DoWrite();
        });
    return 0;
}

void CSerialPortImpl::DoWrite()
{
    if (write_queue_.empty()) return;

    auto& data = write_queue_.front();
    asio::async_write(serial_, asio::buffer(data),
        asio::bind_executor(strand_, [this](std::error_code ec, size_t len)
            {
                if (ec)
                {
                    SerialLogger::Info("Write Error");
                    HandleReconnect();
                }
                else
                {
                    if (handler_) handler_->OnWrite(write_queue_.front(), {});
                    write_queue_.pop_front();
                    if (!write_queue_.empty()) DoWrite();
                }
            })
    );
}

bool CSerialPortImpl::IsConnected() const { return connected_; }
int CSerialPortImpl::Disconnect()
{
    running_ = false;
    std::error_code ec;
    serial_.close(ec);
    io_.stop();
    if (io_thread_ && io_thread_->joinable()) io_thread_->join();
    connected_ = false;
    if (handler_) handler_->OnDisconnect({});
    return 0;
}

void CSerialPortImpl::HandleReconnect()
{
    connected_ = false;
    if (handler_) handler_->OnDisconnect({});
    asio::steady_timer t(io_, std::chrono::milliseconds(config_.reconnect_interval_ms));
    t.async_wait([this](std::error_code) {
        if (running_) Connect(portname_, baudrate_);
        });
}

int CSerialPortImpl::AsyncConnect(const std::string& portname, int baudrate)
{
    asio::post(strand_, [this, portname, baudrate]() {
        Connect(portname, baudrate);
        });
    return 0;
}

int CSerialPortImpl::AsyncWrite(const std::vector<char>& data)
{
    return Write(data);
}

int CSerialPortImpl::Write(const std::vector<char>& data, std::vector<char>& response_data, int timeout_ms)
{
    std::promise<std::vector<char>> p;
    auto f = p.get_future();

    Write(data);

    if (f.wait_for(std::chrono::milliseconds(timeout_ms)) == std::future_status::ready)
    {
        response_data = f.get();
        return 0;
    }
    else
    {
        return -1; // timeout
    }
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
