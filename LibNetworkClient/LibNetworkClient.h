// 下列 ifdef 块是创建使从 DLL 导出更简单的
// 宏的标准方法。此 DLL 中的所有文件都是用命令行上定义的 LIBNETWORKCLIENT_EXPORTS
// 符号编译的。在使用此 DLL 的
// 任何其他项目上不应定义此符号。这样，源文件中包含此文件的任何其他项目都会将
// LIBNETWORKCLIENT_API 函数视为是从 DLL 导入的，而此 DLL 则将用此宏定义的
// 符号视为是被导出的。
#ifdef LIBNETWORKCLIENT_EXPORTS
#define LIBNETWORKCLIENT_API __declspec(dllexport)
#else
#define LIBNETWORKCLIENT_API __declspec(dllimport)
#endif


#include "tcpclienthandler.h"

class ILibNetworkClient
{

public:
	virtual int RegisterHandler(ITcpClientHandler* tcpclient_handler) = 0;
	virtual int TcpConnect(const string& ip = "127.0.0.1", int port = 1000) = 0;
	virtual int AsyncTcpConnect(const string& ip = "127.0.0.1", int port = 1000) = 0;
	virtual int TcpDisconnect() = 0;
	virtual int TcpWrite(const char* data, size_t size) = 0;
	virtual int AsyncTcpWrite(const char* data, size_t size) = 0;
};

#ifdef __cplusplus
extern "C" {
#endif

	LIBNETWORKCLIENT_API ILibNetworkClient* GetNetworkClient(void);
	typedef ILibNetworkClient* (*GetNetworkClientFun)(void);

#ifdef __cplusplus
}
#endif