#pragma once

#include "kutility.h"
#include "tcpserverhandler.h"
#include <boost/asio.hpp>
#include "NetDataPackage.h"

using namespace boost::asio;
using ip::tcp;
class CTcpServerSocket;

typedef function<int (shared_ptr<CTcpServerSocket> connect, int status)> f_connect;
typedef function<int (shared_ptr<CTcpServerSocket> connect, int status)> f_disconnect;
typedef function<int(shared_ptr<CTcpServerSocket> connect, const char* data, size_t size, int status)> f_read;
typedef function<int(shared_ptr<CTcpServerSocket> connect, const char* data, size_t size, int status)> f_write;

class CTcpServerSocket : public std::enable_shared_from_this<CTcpServerSocket>, public ITcpConnect
{
public:
	CTcpServerSocket(io_service& service);
	virtual ~CTcpServerSocket();

public:	
	virtual int Write(const char* data, size_t size) override;
	virtual int AsyncWrite(const char* data, size_t size) override;
	virtual string RemoteIP() override;
	virtual int RemotePort() override;

public:
	int RegisterHandler(f_connect connect_callback = nullptr, f_disconnect disconnect_callback = nullptr, f_read read_callback = nullptr, f_write write_callback = nullptr);
	void StartRead();
	void AsyncStartRead();
	void Disconnect();	
	tcp::socket& socket()	{ return m_socket; }

private:
	void DoWrite();
	void ReadHeader();
	void ReadBody();
	int ReadErrorCheck(boost::system::error_code ec, size_t readed_size, size_t require_read_size);
	int WriteErrorCheck(boost::system::error_code ec, size_t writed_size, size_t require_write_size);
	int ProcessSocketError(int error_code);

private:
	tcp::socket m_socket;
	//enum { MAXBUFFER = 1024 };
	//char m_readbuffer[MAXBUFFER];
	//char m_writebuffer[MAXBUFFER];
	shared_ptr<NetDataPackage> m_read_package;
	list<shared_ptr<NetDataPackage>> m_write_packages;
	f_connect          m_connect_callback;
	f_disconnect       m_disconnect_callback;
	f_read             m_read_callback;
	f_write            m_write_callback;
};

