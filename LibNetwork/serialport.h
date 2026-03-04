#pragma once
#ifdef LIBSERIALPORT_EXPORTS
#define LIBSERIALPORT_API __declspec(dllexport)
#else
#define LIBSERIALPORT_API __declspec(dllimport)
#endif

#include "serialporthandler.h"

class ISerialPort
{
public:
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

