#pragma once
#include "tcpclienthandler.h"

class TcpClientHandlerCenter : public ITcpClientHandler
{
public:
	TcpClientHandlerCenter();
	~TcpClientHandlerCenter();
	static TcpClientHandlerCenter& instance();

public:
	virtual int OnTcpConnect(int status) override;
	virtual int OnTcpDisconnect(int status) override;
	virtual int OnTcpRead(const char* data, size_t size, int status) override;
	virtual int OnTcpWrite(const char* data, size_t size, int status) override;
};

