// 1. 实现 ISerialPortHandler，用于打印回调
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
	cout << "[Callback] Connect: " << errorcode << " " << errormsg << endl;

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
	cout << "[Callback] Disconnect: " << errorcode << " " << errormsg << endl;

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
	cout << "[Callback] Read: " << size << " bytes, error=" << errorcode << ", " << errormsg << endl;
	cout << "[Data Hex] ";
	for (size_t i = 0; i < size; i++)
		cout << hex << (0xFF & data[i]) << " ";
	cout << dec << endl;

	if (data)
	{
		string str(data, size);
		cout << "[Data Char]" << str << endl;
	}
	return 0;
}

int SerialPortHandlerCenter::OnSerialPortWrite(const char* data, size_t size, int errorcode, string errormsg)
{
	cout << "[Callback] Write: " << size << " bytes, error=" << errorcode << ", " << errormsg << endl;
	cout << "[Data] ";
	for (size_t i = 0; i < size; i++)
		cout << hex << (0xFF & data[i]) << " ";
	cout << dec << endl;

	if (data)
	{
		string str(data, size);
		cout << "writed data:" << str << endl;
	}
	return 0;
}

int SerialPortHandlerCenter::OnSerialPortError(int errorcode, string errormsg)
{
	cout << "[Callback] Error: " << errorcode << " " << errormsg << endl;

	if (errorcode)
	{
		cout << "occure error errorcode:" << errorcode << "errormsg:" << errormsg << endl;
	}
	return 0;
}