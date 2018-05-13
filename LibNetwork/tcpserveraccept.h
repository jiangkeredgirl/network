#pragma once
  
#include "kutility.h"
#include <boost/asio.hpp>
#include "tcpserversocket.h"

using namespace boost::asio;
using ip::tcp;

class CTcpServerSocket;

class CTcpAccept
{
public:
	CTcpAccept(int port);
	virtual ~CTcpAccept();

public:	
	int RegisterHandler(ITcpServerHandler* tcpserver_handler);
	int Start();
	int AsyncStart();
	int Stop();
	list<shared_ptr<CTcpServerSocket>>& GetConnects();

private:
	int TcpAccepterRunThread(bool async);
	int StartAccept();
	int AsyncStartAccept();
	void AcceptHandler(shared_ptr<CTcpServerSocket> connect, boost::system::error_code error_code);
	void DisplayIP();
	shared_ptr<CTcpServerSocket> NewConnect();
	int DeleteConnect(shared_ptr<CTcpServerSocket> connect);

private:
	//int OnTcpRead(shared_ptr<CTcpServerSocket> connect, const char* data, size_t size, int status);
	//int OnTcpWrite(shared_ptr<CTcpServerSocket> connect, const char* data, size_t size, int status);
	//int OnTcpConnect(shared_ptr<CTcpServerSocket> connect, int status);
	int OnTcpDisconnect(shared_ptr<CTcpServerSocket> connect, int status);
	
private:
	std::thread        m_thread_server;
	mutex              m_mutex_server;
	condition_variable m_condition_server;

private:
	io_service        m_ioservice;
	shared_ptr<ip::tcp::acceptor> m_acceptor;
	list<shared_ptr<CTcpServerSocket>> m_connect_list;
	mutex              m_mutex_connect_list;
	ITcpServerHandler* m_tcpserver_handler;
	bool m_server_stop = false;
};

