// ���� ifdef ���Ǵ���ʹ�� DLL �������򵥵�
// ��ı�׼�������� DLL �е������ļ��������������϶���� LIBNETWORKCLIENT_EXPORTS
// ���ű���ġ���ʹ�ô� DLL ��
// �κ�������Ŀ�ϲ�Ӧ����˷��š�������Դ�ļ��а������ļ����κ�������Ŀ���Ὣ
// LIBNETWORKCLIENT_API ������Ϊ�Ǵ� DLL ����ģ����� DLL ���ô˺궨���
// ������Ϊ�Ǳ������ġ�
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