#include "tcpserveraccept.h"


CTcpAccept::CTcpAccept(int port)
{
	m_tcpserver_handler = nullptr;
	//m_acceptor = shared_ptr<ip::tcp::acceptor>(new ip::tcp::acceptor(m_ioservice, tcp::endpoint(tcp::v4(), port)));
	//// ´òÓ¡µ±Ç°·þÎñÆ÷µØÖ·  
	//std::cout << "server addr: " << m_acceptor->local_endpoint().address() << ", server port: " << m_acceptor->local_endpoint().port() << std::endl;
	//DisplayIP();

	m_acceptor = shared_ptr<ip::tcp::acceptor>(new ip::tcp::acceptor(m_ioservice, tcp::endpoint(tcp::v4(), port)));
	// ´òÓ¡µ±Ç°·þÎñÆ÷µØÖ·  
	std::cout << "server addr: " << m_acceptor->local_endpoint().address() << ", server port: " << m_acceptor->local_endpoint().port() << std::endl;
	DisplayIP();
}

CTcpAccept::~CTcpAccept()
{
}

int CTcpAccept::RegisterHandler(ITcpServerHandler* tcpserver_handler)
{
	m_tcpserver_handler = tcpserver_handler;
	return 0;
}

int CTcpAccept::Start()
{
	m_thread_server = thread(bind(&CTcpAccept::TcpAccepterRunThread, this, false));	
	return 0;
}

int CTcpAccept::AsyncStart()
{
	m_thread_server = thread(bind(&CTcpAccept::TcpAccepterRunThread, this, true));	
	return 0;
}

int CTcpAccept::Stop()
{
	while (!GetConnects().empty())
	{	
		auto item = *GetConnects().begin();
		item->Disconnect();
	}
	m_ioservice.stop();
	if (m_thread_server.joinable())
	{
		cout << "waiting TcpAccepterRunThread end" << endl;
		m_thread_server.join();
	}
	m_acceptor->close();
	return 0;
}

int CTcpAccept::TcpAccepterRunThread(bool async)
{
	if (async)
	{
		AsyncStartAccept();
		cout << "tcp server accepter runing" << endl;
		m_ioservice.run();
		cout << "tcp server accepter run over" << endl;
	}
	else
	{
		while (true)
		{
			StartAccept();
		}
	}
	return 0;
}

int CTcpAccept::StartAccept()
{
	shared_ptr<CTcpServerSocket> connect = NewConnect();
	m_acceptor->accept(connect->socket());
	cout << "tcp server accept a connect, client ip:" << connect->socket().remote_endpoint().address() << endl;
	m_connect_list.push_back(connect);
	connect->StartRead();
	return 0;
}

int CTcpAccept::AsyncStartAccept()
{
	shared_ptr<CTcpServerSocket> connect = NewConnect();	
	m_acceptor->async_accept(connect->socket(), bind(&CTcpAccept::AcceptHandler, this, connect, std::placeholders::_1));
	return 0;
}

void CTcpAccept::AcceptHandler(shared_ptr<CTcpServerSocket> connect, boost::system::error_code ec)
{
	cout << "tcp server accept a connect, client ip:" << connect->socket().remote_endpoint().address() << ", error code : " << ec.value() << ", error message : " << ec.message() << endl;
	if (!ec)
	{
		// ¼ÌÐøµÈ´ýÁ¬½Ó
		AsyncStartAccept();
		m_connect_list.push_back(connect);
		connect->AsyncStartRead();
	}
}

void CTcpAccept::DisplayIP()
{
	//tcp::resolver resolver(m_ioservice);
	//tcp::resolver::query query(boost::asio::ip::host_name(), "");
	//tcp::resolver::iterator iter = resolver.resolve(query);
	//tcp::resolver::iterator end; // End marker.  
	//while (iter != end)
	//{
	//	tcp::endpoint ep = *iter++;
	//	std::cout << ep.address().to_string() << std::endl;
	//}
	boost::asio::io_service io_service;
	tcp::resolver resolver(io_service);
	tcp::resolver::query query(boost::asio::ip::host_name(), "");
	tcp::resolver::iterator iter = resolver.resolve(query);
	tcp::resolver::iterator end; // End marker.
	while (iter != end)
	{
		tcp::endpoint ep = *iter++;
		std::cout << "ip:" << ep.address().to_string() << ", port:" << ep.port() << std::endl;
	}
}

list<shared_ptr<CTcpServerSocket>>& CTcpAccept::GetConnects() 
{ 
	return m_connect_list;
}

shared_ptr<CTcpServerSocket> CTcpAccept::NewConnect()
{
	lock_guard<mutex> lock(m_mutex_connect_list);
	shared_ptr<CTcpServerSocket> connect(new CTcpServerSocket(m_ioservice));
	connect->RegisterHandler(
		std::bind(&CTcpAccept::OnTcpConnect, this, std::placeholders::_1, std::placeholders::_2)
		, std::bind(&CTcpAccept::OnTcpDisconnect, this, std::placeholders::_1, std::placeholders::_2)
		, std::bind(&CTcpAccept::OnTcpRead, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4)
		, std::bind(&CTcpAccept::OnTcpWrite, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	//m_connect_list.push_back(connect);
	return connect;
}

int CTcpAccept::DeleteConnect(shared_ptr<CTcpServerSocket> connect)
{
	lock_guard<mutex> lock(m_mutex_connect_list);
	for (list<shared_ptr<CTcpServerSocket>>::iterator item = m_connect_list.begin(); item != m_connect_list.end(); ++item)
	{
		if (*item == connect)
		{
			m_connect_list.erase(item);
			break;
		}
	}
	return 0;
}

int  CTcpAccept::OnTcpRead(shared_ptr<CTcpServerSocket> connect, const char* data, size_t size, int status)
{
	cout << "from client ip:" << connect->socket().remote_endpoint().address() << "read completed, data:" << endl;
	cout.write(data, size) << endl;
	if (m_tcpserver_handler && size > 0)
	{
		m_tcpserver_handler->OnTcpRead(connect, data, size, status);
	}
	return 0;
}

int  CTcpAccept::OnTcpWrite(shared_ptr<CTcpServerSocket> connect, const char* data, size_t size, int status)
{
	cout << "to client ip:" << connect->socket().remote_endpoint().address() << " write completed, data:" << endl;
	cout.write(data, size) << endl;
	if (m_tcpserver_handler)
	{
		m_tcpserver_handler->OnTcpWrite(connect, data, size, status);
	}
	return 0;
}

int  CTcpAccept::OnTcpConnect(shared_ptr<CTcpServerSocket> connect, int status)
{
	cout << "a connect connected from ip:" << connect->socket().remote_endpoint().address() << std::endl;
	cout << "current connect count is " << GetConnects().size() << std::endl;
	if (m_tcpserver_handler)
	{
		m_tcpserver_handler->OnTcpConnect(connect, status);
	}
	return 0;
}

int  CTcpAccept::OnTcpDisconnect(shared_ptr<CTcpServerSocket> connect, int status)
{
	DeleteConnect(connect);
	cout << "current connect count is " << GetConnects().size() << std::endl;
	if (m_tcpserver_handler)
	{
		m_tcpserver_handler->OnTcpDisconnect(connect, status);
	}
	return 0;
}