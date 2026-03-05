#pragma once
#ifdef LIBSERIALPORT_EXPORTS
#define LIBSERIALPORT_API __declspec(dllexport)
#else
#define LIBSERIALPORT_API __declspec(dllimport)
#endif

#include <string>
#include <memory>
#include <cstddef>
#include <codecvt>
#include <thread>
#include <mutex>
#include <atomic>
#include <functional>
#include <condition_variable>

using namespace std;


typedef std::function<int(int errorcode, string errormsg)> OnConnectFunction;                              // 串口连接的回调事件
typedef std::function<int(int errorcode, string errormsg)> OnDisconnectFunction;                           // 串口断开的回调事件
typedef std::function<int(const char* data, size_t size, int errorcode, string errormsg)> OnReadFunction;  // 串口读取数据的回调事件
typedef std::function<int(const char* data, size_t size, int errorcode, string errormsg)> OnWriteFunction; // 串口写入数据的回调事件
typedef std::function<int(int errorcode, string errormsg)> OnErrorFunction;                                // 串口发生错误的回调事件

struct ISerialPortHandlerFunction
{
	OnConnectFunction    onconnectfun    = nullptr;
	OnDisconnectFunction ondisconnectfun = nullptr;
	OnReadFunction       onreadfun       = nullptr;
	OnWriteFunction      onwritefun      = nullptr;
	OnErrorFunction      onerrorfun      = nullptr;
};

class ISerialPortHandler
{
public:
	virtual int OnSerialPortConnect(int errorcode, string errormsg) = 0;                              // 串口连接的回调事件
	virtual int OnSerialPortDisconnect(int errorcode, string errormsg) = 0;                           // 串口断开的回调事件
	virtual int OnSerialPortRead(const char* data, size_t size, int errorcode, string errormsg) = 0;  // 串口读取数据的回调事件
	virtual int OnSerialPortWrite(const char* data, size_t size, int errorcode, string errormsg) = 0; // 串口写入数据的回调事件
	virtual int OnSerialPortError(int errorcode, string errormsg) = 0;                                // 串口发生错误的回调事件

	virtual ~ISerialPortHandler() {}
};

class ISerialPort
{
public:
	virtual int RegisterHandler(ISerialPortHandlerFunction handler_fun) = 0;       // 注册串口事件
	virtual int RegisterHandler(ISerialPortHandler* serialport_handler) = 0;       // 注册串口事件
	virtual int Connect(const string& portname, int baudrate = 115200) = 0;        // 连接串口
	virtual int AsyncConnect(const string& portname, int baudrate = 115200) = 0;   // 异步连接串口
	virtual int Disconnect() = 0;                                                  // 断开串口
	virtual int Write(const char* data, size_t size) = 0;                          // 同步写入串口数据
	virtual int Write(const char* data, size_t size, char** response_data, size_t& response_data_size, int timeout_ms) = 0; // 同步写入串口数据，并返回读取的数据,设置返回数据超时
	virtual int AsyncWrite(const char* data, size_t size) = 0;                     // 异步写入串口数据
	virtual bool IsConnected() const = 0;                                          // 连接状态

	virtual ~ISerialPort() {}
};

typedef string any_type;
typedef std::function<void(const std::vector<std::byte>& bytes)>     ReadBytesFunction;
typedef std::function<void(const std::string& hexstr)>               ReadHexStrFunction;
typedef std::function<void(int error_code, std::string error_msg)>   SerialErrorFunction;

class ISerialPortk
{
public:
	virtual int RegisterHandler(ReadBytesFunction read_byte_callback, ReadHexStrFunction read_hex_callback, SerialErrorFunction error_callback) = 0;
	virtual int Connect(const any_type& port) = 0;
	virtual int Disconnect() = 0;
	virtual int WriteHexStr(const string& wirte_hexstr) = 0;
	virtual int WriteHexStr(const string& wirte_hexstr, string& read_hexstr, int timeout_msec = 1000) = 0;
};



#ifdef __cplusplus
extern "C" {
#endif

	LIBSERIALPORT_API ISerialPort* NewSerialPort(void);
	typedef ISerialPort* (*NewSerialPortFun)(void);
	LIBSERIALPORT_API void DeleteSerialPort(ISerialPort* serial_port);
	typedef void (*DeleteSerialPortFun)(ISerialPort* serial_port);

	LIBSERIALPORT_API ISerialPortk* NewSerialPortk(void);
	typedef ISerialPortk* (*NewSerialPortkFun)(void);
	LIBSERIALPORT_API void DeleteSerialPortk(ISerialPortk* serial_port);
	typedef void (*DeleteSerialPortkFun)(ISerialPortk* serial_port);

#ifdef __cplusplus
}
#endif

