#pragma once
#include "tcpclienthandler.h"

class BusinessClientHandlerCenter : public ITcpClientHandler
{
public:
	BusinessClientHandlerCenter();
	~BusinessClientHandlerCenter();
	static BusinessClientHandlerCenter& instance();

public:
	virtual int OnTcpConnect(int status) override;
	virtual int OnTcpDisconnect(int status) override;
	virtual int OnTcpRead(const char* data, size_t size, int status) override;
	virtual int OnTcpWrite(const char* data, size_t size, int status) override;
};

