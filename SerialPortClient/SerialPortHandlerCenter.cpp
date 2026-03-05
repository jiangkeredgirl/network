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


int SerialPortHandlerCenter::OnConnect(std::error_code ec)
{
	cout << "[Callback] Connect: " << ec.value() << " " << ec.message() << endl;

	if (ec)
	{
		cout << "connect failed, errorcode:" << ec << endl;
		std::thread t([this]() {
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			m_serialport_client->AsyncConnect(m_portname, m_baudrate);
		});
		t.detach();

	}
	else
	{
		cout << "connect success, errorcode:" << ec << endl;
	}

	return 0;
}

int SerialPortHandlerCenter::OnDisconnect(std::error_code ec)
{
	cout << "[Callback] Disconnect: " << ec.value() << " " << ec.message() << endl;

	cout << "have disconnected, errorcode:" << ec << endl;
	if (ec)
	{
		cout << "abnormal disconnect, reconnect"<< endl;
		std::thread t([this]() {
			m_serialport_client->AsyncConnect(m_portname, m_baudrate);
		});
		t.detach();
	}
	return 0;
}

int SerialPortHandlerCenter::OnRead(const vector<char>& data, std::error_code ec)
{
	cout << "[Callback] Read: " << data.size() << " bytes, error=" << ec.value() << ", " << ec.message() << endl;
	cout << "[Data Hex] ";
	for (size_t i = 0; i < data.size(); i++)
		cout << hex << (0xFF & data[i]) << " ";
	cout << dec << endl;

	if (!data.empty())
	{
		string str(data.begin(), data.end());
		cout << "[Data Char]" << str << endl;
	}
	return 0;
}

int SerialPortHandlerCenter::OnWrite(const vector<char>& data, std::error_code ec)
{
	cout << "[Callback] Write: " << data.size() << " bytes, error=" << ec.value() << ", " << ec.message() << endl;
	cout << "[Data] ";
	for (size_t i = 0; i < data.size(); i++)
		cout << hex << (0xFF & data[i]) << " ";
	cout << dec << endl;

	if (!data.empty())
	{
		string str(data.begin(), data.end());
		cout << "writed data:" << str << endl;
	}
	return 0;
}

int SerialPortHandlerCenter::OnError(std::error_code ec)
{
	cout << "[Callback] Error: " << ec.value() << " " << ec.message() << endl;

	if (ec)
	{
		cout << "occure error errorcode:" << ec.value() << "errormsg:" << ec.message() << endl;
	}
	return 0;
}