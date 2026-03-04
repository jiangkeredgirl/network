#pragma once
class TcpClientCenter
{
public:
	TcpClientCenter();
	~TcpClientCenter();
	static TcpClientCenter& instance();

public:
	int TestSerialPort();
	int Run(bool async);
};

