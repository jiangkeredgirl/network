#include "TcpServerCenter.h"
#include "LibNetworkServer.h"
#include "TcpServerHandlerCenter.h"


TcpServerCenter::TcpServerCenter()
{
}


TcpServerCenter::~TcpServerCenter()
{
}

TcpServerCenter& TcpServerCenter::instance()
{
	static TcpServerCenter _instance;
	return _instance;
}

int TcpServerCenter::Run(bool async)
{
	cout << "输入服务器端口, 例如:9000" << endl;
	string strport;
	getline(std::cin, strport);
	int port = 9000;
	if (!strport.empty())
	{
		port = stoi(strport);
	}

	ILibNetworkServer* tcpserver = NewNetworkServer(port);
	tcpserver->RegisterHandler(&TcpServerHandlerCenter::instance());
	
	if (async)
	{
		tcpserver->AsyncStart();
		cout << "tcp异步服务器已启动, 端口:" << port << endl;
	}
	else
	{
		tcpserver->Start();
		cout << "tcp同步服务器已启动, 端口:" << port << endl;
	}
	string input_flag;
	do
	{
		cout << "输入字符串将发送到客户端, c关闭服务器" << endl;
		cin >> input_flag;
		if (input_flag == "c")
		{
			break;
		}
		if (async)
		{
			tcpserver->AsyncBroadcast(input_flag.c_str(), input_flag.size());
		}
		else
		{
			tcpserver->Broadcast(input_flag.c_str(), input_flag.size());
		}
	} while (true);
	tcpserver->Stop();
	cout << "tcp服务器已关闭" << endl;
	system("pause");
	return 0;
}