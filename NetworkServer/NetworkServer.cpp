// BusinessServer.cpp : 定义控制台应用程序的入口点。
//

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>
#include "TcpServerCenter.h"
#include "cstandard.h"
using namespace std;

int _tmain(int argc, _TCHAR* argv[])
{
	cout << "please server type, defualt async, \'s\' indicate sync" << endl;
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

