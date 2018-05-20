#pragma once
class TcpClientCenter
{
public:
	TcpClientCenter();
	~TcpClientCenter();
	static TcpClientCenter& instance();

public:
	int Run(bool async);
};

