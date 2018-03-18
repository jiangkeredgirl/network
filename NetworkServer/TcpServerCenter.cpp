#include "TcpServerCenter.h"
#include "LibNetworkServer.h"
#include "TcpServerHandlerCenter.h"
#include "BusinessServerHandlerCenter.h"
#include "BusinessServerCenter.h"

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
	cout << "����������˿�, ����:9000" << endl;
	string strport;
	getline(std::cin, strport);
	int port = 9000;
	if (!strport.empty())
	{
		port = stoi(strport);
	}
	cout << "���������ҵ������, Ĭ��tcp������, b ҵ�������, d �豸Դ������" << endl;
	string str_server_type;
	getline(std::cin, str_server_type);

	ILibNetworkServer* tcpserver = NewNetworkServer(port);
	if (str_server_type == "b")
	{
		tcpserver->RegisterHandler(&BusinessServerHandlerCenter::instance());
	}
	else
	{
		tcpserver->RegisterHandler(&TcpServerHandlerCenter::instance());
	}
	if (async)
	{
		tcpserver->AsyncStart();
		cout << "tcp�첽������������, �˿�:" << port << endl;
	}
	else
	{
		tcpserver->Start();
		cout << "tcpͬ��������������, �˿�:" << port << endl;
	}
	string input_flag;
	do
	{
		if (str_server_type == "b")
		{
			BusinessServerCenter::instance().Run();
		}
		else if (str_server_type == "d")
		{

		}
		cout << "�����ַ��������͵��ͻ���, c�رշ�����" << endl;
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
	cout << "tcp�������ѹر�" << endl;
	system("pause");
	return 0;
}