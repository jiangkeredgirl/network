// BusinessServerTest.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>
#include "TcpClientCenter.h"
#include "cstandard.h"
using namespace std;

int _tmain(int argc, _TCHAR* argv[])
{
	cout << "����ͻ�������, Ĭ���첽, sͬ��" << endl;
	string client_type;
	getline(std::cin, client_type);
	if (client_type == "s")
	{
		TcpClientCenter::instance().Run(false);
	}
	else
	{
		TcpClientCenter::instance().Run(true);
	}
	return 0;
}

