﻿#include "tcpclientimpl.h"
#include "kloglib.h"

CTcpClientImpl::CTcpClientImpl()
{
	m_handler = nullptr;
}


CTcpClientImpl::~CTcpClientImpl()
{
}

int CTcpClientImpl::RegisterHandler(ITcpClientHandler* tcpclient_handler)
{
	m_handler = tcpclient_handler;
	if (m_handler)
	{
		m_tcp_client_socket.RegisterHandler(
			std::bind(&ITcpClientHandler::OnTcpConnect, m_handler, std::placeholders::_1)
			, std::bind(&ITcpClientHandler::OnTcpDisconnect, m_handler, std::placeholders::_1)
			, std::bind(&ITcpClientHandler::OnTcpRead, m_handler, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
			, std::bind(&ITcpClientHandler::OnTcpWrite, m_handler, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	}
	else
	{
		m_tcp_client_socket.RegisterHandler(
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
	error_code = m_tcp_client_socket.Connect(ip, port);
	return error_code;
}

int CTcpClientImpl::AsyncTcpConnect(const string& ip, int port)
{
	int error_code = 0;
	error_code =  m_tcp_client_socket.AsyncConnect(ip, port);
	return error_code;
}

int CTcpClientImpl::TcpDisconnect()
{
	return m_tcp_client_socket.Disconnect();
}

int CTcpClientImpl::TcpWrite(const char* data, size_t size)
{
	return m_tcp_client_socket.Write(data, size);
}

int CTcpClientImpl::AsyncTcpWrite(const char* data, size_t size)
{
	return m_tcp_client_socket.AsyncWrite(data, size);
}
 

LIBNETWORK_API ITcpClient* NewTcpClient(void)
{
	return new CTcpClientImpl();
}

LIBNETWORK_API void DeleteTcpClient(ITcpClient* tcp_client)
{
	if (tcp_client)
	{
		delete dynamic_cast<CTcpClientImpl*>(tcp_client);
		tcp_client = nullptr;
	}
}