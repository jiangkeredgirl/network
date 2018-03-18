#include "tcpserverconnect.h"

CTcpConnect::CTcpConnect(io_service& service) :m_socket(service)
{

}


CTcpConnect::~CTcpConnect()
{
}

int CTcpConnect::RegisterHandler(f_connect connect_callback, f_disconnect disconnect_callback, f_read read_callback, f_write write_callback)
{
	m_connect_callback = connect_callback;
	m_disconnect_callback = disconnect_callback;
	m_read_callback = read_callback;
	m_write_callback = write_callback;
	return 0;
}

int CTcpConnect::Write(const char* data, size_t size)
{
	boost::system::error_code ec;
	size_t write_size = 0;
	if(m_socket.is_open())
	{
		write_size = m_socket.write_some(buffer(data, size), ec);
	}
	return ec.value();
}

int CTcpConnect::AsyncWrite(const char* data, size_t size)
{
	memset(m_writebuffer, 0, sizeof(m_writebuffer));
	memcpy(m_writebuffer, data, size);
	m_socket.async_write_some(buffer(m_writebuffer, MAXBUFFER), std::bind(&CTcpConnect::WriteHandler, this, std::placeholders::_1, std::placeholders::_2));
	return 0;
}

string CTcpConnect::RemoteIP()
{
	return m_socket.remote_endpoint().address().to_string();
}

int CTcpConnect::RemotePort()
{
	return m_socket.remote_endpoint().port();
}

void CTcpConnect::StartRead()
{
	cout << "tcp server accept a connet, client ip:" << m_socket.remote_endpoint().address().to_string() << endl;
	cout << "tcp server start read data and add current connect to connects list" << endl;
	if (m_connect_callback)
	{
		m_connect_callback(shared_from_this(), 0);
	}
	boost::system::error_code ec;
	size_t bytes_readed = m_socket.read_some(buffer(m_readbuffer, MAXBUFFER), ec);
	if (ec)
	{
		Disconnect();
	}
	else
	{
		if (m_read_callback)
		{
			m_read_callback(shared_from_this(), m_readbuffer, bytes_readed, 0);
		}
		memset(m_readbuffer, 0, sizeof(m_readbuffer));
	}
}

void CTcpConnect::AsyncStartRead()
{
	cout << "tcp server accept a connet, client ip:" << m_socket.remote_endpoint().address().to_string() << endl;
	cout << "tcp server start read data and add current connect to connects list" << endl;
	if (m_connect_callback)
	{
		m_connect_callback(shared_from_this(), 0);
	}
	m_socket.async_read_some(buffer(m_readbuffer, MAXBUFFER), std::bind(&CTcpConnect::ReadHandler, this, std::placeholders::_1, std::placeholders::_2));
}

void CTcpConnect::Disconnect()
{
	cout << "tcp server socket closed and delete current connect from connects list" << endl;
	boost::system::error_code ec;
	if (m_socket.is_open())
	{
		//cout << "tcp server disconected a connet, client ip:" << m_socket.remote_endpoint().address().to_string() << endl;
		m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
		m_socket.close(ec); // excute this statement occure crash
	}
	//shared_ptr<CTcpConnect> connect = shared_from_this();
	//if (m_disconnect_callback)
	//{
	//	m_disconnect_callback(connect, 0);
	//}
}

void CTcpConnect::ReadHandler(boost::system::error_code ec, size_t bytes_readed)
{
	cout  << "tcp serever read, error code :" << ec.value() << ", error message:"  <<ec.message() << endl;
	if (ec)	
	{
		//Disconnect();
		shared_ptr<CTcpConnect> connect = shared_from_this();
		if (m_disconnect_callback)
		{
			m_disconnect_callback(connect, 0);
		}
	}
	else
	{
		if (m_read_callback)
		{
			m_read_callback(shared_from_this(), m_readbuffer, bytes_readed, 0);
		}
		memset(m_readbuffer, 0, sizeof(m_readbuffer));
		m_socket.async_read_some(buffer(m_readbuffer, MAXBUFFER), std::bind(&CTcpConnect::ReadHandler, this, std::placeholders::_1, std::placeholders::_2));
	}	
}

void CTcpConnect::WriteHandler(boost::system::error_code ec, size_t bytes_writed)
{
	cout  << "tcp serever write, error code :" << ec.value() << ", error message:"  <<ec.message() << endl;
	if (ec)	
	{
		//Disconnect();
		shared_ptr<CTcpConnect> connect = shared_from_this();
		if (m_disconnect_callback)
		{
			m_disconnect_callback(connect, 0);
		}
	}
	else
	{
		if (m_write_callback)
		{
			m_write_callback(shared_from_this(), m_writebuffer, bytes_writed, 0);
		}
		memset(m_writebuffer, 0, sizeof(m_writebuffer));
	}	
}