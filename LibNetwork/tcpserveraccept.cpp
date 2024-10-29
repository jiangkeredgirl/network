#include "tcpserveraccept.h"
#include "kloglib.h"

CTcpAccept::CTcpAccept(int port)
{
	m_tcpserver_handler = nullptr;
	m_acceptor = shared_ptr<ip::tcp::acceptor>(new ip::tcp::acceptor(m_ioservice, tcp::endpoint(tcp::v4(), port)));
	// 打印当前服务器地址  
	TraceInfoCout << "server addr: " << m_acceptor->local_endpoint().address() << ", server port: " << m_acceptor->local_endpoint().port();
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
	m_server_stop = false;
	m_thread_server = std::thread(std::bind(&CTcpAccept::TcpAccepterRunThread, this, false));
	return 0;
}

int CTcpAccept::AsyncStart()
{
	m_server_stop = false;
	m_thread_server = std::thread(std::bind(&CTcpAccept::TcpAccepterRunThread, this, true));
	return 0;
}

int CTcpAccept::Stop()
{
	m_server_stop = true;
	while (!GetConnects().empty())
	{	
		auto item = *GetConnects().begin();
		item->Disconnect();
	}
	GetConnects().clear();
	TraceInfoCout << "disconnect all connects";
	m_ioservice.stop();
	if (m_thread_server.joinable())
	{
		TraceInfoCout << "waiting TcpAccepterRunThread end";
		m_thread_server.join();
	}
	m_acceptor->close();
	TraceInfoCout << "acceptor closed";
	return 0;
}

int CTcpAccept::TcpAccepterRunThread(bool async)
{
	TrackCout;
	if (async)
	{
		AsyncStartAccept();
		TraceInfoCout << "tcp server accepter runing";
		m_ioservice.reset();
		m_ioservice.restart();
		m_ioservice.run();
		m_ioservice.reset();
		TraceInfoCout << "tcp server accepter run over";
	}
	else
	{
		while (true)
		{
			if (m_server_stop)
			{
				break;
			}
			StartAccept();
		}
	}
	return 0;
}

int CTcpAccept::StartAccept()
{
	shared_ptr<CTcpServerSocket> connect = NewConnect();
	m_acceptor->accept(connect->socket());
	TraceInfoCout << "tcp server accept a connect, client ip:" << connect->socket().remote_endpoint().address();
	m_connect_list.push_back(connect);
	TraceInfoCout << "current connects count is " << GetConnects().size();
	connect->StartRead();
	return 0;
}

int CTcpAccept::AsyncStartAccept()
{
	shared_ptr<CTcpServerSocket> connect = NewConnect();	
	m_acceptor->async_accept(connect->socket(), bind(&CTcpAccept::AcceptHandler, this, connect, std::placeholders::_1));
	return 0;
}

void CTcpAccept::AcceptHandler(shared_ptr<CTcpServerSocket> connect, asio::error_code ec)
{
	if (ec)
	{
		TraceErrorCout << "tcp server accept a connect occur error, client ip:" << connect->socket().remote_endpoint().address() << ", error code : " << ec.value() << ", error message : " << ec.message();
	}
	else
	{
		TraceOKCout << "tcp server accept a connect success, client ip:" << connect->socket().remote_endpoint().address();
		// 继续等待连接
		AsyncStartAccept();
		m_connect_list.push_back(connect);
		TraceInfoCout << "current connects count is " << GetConnects().size();
		connect->AsyncStartRead();
	}
}

void CTcpAccept::DisplayIP()
{
	//tcp::resolver resolver(m_ioservice);
	//tcp::resolver::query query(asio::ip::host_name(), "");
	//tcp::resolver::iterator iter = resolver.resolve(query);
	//tcp::resolver::iterator end; // End marker.  
	//while (iter != end)
	//{
	//	tcp::endpoint ep = *iter++;
	//	std::cout << ep.address().to_string() << std::endl;
	//}
	asio::io_service io_service;
	tcp::resolver resolver(io_service);
	tcp::resolver::query query(asio::ip::host_name(), "");
	tcp::resolver::iterator iter = resolver.resolve(query);
	tcp::resolver::iterator end; // End marker.
	while (iter != end)
	{
		tcp::endpoint ep = *iter++;
		TraceInfoCout << "ip:" << ep.address().to_string() << ", port:" << ep.port();
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
	if (m_tcpserver_handler)
	{
		connect->RegisterHandler(
			std::bind(&ITcpServerHandler::OnTcpConnect, m_tcpserver_handler, std::placeholders::_1, std::placeholders::_2)
			, std::bind(&CTcpAccept::OnTcpDisconnect, this, std::placeholders::_1, std::placeholders::_2)
			, std::bind(&ITcpServerHandler::OnTcpRead, m_tcpserver_handler, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4)
			, std::bind(&ITcpServerHandler::OnTcpWrite, m_tcpserver_handler, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	}
	else
	{
		connect->RegisterHandler(
			nullptr
			, std::bind(&CTcpAccept::OnTcpDisconnect, this, std::placeholders::_1, std::placeholders::_2)
			, nullptr
			, nullptr);
	}
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
	TraceInfoCout << "current connects count is " << GetConnects().size();
	return 0;
}

//int  CTcpAccept::OnTcpRead(shared_ptr<CTcpServerSocket> connect, const char* data, size_t size, int status)
//{
//	cout << "from client ip:" << connect->socket().remote_endpoint().address() << "read completed, data:" << endl;
//	cout.write(data, size) << endl;
//	if (m_tcpserver_handler && size > 0)
//	{
//		m_tcpserver_handler->OnTcpRead(connect, data, size, status);
//	}
//	return 0;
//}

//int  CTcpAccept::OnTcpWrite(shared_ptr<CTcpServerSocket> connect, const char* data, size_t size, int status)
//{
//	cout << "to client ip:" << connect->socket().remote_endpoint().address() << " write completed, data:" << endl;
//	cout.write(data, size) << endl;
//	if (m_tcpserver_handler)
//	{
//		m_tcpserver_handler->OnTcpWrite(connect, data, size, status);
//	}
//	return 0;
//}

//int  CTcpAccept::OnTcpConnect(shared_ptr<CTcpServerSocket> connect, int status)
//{
//	cout << "a connect connected from ip:" << connect->socket().remote_endpoint().address() << std::endl;
//	cout << "current connect count is " << GetConnects().size() << std::endl;
//	if (m_tcpserver_handler)
//	{
//		m_tcpserver_handler->OnTcpConnect(connect, status);
//	}
//	return 0;
//}

int  CTcpAccept::OnTcpDisconnect(shared_ptr<CTcpServerSocket> connect, int status)
{
	DeleteConnect(connect);
	if (m_tcpserver_handler)
	{
		m_tcpserver_handler->OnTcpDisconnect(connect, status);
	}
	return 0;
}