/*********************************************************************
 * \file   SerialPortImpl.h
 * \brief  工业级串口类.
 * \author 蒋珂
 * \date   2026.03.04
 *********************************************************************/

#pragma once

#include "serialport.h"

#include "SerialConfig.h"
#include "SerialError.h"
#include "SerialLogger.h"
#include "SerialRequest.h"

#include <asio.hpp>
#include <deque>
#include <thread>

class CSerialPortImpl : public ISerialPort
{
public:

    CSerialPortImpl();

    ~CSerialPortImpl();

public:

    int RegisterHandler(ISerialPortHandlerFunction handler_fun) override;

    int RegisterHandler(ISerialPortHandler* handler) override;

    int Connect(const string& portname, int baudrate) override;

    int AsyncConnect(const string& portname, int baudrate) override;

    int Disconnect() override;

    int Write(const vector<char>& data) override;

    int Write(const vector<char>& data,
        vector<char>& response,
        int timeout_ms) override;

    int AsyncWrite(const vector<char>& data) override;

    bool IsConnected() const override;

private:

    void StartRead();

    void HandleRead(const std::error_code& ec, size_t bytes);

    void DoReconnect();

    void ProcessRequestQueue();

    void CompleteRequest(const vector<char>& data);

private:

    asio::io_context io_;

    asio::serial_port port_;

    std::thread io_thread_;

    std::atomic<bool> connected_{ false };

    std::atomic<bool> stopping_{ false };

    string port_name_;

    SerialConfig config_;

private:

    ISerialPortHandler* handler_interface_ = nullptr;

    ISerialPortHandlerFunction handler_fun_;

private:

    std::mutex write_mutex_;

    std::deque<std::shared_ptr<SerialRequest>> request_queue_;

private:

    std::vector<char> read_buffer_;

    char temp_read_[1024];

private:

    std::unique_ptr<asio::steady_timer> reconnect_timer_;
};
