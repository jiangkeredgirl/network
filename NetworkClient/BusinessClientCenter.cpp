#include "BusinessClientCenter.h"
#include "TcpClientCenter.h"
#include "../NetworkServer/BusinessProtocol.h"
#include "cstandard.h"
using namespace std;

BusinessClientCenter::BusinessClientCenter()
{
}


BusinessClientCenter::~BusinessClientCenter()
{
}

BusinessClientCenter& BusinessClientCenter::instance()
{
	static BusinessClientCenter _instance;
	return _instance;
}

int BusinessClientCenter::Run()
{
	int error_code = 0;
	string input_flag;
	do
	{
		cout << "ÒÑÆô¶¯ÒµÎñ¿Í»§¶Ë²âÊÔ" << endl;
		cout << "ÊäÈëº¯ÊýÃû, c¹Ø±Õ¿Í»§¶Ë²âÊÔ\n" << endl;
		RegisterSendFuns();
		cin >> input_flag;
		if (input_flag == "c")
		{
			break;
		}
		else
		{
			TestSendFun(input_flag);
		}
	} while (true);
	cout << "¿Í»§¶Ë²âÊÔÒÑ¹Ø±Õ" << endl;
	return 0;
}

int BusinessClientCenter::SendCamConfig()
{
	CameraConfig cam_config;
	cam_config.id = 87;
	strcpy_s(cam_config.ip, 16, "199.199.199.199");
	cam_config.port = 34;
	strcpy_s(cam_config.username, 16, "kkk");
	strcpy_s(cam_config.password, 16, "123");
	cam_config.cycle = 3;
	cam_config.initialFlow = 50;
	cam_config.algFuncType = 103002;
	cam_config.autoUpload = 0x02;
	//size_t data_size = sizeof(cam_config);
	//char* data = new char[data_size];
	//memset(data, 0, data_size);
	//memcpy(data, &cam_config.business_type, sizeof(cam_config.business_type));
	//memcpy(data + sizeof(cam_config.business_type), &cam_config.id, sizeof(cam_config.id));
	//memcpy(data + sizeof(cam_config.business_type) + sizeof(cam_config.id), &cam_config.ip, sizeof(cam_config.ip));
	//memcpy(data + sizeof(cam_config.business_type) + sizeof(cam_config.id) + sizeof(cam_config.ip)
	//	, &cam_config.port, sizeof(cam_config.port));
	//SendBinaryData(data, data_size);
	SendBinaryData(reinterpret_cast<char*>(&cam_config), sizeof(cam_config));
	return 0;
}

int BusinessClientCenter::SendBinaryData(const char* data, size_t size)
{
	/// ×é°ü
	NetPackage package;
	//package.package_type = NetPackage::BINARY;
	package.business_data_size = size + sizeof(package.check) + sizeof(package.end);
	package.business_data = const_cast<char*>(data);
	size_t binary_data_size = sizeof(package.begin) + sizeof(package.business_data_size) + package.business_data_size;
	char* binary_data = new char[binary_data_size];
	memset(binary_data, 0, binary_data_size);
	memcpy(binary_data, package.begin, sizeof(package.begin));
	memcpy(binary_data + sizeof(package.begin), &package.business_data_size, sizeof(package.business_data_size));
	memcpy(binary_data + sizeof(package.begin) + sizeof(package.business_data_size), package.business_data, size);
	memcpy(binary_data + sizeof(package.begin) + sizeof(package.business_data_size) + size, &package.check, sizeof(package.check));
	memcpy(binary_data + sizeof(package.begin) + sizeof(package.business_data_size) + size + sizeof(package.check), package.end, sizeof(package.end));
	cout << "send data size:" << binary_data_size << endl;
	TcpClientCenter::instance().SendData(binary_data, binary_data_size);
	delete[] binary_data;
	return 0;
}

int BusinessClientCenter::TestSendFun(const string& fun_name)
{
	if (m_sendfun.count(fun_name))
	{
		m_sendfun[fun_name]();
	}
	else
	{
		cout << "²»´æÔÚ¸Ãº¯ÊýÃû:" << fun_name << endl;
	}
	return 0;
}

int BusinessClientCenter::RegisterSendFuns()
{
	m_sendfun["SendCamConfig"] = std::bind(&BusinessClientCenter::SendCamConfig, this);
	cout << "test function name:" << "SendCamConfig" << endl;
	return 0;
}