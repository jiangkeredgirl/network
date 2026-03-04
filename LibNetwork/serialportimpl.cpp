/*********************************************************************
 * \file   SerialPortImpl.cpp
 * \brief  串口类.
 * \author 蒋珂
 * \date   2024.08.29
 *********************************************************************/

#include "serialportimpl.h"

using error_code = asio::error_code;

static const uint8_t FRAME_HEAD = 0xAA;
static const uint8_t FRAME_TAIL = 0x55;

CSerialPortImpl::CSerialPortImpl()
    : serial_(io_),
    strand_(asio::make_strand(io_)),
    state_(State::Disconnected),
    handler_(nullptr),
    request_id_(0),
    heartbeat_interval_ms_(3000),
    enable_crc_(true),
    reconnect_attempt_(0),
    use_frame_(false),
    frame_head_(0xAA),
    frame_tail_(0x55),
    enable_heartbeat_(false)  // 默认开启
{
}

CSerialPortImpl::~CSerialPortImpl()
{
    Disconnect();
}

bool CSerialPortImpl::IsConnected() const
{
    return state_ == State::Connected;
}

int CSerialPortImpl::RegisterHandler(ISerialPortHandler* handler)
{
    handler_ = handler;
    return 0;
}

int CSerialPortImpl::Connect(const std::string& portname, int baudrate)
{
    io_.restart();

    portname_ = portname;
    baudrate_ = baudrate;

    try
    {
        serial_.open(portname_);
        serial_.set_option(asio::serial_port_base::baud_rate(baudrate_));

        state_ = State::Connected;

        io_thread_.reset(new std::thread([this]() {
            std::cout << "[Serial] IO Thread Start\n";
            io_.run();
            }));

        std::cout << "[Serial] Connected\n";

        DoRead();
        StartHeartbeat();

        if (handler_)
            handler_->OnSerialPortConnect(0, "Connected");

        return 0;
    }
    catch (std::exception& e)
    {
        std::cout << "[Serial] Connect Failed: " << e.what() << "\n";
        return -1;
    }
}

int CSerialPortImpl::AsyncConnect(const std::string& portname, int baudrate)
{
    portname_ = portname;
    baudrate_ = baudrate;

    io_.restart();

    io_thread_.reset(new std::thread([this]() {
        io_.run();
        }));

    asio::post(strand_, [this]() { DoConnect(); });

    return 0;
}

void CSerialPortImpl::DoConnect()
{
    try
    {
        state_ = State::Connecting;

        serial_.open(portname_);
        serial_.set_option(asio::serial_port_base::baud_rate(baudrate_));

        state_ = State::Connected;
        reconnect_attempt_ = 0;

        std::cout << "[Serial] Async Connected\n";

        DoRead();
        StartHeartbeat();

        if (handler_)
            handler_->OnSerialPortConnect(0, "Connected");
    }
    catch (...)
    {
        std::cout << "[Serial] Async Connect Failed\n";
        HandleReconnect();
    }
}

void CSerialPortImpl::HandleReconnect()
{
    state_ = State::Reconnecting;
    reconnect_attempt_++;

    int delay = std::min(5000, reconnect_attempt_ * 1000);

    std::cout << "[Serial] Reconnecting in " << delay << " ms\n";

    auto timer = std::make_shared<asio::steady_timer>(io_);
    timer->expires_after(std::chrono::milliseconds(delay));

    timer->async_wait(asio::bind_executor(strand_,
        [this, timer](const error_code&) {
            DoConnect();
        }));
}

void CSerialPortImpl::DoRead()
{
    serial_.async_read_some(
        asio::buffer(read_buffer_),
        asio::bind_executor(strand_,
            [this](error_code ec, size_t length)
            {
                if (ec)
                {
                    cout << "[Serial] Read Error: " << ec.message() << endl;
                    if (state_ != State::Closing)
                        HandleReconnect();
                    return;
                }

                if (length > 0)
                    ProcessRawData(read_buffer_.data(), length);

                // 永远循环读
                DoRead();
            }));
}

void CSerialPortImpl::ProcessRawData(const char* data, size_t len)
{
    recv_cache_.insert(recv_cache_.end(), data, data + len);

    while (!recv_cache_.empty())
    {
        if (use_frame_)
        {
            // 查找帧头
            if (recv_cache_[0] != frame_head_)
            {
                recv_cache_.erase(recv_cache_.begin());
                continue;
            }

            auto tail_it = std::find(recv_cache_.begin(), recv_cache_.end(), frame_tail_);
            if (tail_it == recv_cache_.end())
                break; // 等待完整帧

            size_t frame_len = std::distance(recv_cache_.begin(), tail_it) + 1;
            std::vector<char> frame(recv_cache_.begin(), recv_cache_.begin() + frame_len);
            recv_cache_.erase(recv_cache_.begin(), recv_cache_.begin() + frame_len);

            HandleFrame(frame);
        }
        else
        {
            // 无帧头帧尾模式，直接把缓存全部当作一条消息
            std::vector<char> frame = std::move(recv_cache_);
            recv_cache_.clear();
            HandleFrame(frame);
            break;
        }
    }
}

void CSerialPortImpl::HandleFrame(const std::vector<char>& frame)
{
    asio::post(strand_, [this, frame]() {

        if (frame.size() < 5)
            return;

        uint16_t id = *(uint16_t*)&frame[1];

        auto it = pending_requests_.find(id);
        if (it != pending_requests_.end())
        {
            auto req = it->second;

            if (!req->completed.exchange(true))
            {
                req->promise.set_value(frame);
                pending_requests_.erase(it);
                std::cout << "[Serial] Matched Response ID=" << id << "\n";
            }
            return;
        }

        if (handler_)
            handler_->OnSerialPortRead(frame.data(), frame.size(), 0, "OK");
        });
}

int CSerialPortImpl::Write(const char* data, size_t size)
{
    asio::post(strand_,
        [this, vec = std::vector<char>(data, data + size)]()
        {
            write_queue_.push_back(vec);
            if (write_queue_.size() == 1)
                DoWrite();
        });

    return 0;
}

int CSerialPortImpl::AsyncWrite(const char* data, size_t size)
{
    return Write(data, size);
}

void CSerialPortImpl::DoWrite()
{
    if (write_queue_.empty())
        return;

    asio::async_write(serial_,
        asio::buffer(write_queue_.front()),
        asio::bind_executor(strand_,
            [this](error_code ec, size_t length)
            {
                if (ec)
                {
                    std::cout << "[Serial] Write Error\n";
                    FailAllPending();
                    HandleReconnect();
                    return;
                }

                std::cout << "[Serial] Write Success: " << length << "\n";

                write_queue_.pop_front();

                if (!write_queue_.empty())
                    DoWrite();
            }));
}

int CSerialPortImpl::Write(const char* data,
    size_t size,
    char** response_data,
    size_t& response_data_size,
    int timeout_ms)
{
    response_data_size = 0;

    if (!IsConnected())
        return -1;

    uint16_t id = GenerateRequestID();

    auto req = std::make_shared<PendingRequest>();
    req->id = id;
    req->timer = std::make_shared<asio::steady_timer>(io_);

    asio::post(strand_, [this, req]() {
        pending_requests_[req->id] = req;
        });

    Write(data, size);

    req->timer->expires_after(std::chrono::milliseconds(timeout_ms));
    req->timer->async_wait(asio::bind_executor(strand_,
        [this, req](const error_code& ec)
        {
            if (!ec && !req->completed.exchange(true))
            {
                std::cout << "[Serial] Timeout ID=" << req->id << "\n";
                req->promise.set_value({});
                pending_requests_.erase(req->id);
            }
        }));

    auto future = req->promise.get_future();

    if (future.wait_for(std::chrono::milliseconds(timeout_ms + 100))
        != std::future_status::ready)
        return -2;

    auto result = future.get();

    if (result.empty())
        return -3;

    response_data_size = result.size();
    *response_data = new char[result.size()];
    memcpy(*response_data, result.data(), result.size());

    return 0;
}

void CSerialPortImpl::FailAllPending()
{
    asio::post(strand_, [this]() {

        for (auto& kv : pending_requests_)
        {
            auto& req = kv.second;
            if (!req->completed.exchange(true))
                req->promise.set_value({});
        }

        pending_requests_.clear();
        });
}

void CSerialPortImpl::StartHeartbeat()
{
    heartbeat_timer_ = std::make_shared<asio::steady_timer>(io_);
    SendHeartbeat();
}

void CSerialPortImpl::SendHeartbeat()
{
    if (!enable_heartbeat_)
        return;

    heartbeat_timer_->expires_after(chrono::milliseconds(heartbeat_interval_ms_));
    heartbeat_timer_->async_wait(asio::bind_executor(strand_,
        [this](error_code ec)
        {
            if (!ec && state_ == State::Connected)
            {
                cout << "[Serial] Send Heartbeat\n";
                char hb[3] = { frame_head_, 0x00, frame_tail_ };
                Write(hb, 3);

                // 下次心跳
                SendHeartbeat();
            }
        }));
}

void CSerialPortImpl::StopHeartbeat()
{
    if (heartbeat_timer_)
        heartbeat_timer_->cancel();
}

uint16_t CSerialPortImpl::GenerateRequestID()
{
    return ++request_id_;
}

uint16_t CSerialPortImpl::CRC16(const uint8_t* data, size_t len)
{
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++)
    {
        crc ^= data[i];
        for (int j = 0; j < 8; j++)
            crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : crc >> 1;
    }
    return crc;
}

void CSerialPortImpl::SetFrameConfig(bool use_frame, uint8_t head, uint8_t tail)
{
	asio::post(strand_, [this, use_frame, head, tail]() {
		use_frame_ = use_frame;
		frame_head_ = head;
		frame_tail_ = tail;
		std::cout << "[Serial] Frame Config Updated: use_frame=" << use_frame_
			<< " head=" << std::hex << (int)frame_head_
			<< " tail=" << (int)frame_tail_ << std::dec << std::endl;
		});
}

void CSerialPortImpl::SetHeartbeatEnable(bool enable)
{
	asio::post(strand_, [this, enable]() {
		enable_heartbeat_ = enable;
		if (!enable_heartbeat_ && heartbeat_timer_)
			heartbeat_timer_->cancel();
		cout << "[Serial] Heartbeat " << (enable ? "Enabled" : "Disabled") << endl;
		});
}

int CSerialPortImpl::Disconnect()
{
    state_ = State::Closing;

    StopHeartbeat();
    FailAllPending();

    asio::post(strand_, [this]() {
        error_code ec;
        serial_.close(ec);
        });

    io_.stop();

    if (io_thread_ && io_thread_->joinable())
        io_thread_->join();

    state_ = State::Disconnected;

    std::cout << "[Serial] Disconnected\n";

    if (handler_)
        handler_->OnSerialPortDisconnect(0, "Disconnected");

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
