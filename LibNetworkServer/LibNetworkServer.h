// ���� ifdef ���Ǵ���ʹ�� DLL �������򵥵�
// ��ı�׼�������� DLL �е������ļ��������������϶���� LIBNETWORKSERVER_EXPORTS
// ���ű���ġ���ʹ�ô� DLL ��
// �κ�������Ŀ�ϲ�Ӧ����˷��š�������Դ�ļ��а������ļ����κ�������Ŀ���Ὣ
// LIBNETWORKSERVER_API ������Ϊ�Ǵ� DLL ����ģ����� DLL ���ô˺궨���
// ������Ϊ�Ǳ������ġ�
#ifdef LIBNETWORKSERVER_EXPORTS
#define LIBNETWORKSERVER_API __declspec(dllexport)
#else
#define LIBNETWORKSERVER_API __declspec(dllimport)
#endif


#include "tcpserverhandler.h"

class ILibNetworkServer
{

public:
	virtual int RegisterHandler(ITcpServerHandler* tcpserver_handler) = 0;
	virtual int Start() = 0;
	virtual int AsyncStart() = 0;
	virtual int Stop() = 0;
	virtual int Broadcast(const char* data, size_t size) = 0;
	virtual int AsyncBroadcast(const char* data, size_t size) = 0;
};

#ifdef __cplusplus
extern "C" {
#endif

	LIBNETWORKSERVER_API ILibNetworkServer* NewNetworkServer(int port);
	typedef ILibNetworkServer* (*NewNetworkServerFun)(int port);

#ifdef __cplusplus
}
#endif