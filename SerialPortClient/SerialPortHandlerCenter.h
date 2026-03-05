#pragma once
#include "serialport.h"
//#include "serialporthandler.h"


class SerialPortHandlerCenter : public ISerialPortHandler
{
public:
	SerialPortHandlerCenter(const string& portname, int baudrate, ISerialPort* serialport_client);
	~SerialPortHandlerCenter();

public:
	virtual int OnConnect(std::error_code ec) override;                         // 串口连接的回调事件
	virtual int OnDisconnect(std::error_code ec) override;                      // 串口断开的回调事件
	virtual int OnRead(const vector<char>& data, std::error_code ec) override;  // 串口读取数据的回调事件
	virtual int OnWrite(const vector<char>& data, std::error_code ec) override; // 串口写入数据的回调事件
	virtual int OnError(std::error_code ec) override;                           // 串口发生错误的回调事件

private:
	ISerialPort*  m_serialport_client = nullptr;
	string m_portname;
	int    m_baudrate = 115200;
};

