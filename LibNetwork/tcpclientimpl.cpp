#include "tcpclientimpl.h"
#include "tcpclientsocket.h"


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
	CTcpClientSocket::instance().RegisterHandler(
		std::bind(&CTcpClientImpl::OnTcpConnect, this, std::placeholders::_1)
		, std::bind(&CTcpClientImpl::OnTcpDisconnect, this, std::placeholders::_1)
		, std::bind(&CTcpClientImpl::OnTcpRead, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
		, std::bind(&CTcpClientImpl::OnTcpWrite, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
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

int CTcpClientImpl::OnTcpConnect(int status)
{
	cout << "tcp client connect status code:" << status << endl;
	if(m_handler)
	{
		m_handler->OnTcpConnect(status);
	}
	return 0;
}
int CTcpClientImpl::OnTcpDisconnect(int status)
{
	cout << "tcp client disconnect status code:" << status << endl;
	if(m_handler)
	{
		m_handler->OnTcpDisconnect(status);
	}
	return 0;
}

int CTcpClientImpl::OnTcpRead(const char* data, size_t size, int status)
{
	cout << "tcp client read completed, status code:" << status << ", read data : " << data << endl;
	if(m_handler)
	{
		m_handler->OnTcpRead(data, size, status);
	}
	return 0;
}
int CTcpClientImpl::OnTcpWrite(const char* data, size_t size, int status)
{
	cout << "tcp client write completed, status code:" << status << ", write data : " << data << endl;
	if(m_handler)
	{
		m_handler->OnTcpWrite(data, size, status);
	}
	return 0;
}

LIBNETWORK_API ITcpClient* GetTcpClientSingleInstance(void)
{
	return &CTcpClientImpl::instance();
}