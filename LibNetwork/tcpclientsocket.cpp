#include "tcpclientsocket.h"
#include <boost/bind.hpp>


CTcpClientSocket::CTcpClientSocket() : m_ioservice(), m_socket(m_ioservice)
{
}


CTcpClientSocket::~CTcpClientSocket()
{
}

CTcpClientSocket& CTcpClientSocket::instance()
{
	static CTcpClientSocket _instance;
	return _instance;
}

int CTcpClientSocket::RegisterHandler(f_connect connect_callback, f_disconnect disconnect_callback, f_read read_callback, f_write write_callback)
{
	m_connect_callback = connect_callback;
	m_disconnect_callback = disconnect_callback;
	m_read_callback = read_callback;
	m_write_callback = write_callback;
	return 0;
}

int CTcpClientSocket::Connect(const string& ip, int port)
{
	m_thread_client = thread(bind(&CTcpClientSocket::SocketClientRunThread, this, false));
	ip::tcp::endpoint ep(boost::asio::ip::address_v4::from_string(ip), port);
	boost::system::error_code ec;
	m_socket.connect(ep, ec);
	if (ec)
	{
		Disconnect();
	}
	cout << "tcp client connect, error code:" << ec.value() << ", error message:" << ec.message() <<  endl;
	return ec.value();
}

int CTcpClientSocket::AsyncConnect(const string& ip, int port)
{
	ip::tcp::endpoint ep(boost::asio::ip::address_v4::from_string(ip), port);
	m_socket.async_connect(ep, std::bind(&CTcpClientSocket::ConnectHandler, this, std::placeholders::_1));
	m_thread_client = thread(bind(&CTcpClientSocket::SocketClientRunThread, this, true));
	return 0;
}

int CTcpClientSocket::Disconnect()
{
	//cout << "tcp client disconected, server ip:" << m_socket.remote_endpoint().address().to_string() << endl;
	cout << "tcp client socket closed" << endl;
	boost::system::error_code ec;
	if (m_socket.is_open())
	{
		m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
		m_socket.close(ec); // excute this statement occure crash
	}
	cout << "tcp client disconnected" << endl;
	m_ioservice.stop();
	if (m_thread_client.joinable())
	{
		cout << "waiting SocketClientRunThread end" << endl;
		m_thread_client.join();
	}
	return 0;
}

int CTcpClientSocket::Write(const char* data, size_t size)
{
	boost::system::error_code ec;
	size_t write_size = 0;
	if(m_socket.is_open())
	{
		write_size = m_socket.write_some(buffer(data, size), ec);
	}
	return ec.value();
}

int CTcpClientSocket::AsyncWrite(const char* data, size_t size)
{
	if (m_socket.is_open())
	{
		memset(m_writebuffer, 0, sizeof(m_writebuffer));
		memcpy(m_writebuffer, data, size);
		m_socket.async_write_some(buffer(m_writebuffer, size), bind(&CTcpClientSocket::WriteHandler, this, std::placeholders::_1, std::placeholders::_2));
	}
	return 0;
}

int CTcpClientSocket::SocketClientRunThread(bool async)
{
	if (async)
	{
		cout << "tcp client ioservice runing" << endl;
		m_ioservice.run();
		cout << "tcp client ioservice run over" << endl;
	}
	else
	{
		boost::system::error_code ec;
		char readbuffer[MAXBUFFER];
		while (true)
		{
			memset(readbuffer, 0, sizeof(readbuffer));
			size_t bytes_readed = m_socket.read_some(boost::asio::buffer(readbuffer), ec);
			cout << "tcp client read:" << readbuffer << ", error code:" << ec.value() << ", error message:" << ec.message() << endl;
			if (ec == boost::asio::error::eof)
			{
				break;
			}
			else if (ec)
			{
				std::thread th = std::thread([this](){Disconnect(); });
				th.detach();
				break;
			}
			else
			{
				if (m_read_callback)
				{
					m_read_callback(readbuffer, bytes_readed, ec.value());
				}
			}
		}
	}
	return 0;
}

void CTcpClientSocket::ConnectHandler(const boost::system::error_code& ec)
{
	cout << "tcp client connect, error code:" << ec.value() << ", error message:" << ec.message() << endl;
	if(ec)
	{
		//Disconnect();
		DisconnectHandler();
	}
	else
	{
		if (m_connect_callback)
		{
			m_connect_callback(ec.value());
		}
		memset(m_readbuffer, 0, sizeof(m_readbuffer));
		m_socket.async_read_some(buffer(m_readbuffer, MAXBUFFER), std::bind(&CTcpClientSocket::ReadHandler, this, std::placeholders::_1, std::placeholders::_2));
	}
}

void CTcpClientSocket::DisconnectHandler()
{
	m_ioservice.stop();
	// 不要等线程结束会阻塞
	//if (m_thread_client.joinable())
	//{
	//	cout << "waiting SocketClientRunThread end" << endl;
	//	m_thread_client.join();
	//}
	if (m_disconnect_callback)
	{
		m_disconnect_callback(0);
	}
}

void CTcpClientSocket::ReadHandler(boost::system::error_code ec, size_t bytes_readed)
{
	cout << "tcp client read error code:" << ec.value() << ", error message:" << ec.message() << endl;
	cout << "read buffer:" << endl;
	cout.write(m_readbuffer, bytes_readed) << endl;
	if (ec)	
	{
		//Disconnect();
		DisconnectHandler();
	}
	else
	{
		if (m_read_callback)
		{
			m_read_callback(m_readbuffer, bytes_readed, ec.value());
		}
		memset(m_readbuffer, 0, sizeof(m_readbuffer));
		m_socket.async_read_some(buffer(m_readbuffer, MAXBUFFER), std::bind(&CTcpClientSocket::ReadHandler, this, std::placeholders::_1, std::placeholders::_2));
	}
}

void CTcpClientSocket::WriteHandler(boost::system::error_code ec, size_t bytes_writed)
{
	cout << "tcp client write, error code:" << ec.value() << ", error message:" << ec.message() << endl;
	cout << "write buffer:" << endl;
	cout.write(m_writebuffer, bytes_writed) << endl;
	if (ec)	
	{
		//Disconnect();
		DisconnectHandler();
	}
	else
	{
		if (m_write_callback)
		{
			m_write_callback(m_writebuffer, bytes_writed, ec.value());
		}	
		memset(m_writebuffer, 0, sizeof(m_writebuffer));
	}	
}