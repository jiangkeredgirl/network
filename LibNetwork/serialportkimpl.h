/*********************************************************************
 * \file   SerialPortImplk.h
 * \brief  串口类.
 * \author 蒋珂
 * \date   2024.08.29
 *********************************************************************/
#pragma once
#include "serialportk.h"
#include <string>
#include <cstddef>
#include <memory>
#include <asio.hpp>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <functional>
#include <chrono>



// 工具函数：十六进制字符串转字节数组（补充实现）
inline std::vector<std::byte> HexStringToBytes(const std::string& hex_str)
{
    std::vector<std::byte> bytes;
    for (size_t i = 0; i < hex_str.size(); i += 2)
    {
        std::string byte_str = hex_str.substr(i, 2);
        char byte = static_cast<char>(std::strtol(byte_str.c_str(), nullptr, 16));
        bytes.push_back(static_cast<std::byte>(byte));
    }
    return bytes;
}

// 工具函数：字节数组转十六进制字符串（补充实现）
inline std::string BytesToHexString(const std::vector<std::byte>& bytes)
{
    std::stringstream ss;
    for (const auto& b : bytes)
    {
        ss << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(b & std::byte{ 0xFF });
    }
    return ss.str();
}

// 工具函数：字节数组转字符串（补充实现）
inline std::string BytesToString(const std::vector<std::byte>& bytes)
{
    std::string str;
    for (const auto& b : bytes)
    {
        str.push_back(static_cast<char>(b));
    }
    return str;
}

class CSerialPortImplk : public ISerialPortk
{
public:
    virtual int RegisterHandler(ReadBytesFunction read_byte_callback,
        ReadHexStrFunction read_hex_callback,
        SerialErrorFunction error_callback) override
    {
        m_read_bytes_callback = read_byte_callback;
        m_read_hexstr_callback = read_hex_callback;
        m_error_callback = error_callback;
        return 0;
    }

    virtual int Connect(const any_type& port) override
    {
        return open(port);
    }

    virtual int Disconnect() override
    {
        return close();
    }

    virtual int WriteHexStr(const std::string& wirte_hexstr) override
    {
        return writeHexStr(wirte_hexstr);
    }

    virtual int WriteHexStr(const std::string& wirte_hexstr,
        std::string& read_hexstr,
        int timeout_msec = 1000) override
    {
        return writeHexStr(wirte_hexstr, read_hexstr, timeout_msec);
    }

public:
    CSerialPortImplk()
        : m_read_cancel(false)
        , m_is_read_wait(false)
    {
        // 使用智能指针管理串口对象，避免裸指针泄漏
        m_serial = std::make_unique<asio::serial_port>(m_ios);
    }

    virtual ~CSerialPortImplk() override
    {
        close();
        // unique_ptr 自动释放 m_serial，无需手动 delete
    }

public:
    // 打开串口
    int open(const any_type& port)
    {
        int error_code = 1;
        if (!m_serial)
        {
            std::cerr << "Serial port object is null!" << std::endl;
            return error_code;
        }

        try
        {
            // 关闭已打开的串口
            if (m_serial->is_open())
            {
                m_serial->close(m_ec);
            }

            // 打开串口
            m_serial->open(port, m_ec);
            if (m_ec)
            {
                std::cerr << "Open serial port failed: " << m_ec.message() << std::endl;
                return error_code;
            }

            if (m_serial->is_open())
            {
                std::cout << "Serial port " << port << " opened successfully." << std::endl;

                // 配置串口参数
                setSerialOptions();

                // 启动 IO 上下文线程（异步操作依赖）
                startIoContextThread();

                // 启动读取线程
                startReadThread();

                error_code = 0;
                m_port = port;
            }
            else
            {
                std::cerr << "Serial port is not open after open call!" << std::endl;
            }
        }
        catch (const std::exception& err)
        {
            std::cerr << "Open serial port exception: " << err.what() << std::endl;
        }

        return error_code;
    }

    // 关闭串口
    int close()
    {
        // 停止读取线程
        m_read_cancel = true;
        if (m_read_thread.joinable())
        {
            m_read_thread.join();
        }

        // 取消串口异步操作并关闭
        if (m_serial && m_serial->is_open())
        {
            asio::error_code ec;
            m_serial->cancel(ec);
            if (ec)
            {
                std::cerr << "Cancel serial port failed: " << ec.message() << std::endl;
            }

            m_serial->close(ec);
            if (ec)
            {
                std::cerr << "Close serial port failed: " << ec.message() << std::endl;
            }
        }

        // 停止 IO 上下文
        m_ios.stop();
        if (m_io_thread.joinable())
        {
            m_io_thread.join();
        }

        std::cout << "Serial port closed successfully." << std::endl;
        return 0;
    }

public:
    // 写入十六进制字符串（无返回读取）
    int writeHexStr(const std::string& hexstr)
    {
        m_is_read_wait = false;
        std::vector<std::byte> bytes = HexStringToBytes(hexstr);
        return write(bytes);
    }

    // 写入十六进制字符串并等待读取返回
    int writeHexStr(const std::string& wirte_hexstr,
        std::string& read_hexstr,
        int timeoutMsec = 1000)
    {
        int error_code = 1;
        int try_count = 3;

        do
        {
            m_is_read_wait = true;
            std::vector<std::byte> write_bytes = HexStringToBytes(wirte_hexstr);

            // 写入数据
            error_code = write(write_bytes);
            if (error_code != 0)
            {
                std::cerr << "Write hex string failed, retry count: " << try_count << std::endl;
                continue;
            }

            // 等待读取返回
            std::vector<std::byte> read_bytes;
            error_code = waitRead(read_bytes, timeoutMsec);
            if (error_code != 0)
            {
                std::cerr << "Wait read failed, retry count: " << try_count << std::endl;
                continue;
            }

            // 转换为十六进制字符串
            read_hexstr = BytesToHexString(read_bytes);
            if (read_hexstr.empty())
            {
                error_code = 1;
                std::cerr << "Read hex string is empty, retry count: " << try_count << std::endl;
                continue;
            }

            // 操作成功
            m_is_read_wait = false;
            error_code = 0;
            break;

        } while (try_count-- > 0);

        return error_code;
    }

private:
    // 配置串口参数
    void setSerialOptions()
    {
        // 波特率 115200
        m_serial->set_option(asio::serial_port::baud_rate(115200), m_ec);
        if (m_ec) std::cerr << "Set baud rate failed: " << m_ec.message() << std::endl;

        // 无流控
        m_serial->set_option(asio::serial_port::flow_control(asio::serial_port::flow_control::none), m_ec);
        if (m_ec) std::cerr << "Set flow control failed: " << m_ec.message() << std::endl;

        // 无奇偶校验
        m_serial->set_option(asio::serial_port::parity(asio::serial_port::parity::none), m_ec);
        if (m_ec) std::cerr << "Set parity failed: " << m_ec.message() << std::endl;

        // 1个停止位
        m_serial->set_option(asio::serial_port::stop_bits(asio::serial_port::stop_bits::one), m_ec);
        if (m_ec) std::cerr << "Set stop bits failed: " << m_ec.message() << std::endl;

        // 8位数据位
        m_serial->set_option(asio::serial_port::character_size(8), m_ec);
        if (m_ec) std::cerr << "Set character size failed: " << m_ec.message() << std::endl;
    }

    // 启动 IO 上下文线程
    void startIoContextThread()
    {
        if (!m_io_thread.joinable())
        {
            m_io_thread = std::thread([this]() {
                // work 对象防止 io_context 无任务时退出
                asio::io_context::work work(m_ios);
                m_ios.run();
                });
        }
    }

    // 启动读取线程
    void startReadThread()
    {
        if (!m_read_thread.joinable())
        {
            m_read_cancel = false;
            m_read_thread = std::thread([this]() {
                while (!m_read_cancel)
                {
                    std::vector<std::byte> bytes;
                    read(bytes);

                    // 通知等待读取的线程
                    std::lock_guard<std::mutex> lock(m_read_wait_mutex);
					if (m_is_read_wait)
					{
                        std::cout << "请求应答数据已返回" << std::endl;
						m_is_read_wait = false;
                        m_responsed_bytes = m_readed_bytes;
						m_read_wait_condition.notify_all();
					}
                    // 避免过度占用 CPU
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
                });
        }
    }

    // 同步写入数据
    int write(const std::vector<std::byte>& bytes)
    {
        int error_code = 1;
        if (!m_serial || !m_serial->is_open())
        {
            std::cerr << "Serial port is not open (write)!" << std::endl;
            return error_code;
        }

        std::lock_guard<std::mutex> lock(m_mutex);
        std::string data = BytesToString(bytes);

        // 打印写入数据
        std::stringstream write_ss;
        write_ss << "**********serial write  " << bytes.size() << " bytes: ";
        for (size_t i = 0; i < bytes.size(); ++i)
        {
            write_ss << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(bytes[i] & std::byte{ 0xFF }) << " ";
        }
        std::cout << write_ss.str() << std::endl;

        int try_count = 3;
        do
        {
            size_t writed_size = m_serial->write_some(asio::buffer(data), m_ec);

            // 打印实际写入的数据
            std::stringstream writed_ss;
            writed_ss << "**********serial writed " << writed_size << " bytes: ";
            for (size_t i = 0; i < writed_size; ++i)
            {
                writed_ss << std::hex << std::setw(2) << std::setfill('0')
                    << static_cast<int>(data[i] & 0xFF) << " ";
            }
            std::cout << writed_ss.str() << std::endl;

            // 写入成功（字节数匹配）
            if (writed_size == bytes.size())
            {
                error_code = 0;
                std::cout << "Write data success!" << std::endl;
                break;
            }

            // 重试次数耗尽
            if (--try_count == 0)
            {
                std::cerr << "Write data failed after 3 retries! " << m_ec.message() << std::endl;
                if (m_error_callback)
                {
                    m_error_callback(m_ec.value(), m_ec.message());
                }
                break;
            }

            // 短暂休眠后重试
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

        } while (true);

        return error_code;
    }

    // 异步写入数据（备用接口）
    void asyncWrite(const std::vector<std::byte>& bytes)
    {
        if (!m_serial || !m_serial->is_open())
        {
            std::cerr << "Serial port is not open (async write)!" << std::endl;
            return;
        }

        m_serial->async_write_some(
            asio::buffer(bytes.data(), bytes.size()),
            std::bind(&CSerialPortImplk::writeCallback, this,
                std::placeholders::_1, std::placeholders::_2)
        );
    }

    // 异步写入回调
    void writeCallback(asio::error_code ec, std::size_t written)
    {
        if (ec)
        {
            std::cerr << "Async write error: " << ec.message() << std::endl;
            if (m_error_callback)
            {
                m_error_callback(ec.value(), ec.message());
            }
        }
        else
        {
            std::cout << "Async write success, written bytes: " << written << std::endl;
        }
    }

    // 等待读取数据（带超时）
    int waitRead(std::vector<std::byte>& bytes, int timeoutMsec)
    {
        int error_code = 1;
        int try_count = 3;

        do
        {
            std::unique_lock<std::mutex> lock(m_read_wait_mutex);
            // 等待条件满足或超时
            bool status = m_read_wait_condition.wait_for(
                lock,
                std::chrono::milliseconds(timeoutMsec),
                [this]() { return !m_is_read_wait; }
            );

            if (status)
            {
                std::cout << "Wait read completed" << std::endl;
                if (!m_responsed_bytes.empty())
                {
                    bytes = m_responsed_bytes;
                    m_responsed_bytes.clear();  // 清空已读取数据，避免重复使用
                    error_code = 0;
                    std::cout << "Read success, bytes count: " << bytes.size() << std::endl;
                    break;
                }
                else
                {
                    std::cerr << "Read failed: no data received" << std::endl;
                }
            }
            else
            {
                std::cerr << "Read failed: timeout (" << timeoutMsec << "ms)" << std::endl;
            }

        } while (try_count-- > 0);

        return error_code;
    }

    // 同步读取数据
    int read(std::vector<std::byte>& bytes)
    {
        int error_code = 1;
        if (!m_serial || !m_serial->is_open())
        {
            std::cerr << "Serial port is not open (read)!" << std::endl;
            return error_code;
        }

        bytes.clear();
        m_readed_bytes.clear();

        char read_buf[1024] = { 0 };
        try
        {
            // 同步读取（非阻塞？asio::serial_port::read_some 是阻塞的，直到有数据或错误）
            size_t readed_size = m_serial->read_some(asio::buffer(read_buf), m_ec);

            if (m_ec)
            {
                if (m_ec != asio::error::operation_aborted)
                {
                    std::cerr << "Read error: " << m_ec.message() << std::endl;
                    if (m_error_callback)
                    {
                        m_error_callback(m_ec.value(), m_ec.message());
                    }
                }
                return error_code;
            }

            if (readed_size > 0)
            {
                // 处理读取到的数据
                std::stringstream read_ss;
                read_ss << "**********serial readed " << readed_size << " bytes: ";
                for (size_t i = 0; i < readed_size; ++i)
                {
                    read_ss << std::hex << std::setw(2) << std::setfill('0')
                        << static_cast<int>(read_buf[i] & 0xFF) << " ";
                    bytes.push_back(static_cast<std::byte>(read_buf[i]));
                }
                std::cout << read_ss.str() << std::endl;

                // 非等待模式下触发回调
                if (!m_is_read_wait)
                {
                    if (m_read_bytes_callback)
                    {
                        m_read_bytes_callback(bytes);
                    }
                    if (m_read_hexstr_callback)
                    {
                        m_read_hexstr_callback(BytesToHexString(bytes));
                    }
                }

                // 保存读取的数据供 waitRead 使用
                m_readed_bytes = bytes;               
                error_code = 0;
            }
            else
            {
                std::cerr << "Read 0 bytes!" << std::endl;
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "Read exception: " << e.what() << std::endl;
        }

        return error_code;
    }

    // 异步读取数据（备用接口）
    void asyncRead()
    {
        if (!m_serial || !m_serial->is_open())
        {
            std::cerr << "Serial port is not open (async read)!" << std::endl;
            return;
        }

        m_serial->async_read_some(
            asio::buffer(m_read_buffer),
            std::bind(&CSerialPortImplk::readCallback, this,
                std::placeholders::_1, std::placeholders::_2)
        );
    }

    // 异步读取回调
    void readCallback(asio::error_code ec, std::size_t bytes_transferred)
    {
        std::vector<std::byte> bytes;

        if (ec)
        {
            if (ec != asio::error::operation_aborted)
            {
                std::cerr << "Async read error: " << ec.message() << std::endl;
                if (m_error_callback)
                {
                    m_error_callback(ec.value(), ec.message());
                }
            }
            return;
        }

        // 处理读取到的数据
        std::stringstream read_ss;
        read_ss << "**********serial async readed " << bytes_transferred << " bytes: ";
        for (size_t i = 0; i < bytes_transferred; ++i)
        {
            read_ss << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(m_read_buffer[i] & 0xFF) << " ";
            bytes.push_back(static_cast<std::byte>(m_read_buffer[i]));
        }
        std::cout << read_ss.str() << std::endl;

        // 触发回调
        if (m_read_bytes_callback)
        {
            m_read_bytes_callback(bytes);
        }
        if (m_read_hexstr_callback)
        {
            m_read_hexstr_callback(BytesToHexString(bytes));
        }

        // 保存数据供 waitRead 使用
        m_readed_bytes = bytes;

        // 继续异步读取
        if (!m_read_cancel)
        {
            asyncRead();
        }
    }

private:
    asio::io_context        m_ios;                  // IO 上下文（替代过时的 io_service）
    std::unique_ptr<asio::serial_port> m_serial;    // 串口对象（智能指针管理）
    any_type                m_port;                 // 串口名
    asio::error_code        m_ec;                   // 错误码
    std::atomic<bool>       m_read_cancel;          // 读取线程取消标志
    std::thread             m_read_thread;          // 读取线程
    std::thread             m_io_thread;            // IO 上下文线程
    ReadBytesFunction       m_read_bytes_callback;  // 字节读取回调
    ReadHexStrFunction      m_read_hexstr_callback; // 十六进制字符串读取回调
    SerialErrorFunction     m_error_callback;       // 错误回调
    char                    m_read_buffer[1024]{};  // 读取缓冲区
    std::vector<std::byte>  m_readed_bytes;         // 已读取的数据
    std::vector<std::byte>  m_responsed_bytes;      // 请求-响应的数据
    std::mutex              m_read_wait_mutex;      // 读取等待互斥锁
    std::condition_variable m_read_wait_condition;  // 读取等待条件变量
    std::atomic<bool>       m_is_read_wait;         // 读取等待标志（原子变量）
    std::mutex              m_mutex;                // 写入互斥锁
};