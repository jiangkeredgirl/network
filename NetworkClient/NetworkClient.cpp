// BusinessServerTest.cpp : 定义控制台应用程序的入口点。
//

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>
#include "TcpClientCenter.h"
#include "cstandard.h"
using namespace std;

int _tmain(int argc, _TCHAR* argv[])
{
	cout << "输入客户端类型, 默认异步, s同步" << endl;
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

