#pragma once

class SerialPortCenter
{
public:
	SerialPortCenter();
	~SerialPortCenter();
	static SerialPortCenter& instance();

public:
	int Run(bool async);
};

