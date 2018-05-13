#pragma once
class TcpClientCenter
{
public:
	TcpClientCenter();
	~TcpClientCenter();
	static TcpClientCenter& instance();

public:
	int Run(bool async);
	int SendData(const char* data, size_t size);
};

