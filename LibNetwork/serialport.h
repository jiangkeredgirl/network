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


typedef std::function<int(std::error_code ec)> OnConnectFunction;                         // 串口连接的回调事件
typedef std::function<int(std::error_code ec)> OnDisconnectFunction;                      // 串口断开的回调事件
typedef std::function<int(const vector<char>& data, std::error_code ec)> OnReadFunction;  // 串口读取数据的回调事件
typedef std::function<int(const vector<char>& data, std::error_code ec)> OnWriteFunction; // 串口写入数据的回调事件
typedef std::function<int(std::error_code ec)> OnErrorFunction;                           // 串口发生错误的回调事件

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
	virtual int OnConnect(std::error_code ec) = 0;                         // 串口连接的回调事件
	virtual int OnDisconnect(std::error_code ec) = 0;                      // 串口断开的回调事件
	virtual int OnRead(const vector<char>& data, std::error_code ec) = 0;  // 串口读取数据的回调事件
	virtual int OnWrite(const vector<char>& data, std::error_code ec) = 0; // 串口写入数据的回调事件
	virtual int OnError(std::error_code ec) = 0;                           // 串口发生错误的回调事件
	virtual ~ISerialPortHandler() = default;
};

class ISerialPort
{
public:
	virtual int RegisterHandler(ISerialPortHandlerFunction handler_fun) = 0;                      // 注册串口事件
	virtual int RegisterHandler(ISerialPortHandler* serialport_handler) = 0;                      // 注册串口事件
	virtual int Connect(const string& portname, int baudrate = 115200) = 0;                       // 连接串口
	virtual int AsyncConnect(const string& portname, int baudrate = 115200) = 0;                  // 异步连接串口
	virtual int Disconnect() = 0;                                                                 // 断开串口
	virtual int Write(const vector<char>& data) = 0;                                              // 同步写入串口数据
	virtual int Write(const vector<char>& data, vector<char>& response_data, int timeout_ms) = 0; // 同步写入串口数据，并返回读取的数据,设置返回数据超时
	virtual int AsyncWrite(const vector<char>& data) = 0;                                         // 异步写入串口数据
	virtual bool IsConnected() const = 0;                                                         // 连接状态
	virtual ~ISerialPort() = default;
};



#ifdef __cplusplus
extern "C" {
#endif

	LIBSERIALPORT_API ISerialPort* NewSerialPort(void);
	typedef ISerialPort* (*NewSerialPortFun)(void);
	LIBSERIALPORT_API void DeleteSerialPort(ISerialPort* serial_port);
	typedef void (*DeleteSerialPortFun)(ISerialPort* serial_port);

#ifdef __cplusplus
}
#endif

