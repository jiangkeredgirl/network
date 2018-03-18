#pragma once

#include "kutility.h"
#include "tcpserverhandler.h"
#include <boost/asio.hpp>
//#include <boost/bind.hpp>
//#include <boost/enable_shared_from_this.hpp>
//#include <boost/shared_ptr.hpp>

using namespace boost::asio;
using ip::tcp;
class CTcpConnect;

typedef function<int (shared_ptr<CTcpConnect> connect, int status)> f_connect;
typedef function<int (shared_ptr<CTcpConnect> connect, int status)> f_disconnect;
typedef function<int(shared_ptr<CTcpConnect> connect, const char* data, size_t size, int status)> f_read;
typedef function<int(shared_ptr<CTcpConnect> connect, const char* data, size_t size, int status)> f_write;

class CTcpConnect : public std::enable_shared_from_this<CTcpConnect>, public ITcpConnect
{
public:
	CTcpConnect(io_service& service);
	virtual ~CTcpConnect();

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
	void ReadHandler(boost::system::error_code err_code, size_t bytes_readed);
	void WriteHandler(boost::system::error_code err_code, size_t bytes_writed);
	tcp::socket& socket()	{ return m_socket; }

private:
	tcp::socket m_socket;
	enum { MAXBUFFER = 1024 };
	char m_readbuffer[MAXBUFFER];
	char m_writebuffer[MAXBUFFER];
	f_connect          m_connect_callback;
	f_disconnect       m_disconnect_callback;
	f_read             m_read_callback;
	f_write            m_write_callback;
};

