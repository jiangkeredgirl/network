#pragma once
#ifdef LIBSERIALPORT_EXPORTS
#define LIBSERIALPORT_API __declspec(dllexport)
#else
#define LIBSERIALPORT_API __declspec(dllimport)
#endif

#include <string>
#include <functional>

using namespace std;

using any_type = std::string;  // 串口名类型，如 "COM3" 或 "/dev/ttyUSB0"
// 回调函数类型定义
using ReadBytesFunction   = std::function<void(const std::vector<std::byte>& readed_bytes)>;
using ReadHexStrFunction  = std::function<void(const std::string& readed_hexs)>;
using SerialErrorFunction = std::function<void(int errorcode, const std::string& errormsg)>;


class ISerialPortk
{
public:
	virtual int RegisterHandler(ReadBytesFunction read_byte_callback, ReadHexStrFunction read_hex_callback, SerialErrorFunction error_callback) = 0;
	virtual int Connect(const any_type& port) = 0;
	virtual int Disconnect() = 0;
	virtual int WriteHexStr(const string& wirte_hexstr) = 0;
	virtual int WriteHexStr(const string& wirte_hexstr, string& read_hexstr, int timeout_msec = 1000) = 0;
	virtual ~ISerialPortk() = default;
};



#ifdef __cplusplus
extern "C" {
#endif

	LIBSERIALPORT_API ISerialPortk* NewSerialPortk(void);
	typedef ISerialPortk* (*NewSerialPortkFun)(void);
	LIBSERIALPORT_API void DeleteSerialPortk(ISerialPortk* serial_port);
	typedef void (*DeleteSerialPortkFun)(ISerialPortk* serial_port);

#ifdef __cplusplus
}
#endif

