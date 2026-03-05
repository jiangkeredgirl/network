/*********************************************************************
 * \file   SerialPortImpl.cpp
 * \brief  工业级串口类.
 * \author 蒋珂
 * \date   2026.03.04
 *********************************************************************/
#include "SerialPortImpl.h"

using error_code = asio::error_code;

CSerialPortImpl::CSerialPortImpl()
    : serial_(io_),
    strand_(asio::make_strand(io_)),
    state_(State::Disconnected),
    handler_(nullptr),
    enable_crc_(true),
    use_frame_(true),
    frame_head_(0xAA),
    frame_tail_(0x55),
    heartbeat_timer_(nullptr),
    heartbeat_interval_ms_(3000),
    heartbeat_enable_(true),
    reconnect_attempt_(0)
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

// ---------------- 工业级配置 ----------------
void CSerialPortImpl::EnableCRC(bool enable)
{
    enable_crc_ = enable;
}

void CSerialPortImpl::SetFrameHeadTail(bool enable, char head, char tail)
{
    use_frame_ = enable;
    frame_head_ = head;
    frame_tail_ = tail;
}

void CSerialPortImpl::EnableHeartbeat(bool enable, int interval_ms)
{
    heartbeat_enable_ = enable;
    heartbeat_interval_ms_ = interval_ms;
}

// ---------------- 连接 ----------------
int CSerialPortImpl::Connect(const string& portname, int baudrate)
{
    portname_ = portname;
    baudrate_ = baudrate;

    try
    {
        serial_.open(portname_);
        serial_.set_option(asio::serial_port_base::baud_rate(baudrate_));
        state_ = State::Connected;

        io_thread_ = make_unique<thread>([this]() {
            cout << "[Serial] IO Thread Start\n";
            io_.run();
            });

        cout << "[Serial] Connected\n";

        DoRead();
        StartHeartbeat();

        if (handler_)
            handler_->OnSerialPortConnect(0, "Connected");

        return 0;
    }
    catch (exception& e)
    {
        cout << "[Serial] Connect Failed: " << e.what() << endl;
        return -1;
    }
}

int CSerialPortImpl::AsyncConnect(const string& portname, int baudrate)
{
    portname_ = portname;
    baudrate_ = baudrate;

    io_thread_ = make_unique<thread>([this]() { io_.run(); });
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

        cout << "[Serial] Async Connected\n";

        DoRead();
        StartHeartbeat();

        if (handler_)
            handler_->OnSerialPortConnect(0, "Connected");
    }
    catch (...)
    {
        cout << "[Serial] Async Connect Failed\n";
        HandleReconnect();
    }
}

void CSerialPortImpl::HandleReconnect()
{
    state_ = State::Reconnecting;
    reconnect_attempt_++;
    int delay = min(5000, reconnect_attempt_ * 1000);

    cout << "[Serial] Reconnecting in " << delay << " ms\n";

    auto timer = make_shared<asio::steady_timer>(io_);
    timer->expires_after(chrono::milliseconds(delay));
    timer->async_wait(asio::bind_executor(strand_,
        [this, timer](const error_code&) {
            DoConnect();
        }));
}

// ---------------- 接收数据 ----------------
void CSerialPortImpl::DoRead()
{
    serial_.async_read_some(
        asio::buffer(read_buffer_),
        asio::bind_executor(strand_,
            [this](error_code ec, size_t length)
            {
                if (ec)
                {
                    cout << "[Serial] Read Error\n";
                    HandleReconnect();
                    return;
                }
                ProcessRawData(read_buffer_.data(), length);
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
            auto head_it = find(recv_cache_.begin(), recv_cache_.end(), frame_head_);
            if (head_it == recv_cache_.end()) { recv_cache_.clear(); break; }
            if (head_it != recv_cache_.begin()) recv_cache_.erase(recv_cache_.begin(), head_it);

            auto tail_it = find(recv_cache_.begin() + 1, recv_cache_.end(), frame_tail_);
            if (tail_it == recv_cache_.end()) break;

            size_t frame_len = distance(recv_cache_.begin(), tail_it) + 1;
            vector<char> frame(recv_cache_.begin(), recv_cache_.begin() + frame_len);
            recv_cache_.erase(recv_cache_.begin(), recv_cache_.begin() + frame_len);

            HandleFrame(frame);
        }
        else
        {
            vector<char> frame = move(recv_cache_);
            recv_cache_.clear();
            HandleFrame(frame);
            break;
        }
    }
}

void CSerialPortImpl::HandleFrame(const vector<char>& frame)
{
    if (frame.empty()) return;

    cout << "[Serial] Frame Received, size=" << frame.size() << endl;

    if (enable_crc_ && frame.size() >= 3)
    {
        uint16_t recv_crc = *(uint16_t*)&frame[frame.size() - 3];
        uint16_t calc_crc = CRC16((uint8_t*)frame.data(), frame.size() - 3);
        if (recv_crc != calc_crc)
        {
            if (handler_) handler_->OnSerialPortError(-1, "CRC Error");
            return;
        }
    }

    // FIFO 匹配未超时请求
    while (!pending_requests_queue_.empty())
    {
        auto req = pending_requests_queue_.front();
        pending_requests_queue_.pop_front();
        if (!req->completed.exchange(true))
        {
            req->response = frame;
            req->promise.set_value(frame);
            cout << "[Serial] Response matched to pending request\n";
            return;
        }
    }

    if (handler_)
        handler_->OnSerialPortRead(frame.data(), frame.size(), 0, "OK");
}

// ---------------- 写数据 ----------------
int CSerialPortImpl::Write(const char* data, size_t size)
{
    asio::post(strand_, [this, vec = vector<char>(data, data + size)]() {
        write_queue_.push_back(vec);
        if (write_queue_.size() == 1) DoWrite();
        });
    return 0;
}

int CSerialPortImpl::Write(const char* data, size_t size, char** response_data, size_t& response_data_size, int timeout_ms)
{
    response_data_size = 0;
    if (!IsConnected()) return -1;

    auto req = make_shared<PendingRequest>();
    req->timer = make_shared<asio::steady_timer>(io_);
    asio::post(strand_, [this, req]() { pending_requests_queue_.push_back(req); });

    Write(data, size);

    req->timer->expires_after(chrono::milliseconds(timeout_ms));
    req->timer->async_wait(asio::bind_executor(strand_, [req](const error_code& ec) {
        if (!ec && !req->completed.exchange(true))
        {
            cout << "[Serial] Request Timeout\n";
            req->promise.set_value(vector<char>());
        }
        }));

    future<vector<char>> fut = req->promise.get_future();
    vector<char> result = fut.get();

    if (result.empty()) return -2;

    response_data_size = result.size();
    *response_data = new char[result.size()];
    memcpy(*response_data, result.data(), result.size());
    return 0;
}

int CSerialPortImpl::AsyncWrite(const char* data, size_t size)
{
    return Write(data, size);
}

void CSerialPortImpl::DoWrite()
{
    if (write_queue_.empty()) return;

    asio::async_write(serial_, asio::buffer(write_queue_.front()),
        asio::bind_executor(strand_, [this](error_code ec, size_t length) {
            if (ec)
            {
                cout << "[Serial] Write Error\n";
                HandleReconnect();
                return;
            }

            cout << "[Serial] Write Success: " << length << endl;
            write_queue_.pop_front();
            if (!write_queue_.empty()) DoWrite();
            }));
}

// ---------------- CRC ----------------
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

// ---------------- 心跳 ----------------
void CSerialPortImpl::StartHeartbeat()
{
    if (!heartbeat_enable_) return;
    heartbeat_timer_ = make_shared<asio::steady_timer>(io_);
    SendHeartbeat();
}

void CSerialPortImpl::SendHeartbeat()
{
    if (!heartbeat_enable_ || state_ != State::Connected) return;

    heartbeat_timer_->expires_after(chrono::milliseconds(heartbeat_interval_ms_));
    heartbeat_timer_->async_wait(asio::bind_executor(strand_, [this](error_code ec) {
        if (!ec && state_ == State::Connected)
        {
            char hb[3] = { use_frame_ ? frame_head_ : 0x00, 0x00, use_frame_ ? frame_tail_ : 0x00 };
            Write(hb, 3);
            SendHeartbeat();
        }
        }));
}

void CSerialPortImpl::StopHeartbeat()
{
    if (heartbeat_timer_) heartbeat_timer_->cancel();
}

// ---------------- 断开 ----------------
int CSerialPortImpl::Disconnect()
{
    state_ = State::Closing;
    StopHeartbeat();

    asio::post(strand_, [this]() { error_code ec; serial_.close(ec); });
    io_.stop();

    if (io_thread_ && io_thread_->joinable()) io_thread_->join();

    state_ = State::Disconnected;
    cout << "[Serial] Disconnected\n";
    if (handler_) handler_->OnSerialPortDisconnect(0, "Disconnected");
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
