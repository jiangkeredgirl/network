#pragma once
#include <windows.h>
#include "cstandard.h"
using namespace std;
//#pragma warning(disable : C4309)

#pragma pack(push)  /**< �������״̬. */
#pragma pack(1)     /**< �趨Ϊ1�ֽڶ���. */

enum BusinessType
{
	CAMERA_CONFIG_TYPE = 0,
	PED_FLOW_INFO_TYPE,
	VEH_FLOW_INFO_TYPE,
	GET_VIDEO_TYPE,
	STOP_VIDEO_TYPE,
	CAMERA_INFO_TYPE,
	CAMERA_STATE_TYPE,
	PORT_INFO_TYPE = 0xA0
};

struct BusinessPackage
{
	unsigned char business_type;
	bool response_flag;
};

struct CameraConfig : public BusinessPackage
{
	__int64 id;         /**< ID. */
	char ip[16];     /**< IP��ַ. */
	int port;         /**< port. */
	char username[20]; /**< �û��� . */
	char password[20];  /**< ����. */
	int cycle;		/*�ɼ�����*/
	__int64 initialFlow; /*��ʼ����*/
	int algFuncType;    /**< �㷨����. 103001������103002���쳵	103003��ȫ��103004��ͣ��λ*/
	int autoUpload;    /**< �����ϴ���־. 0x01:�����ϴ���0x02�������ϴ�*/
	CameraConfig()
	{
		business_type = static_cast<char>(BusinessType::CAMERA_CONFIG_TYPE);
	}
};//��������ͷ

struct PedFlowInfo : public BusinessPackage
{
	__int64 devID;  /**< �豸ID.*/
	__int64 num;    /**< ��������(�ӿ�ʼ����ǰ�ɼ�ʱ��).*/
	__int64 time;   /**< �ɼ�ʱ�䣨ʱ�����.*/
	PedFlowInfo()
	{
		business_type = static_cast<char>(BusinessType::PED_FLOW_INFO_TYPE);
	}
} ;//��������Ϣ

struct VehFlowInfo : public BusinessPackage
{
	__int64 devID;  /**< �豸ID.*/
	__int64 num;    /**< ��������(�ӿ�ʼ����ǰ�ɼ�ʱ��).*/
	__int64 time;   /**< �ɼ�ʱ�䣨ʱ�����.*/
	VehFlowInfo()
	{
		business_type = static_cast<char>(BusinessType::VEH_FLOW_INFO_TYPE);
	}
};//��������Ϣ

struct GetVideo : public BusinessPackage
{
	__int64 camid; /**< �豸ID. */
	char ip[16]; /**< Ŀ��IP. */
	int port; /**< Ŀ��˿�. */
	char osdFlag;/*�Ƿ����OSD��־*/
	char type;  /*!< ��Ƶ��������. */
	GetVideo()
	{
		business_type = static_cast<char>(BusinessType::GET_VIDEO_TYPE);
	}
};

struct StopVideo : public BusinessPackage
{
	__int64 camid; /**< �豸ID. */
	char ip[16]; /**< Ŀ��IP. */
	int port; /**< Ŀ��˿�. */
	char osdFlag;/*�Ƿ����OSD��־*/
	char type;  /*!< ��Ƶ��������. */
	StopVideo()
	{
		business_type = static_cast<char>(BusinessType::STOP_VIDEO_TYPE);
	}
} ;

struct CamInfo : public BusinessPackage
{
	//char ip[16];//�豸IP
	__int64 id;	//�豸id
	CamInfo()
	{
		business_type = static_cast<char>(BusinessType::CAMERA_INFO_TYPE);
	}
};

struct CamState : public BusinessPackage
{
	__int64 id;         /**< ID. */
	char ip[16];     /**< IP��ַ. */
	int port;         /**< port. */
	char username[20]; /**< �û��� . */
	char password[20];  /**< ����. */
	int cycle;		/*�ɼ�����*/
	__int64 initialFlow; /*��ʼ����*/
	int algFuncType;    /**< �㷨����. */
	int camstate;    /**< �����ϴ���־. */
	CamState()
	{
		business_type = static_cast<char>(BusinessType::CAMERA_STATE_TYPE);
	}
};

struct PortInfo : public BusinessPackage
{
	string ip;
	int port;
	int selfTcpPort;
	int udpVideoPort;
	int udpSendLoopPort;
	int udpRcvLoopPort;
	PortInfo()
	{
		business_type = static_cast<unsigned char>(BusinessType::PORT_INFO_TYPE);
		response_flag = false;
	}
};

struct NetPackage
{
	enum PackageType
	{
		BINARY,
		TEXT
	};
	//PackageType  package_type; // 0 binanry, 1 text
	char begin[7];	
	size_t business_data_size;
	char*  business_data;	
	char check;
	char end[5];
	NetPackage()
	{
		memcpy(begin, "#BEGIN#", 7);
		memcpy(end, "#END#", 5);
	}
};

#pragma pack(pop)   /**< �ָ�����״̬. */