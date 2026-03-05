#pragma once
#ifdef LIBSERIALPORT_EXPORTS
#define LIBSERIALPORT_API __declspec(dllexport)
#else
#define LIBSERIALPORT_API __declspec(dllimport)
#endif

#include <string>
#include <memory>
#include <cstddef>
#include <codecvt>
#include <thread>
#include <mutex>
#include <atomic>
#include <functional>
#include <condition_variable>

using namespace std;



typedef string any_type;
typedef std::function<void(const std::vector<std::byte>& bytes)>     ReadBytesFunction;
typedef std::function<void(const std::string& hexstr)>               ReadHexStrFunction;
typedef std::function<void(int error_code, std::string error_msg)>   SerialErrorFunction;

class ISerialPortk
{
public:
	virtual int RegisterHandler(ReadBytesFunction read_byte_callback, ReadHexStrFunction read_hex_callback, SerialErrorFunction error_callback) = 0;
	virtual int Connect(const any_type& port) = 0;
	virtual int Disconnect() = 0;
	virtual int WriteHexStr(const string& wirte_hexstr) = 0;
	virtual int WriteHexStr(const string& wirte_hexstr, string& read_hexstr, int timeout_msec = 1000) = 0;
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

