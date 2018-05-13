#pragma once
#include "cstandard.h"
using namespace std;

class BusinessClientCenter
{
public:
	BusinessClientCenter();
	~BusinessClientCenter();
	static BusinessClientCenter& instance();

public:
	int Run();
private:
	int SendCamConfig();
	int SendBinaryData(const char* data, size_t size);

private:
	int TestSendFun(const string& fun_name);
	int RegisterSendFuns();
private:
	map<string, function<int()>> m_sendfun;
};

