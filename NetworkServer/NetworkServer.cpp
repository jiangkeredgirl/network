// BusinessServer.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>
#include "TcpServerCenter.h"
#include "cstandard.h"
using namespace std;

int _tmain(int argc, _TCHAR* argv[])
{
	cout << "�������������, Ĭ���첽, sͬ��" << endl;
	string server_type;
	getline(std::cin, server_type);
	if (server_type == "s")
	{
		TcpServerCenter::instance().Run(false);
	}
	else
	{
		TcpServerCenter::instance().Run(true);
	}
	return 0;
}

