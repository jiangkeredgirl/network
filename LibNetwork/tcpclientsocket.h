#pragma once

#include "kutility.h"
#include <boost/asio.hpp>

using namespace boost::asio;
using ip::tcp;

typedef function<int (int status)> f_connect;
typedef function<int (int status)> f_disconnect;
typedef function<int(const char* data, size_t size, int status)> f_read;
typedef function<int(const char* data, size_t size, int status)> f_write;

class CTcpClientSocket : public enable_shared_from_this<CTcpClientSocket>
{
private:
	CTcpClientSocket();
public:
	virtual ~CTcpClientSocket();
public:
	static CTcpClientSocket& instance();
public:
	int RegisterHandler(f_connect connect_callback = nullptr, f_disconnect disconnect_callback = nullptr, f_read read_callback = nullptr, f_write write_callback = nullptr);
	int Connect(const string& ip, int port);
	int AsyncConnect(const string& ip, int port);
	int Disconnect();
	int Write(const char* data, size_t size);
	int AsyncWrite(const char* data, size_t size);

private:
	int  SocketClientRunThread(bool async);
	void ConnectHandler(const boost::system::error_code& error);
	void DisconnectHandler();
	void ReadHandler(boost::system::error_code err_code, size_t bytes_readed);
	void WriteHandler(boost::system::error_code err_code, size_t bytes_writed);

	f_connect          m_connect_callback;
	f_disconnect       m_disconnect_callback;
	f_read             m_read_callback;
	f_write            m_write_callback;

	std::thread        m_thread_client;
	mutex              m_mutex_client;
	condition_variable m_condition_client;

	io_service         m_ioservice;
	tcp::socket        m_socket;

	enum { MAXBUFFER = 1024 };
	char m_readbuffer[MAXBUFFER];
	char m_writebuffer[MAXBUFFER];
};

