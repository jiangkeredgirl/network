#include "LibNetworkClientImpl.h"
#include "tcpclient.h"


CLibNetworkClientImpl::CLibNetworkClientImpl()
{
}


CLibNetworkClientImpl::~CLibNetworkClientImpl()
{
}

CLibNetworkClientImpl& CLibNetworkClientImpl::instance()
{
	static CLibNetworkClientImpl _instance;
	return _instance;
}

int CLibNetworkClientImpl::RegisterHandler(ITcpClientHandler* tcpclient_handler)
{
	return GetTcpClientSingleInstance()->RegisterHandler(tcpclient_handler);
}

int CLibNetworkClientImpl::TcpConnect(const string& ip, int port)
{
	return GetTcpClientSingleInstance()->TcpConnect(ip, port);
}

int CLibNetworkClientImpl::AsyncTcpConnect(const string& ip, int port)
{
	return GetTcpClientSingleInstance()->AsyncTcpConnect(ip, port);
}

int CLibNetworkClientImpl::TcpDisconnect()
{
	return GetTcpClientSingleInstance()->TcpDisconnect();
}

int CLibNetworkClientImpl::TcpWrite(const char* data, size_t size)
{
	return GetTcpClientSingleInstance()->TcpWrite(data, size);
}

int CLibNetworkClientImpl::AsyncTcpWrite(const char* data, size_t size)
{
	return GetTcpClientSingleInstance()->AsyncTcpWrite(data, size);
}


LIBNETWORKCLIENT_API ILibNetworkClient* GetNetworkClient(void)
{
	return &CLibNetworkClientImpl::instance();
}