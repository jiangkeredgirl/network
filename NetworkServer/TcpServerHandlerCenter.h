#pragma once
#include "cstandard.h"
#include "tcpserverhandler.h"

class TcpServerHandlerCenter : public ITcpServerHandler
{
public:
	TcpServerHandlerCenter();
	~TcpServerHandlerCenter();
	static TcpServerHandlerCenter& instance();

public:
	virtual int  OnTcpRead(shared_ptr<ITcpConnect> connect, const char* data, size_t size, int status) override;
	virtual int  OnTcpWrite(shared_ptr<ITcpConnect> connect, const char* data, size_t size, int status) override;
	virtual int  OnTcpConnect(shared_ptr<ITcpConnect> connect, int status) override;
	virtual int  OnTcpDisconnect(shared_ptr<ITcpConnect> connect, int status) override;
};

