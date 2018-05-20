#include "TcpClientCenter.h"
#include "TcpClientHandlerCenter.h"
#include "tcpclient.h"
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
	ITcpClient*  tcp_client = NewTcpClient();
	do
	{
		cout << "please input server ip, such as:127.0.0.1" << endl;
		string ip;
		getline(std::cin, ip);
		cout << "input server port, such as:9000" << endl;
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
		
		error_code = tcp_client->RegisterHandler(&TcpClientHandlerCenter::instance());

		if (async)
		{
			error_code = tcp_client->AsyncTcpConnect(ip, port);
			if (error_code == 0)
			{
				cout << "tcp async client have connected, server ip:" << ip << ", port:" << port << endl;
			}
			else
			{
				cout << "tcp async server connect occur error, server ip:" << ip << ", port:" << port << endl;
			}
		}
		else
		{
			error_code = tcp_client->TcpConnect(ip, port);
			if (error_code == 0)
			{
				cout << "tcp sync client have connected, server ip:" << ip << ", port:" << port << endl;
			}
			else
			{
				cout << "tcp sync client ocurr error, server ip:" << ip << ", port:" << port << endl;
			}
		}
	} while (error_code != 0);
	string input_flag;
	do
	{
		cout << "please text for sending to server, \'c\' close connect\n" << endl;
		cin >> input_flag;
		if (input_flag == "c")
		{
			break;
		}
		else
		{
			if (async)
			{
				tcp_client->AsyncTcpWrite(input_flag.c_str(), input_flag.size());
			}
			else
			{
				tcp_client->TcpWrite(input_flag.c_str(), input_flag.size());
			}
		}
	} while (true);

	if (error_code == 0)
	{
		tcp_client->TcpDisconnect();
	}	
	cout << "tcp client have disconnected" << endl;
	if (tcp_client)
	{
		DeleteTcpClient(tcp_client);
	}	
	system("pause");
	return 0;
}
