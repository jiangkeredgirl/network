#pragma once
#ifdef LIBNETWORK_EXPORTS
#define LIBNETWORK_API __declspec(dllexport)
#else
#define LIBNETWORK_API __declspec(dllimport)
#endif

#include "tcpclienthandler.h"

class ITcpClient
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

	LIBNETWORK_API ITcpClient* GetTcpClientSingleInstance(void);
	typedef ITcpClient* (*GetTcpClientSingleInstanceFun)(void);

#ifdef __cplusplus
}
#endif