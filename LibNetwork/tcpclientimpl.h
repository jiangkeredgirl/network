#pragma once

#include "kutility.h"
#include "tcpclient.h"

class CTcpClientImpl : public ITcpClient
{
public:
	CTcpClientImpl();
	virtual ~CTcpClientImpl();


public:
	virtual int RegisterHandler(ITcpClientHandler* tcpclient_handler) override;
	virtual int TcpConnect(const string& ip = "127.0.0.1", int port = 1000) override;
	virtual int AsyncTcpConnect(const string& ip = "127.0.0.1", int port = 1000) override;
	virtual int TcpDisconnect() override;
	virtual int TcpWrite(const char* data, size_t size) override;
	virtual int AsyncTcpWrite(const char* data, size_t size) override;


private:
	ITcpClientHandler* m_handler;
};

