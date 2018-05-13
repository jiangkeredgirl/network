#pragma once
#include "cstandard.h"
using namespace std;

class ITcpConnect;

class BusinessServerCenter
{
public:
	BusinessServerCenter();
	~BusinessServerCenter();
	static BusinessServerCenter& instance();

public:
	int Run();
	int AddConnectSession(shared_ptr<ITcpConnect> connect);
private:
	int SendCamConfig(shared_ptr<ITcpConnect> connect);
	int SendBinaryData(shared_ptr<ITcpConnect> connect, const char* data, size_t size);

private:
	int TestSendFun(const string& fun_name, int connect_session);
	int RegisterSendFuns();
private:
	map<string, function<int(shared_ptr<ITcpConnect> connect)>> m_sendfun;
	map<int, shared_ptr<ITcpConnect>> m_connect_sessions;
	static atomic<int> m_connect_increment;
};

