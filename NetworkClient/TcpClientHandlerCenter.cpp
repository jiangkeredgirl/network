#include "TcpClientHandlerCenter.h"
#include "cstandard.h"
#include "TcpPackage.h"

TcpClientHandlerCenter::TcpClientHandlerCenter(const string& ip, int port, ITcpClient* tcp_client)
{
	m_ip = ip;
	m_port = port;
	m_tcp_client = tcp_client;
}


TcpClientHandlerCenter::~TcpClientHandlerCenter()
{
}


int TcpClientHandlerCenter::OnTcpConnect(int status)
{
	if (status)
	{
		cout << "connect failed, status:" << status << endl;
		std::thread t([this]() {
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			m_tcp_client->AsyncTcpConnect(m_ip, m_port);
		});
		t.detach();

	}
	else
	{
		cout << "connect success, status:" << status << endl;
	}

	return 0;
}

int TcpClientHandlerCenter::OnTcpDisconnect(int status)
{
	cout << "have disconnected, status:" << status << endl;
	if (status != 1236)
	{
		cout << "abnormal disconnect, reconnect"<< endl;
		std::thread t([this]() {
			m_tcp_client->AsyncTcpConnect(m_ip, m_port);
		});
		t.detach();
	}
	return 0;
}

int TcpClientHandlerCenter::OnTcpRead(const char* data, size_t size, int status)
{
	if (data)
	{
		cout << "readed data:" << data << endl;
	}
	return 0;
}

int TcpClientHandlerCenter::OnTcpWrite(const char* data, size_t size, int status)
{
	if (data)
	{
		cout << "writed data:" << data << endl;
	}
	return 0;
}