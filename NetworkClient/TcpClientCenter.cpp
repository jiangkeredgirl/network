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
		cout << "ÊäÈë·þÎñÆ÷ipµØÖ·, ÀýÈç:127.0.0.1" << endl;
		string ip;
		getline(std::cin, ip);
		cout << "ÊäÈë·þÎñÆ÷¶Ë¿Ú, ÀýÈç:9000" << endl;
		string strport;
		getline(std::cin, strport);
		cout << "ÊäÈë¿Í»§¶ËÀàÐÍ, Ä¬ÈÏtcp¿Í»§¶Ë, bÒµÎñ¿Í»§¶Ë, d Éè±¸Ô´¿Í»§¶Ë" << endl;		
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
				cout << "tcpÒì²½¿Í»§¶ËÒÑÁ´½Ó, ·þÎñÆ÷ip:" << ip << ", ¶Ë¿Ú:" << port << endl;
			}
			else
			{
				cout << "tcpÒì²½¿Í»§¶ËÁ´½ÓÊ§°Ü, ·þÎñÆ÷ip:" << ip << ", ¶Ë¿Ú:" << port << endl;
			}
		}
		else
		{
			error_code = GetNetworkClient()->TcpConnect(ip, port);
			if (error_code == 0)
			{
				cout << "tcpÍ¬²½¿Í»§¶ËÒÑÁ´½Ó, ·þÎñÆ÷ip:" << ip << ", ¶Ë¿Ú:" << port << endl;
			}
			else
			{
				cout << "tcpÍ¬²½¿Í»§¶ËÁ´½ÓÊ§°Ü, ·þÎñÆ÷ip:" << ip << ", ¶Ë¿Ú:" << port << endl;
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
		cout << "ÊäÈë×Ö·û´®·¢ËÍµ½¿Í»§¶Ë, c¹Ø±ÕÁ¬½Ó\n" << endl;
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
	cout << "tcp¿Í»§¶ËÒÑ¶Ï¿ª" << endl;
	system("pause");
	return 0;
}

int TcpClientCenter::SendData(const char* data, size_t size)
{
	GetNetworkClient()->AsyncTcpWrite(data, size);
	return 0;
}