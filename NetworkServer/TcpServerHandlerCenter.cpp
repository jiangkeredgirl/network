#include "TcpServerHandlerCenter.h"

TcpServerHandlerCenter::TcpServerHandlerCenter()
{
}


TcpServerHandlerCenter::~TcpServerHandlerCenter()
{
}

TcpServerHandlerCenter& TcpServerHandlerCenter::instance()
{
	static TcpServerHandlerCenter _instance;
	return _instance;
}

int  TcpServerHandlerCenter::OnTcpRead(shared_ptr<ITcpConnect> connect, const char* data, size_t size, int status)
{
	return 0;
}

int  TcpServerHandlerCenter::OnTcpWrite(shared_ptr<ITcpConnect> connect, const char* data, size_t size, int status)
{
	return 0;
}

int  TcpServerHandlerCenter::OnTcpConnect(shared_ptr<ITcpConnect> connect, int status)
{
	return 0;
}

int  TcpServerHandlerCenter::OnTcpDisconnect(shared_ptr<ITcpConnect> connect, int status)
{
	return 0;
}