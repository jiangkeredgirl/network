#include "LibNetworkServerImpl.h"
#include "tcpserver.h"


CLibNetworkImpl::CLibNetworkImpl(int port)
{
	m_tcpserver = NewTcpServer(port);
}

CLibNetworkImpl::~CLibNetworkImpl()
{
}

int CLibNetworkImpl::RegisterHandler(ITcpServerHandler* tcpserver_handler)
{
	return m_tcpserver->RegisterHandler(tcpserver_handler);
}

int CLibNetworkImpl::Start()
{
	return m_tcpserver->Start();
}

int CLibNetworkImpl::AsyncStart()
{
	return m_tcpserver->AsyncStart();
}

int CLibNetworkImpl::Stop()
{
	return m_tcpserver->Stop();
}

int CLibNetworkImpl::Broadcast(const char* data, size_t size)
{
	return m_tcpserver->Broadcast(data, size);
}

int CLibNetworkImpl::AsyncBroadcast(const char* data, size_t size)
{
	return m_tcpserver->AsyncBroadcast(data, size);
}

LIBNETWORKSERVER_API ILibNetworkServer* NewNetworkServer(int port)
{
	return new CLibNetworkImpl(port);
}