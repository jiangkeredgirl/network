/*********************************************************************
 * \file   SerialPortImpl.cpp
 * \brief  工业级串口类.
 * \author 蒋珂
 * \date   2026.03.04
 *********************************************************************/
#include "SerialPortImpl.h"


using error_code = asio::error_code;

static const uint8_t FRAME_HEAD_DEFAULT = 0xAA;
static const uint8_t FRAME_TAIL_DEFAULT = 0x55;

CSerialPortImpl::CSerialPortImpl()
    : serial_(io_), strand_(asio::make_strand(io_)), state_(State::Disconnected),
    handler_interface_(nullptr), request_id_(0), reconnect_attempt_(0)
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
    handler_interface_ = handler;
    return 0;
}

int CSerialPortImpl::RegisterHandler(ISerialPortHandlerFunction handler_fun)
{
    handler_function_ = handler_fun;
    return 0;
}

int CSerialPortImpl::Connect(const string& portname, int baudrate)
{
    portname_ = portname;
    baudrate_ = baudrate;

    try
    {
        serial_.open(portname_);
        serial_.set_option(asio::serial_port_base::baud_rate(baudrate_));

        state_ = State::Connected;
        reconnect_attempt_ = 0;

        io_thread_.reset(new thread([this]() { io_.run(); }));

        DoRead();
        if (config_.enable_heartbeat) SendHeartbeat();

        error_code ec;
        if (handler_interface_) handler_interface_->OnConnect(ec);
        if (handler_function_.onconnectfun) handler_function_.onconnectfun(ec);

        cout << "[Serial] Connected\n";
        return 0;
    }
    catch (exception& e)
    {
        cout << "[Serial] Connect Failed: " << e.what() << endl;
        HandleReconnect();
        return -1;
    }
}

int CSerialPortImpl::AsyncConnect(const string& portname, int baudrate)
{
    portname_ = portname;
    baudrate_ = baudrate;

    io_thread_.reset(new thread([this]() { io_.run(); }));
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

        DoRead();
        if (config_.enable_heartbeat) SendHeartbeat();

        error_code ec;
        if (handler_interface_) handler_interface_->OnConnect(ec);
        if (handler_function_.onconnectfun) handler_function_.onconnectfun(ec);

        cout << "[Serial] Async Connected\n";
    }
    catch (...)
    {
        cout << "[Serial] Async Connect Failed\n";
        HandleReconnect();
    }
}

void CSerialPortImpl::HandleReconnect()
{
    if (!config_.enable_auto_reconnect) return;

    if (config_.max_reconnect_attempts != -1 &&
        reconnect_attempt_ >= config_.max_reconnect_attempts)
    {
        cout << "[Serial] Max reconnect attempts reached\n";
        state_ = State::Disconnected;
        error_code ec = make_error_code(errc::connection_refused);
        if (handler_interface_) handler_interface_->OnDisconnect(ec);
        if (handler_function_.ondisconnectfun) handler_function_.ondisconnectfun(ec);
        return;
    }

    reconnect_attempt_++;
    cout << "[Serial] Reconnecting in " << config_.reconnect_interval_ms
        << " ms, Attempt " << reconnect_attempt_ << "\n";

    auto timer = make_shared<asio::steady_timer>(io_);
    timer->expires_after(chrono::milliseconds(config_.reconnect_interval_ms));
    timer->async_wait(asio::bind_executor(strand_, [this, timer](error_code) {
        DoConnect();
        }));
}

int CSerialPortImpl::Disconnect()
{
    State expected = state_.load();
    if (expected == State::Disconnected || expected == State::Closing)
        return 0;  // 已经断开或正在关闭

    state_ = State::Closing;

    // 停止心跳
    StopHeartbeat();

    // 关闭串口
    asio::post(strand_, [this]() {
        asio::error_code ec;
        serial_.close(ec);
        if (ec)
            cout << "[Serial] Error closing serial: " << ec.message() << endl;
        });

    // 停止 io_context
    io_.stop();

    if (io_thread_ && io_thread_->joinable())
        io_thread_->join();

    state_ = State::Disconnected;

    cout << "[Serial] Disconnected\n";

    if (handler_interface_) {
        std::error_code ec; // 可以填具体错误码
        handler_interface_->OnDisconnect(ec);
    }
    if (handler_function_.ondisconnectfun) {
        std::error_code ec; // 可以填具体错误码
        handler_function_.ondisconnectfun(ec);
    }

    return 0; // 返回成功
}

void CSerialPortImpl::DoRead()
{
    serial_.async_read_some(asio::buffer(read_buffer_),
        asio::bind_executor(strand_,
            [this](error_code ec, size_t len)
            {
                if (ec)
                {
                    HandleReconnect();
                    return;
                }

                ProcessRawData(read_buffer_.data(), len);
                DoRead();
            }));
}

void CSerialPortImpl::ProcessRawData(const char* data, size_t len)
{
    recv_cache_.insert(recv_cache_.end(), data, data + len);

    while (config_.enable_frame && recv_cache_.size() >= 2)
    {
        if (recv_cache_[0] != config_.frame_header)
        {
            recv_cache_.erase(recv_cache_.begin());
            continue;
        }

        auto tail_it = find(recv_cache_.begin(), recv_cache_.end(), config_.frame_tail);
        if (tail_it == recv_cache_.end()) break;

        vector<char> frame(recv_cache_.begin(), tail_it + 1);
        recv_cache_.erase(recv_cache_.begin(), tail_it + 1);

        HandleFrame(frame);
    }

    if (!config_.enable_frame && !recv_cache_.empty())
    {
        vector<char> frame = recv_cache_;
        recv_cache_.clear();
        HandleFrame(frame);
    }
}

void CSerialPortImpl::HandleFrame(const vector<char>& frame)
{
    if (config_.enable_crc && frame.size() >= 3)
    {
        uint16_t recv_crc = *(uint16_t*)&frame[frame.size() - 2];
        uint16_t calc_crc = CRC16((uint8_t*)frame.data(), frame.size() - 2);
        if (recv_crc != calc_crc)
        {
            cout << "[Serial] CRC Error\n";
            return;
        }
    }

    error_code ec;
    if (handler_interface_) handler_interface_->OnRead(frame, ec);
    if (handler_function_.onreadfun) handler_function_.onreadfun(frame, ec);
}

int CSerialPortImpl::Write(const vector<char>& data)
{
    asio::post(strand_, [this, data]()
        {
            lock_guard<mutex> lock(write_mutex_);
            write_queue_.push_back(data);
            if (write_queue_.size() == 1) DoWrite();
        });
    return 0;
}

int CSerialPortImpl::Write(const vector<char>& data, vector<char>& response_data, int timeout_ms)
{
    response_data.clear();
    if (!IsConnected())
        return -1;

    {
        std::lock_guard<std::mutex> lock(resp_mutex_);
        resp_ready_ = false;
        resp_buffer_.clear();
    }

    // 发送请求
    int ret = AsyncWrite(data);
    if (ret != 0)
        return ret;

    // 等待响应
    std::unique_lock<std::mutex> lock(resp_mutex_);

    if (!resp_cv_.wait_for(
        lock,
        std::chrono::milliseconds(timeout_ms),
        [this] { return resp_ready_; }))
    {
        std::cout << "[Serial] Response timeout\n";
        return -1;
    }

    response_data = resp_buffer_;

    return 0;
}

int CSerialPortImpl::AsyncWrite(const vector<char>& data)
{
    return Write(data);
}

void CSerialPortImpl::DoWrite()
{
    if (write_queue_.empty()) return;

    asio::async_write(serial_, asio::buffer(write_queue_.front()),
        asio::bind_executor(strand_, [this](error_code ec, size_t len) {
            lock_guard<mutex> lock(write_mutex_);
            if (ec) { HandleReconnect(); return; }

            write_queue_.pop_front();
            if (!write_queue_.empty()) DoWrite();
            }));
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

void CSerialPortImpl::SendHeartbeat()
{
    if (!config_.enable_heartbeat) return;

    if (!heartbeat_timer_) heartbeat_timer_ = make_shared<asio::steady_timer>(io_);
    heartbeat_timer_->expires_after(chrono::milliseconds(config_.heartbeat_interval_ms));
    heartbeat_timer_->async_wait(asio::bind_executor(strand_, [this](error_code ec) {
        if (!ec && state_ == State::Connected)
        {
            char hb[3] = { config_.frame_header, 0x00, config_.frame_tail };
            Write(vector<char>(hb, hb + 3));
            SendHeartbeat();
        }
        }));
}

void CSerialPortImpl::StopHeartbeat()
{
    if (heartbeat_timer_) heartbeat_timer_->cancel();
}

void CSerialPortImpl::OnReceive(const std::vector<char>& data)
{
    std::cout << "[Serial] Receive size=" << data.size() << std::endl;

    {
        std::lock_guard<std::mutex> lock(resp_mutex_);

        // 累加数据（解决分包）
        resp_buffer_.insert(
            resp_buffer_.end(),
            data.begin(),
            data.end()
        );

        resp_ready_ = true;
    }

    resp_cv_.notify_one();

    // 回调用户
    if (handler_interface_)
    {
        handler_interface_->OnRead(data, {});
    }

    if (handler_function_.onreadfun)
    {
        handler_function_.onreadfun(data, {});
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
