#pragma once

#include "kutility.h"
#include "tcpserver.h"

class CTcpAccept;

class CTcpServerImpl : public ITcpServer
{
public:
	CTcpServerImpl(int port);
	virtual ~CTcpServerImpl();

public:
	virtual int RegisterHandler(ITcpServerHandler* tcpserver_handler) override;
	virtual int Start() override;
	virtual int AsyncStart() override;
	virtual int Stop() override;
	virtual int Broadcast(const char* data, size_t size) override;
	virtual int AsyncBroadcast(const char* data, size_t size) override;	

private:
	shared_ptr<CTcpAccept> m_tcpacceptor;
};

