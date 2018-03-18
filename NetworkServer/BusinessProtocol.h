#pragma once
#include <windows.h>
#include "cstandard.h"
using namespace std;
//#pragma warning(disable : C4309)

#pragma pack(push)  /**< 保存对齐状态. */
#pragma pack(1)     /**< 设定为1字节对齐. */

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
	char ip[16];     /**< IP地址. */
	int port;         /**< port. */
	char username[20]; /**< 用户名 . */
	char password[20];  /**< 密码. */
	int cycle;		/*采集周期*/
	__int64 initialFlow; /*初始流量*/
	int algFuncType;    /**< 算法功能. 103001：检人103002：检车	103003：全景103004：停车位*/
	int autoUpload;    /**< 主动上传标志. 0x01:主动上传，0x02不主动上传*/
	CameraConfig()
	{
		business_type = static_cast<char>(BusinessType::CAMERA_CONFIG_TYPE);
	}
};//链接摄像头

struct PedFlowInfo : public BusinessPackage
{
	__int64 devID;  /**< 设备ID.*/
	__int64 num;    /**< 行人数量(从开始到当前采集时间).*/
	__int64 time;   /**< 采集时间（时间戳）.*/
	PedFlowInfo()
	{
		business_type = static_cast<char>(BusinessType::PED_FLOW_INFO_TYPE);
	}
} ;//人流量信息

struct VehFlowInfo : public BusinessPackage
{
	__int64 devID;  /**< 设备ID.*/
	__int64 num;    /**< 车辆数量(从开始到当前采集时间).*/
	__int64 time;   /**< 采集时间（时间戳）.*/
	VehFlowInfo()
	{
		business_type = static_cast<char>(BusinessType::VEH_FLOW_INFO_TYPE);
	}
};//车流量信息

struct GetVideo : public BusinessPackage
{
	__int64 camid; /**< 设备ID. */
	char ip[16]; /**< 目标IP. */
	int port; /**< 目标端口. */
	char osdFlag;/*是否添加OSD标志*/
	char type;  /*!< 视频数据类型. */
	GetVideo()
	{
		business_type = static_cast<char>(BusinessType::GET_VIDEO_TYPE);
	}
};

struct StopVideo : public BusinessPackage
{
	__int64 camid; /**< 设备ID. */
	char ip[16]; /**< 目标IP. */
	int port; /**< 目标端口. */
	char osdFlag;/*是否添加OSD标志*/
	char type;  /*!< 视频数据类型. */
	StopVideo()
	{
		business_type = static_cast<char>(BusinessType::STOP_VIDEO_TYPE);
	}
} ;

struct CamInfo : public BusinessPackage
{
	//char ip[16];//设备IP
	__int64 id;	//设备id
	CamInfo()
	{
		business_type = static_cast<char>(BusinessType::CAMERA_INFO_TYPE);
	}
};

struct CamState : public BusinessPackage
{
	__int64 id;         /**< ID. */
	char ip[16];     /**< IP地址. */
	int port;         /**< port. */
	char username[20]; /**< 用户名 . */
	char password[20];  /**< 密码. */
	int cycle;		/*采集周期*/
	__int64 initialFlow; /*初始流量*/
	int algFuncType;    /**< 算法功能. */
	int camstate;    /**< 主动上传标志. */
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

#pragma pack(pop)   /**< 恢复对齐状态. */