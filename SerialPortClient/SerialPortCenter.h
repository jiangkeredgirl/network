#pragma once

#include "cstandard.h"

using namespace std;


class SerialPortCenter
{
public:
	SerialPortCenter();
	~SerialPortCenter();
	static SerialPortCenter& instance();

public:
	int Run();
private:
	int TestSeriolport();
	int TestSeriolportk();

private:
	void onReadk(const string& hexstr);
	void onErrork(int error_code, string error_msg);
};

