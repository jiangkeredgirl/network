#pragma once

#include <string>
#include <memory>

using namespace std;

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




