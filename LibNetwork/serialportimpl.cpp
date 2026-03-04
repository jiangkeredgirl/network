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
    enable_crc_(false),
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
                {
                    cout << "[Serial] Read Length: " << length << endl;
                    ProcessRawData(read_buffer_.data(), length);
                }

                // 永远循环读
                DoRead();
            }));
}

// 工业级 ProcessRawData
void CSerialPortImpl::ProcessRawData(const char* data, size_t len)
{
    // 将新收到的数据加入缓存
    recv_cache_.insert(recv_cache_.end(), data, data + len);

    while (!recv_cache_.empty())
    {
        // 如果启用帧头帧尾模式
        if (use_frame_)
        {
            // 找到帧头
            auto head_it = std::find(recv_cache_.begin(), recv_cache_.end(), frame_head_);
            if (head_it == recv_cache_.end())
            {
                // 没有找到帧头，清空缓存
                recv_cache_.clear();
                break;
            }

            // 丢弃帧头之前的数据
            if (head_it != recv_cache_.begin())
                recv_cache_.erase(recv_cache_.begin(), head_it);

            // 查找帧尾
            auto tail_it = std::find(recv_cache_.begin() + 1, recv_cache_.end(), frame_tail_);
            if (tail_it == recv_cache_.end())
            {
                // 等待更多数据
                break;
            }

            size_t frame_len = std::distance(recv_cache_.begin(), tail_it) + 1;
            vector<char> frame(recv_cache_.begin(), recv_cache_.begin() + frame_len);

            // 从缓存移除已处理的数据
            recv_cache_.erase(recv_cache_.begin(), recv_cache_.begin() + frame_len);

            // 处理帧
            HandleFrame(frame);
        }
        else
        {
            // 禁用帧头帧尾模式：将缓存中的所有数据作为一帧处理
            vector<char> frame = std::move(recv_cache_);
            recv_cache_.clear();
            HandleFrame(frame);
            break; // 一次处理完所有数据
        }
    }
}

// 工业级 HandleFrame
void CSerialPortImpl::HandleFrame(const vector<char>& frame)
{
    if (frame.empty()) return;

    cout << "[Serial] Frame Received, size=" << frame.size() << endl;

    // CRC 校验可选
    if (enable_crc_ && frame.size() >= 3)
    {
        uint16_t recv_crc = *(uint16_t*)&frame[frame.size() - 3];
        uint16_t calc_crc = CRC16((uint8_t*)frame.data(), frame.size() - 3);
        if (recv_crc != calc_crc)
        {
            if (handler_)
                handler_->OnSerialPortError(-1, "CRC Error");
            return;
        }
    }

    // 匹配第一个未超时请求
    while (!pending_requests_queue_.empty())
    {
        auto req = pending_requests_queue_.front();
        pending_requests_queue_.pop_front();

        // 只匹配未完成请求
        if (!req->completed.exchange(true))
        {
            req->response = frame;
            req->promise.set_value(frame);

            cout << "[Serial] Response matched to pending request\n";
            return;
        }
        else
        {
            // 已超时，跳过到下一个
            cout << "[Serial] Skipped expired request\n";
            continue;
        }
    }

    // 如果没有未超时请求，普通回调
    if (handler_)
        handler_->OnSerialPortRead(frame.data(), frame.size(), 0, "OK");
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

int CSerialPortImpl::Write(
    const char* data,
    size_t size,
    char** response_data,
    size_t& response_data_size,
    int timeout_ms)
{
    response_data_size = 0;
    if (!IsConnected()) return -1;

    auto req = make_shared<PendingRequest>();
    req->timer = make_shared<asio::steady_timer>(io_);

    // 放入 pending 请求队列
    asio::post(strand_, [this, req]()
        {
            pending_requests_queue_.push_back(req);
        });

    // 发送数据
    Write(data, size);

    // 设置超时
    req->timer->expires_after(chrono::milliseconds(timeout_ms));
    req->timer->async_wait(asio::bind_executor(strand_,
        [req](const asio::error_code& ec)
        {
            if (!ec && !req->completed.exchange(true))
            {
                cout << "[Serial] Request Timeout\n";
                req->promise.set_value(vector<char>()); // 超时返回空
            }
        }));

    // 等待返回
    future<vector<char>> fut = req->promise.get_future();
    vector<char> result = fut.get(); // 阻塞等待结果或超时

    if (result.empty()) return -2; // 超时或无数据

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
