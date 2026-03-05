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
#include <atomic>
#include <deque>
#include <vector>
#include <map>
#include <memory>
#include <future>
#include <iostream>
#include <mutex>

using namespace std;

struct SerialConfig
{
    bool enable_crc = false;
    bool enable_frame = false;
    char frame_header = 0xAA;
    char frame_tail = 0x55;

    bool enable_heartbeat = false;
    int heartbeat_interval_ms = 3000;

    // 自动重连配置
    bool enable_auto_reconnect = false;
    int reconnect_interval_ms = 1000;
    int max_reconnect_attempts = 5; // -1 无限重连
};

class CSerialPortImpl : public ISerialPort
{
public:
    CSerialPortImpl();
    ~CSerialPortImpl();

    // 注册处理器
    int RegisterHandler(ISerialPortHandler* handler) override;
    int RegisterHandler(ISerialPortHandlerFunction handler_fun) override;

    // 连接
    int Connect(const string& portname, int baudrate = 115200) override;
    int AsyncConnect(const string& portname, int baudrate = 115200) override;

    // 断开
    int Disconnect() override;

    // 写入
    int Write(const vector<char>& data) override;
    int Write(const vector<char>& data, vector<char>& response_data, int timeout_ms) override;
    int AsyncWrite(const vector<char>& data) override;

    bool IsConnected() const override;

    // 配置
    void SetConfig(const SerialConfig& cfg) { config_ = cfg; }

private:
    enum class State { Disconnected, Connecting, Connected, Reconnecting, Closing };

    struct PendingRequest
    {
        vector<char> response;
        shared_ptr<asio::steady_timer> timer;
        promise<vector<char>> promise;
    };

    void DoConnect();
    void HandleReconnect();
    void DoRead();
    void DoWrite();
    void ProcessRawData(const char* data, size_t len);
    void HandleFrame(const vector<char>& frame);
    void SendHeartbeat();
    void StopHeartbeat();
    uint16_t CRC16(const uint8_t* data, size_t len);

private:
    asio::io_context io_;
    asio::serial_port serial_;
    asio::strand<asio::io_context::executor_type> strand_;
    unique_ptr<thread> io_thread_;
    atomic<State> state_;

    string portname_;
    int baudrate_;

    ISerialPortHandler* handler_interface_;
    ISerialPortHandlerFunction handler_function_;

    array<char, 4096> read_buffer_;
    vector<char> recv_cache_;
    deque<vector<char>> write_queue_;
    map<int, shared_ptr<PendingRequest>> pending_requests_;
    atomic<int> request_id_;

    shared_ptr<asio::steady_timer> heartbeat_timer_;
    SerialConfig config_;
    int reconnect_attempt_;
    mutex write_mutex_;
};