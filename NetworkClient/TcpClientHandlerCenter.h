#pragma once
#include "tcpclienthandler.h"

class ITcpClient;
class TcpClientHandlerCenter : public ITcpClientHandler
{
public:
	TcpClientHandlerCenter(const string& ip, int port, ITcpClient* tcp_client);
	~TcpClientHandlerCenter();

public:
	virtual int OnTcpConnect(int status) override;
	virtual int OnTcpDisconnect(int status) override;
	virtual int OnTcpRead(const char* data, size_t size, int status) override;
	virtual int OnTcpWrite(const char* data, size_t size, int status) override;

private:
	ITcpClient*  m_tcp_client = nullptr;
	string m_ip;
	int m_port = 0;
};

