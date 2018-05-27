#include "TcpClientHandlerCenter.h"
#include <iostream>
using namespace std;

TcpClientHandlerCenter::TcpClientHandlerCenter()
{
}


TcpClientHandlerCenter::~TcpClientHandlerCenter()
{
}


int TcpClientHandlerCenter::OnTcpConnect(int status)
{
	cout << "have connected, status:" << status << endl;
	return 0;
}

int TcpClientHandlerCenter::OnTcpDisconnect(int status)
{
	cout << "have disconnected, status:" << status << endl;
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