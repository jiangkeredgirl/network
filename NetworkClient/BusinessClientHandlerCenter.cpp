#include "BusinessClientHandlerCenter.h"


BusinessClientHandlerCenter::BusinessClientHandlerCenter()
{
}


BusinessClientHandlerCenter::~BusinessClientHandlerCenter()
{
}

BusinessClientHandlerCenter& BusinessClientHandlerCenter::instance()
{
	static BusinessClientHandlerCenter _instance;
	return _instance;
}

int BusinessClientHandlerCenter::OnTcpConnect(int status)
{
	return 0;
}

int BusinessClientHandlerCenter::OnTcpDisconnect(int status)
{
	return 0;
}

int BusinessClientHandlerCenter::OnTcpRead(const char* data, size_t size, int status)
{
	return 0;
}

int BusinessClientHandlerCenter::OnTcpWrite(const char* data, size_t size, int status)
{
	return 0;
}