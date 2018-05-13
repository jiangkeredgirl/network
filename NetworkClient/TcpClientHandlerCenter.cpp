#include "TcpClientHandlerCenter.h"


TcpClientHandlerCenter::TcpClientHandlerCenter()
{
}


TcpClientHandlerCenter::~TcpClientHandlerCenter()
{
}

TcpClientHandlerCenter& TcpClientHandlerCenter::instance()
{
	static TcpClientHandlerCenter _instance;
	return _instance;
}

int TcpClientHandlerCenter::OnTcpConnect(int status)
{
	return 0;
}

int TcpClientHandlerCenter::OnTcpDisconnect(int status)
{
	return 0;
}

int TcpClientHandlerCenter::OnTcpRead(const char* data, size_t size, int status)
{
	return 0;
}

int TcpClientHandlerCenter::OnTcpWrite(const char* data, size_t size, int status)
{
	return 0;
}