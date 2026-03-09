/*********************************************************************
 * \file   SerialPortImplk.h
 * \brief  串口类.
 * \author 蒋珂
 * \date   2024.08.29
 *********************************************************************/
#pragma once


#include <string>
#include <cstddef>
#include <memory>
#include <asio.hpp>
#include "kutility.h"
#include "kcommon.h"
#include "serialportk.h"
using namespace std;
using namespace asio;


class CSerialPortImplk : public ISerialPortk
{

public:
    virtual int RegisterHandler(ReadBytesFunction read_byte_callback, ReadHexStrFunction read_hex_callback, SerialErrorFunction error_callback) override
    {
        m_read_bytes_callback  = read_byte_callback;
        m_read_hexstr_callback = read_hex_callback;        
        m_error_callback       = error_callback;
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
    virtual int WriteHexStr(const string& wirte_hexstr) override
    {
        return writeHexStr(wirte_hexstr);
    }
    virtual int WriteHexStr(const string& wirte_hexstr, string& read_hexstr, int timeout_msec = 1000) override
    {
        return writeHexStr(wirte_hexstr, read_hexstr, timeout_msec);
    }

public:
    CSerialPortImplk()
    {

        m_serial = new asio::serial_port( m_ios );
        // if ( m_serial )
        // {
        //     open( port_name);
        // }
    }
    virtual ~CSerialPortImplk()
    {
        close();
        if( m_serial )
        {
            delete m_serial;
            m_serial = nullptr;
        }
    }
public:
    //opn com port
    int open( const any_type &port )
    {
        int error_code = 1;
        if ( m_serial )
        {            
            try
            {
                //Open Serial port object
                m_serial->open( port, m_ec );
                if (m_serial->is_open())
                {
                    std::cout << "Serial port opened successfully." << std::endl;
                    //Set port argument
                    m_serial->set_option( asio::serial_port::baud_rate( 115200 ), m_ec );
                    m_serial->set_option( asio::serial_port::flow_control( asio::serial_port::flow_control::none ), m_ec );
                    m_serial->set_option( asio::serial_port::parity( asio::serial_port::parity::none ), m_ec );
                    m_serial->set_option( asio::serial_port::stop_bits( asio::serial_port::stop_bits::one ), m_ec);
                    m_serial->set_option( asio::serial_port::character_size( 8 ), m_ec);
                    run();
                    if(!m_read_thread.joinable())
                    {
                        m_read_cancel = false;
                        m_read_thread = std::thread([this]()
                                                    {
                                                        while(!m_read_cancel)
                                                        {
                                                            //There can use deadline_timer to cancle serial_port read data
                                                            //asyncRead();
                                                            std::vector<std::byte> bytes;
                                                            read(bytes);
                                                            m_is_read_wait = false;
                                                            m_read_wait_condition.notify_all();
                                                            std::this_thread::sleep_for(std::chrono::milliseconds(10));
                                                        }
                                                    });
                    }
                    error_code = 0;
                }
                else
                {
                    std::cerr << "Serial port is not open." << std::endl;
                    error_code = 1;
                }
            }
            catch (exception& err)
            {
                cout << "Exception Error: " << err.what() << endl;
                error_code = 1;
            }
        }
        return error_code;
    }

    /// close com port
    int close()
    {
        m_read_cancel = true;
        if( m_serial->is_open() )
        {
            m_serial->cancel();
        }
        if (m_read_thread.joinable())
        {
            m_read_thread.join();
        }
        if( m_serial )
        {
            m_serial->close();
        }
        return 0;
    }

public:
    //Write some data to port
    int writeHexStr( const string& hexstr )
    {
        m_is_read_wait = false;
        std::vector<std::byte> bytes = HexStringToBytes(hexstr);
        return write(bytes);
    }

    int writeHexStr( const string& wirte_hexstr, string& read_hexstr, int timeoutMsec = 100 )
    {
        int error_code = 1;
        int try_count = 3;
        do
        {
            m_is_read_wait = true;
            std::vector<std::byte> bytes = HexStringToBytes(wirte_hexstr);
            error_code = write(bytes);
            if(error_code)
            {
                continue;
            }
            error_code = waitReadHexStr(read_hexstr, timeoutMsec);
            if(error_code)
            {
                continue;
            }
            if(read_hexstr.empty())
            {
                error_code = 1;
                continue;
            }
            m_is_read_wait = false;
            if(error_code == 0)
            {
                break;
            }
        }while(try_count-- > 0);
        return error_code;
    }

private:
    int waitReadHexStr(string& hexstr, int timeoutMsec)
    {
        int error_code = 0;
        std::vector<std::byte> bytes;
        error_code = waitRead(bytes, timeoutMsec);
        hexstr = BytesToHexString(bytes);
        return error_code;
    }

private:
    //Write some data to port
    int write( const std::vector<std::byte>& bytes )
    {
        int error_code = 1;
        lock_guard<mutex> lock(m_mutex);
        std::stringstream ss;
        ss << "**********serial write  " << bytes.size() << " bytes: ";
        for (size_t i = 0; i < bytes.size(); ++i)
        {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[i]) << " ";
        }
        std::cout << ss.str() << endl;
        //LOG_INFO(ss.str());
        string data = BytesToString(bytes);
        int try_count = 3;
        do
        {
            size_t writed_size = m_serial->write_some(asio::buffer( data, data.size() ), m_ec );
            std::stringstream ss;
            ss << "**********serial writed " << writed_size << " bytes: ";
            for (size_t i = 0; i < writed_size; ++i)
            {
                ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i] & 0xFF) << " ";
            }
            std::cout << ss.str() << endl;
            //LOG_INFO(ss.str());
            if(writed_size == bytes.size())
            {
                error_code = 0;
                std::cout << "write_some succss" << endl;
                //LOG_INFO("write_some succss");
                break;
            }
            if(--try_count == 0)
            {
                error_code = 1;
                std::cerr << "write_some fail" << endl;
                //LOG_ERROR("write_some fail");
                if (m_error_callback)
                {
                    m_error_callback(m_ec.value(), m_ec.message());
                }
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }while(1);
        //std::this_thread::sleep_for(std::chrono::milliseconds(30));
        return error_code;
    }

    //async Write some data to port
    void asyncWrite(const std::vector<std::byte>& bytes)
    {
        m_serial->async_write_some(asio::buffer(static_cast<const void*>(bytes.data()), bytes.size()), std::bind(&CSerialPortImplk::writeCallback, this, std::placeholders::_1, std::placeholders::_2));
    }

    //The asyanc callback function of asyanc_write
    void writeCallback(asio::error_code ec, std::size_t written)
    {
        std::cout << "\nhandle_write: ";
        if (ec)
        {
            std::cerr << "Error on write: " << ec.message() << std::endl;
            if (m_error_callback)
            {
                m_error_callback(ec.value(), ec.message());
            }
        }
        else if (!ec)
        {
            std::cout << "Written bytes: " << written << std::endl;
        }
    }

 private:
    int waitRead(std::vector<std::byte>& bytes, int timeoutMsec)
    {
        int error_code = 0;
        int try_count = 3;
        do
        {
            /*std::cv_status*/ bool status;
            unique_lock<mutex> lock(m_read_wait_mutex);
            status = m_read_wait_condition.wait_for(lock, std::chrono::milliseconds(timeoutMsec), [&] { return !m_is_read_wait; });
            if (status/* == std::cv_status::no_timeout*/)
            {
                std::cout << "waitRread completed" << std::endl;
                if(m_readed_bytes.empty())
                {
                    std::cerr << "Error on read, readed bytes is 0 " << std::endl;
                    error_code = 1;
                }
                else
                {
                    std::cout << "Success on read, readed bytes is " << m_readed_bytes.size() << std::endl;
                    bytes = m_readed_bytes;
                    error_code = 0;
                    break;
                }
            }
            else/* if (status == std::cv_status::timeout)*/
            {
                std::cerr << "waitRread timeout" << std::endl;
                error_code = 1;
            }
        }
        while(try_count-- > 0);
        return error_code;
    }

private:
    //Read data from port which write data just now
    int read(std::vector<std::byte>& bytes)
    {
        int error_code = 0;
        m_readed_bytes.clear();
        //char v[10];
        //read( *m_serial, asio::buffer(v) );
        // 创建一个缓冲区来存储读取的数据
        char read_buf[1024] = {0};
        // 开始同步读取操作，阻塞直到读取到数据
        size_t readed_size = m_serial->read_some(asio::buffer(read_buf), m_ec);
        // 检查读取操作是否成功
        if (m_ec || readed_size == 0)
        {
            std::cerr << "Error on receive: " << m_ec.message() << std::endl;
            //LOG_ERROR("read_some fail");
            error_code = 1;
            if(m_error_callback)
            {
                m_error_callback(m_ec.value(), m_ec.message());
            }
        }
        else
        {
            std::stringstream ss;
            ss << "**********serial readed " << readed_size << " bytes: ";
            for (size_t i = 0; i < readed_size; ++i)
            {
                ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(read_buf[i] & 0xFF) << " ";
                bytes.push_back(static_cast<std::byte>(read_buf[i]));
            }
            std::cout << ss.str() << std::endl;
            //LOG_INFO(ss.str());
            if(!m_is_read_wait)
            {
                string hexstr = BytesToHexString(bytes);
                if(m_read_bytes_callback)
                {
                    m_read_bytes_callback(bytes);
                }
                if(m_read_hexstr_callback)
                {
                    m_read_hexstr_callback(hexstr);
                }
            }
            m_readed_bytes = bytes;
            error_code = 0;
        }
        return error_code;
    }

    //Read data from port which write data just now
    void asyncRead()
    {        
        m_serial->async_read_some(asio::buffer(m_read_buffer), std::bind( &CSerialPortImplk::readCallback, this, m_read_buffer, std::placeholders::_1, std::placeholders::_2) );
    }

    //The asyanc callback function of asyanc_read
    void readCallback( char read_buf[], asio::error_code ec, std::size_t bytes_transferred )
    {
        cout << "\nhandle_read: ";
        cout.write(read_buf, bytes_transferred);
        std::vector<std::byte> bytes;
        if (ec)
        {
            std::cerr << "Error on receive: " << ec.message() << std::endl;
            if(m_error_callback)
            {
                m_error_callback(ec.value(), ec.message());
            }
        }
        else
        {
            std::stringstream ss;
            ss << "**********serial async readed " << bytes_transferred << " bytes: ";
            for (size_t i = 0; i < bytes_transferred; ++i)
            {
                ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(read_buf[i] & 0xFF) << " ";
                bytes.push_back(static_cast<std::byte>(read_buf[i]));
            }
            std::cout << ss.str() << std::endl;
            //LOG_INFO(ss.str());
            string hexstr = BytesToHexString(bytes);
            if(m_read_bytes_callback)
            {
                m_read_bytes_callback(bytes);
            }
            if(m_read_hexstr_callback)
            {
                m_read_hexstr_callback(hexstr);
            }
            m_readed_bytes = bytes;
            if(!m_read_cancel)
            {
                // 继续异步读取数据
                asyncRead();
            }
        }
    }

    //To Call io_service::run function to call asyanc callback funciton
    void run()
    {
        //Wait for call callback function
        m_ios.reset();
        m_ios.restart();
        m_ios.run();      
    }

private:
    asio::io_service       m_ios; //io_service Object
    asio::serial_port*     m_serial = nullptr; //Serial port Object
    any_type               m_port; // For save com name
    asio::error_code       m_ec;   //Serial_port function exception
    atomic<bool>           m_read_cancel;
    std::thread            m_read_thread;
    ReadBytesFunction      m_read_bytes_callback  = nullptr;
    ReadHexStrFunction     m_read_hexstr_callback = nullptr;
    SerialErrorFunction    m_error_callback       = nullptr;
    char                   m_read_buffer[1024] = {0}; // 数据缓冲区
    std::vector<std::byte> m_readed_bytes;
    mutex                  m_read_wait_mutex;
    condition_variable     m_read_wait_condition;
    bool                   m_is_read_wait = false;
    mutex                  m_mutex;
};
