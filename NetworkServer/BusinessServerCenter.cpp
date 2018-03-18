#include "BusinessServerCenter.h"
#include "BusinessProtocol.h"
#include "tcpserverhandler.h"

atomic<int> BusinessServerCenter::m_connect_increment = 0;

BusinessServerCenter::BusinessServerCenter()
{
}


BusinessServerCenter::~BusinessServerCenter()
{
}

BusinessServerCenter& BusinessServerCenter::instance()
{
	static BusinessServerCenter _instance;
	return _instance;
}

int BusinessServerCenter::Run()
{
	int error_code = 0;
	string input_flag;
	do
	{
		cout << "已启动业务服务器测试" << endl;
		cout << "输入函数名, c关闭客户端测试\n" << endl;
		RegisterSendFuns();
		cin >> input_flag;
		if (input_flag == "c")
		{
			break;
		}
		else
		{
			cout << "输入链接id\n" << endl;
			int connect_session = 0;
			cin >> connect_session;
			TestSendFun(input_flag, connect_session);
		}
	} while (true);
	cout << "客户端测试已关闭" << endl;
	return 0;
}

int BusinessServerCenter::AddConnectSession(shared_ptr<ITcpConnect> connect)
{
	m_connect_sessions[++m_connect_increment] = connect;
	cout << "add a connect session, sessionid:" << m_connect_increment 
		<< ", client ip:" << connect->RemoteIP() << ", client port:" << connect->RemotePort() << endl;
	return 0;
}

int BusinessServerCenter::SendCamConfig(shared_ptr<ITcpConnect> connect)
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
	SendBinaryData(connect, reinterpret_cast<char*>(&cam_config), sizeof(cam_config));
	return 0;
}

int BusinessServerCenter::SendBinaryData(shared_ptr<ITcpConnect> connect, const char* data, size_t size)
{
	/// 组包
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
	connect->AsyncWrite(binary_data, binary_data_size);
	delete[] binary_data;
	return 0;
}

int BusinessServerCenter::TestSendFun(const string& fun_name, int connect_session)
{
	if (m_sendfun.count(fun_name) && m_connect_sessions.count(connect_session))
	{
		m_sendfun[fun_name](m_connect_sessions[connect_session]);
	}
	else
	{
		cout << "不存在该函数名:" << fun_name << endl;
	}
	return 0;
}

int BusinessServerCenter::RegisterSendFuns()
{
	m_sendfun["SendCamConfig"] = std::bind(&BusinessServerCenter::SendCamConfig, this, std::placeholders::_1);
	cout << "test function name:" << "SendCamConfig" << endl;
	return 0;
}