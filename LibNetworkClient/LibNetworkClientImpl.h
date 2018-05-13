#pragma once

#include "kutility.h"
#include "LibNetworkClient.h"

class CLibNetworkClientImpl : public ILibNetworkClient
{
private:
	CLibNetworkClientImpl();
public:
	virtual ~CLibNetworkClientImpl();
public:
	static CLibNetworkClientImpl& instance();

public:
	virtual int RegisterHandler(ITcpClientHandler* tcpclient_handler) override;
	virtual int TcpConnect(const string& ip = "127.0.0.1", int port = 1000) override;
	virtual int AsyncTcpConnect(const string& ip = "127.0.0.1", int port = 1000) override;
	virtual int TcpDisconnect() override;
	virtual int TcpWrite(const char* data, size_t size) override;
	virtual int AsyncTcpWrite(const char* data, size_t size) override;
};

