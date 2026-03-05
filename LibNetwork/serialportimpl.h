/*********************************************************************
 * \file   SerialPortImpl.h
 * \brief  工业级串口类.
 * \author 蒋珂
 * \date   2026.03.04
 *********************************************************************/
#pragma once

#include "serialport.h"
#include "SerialConfig.h"
#include "SerialTypes.h"
#include "SerialFrame.h"
#include "CRC16.h"
#include "RingBuffer.h"
#include "SerialLogger.h"
#include "SerialError.h"
#include <asio.hpp>
#include <thread>
#include <deque>
#include <map>
#include <atomic>

class CSerialPortImpl : public ISerialPort
{
public:
    CSerialPortImpl();
    ~CSerialPortImpl();

    int RegisterHandler(ISerialPortHandlerFunction handler_fun) override;
    int RegisterHandler(ISerialPortHandler* handler) override;
    int Connect(const std::string& portname, int baudrate = 115200) override;
    int AsyncConnect(const std::string& portname, int baudrate = 115200) override;
    int Disconnect() override;
    int Write(const std::vector<char>& data) override;
    int Write(const std::vector<char>& data, std::vector<char>& response_data, int timeout_ms) override;
    int AsyncWrite(const std::vector<char>& data) override;
    bool IsConnected() const override;

private:
    void DoConnect();
    void DoRead();
    void DoWrite();
    void HandleReconnect();
    void ProcessRawData();

private:
    asio::io_context io_;
    asio::serial_port serial_;
    asio::strand<asio::io_context::executor_type> strand_;
    std::unique_ptr<std::thread> io_thread_;
    std::atomic<bool> connected_;
    std::string portname_;
    int baudrate_;
    ISerialPortHandler* handler_;
    ISerialPortHandlerFunction handler_fun_;

    RingBuffer ringbuf_;
    std::deque<std::vector<char>> write_queue_;
    std::vector<char> recv_cache_;

    SerialConfig config_;
    std::atomic<bool> running_;
};