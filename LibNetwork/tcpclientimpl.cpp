#include "tcpclientimpl.h"
#include "tcpclientsocket.h"
#include "kloglib.h"

CTcpClientImpl::CTcpClientImpl()
{
	m_handler = nullptr;
}


CTcpClientImpl::~CTcpClientImpl()
{
}

CTcpClientImpl& CTcpClientImpl::instance()
{
	static CTcpClientImpl _instance;
	return _instance;
}

int CTcpClientImpl::RegisterHandler(ITcpClientHandler* tcpclient_handler)
{
	m_handler = tcpclient_handler;
	if (m_handler)
	{
		CTcpClientSocket::instance().RegisterHandler(
			std::bind(&ITcpClientHandler::OnTcpConnect, m_handler, std::placeholders::_1)
			, std::bind(&ITcpClientHandler::OnTcpDisconnect, m_handler, std::placeholders::_1)
			, std::bind(&ITcpClientHandler::OnTcpRead, m_handler, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
			, std::bind(&ITcpClientHandler::OnTcpWrite, m_handler, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	}
	else
	{
		CTcpClientSocket::instance().RegisterHandler(
			nullptr
			, nullptr
			, nullptr
			, nullptr);
	}
	return 0;
}

int CTcpClientImpl::TcpConnect(const string& ip, int port)
{
	int error_code = 0;
	error_code = CTcpClientSocket::instance().Connect(ip, port);
	return error_code;
}

int CTcpClientImpl::AsyncTcpConnect(const string& ip, int port)
{
	int error_code = 0;
	error_code =  CTcpClientSocket::instance().AsyncConnect(ip, port);
	return error_code;
}

int CTcpClientImpl::TcpDisconnect()
{
	return CTcpClientSocket::instance().Disconnect();
}

int CTcpClientImpl::TcpWrite(const char* data, size_t size)
{
	return CTcpClientSocket::instance().Write(data, size);
}

int CTcpClientImpl::AsyncTcpWrite(const char* data, size_t size)
{
	return CTcpClientSocket::instance().AsyncWrite(data, size);
}

//int CTcpClientImpl::OnTcpConnect(int status)
//{
//	TraceInfoCout() << "tcp client connect status code:" << status;
//	if(m_handler)
//	{
//		m_handler->OnTcpConnect(status);
//	}
//	return 0;
//}
//int CTcpClientImpl::OnTcpDisconnect(int status)
//{
//	TraceInfoCout() << "tcp client disconnect status code:" << status;
//	if(m_handler)
//	{
//		m_handler->OnTcpDisconnect(status);
//	}
//	return 0;
//}

//int CTcpClientImpl::OnTcpRead(const char* data, size_t size, int status)
//{
//	TraceInfoCout() << "tcp client read completed, status code:" << status;
//	if(m_handler)
//	{
//		m_handler->OnTcpRead(data, size, status);
//	}
//	return 0;
//}
//int CTcpClientImpl::OnTcpWrite(const char* data, size_t size, int status)
//{
//	TraceInfoCout() << "tcp client write completed, status code:" << status;
//	if(m_handler)
//	{
//		m_handler->OnTcpWrite(data, size, status);
//	}
//	return 0;
//}

LIBNETWORK_API ITcpClient* GetTcpClientSingleInstance(void)
{
	return &CTcpClientImpl::instance();
}