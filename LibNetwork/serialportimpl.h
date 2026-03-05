/*********************************************************************
 * \file   SerialPortImpl.h
 * \brief  工业级串口类.
 * \author 蒋珂
 * \date   2026.03.04
 *********************************************************************/
#pragma once

#include "serialport.h"   // ISerialPort 和 ISerialPortHandler 已在其他文件声明

#define ASIO_STANDALONE
#include <asio.hpp>
#include <thread>
#include <atomic>
#include <deque>
#include <vector>
#include <iostream>
#include <future>

using namespace std;

class CSerialPortImpl : public ISerialPort
{
public:
    CSerialPortImpl();
    ~CSerialPortImpl();

    // ---------------- 接口实现 ----------------
    int RegisterHandler(ISerialPortHandler* handler) override;

    int Connect(const string& portname, int baudrate = 115200) override;
    int AsyncConnect(const string& portname, int baudrate = 115200) override;
    int Disconnect() override;

    int Write(const char* data, size_t size) override;

    int Write(const char* data,
        size_t size,
        char** response_data,
        size_t& response_data_size,
        int timeout_ms) override;

    int AsyncWrite(const char* data, size_t size) override;

    bool IsConnected() const override;

    // ---------------- 工业级特性 ----------------
    void EnableCRC(bool enable);
    void SetFrameHeadTail(bool enable, char head = 0xAA, char tail = 0x55);
    void EnableHeartbeat(bool enable, int interval_ms = 3000);

private:
    enum class State
    {
        Disconnected,
        Connecting,
        Connected,
        Reconnecting,
        Closing
    };

    struct PendingRequest
    {
        vector<char> response;
        shared_ptr<asio::steady_timer> timer;
        promise<vector<char>> promise;
        atomic<bool> completed{ false };
    };

private:
    void DoConnect();
    void DoRead();
    void ProcessRawData(const char* data, size_t len);
    void HandleFrame(const vector<char>& frame);
    void DoWrite();
    void HandleReconnect();

    void StartHeartbeat();
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
    ISerialPortHandler* handler_;

    array<char, 4096> read_buffer_;
    vector<char> recv_cache_;
    deque<vector<char>> write_queue_;

    deque<shared_ptr<PendingRequest>> pending_requests_queue_;

    // 帧/CRC/心跳
    bool enable_crc_;
    bool use_frame_;
    char frame_head_;
    char frame_tail_;
    shared_ptr<asio::steady_timer> heartbeat_timer_;
    int heartbeat_interval_ms_;
    bool heartbeat_enable_;
    int reconnect_attempt_;
};