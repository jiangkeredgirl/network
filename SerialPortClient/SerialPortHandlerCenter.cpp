#include "cstandard.h"
#include "SerialPortHandlerCenter.h"


SerialPortHandlerCenter::SerialPortHandlerCenter(const string& portname, int baudrate, ISerialPort* serialport_client)
{
	m_portname = portname;
	m_baudrate = baudrate;
	m_serialport_client = serialport_client;
}


SerialPortHandlerCenter::~SerialPortHandlerCenter()
{
}


int SerialPortHandlerCenter::OnSerialPortConnect(int errorcode, string errormsg)
{
	if (errorcode)
	{
		cout << "connect failed, errorcode:" << errorcode << endl;
		std::thread t([this]() {
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			m_serialport_client->AsyncConnect(m_portname, m_baudrate);
		});
		t.detach();

	}
	else
	{
		cout << "connect success, errorcode:" << errorcode << endl;
	}

	return 0;
}

int SerialPortHandlerCenter::OnSerialPortDisconnect(int errorcode, string errormsg)
{
	cout << "have disconnected, errorcode:" << errorcode << endl;
	if (errorcode)
	{
		cout << "abnormal disconnect, reconnect"<< endl;
		std::thread t([this]() {
			m_serialport_client->AsyncConnect(m_portname, m_baudrate);
		});
		t.detach();
	}
	return 0;
}

int SerialPortHandlerCenter::OnSerialPortRead(const char* data, size_t size, int errorcode, string errormsg)
{
	if (data)
	{
		string str(data, size);
		cout << "readed data:" << str << endl;
	}
	return 0;
}

int SerialPortHandlerCenter::OnSerialPortWrite(const char* data, size_t size, int errorcode, string errormsg)
{
	if (data)
	{
		string str(data, size);
		cout << "writed data:" << str << endl;
	}
	return 0;
}

int SerialPortHandlerCenter::OnSerialPortError(int errorcode, string errormsg)
{
	if (errorcode)
	{
		cout << "occure error errorcode:" << errorcode << "errormsg:" << errormsg << endl;
	}
	return 0;
}