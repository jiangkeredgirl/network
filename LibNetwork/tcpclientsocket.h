#pragma once

#include "kutility.h"
#include <boost/asio.hpp>
#include "NetDataPackage.h"

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
	int SetReconnectTime(__int64 reconnect_second);
	int Connect(const string& ip, int port);
	int AsyncConnect(const string& ip, int port);
	int Disconnect();
	int Write(const char* data, size_t size);
	int AsyncWrite(const char* data, size_t size);

private:
	int  SocketClientRunThread(bool async);
	void DoWrite();	
	void ReadHeader();
	void ReadBody();
	int ReadErrorCheckHandler(boost::system::error_code ec, size_t readed_size, size_t require_read_size);
	int WriteErrorCheckHandler(boost::system::error_code ec, size_t writed_size, size_t require_write_size);
	//int Reconnect();
	//int DoConnect();

private:
	f_connect          m_connect_callback;
	f_disconnect       m_disconnect_callback;
	f_read             m_read_callback;
	f_write            m_write_callback;

	std::thread        m_thread_client;
	mutex              m_mutex_client;
	condition_variable m_condition_client;

	io_service         m_ioservice;
	tcp::socket        m_socket;

	//enum { MAXBUFFER = 1024 };
	shared_ptr<NetDataPackage> m_read_package;
	list<shared_ptr<NetDataPackage>> m_write_packages;
	__int64 m_reconnect_second = 0;
	bool m_keep_connect = false;
	bool m_sync_connect = false;
};

