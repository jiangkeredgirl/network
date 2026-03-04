#include "TcpPackage.h"
#include "cstandard.h"
#include "serialport.h"
#include "SerialPortCenter.h"
#include "SerialPortHandlerCenter.h"

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

int SerialPortCenter::Run(bool async)
{
	int error_code = 0;
	ISerialPort* serialport_client = NewSerialPort();
	do
	{
		cout << "please input com number, such as:1，2，3" << endl;
		string com_num;
		getline(std::cin, com_num);
		cout << "input baud rate, such as:9600,115200" << endl;
		string baudrate;
		getline(std::cin, baudrate);
		if (com_num.empty())
		{
			com_num = "COM1";
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
		cout << "please text for sending to server, \'c\' close connect\n" << endl;
		cin >> input_flag;
		if (input_flag == "c")
		{
			break;
		}
		else
		{
			if (async)
			{
				serialport_client->AsyncWrite(input_flag.c_str(), input_flag.size());
			}
			else
			{
				serialport_client->Write(input_flag.c_str(), input_flag.size());
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
	system("pause");
	return 0;
}
