#include "tcpserverimpl.h"
#include "tcpserveraccept.h"


CTcpServerImpl::CTcpServerImpl(int port)
{
	m_tcpacceptor = shared_ptr<CTcpAccept>(new CTcpAccept(port));
}

CTcpServerImpl::~CTcpServerImpl()
{
}

int CTcpServerImpl::RegisterHandler(ITcpServerHandler* tcpserver_handler)
{
	m_tcpacceptor->RegisterHandler(tcpserver_handler);	
	return 0;
}

int CTcpServerImpl::Start()
{
	m_tcpacceptor->Start();
	return 0;
}

int CTcpServerImpl::AsyncStart()
{
	m_tcpacceptor->AsyncStart();
	return 0;
}

int CTcpServerImpl::Stop()
{
	m_tcpacceptor->Stop();
	return 0;
}

int CTcpServerImpl::Broadcast(const char* data, size_t size)
{
	if (!m_tcpacceptor->GetConnects().empty())
	{
		for (auto item : m_tcpacceptor->GetConnects())
		{
			if (item->socket().is_open())
			{
				item->Write(data, size);
			}
		}
	}
	return 0;
}

int CTcpServerImpl::AsyncBroadcast(const char* data, size_t size)
{
	if (!m_tcpacceptor->GetConnects().empty())
	{
		for (auto item : m_tcpacceptor->GetConnects())
		{
			if (item->socket().is_open())
			{
				item->AsyncWrite(data, size);
			}
		}
	}
	return 0;
}

LIBNETWORK_API ITcpServer* NewTcpServer(int port)
{
	return new CTcpServerImpl(port);
}