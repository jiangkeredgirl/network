#pragma once
#include "serialport.h"
//#include "serialporthandler.h"


class SerialPortHandlerCenter : public ISerialPortHandler
{
public:
	SerialPortHandlerCenter(const string& portname, int baudrate, ISerialPort* serialport_client);
	~SerialPortHandlerCenter();

public:
	virtual int OnSerialPortConnect(int errorcode, string errormsg) override;
	virtual int OnSerialPortDisconnect(int errorcode, string errormsg) override;
	virtual int OnSerialPortRead(const char* data, size_t size, int errorcode, string errormsg) override;
	virtual int OnSerialPortWrite(const char* data, size_t size, int errorcode, string errormsg) override;
	virtual int OnSerialPortError(int errorcode, string errormsg) override; 

private:
	ISerialPort*  m_serialport_client = nullptr;
	string m_portname;
	int    m_baudrate = 115200;
};

