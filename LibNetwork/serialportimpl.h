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
#include <mutex>
#include <queue>
#include <vector>
#include <string>
#include <memory>

using namespace std;


class CSerialPortImpl : public ISerialPort
{
public:
    CSerialPortImpl();
    ~CSerialPortImpl();

    int RegisterHandler(ISerialPortHandler* handler) override;
    int Connect(const string& portname, int baudrate = 115200) override;
    int AsyncConnect(const string& portname, int baudrate = 115200) override;
    int Disconnect() override;
    int Write(const char* data, size_t size) override;
    int Write(const char* data, size_t size,
        char** response_data,
        size_t& response_data_size,
        int time_out_ms) override;
    int AsyncWrite(const char* data, size_t size) override;

private:
    void DoOpen();
    void StartRead();
    void HandleRead(const asio::error_code& ec, size_t bytes);
    void DoWrite();
    void HandleWrite(const asio::error_code& ec, size_t bytes);
    void HandleError(const asio::error_code& ec);
    void StartReconnect();

private:
    asio::io_context io_;
    asio::serial_port serial_;
    std::unique_ptr<asio::steady_timer> reconnect_timer_;
    std::thread io_thread_;

    std::atomic<bool> connected_;
    std::atomic<bool> manual_close_;
    std::atomic<bool> auto_reconnect_;

    string portname_;
    int baudrate_;
    int reconnect_delay_;

    ISerialPortHandler* handler_;

    enum { READ_BUFFER_SIZE = 2048 };
    char read_buffer_[READ_BUFFER_SIZE];

    std::mutex write_mutex_;
    std::queue<std::vector<char>> write_queue_;
};