#include "TcpClientCenter.h"
#include "TcpClientHandlerCenter.h"
#include "LibNetworkClient.h"
#include "cstandard.h"
using namespace std;

TcpClientCenter::TcpClientCenter()
{
}


TcpClientCenter::~TcpClientCenter()
{
}

TcpClientCenter& TcpClientCenter::instance()
{
	static TcpClientCenter _instance;
	return _instance;
}

int TcpClientCenter::Run(bool async)
{
	int error_code = 0;
	do
	{
		cout << "输入服务器ip地址, 例如:127.0.0.1" << endl;
		string ip;
		getline(std::cin, ip);
		cout << "输入服务器端口, 例如:9000" << endl;
		string strport;
		getline(std::cin, strport);
		if (ip.empty())
		{
			ip = "127.0.0.1";
		}
		int port = 9000;
		if (!strport.empty())
		{
			port = stoi(strport);
		}
		error_code = GetNetworkClient()->RegisterHandler(&TcpClientHandlerCenter::instance());

		if (async)
		{
			error_code = GetNetworkClient()->AsyncTcpConnect(ip, port);
			if (error_code == 0)
			{
				cout << "tcp异步客户端已链接, 服务器ip:" << ip << ", 端口:" << port << endl;
			}
			else
			{
				cout << "tcp异步客户端链接失败, 服务器ip:" << ip << ", 端口:" << port << endl;
			}
		}
		else
		{
			error_code = GetNetworkClient()->TcpConnect(ip, port);
			if (error_code == 0)
			{
				cout << "tcp同步客户端已链接, 服务器ip:" << ip << ", 端口:" << port << endl;
			}
			else
			{
				cout << "tcp同步客户端链接失败, 服务器ip:" << ip << ", 端口:" << port << endl;
			}
		}
	} while (error_code != 0);
	string input_flag;
	do
	{
		cout << "输入字符串发送到客户端, c关闭连接\n" << endl;
		cin >> input_flag;
		if (input_flag == "c")
		{
			break;
		}
		else
		{
			if (async)
			{
				GetNetworkClient()->AsyncTcpWrite(input_flag.c_str(), input_flag.size());
			}
			else
			{
				GetNetworkClient()->TcpWrite(input_flag.c_str(), input_flag.size());
			}
		}
	} while (true);
	GetNetworkClient()->TcpDisconnect();
	cout << "tcp客户端已断开" << endl;
	system("pause");
	return 0;
}

int TcpClientCenter::SendData(const char* data, size_t size)
{
	GetNetworkClient()->AsyncTcpWrite(data, size);
	return 0;
}