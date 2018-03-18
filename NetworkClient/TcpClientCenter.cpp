#include "TcpClientCenter.h"
#include "TcpClientHandlerCenter.h"
#include "LibNetworkClient.h"
#include "BusinessClientCenter.h"
#include "BusinessClientHandlerCenter.h"
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
	string client_type;
	do
	{
		cout << "���������ip��ַ, ����:127.0.0.1" << endl;
		string ip;
		getline(std::cin, ip);
		cout << "����������˿�, ����:9000" << endl;
		string strport;
		getline(std::cin, strport);
		cout << "����ͻ�������, Ĭ��tcp�ͻ���, bҵ��ͻ���, d �豸Դ�ͻ���" << endl;		
		getline(std::cin, client_type);

		if (ip.empty())
		{
			ip = "127.0.0.1";
		}
		int port = 9000;
		if (!strport.empty())
		{
			port = stoi(strport);
		}

		if (client_type == "b")
		{
			error_code = GetNetworkClient()->RegisterHandler(&BusinessClientHandlerCenter::instance());
		}
		else if (client_type == "d")
		{

		}
		else
		{
			error_code = GetNetworkClient()->RegisterHandler(&TcpClientHandlerCenter::instance());
		}		
		if (async)
		{
			error_code = GetNetworkClient()->AsyncTcpConnect(ip, port);
			if (error_code == 0)
			{
				cout << "tcp�첽�ͻ���������, ������ip:" << ip << ", �˿�:" << port << endl;
			}
			else
			{
				cout << "tcp�첽�ͻ�������ʧ��, ������ip:" << ip << ", �˿�:" << port << endl;
			}
		}
		else
		{
			error_code = GetNetworkClient()->TcpConnect(ip, port);
			if (error_code == 0)
			{
				cout << "tcpͬ���ͻ���������, ������ip:" << ip << ", �˿�:" << port << endl;
			}
			else
			{
				cout << "tcpͬ���ͻ�������ʧ��, ������ip:" << ip << ", �˿�:" << port << endl;
			}
		}
	} while (error_code != 0);
	string input_flag;
	do
	{
		if (client_type == "b")
		{
			BusinessClientCenter::instance().Run();
		}
		else if (client_type == "d")
		{

		}
		cout << "�����ַ������͵��ͻ���, c�ر�����\n" << endl;
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
	cout << "tcp�ͻ����ѶϿ�" << endl;
	system("pause");
	return 0;
}

int TcpClientCenter::SendData(const char* data, size_t size)
{
	GetNetworkClient()->AsyncTcpWrite(data, size);
	return 0;
}