/*********************************************************************
 * \file   SerialPortImpl.h
 * \brief  串口类.
 * \author 蒋珂
 * \date   2024.08.29
 *********************************************************************/

#pragma once

#include "serialport.h"

#define ASIO_STANDALONE
#include <asio.hpp>

#include <thread>
#include <atomic>
#include <deque>
#include <map>
#include <future>
#include <vector>
#include <memory>
#include <iostream>

class CSerialPortImpl : public ISerialPort
{
public:
    CSerialPortImpl();
    virtual ~CSerialPortImpl();

    int RegisterHandler(ISerialPortHandler* handler) override;

    int Connect(const std::string& portname, int baudrate = 115200) override;
    int AsyncConnect(const std::string& portname, int baudrate = 115200) override;
    int Disconnect() override;

    int Write(const char* data, size_t size) override;

    int Write(const char* data,
        size_t size,
        char** response_data,
        size_t& response_data_size,
        int timeout_ms) override;

    int AsyncWrite(const char* data, size_t size) override;

    virtual bool IsConnected() const override;

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
        uint16_t id = 0;
        std::vector<char> response;
        std::shared_ptr<asio::steady_timer> timer;
        std::promise<std::vector<char>> promise;
        std::atomic_bool completed{ false };
    };

private:

    void DoConnect();
    void DoRead();
    void ProcessRawData(const char* data, size_t len);
    void HandleFrame(const std::vector<char>& frame);
    void DoWrite();
    void HandleReconnect();

    void StartHeartbeat();
    void StopHeartbeat();
    void SendHeartbeat();

    void FailAllPending();

    uint16_t GenerateRequestID();
    uint16_t CRC16(const uint8_t* data, size_t len);
    // 设置帧配置
    void SetFrameConfig(bool use_frame, uint8_t head = 0xAA, uint8_t tail = 0x55);
    // 是否启用心跳
    void SetHeartbeatEnable(bool enable);

private:

    asio::io_context io_;
    asio::serial_port serial_;
    asio::strand<asio::io_context::executor_type> strand_;
    std::unique_ptr<std::thread> io_thread_;

    std::atomic<State> state_;

    std::string portname_;
    int baudrate_;

    ISerialPortHandler* handler_;

    std::array<char, 4096> read_buffer_;
    std::vector<char> recv_cache_;
    std::deque<std::vector<char>> write_queue_;

    std::map<uint16_t, std::shared_ptr<PendingRequest>> pending_requests_;
    std::atomic<uint16_t> request_id_;

    std::shared_ptr<asio::steady_timer> heartbeat_timer_;

    int heartbeat_interval_ms_;
    bool enable_crc_;
    int reconnect_attempt_;
private:
    bool use_frame_;               // 是否启用帧头帧尾
    uint8_t frame_head_;           // 帧头
    uint8_t frame_tail_;           // 帧尾
private:
    bool enable_heartbeat_;  // 是否启用心跳
};