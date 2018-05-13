#include "BusinessServerHandlerCenter.h"
#include "cstandard.h"
#include "BusinessProtocol.h"
#include "BusinessServerCenter.h"

BusinessServerHandlerCenter::BusinessServerHandlerCenter()
{
}


BusinessServerHandlerCenter::~BusinessServerHandlerCenter()
{
}

BusinessServerHandlerCenter& BusinessServerHandlerCenter::instance()
{
	static BusinessServerHandlerCenter _instance;
	return _instance;
}

int  BusinessServerHandlerCenter::OnTcpRead(shared_ptr<ITcpConnect> connect, const char* data, size_t size, int status)
{
	cout << "read_buffer size: " << size << endl;
	NetPackage package;
	do
	{
		if (sizeof(package.begin) + sizeof(package.business_data_size) >= size)
		{
			cout << "data error " << endl;
			break;
		}
		memcpy(&package.begin, data, sizeof(package.begin));
		memcpy(&package.business_data_size, data + sizeof(package.begin), sizeof(package.business_data_size));
		if (sizeof(package.begin) + sizeof(package.business_data_size) + package.business_data_size != size)
		{
			cout << "data error " << endl;
			break;
		}
		//if (package.package_type == NetPackage::BINARY)
		//{
		//	cout << "binary data" << endl;
		//}
		//else
		//{
		//	cout << "text data" << endl;
		//}
		ProccessBusinessProtocol(data + sizeof(package.begin) + sizeof(package.business_data_size)
			, package.business_data_size - sizeof(package.check) - sizeof(package.end));
	} while (false);	
	return 0;
}

int  BusinessServerHandlerCenter::OnTcpWrite(shared_ptr<ITcpConnect> connect, const char* data, size_t size, int status)
{
	cout << "\nwrite_buffer: " << data << endl;
	return 0;
}

int  BusinessServerHandlerCenter::OnTcpConnect(shared_ptr<ITcpConnect> connect, int status)
{
	BusinessServerCenter::instance().AddConnectSession(connect);
	return 0;
}

int  BusinessServerHandlerCenter::OnTcpDisconnect(shared_ptr<ITcpConnect> connect, int status)
{
	return 0;
}

int BusinessServerHandlerCenter::ProccessBusinessProtocol(const char* data, size_t size)
{
	unsigned char business_type;
	memcpy(&business_type, data, sizeof(business_type));
	switch (business_type)
	{
	case BusinessType::CAMERA_CONFIG_TYPE:
	{
		CameraConfig cam_config;
		if (size == sizeof(cam_config))
		{
			//memcpy(&cam_config.business_type, data, sizeof(cam_config.business_type));
			//memcpy(&cam_config.id, data + sizeof(cam_config.business_type), sizeof(cam_config.id));
			//memcpy(&cam_config.ip, data + sizeof(cam_config.business_type) + sizeof(cam_config.id), sizeof(cam_config.ip));
			//memcpy(&cam_config.port, data + sizeof(cam_config.business_type) + sizeof(cam_config.id) + sizeof(cam_config.ip), sizeof(cam_config.port));
			memcpy(&cam_config, data, size);
			cout << "process CameraConfig end" << endl;
		}		
		break;
	}
	case BusinessType::PED_FLOW_INFO_TYPE:
	{
		break;
	}
	case BusinessType::VEH_FLOW_INFO_TYPE:
	{
		break;
	}
	case BusinessType::GET_VIDEO_TYPE:
	{
		break;
	}
	case BusinessType::STOP_VIDEO_TYPE:
	{
		break;
	}
	case BusinessType::CAMERA_INFO_TYPE:
	{
		break;
	}
	case BusinessType::CAMERA_STATE_TYPE:
	{
		break;
	}
	default:
		break;
	}
	return 0;
}