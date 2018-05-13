#pragma once

#include "kutility.h"
#include "LibNetworkServer.h"

class ITcpServer;

class CLibNetworkImpl : public ILibNetworkServer
{
public:
	CLibNetworkImpl(int port);
	virtual ~CLibNetworkImpl();

public:
	virtual int RegisterHandler(ITcpServerHandler* tcpserver_handler) override;
	virtual int Start() override;
	virtual int AsyncStart() override;
	virtual int Stop() override;
	virtual int Broadcast(const char* data, size_t size) override;
	virtual int AsyncBroadcast(const char* data, size_t size) override;

private:
	ITcpServer* m_tcpserver;
};

