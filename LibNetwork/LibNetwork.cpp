// LibNetwork.cpp : ���� DLL Ӧ�ó���ĵ���������
//

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             //  �� Windows ͷ�ļ����ų�����ʹ�õ���Ϣ
// Windows ͷ�ļ�: 
#include <windows.h>
#include "LibNetwork.h"


// ���ǵ���������һ��ʾ��
LIBNETWORK_API int nLibNetwork=0;

// ���ǵ���������һ��ʾ����
LIBNETWORK_API int fnLibNetwork(void)
{
	return 42;
}

// �����ѵ�����Ĺ��캯����
// �й��ඨ�����Ϣ������� LibNetwork.h
CLibNetwork::CLibNetwork()
{
	return;
}
