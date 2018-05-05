// ÏÂÁÐ ifdef ¿éÊÇ´´½¨Ê¹´Ó DLL µ¼³ö¸ü¼òµ¥µÄ
// ºêµÄ±ê×¼·½·¨¡£´Ë DLL ÖÐµÄËùÓÐÎÄ¼þ¶¼ÊÇÓÃÃüÁîÐÐÉÏ¶¨ÒåµÄ LIBNETWORKCLIENT_EXPORTS
// ·ûºÅ±àÒëµÄ¡£ÔÚÊ¹ÓÃ´Ë DLL µÄ
// ÈÎºÎÆäËûÏîÄ¿ÉÏ²»Ó¦¶¨Òå´Ë·ûºÅ¡£ÕâÑù£¬Ô´ÎÄ¼þÖÐ°üº¬´ËÎÄ¼þµÄÈÎºÎÆäËûÏîÄ¿¶¼»á½«
// LIBNETWORKCLIENT_API º¯ÊýÊÓÎªÊÇ´Ó DLL µ¼ÈëµÄ£¬¶ø´Ë DLL Ôò½«ÓÃ´Ëºê¶¨ÒåµÄ
// ·ûºÅÊÓÎªÊÇ±»µ¼³öµÄ¡£
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