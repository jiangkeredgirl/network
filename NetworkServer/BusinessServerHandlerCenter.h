#pragma once
#include "tcpserverhandler.h"

class BusinessServerHandlerCenter : public ITcpServerHandler
{
public:
	BusinessServerHandlerCenter();
	~BusinessServerHandlerCenter();
	static BusinessServerHandlerCenter& instance();

public:
	virtual int  OnTcpRead(shared_ptr<ITcpConnect> connect, const char* data, size_t size, int status) override;
	virtual int  OnTcpWrite(shared_ptr<ITcpConnect> connect, const char* data, size_t size, int status) override;
	virtual int  OnTcpConnect(shared_ptr<ITcpConnect> connect, int status) override;
	virtual int  OnTcpDisconnect(shared_ptr<ITcpConnect> connect, int status) override;

private:
	int ProccessBusinessProtocol(const char* data, size_t size);
};

