#pragma once

#include "kutility.h"
#include "tcpclient.h"

class CTcpClientImpl : public ITcpClient
{
private:
	CTcpClientImpl();
public:
	virtual ~CTcpClientImpl();
public:
	static CTcpClientImpl& instance();

public:
	virtual int RegisterHandler(ITcpClientHandler* tcpclient_handler) override;
	virtual int TcpConnect(const string& ip = "127.0.0.1", int port = 1000) override;
	virtual int AsyncTcpConnect(const string& ip = "127.0.0.1", int port = 1000) override;
	virtual int TcpDisconnect() override;
	virtual int TcpWrite(const char* data, size_t size) override;
	virtual int AsyncTcpWrite(const char* data, size_t size) override;

private:
	int OnTcpConnect(int status);
	int OnTcpDisconnect(int status);
	int OnTcpRead(const char* data, size_t size, int status);
	int OnTcpWrite(const char* data, size_t size, int status);

private:
	ITcpClientHandler* m_handler;
};

