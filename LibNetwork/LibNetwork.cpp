// LibNetwork.cpp : 定义 DLL 应用程序的导出函数。
//

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             //  从 Windows 头文件中排除极少使用的信息
// Windows 头文件: 
#include <windows.h>
#include "LibNetwork.h"


// 这是导出变量的一个示例
LIBNETWORK_API int nLibNetwork=0;

// 这是导出函数的一个示例。
LIBNETWORK_API int fnLibNetwork(void)
{
	return 42;
}

// 这是已导出类的构造函数。
// 有关类定义的信息，请参阅 LibNetwork.h
CLibNetwork::CLibNetwork()
{
	return;
}
