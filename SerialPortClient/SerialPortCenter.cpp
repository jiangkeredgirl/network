#include "TcpPackage.h"
#include "cstandard.h"
#include "serialport.h"
#include "serialportk.h"
#include "SerialPortCenter.h"
#include "SerialPortHandlerCenter.h"
#include "kspdlog.h"

using namespace std;

SerialPortCenter::SerialPortCenter()
{
}


SerialPortCenter::~SerialPortCenter()
{
}

SerialPortCenter& SerialPortCenter::instance()
{
	static SerialPortCenter _instance;
	return _instance;
}

int SerialPortCenter::Run()
{
#if 0
	TestSeriolport();
#else
	TestSeriolportk();
#endif
	return 0;
}


int SerialPortCenter::TestSeriolport()
{
	int error_code = 0;
	bool async = true;
	ISerialPort* serialport_client = NewSerialPort();
	do
	{		
		cout << "please input client type, defualt sync, a indicate async" << endl;
		string client_type;
		getline(std::cin, client_type);
		if (client_type == "a")
		{
			async = true;
		}
		else
		{
			async = false;
		}
		cout << "please input com number, defualt 4，such as:1、2、3" << endl;
		string com_num;
		getline(std::cin, com_num);
		cout << "input baud rate, defualt 115200，such as:9600、115200" << endl;
		string baudrate;
		getline(std::cin, baudrate);
		if (com_num.empty())
		{
			com_num = "COM4";
		}
		else
		{
			com_num = "COM" + com_num;
		}
		int baud_rate = 115200;
		if (!baudrate.empty())
		{
			baud_rate = stoi(baudrate);
		}
		static SerialPortHandlerCenter handler_center(com_num, baud_rate, serialport_client);
		error_code = serialport_client->RegisterHandler(&handler_center);
		if (async)
		{
			error_code = serialport_client->AsyncConnect(com_num, baud_rate);
			if (error_code == 0)
			{
				cout << "serialpot async client have connected, com number:" << com_num << ", baudrate:" << baud_rate << endl;
			}
			else
			{
				cout << "serialpot async client connect occur error, com number:" << com_num << ", baudrate:" << baud_rate << endl;
			}
		}
		else
		{
			error_code = serialport_client->Connect(com_num, baud_rate);
			if (error_code == 0)
			{
				cout << "serialpot sync client have connected, com number:" << com_num << ", baudrate:" << baud_rate << endl;
			}
			else
			{
				cout << "serialpot sync client connect ocurr error, com number:" << com_num << ", baudrate:" << baud_rate << endl;
			}
		}
	} while (error_code != 0);
	string input_flag;
	do
	{
		cout << "please text for writing to serialport, \'c\' close connect\n" << endl;
		cin >> input_flag;
		if (input_flag == "c")
		{
			break;
		}
		else
		{
			vector<char> data(input_flag.begin(), input_flag.end());
			if (async)
			{
				serialport_client->AsyncWrite(data);
			}
			else
			{
				serialport_client->Write(data);
				vector<char> response_data;
				int timeout_ms = 10000;
				serialport_client->Write(data, response_data, timeout_ms);
				cout << "[Response] Read: " << response_data.size() << " bytes" << endl;
				cout << "[Response Data Hex] ";
				for (size_t i = 0; i < response_data.size(); i++)
				{
					cout << hex << (0xFF & response_data[i]) << " ";
				}
				cout << dec << endl;
				string str(response_data.begin(), response_data.end());
				cout << "[Response Data Char]" << str << endl;
			}
		}
	} while (true);

	if (error_code == 0)
	{
		serialport_client->Disconnect();
	}
	cout << "serialport client have disconnected" << endl;
	if (serialport_client)
	{
		DeleteSerialPort(serialport_client);
	}	
	return 0;
}

int SerialPortCenter::TestSeriolportk()
{
	int error_code = 0;
	ISerialPortk* serialport_client = NewSerialPortk();
	do
	{
		cout << "please input com number, defualt 4，such as:1、2、3" << endl;
		string com_num;
		getline(std::cin, com_num);
		cout << "input baud rate, defualt 115200，such as:9600、115200" << endl;
		string baudrate;
		getline(std::cin, baudrate);
		if (com_num.empty())
		{
			com_num = "COM4";
		}
		else
		{
			com_num = "COM" + com_num;
		}
		int baud_rate = 115200;
		if (!baudrate.empty())
		{
			baud_rate = stoi(baudrate);
		}
		serialport_client->RegisterHandler(nullptr
			, std::bind(&SerialPortCenter::onReadk, this, std::placeholders::_1)
			, std::bind(&SerialPortCenter::onErrork, this, std::placeholders::_1, std::placeholders::_2));
		error_code = serialport_client->Connect(com_num);
		if (error_code == 0)
		{
			cout << "serialpot sync client have connected, com number:" << com_num << ", baudrate:" << baud_rate << endl;
		}
		else
		{
			cout << "serialpot sync client connect ocurr error, com number:" << com_num << ", baudrate:" << baud_rate << endl;
		}
	} while (error_code != 0);
	string input_flag;
	do
	{
		cout << "please text for writing to serialport, \'c\' close connect\n" << endl;
		cin >> input_flag;
		if (input_flag == "c")
		{
			break;
		}
		else
		{
			string data(input_flag.begin(), input_flag.end());


			serialport_client->WriteHexStr(data);
			string response_data;
			int timeout_ms = 10000;
			serialport_client->WriteHexStr(data, response_data, timeout_ms);
			cout << "[Response] Read: " << response_data.size() << " bytes" << endl;
			cout << "[Response Data Hex] ";
			for (size_t i = 0; i < response_data.size(); i++)
			{
				cout << hex << (0xFF & response_data[i]) << " ";
			}
			cout << dec << endl;
			string str(response_data.begin(), response_data.end());
			cout << "[Response Data Char]" << str << endl;

		}
	} while (true);

	if (error_code == 0)
	{
		serialport_client->Disconnect();
	}
	cout << "serialport client have disconnected" << endl;
	if (serialport_client)
	{
		DeleteSerialPortk(serialport_client);
	}
	return 0;
}


void SerialPortCenter::onReadk(const string& hexstr)
{
	LOG_INFO("readed hexstr:{}", hexstr);
}

void SerialPortCenter::onErrork(int error_code, string error_msg)
{
	if (error_code == 1237)
	{
		LOG_WARN("error_code:{}, error_msg:{}", error_code, localtoutf8(error_msg));
	}
	else
	{
		LOG_ERROR("error_code:{}, error_msg:{}", error_code, localtoutf8(error_msg));
	}
}