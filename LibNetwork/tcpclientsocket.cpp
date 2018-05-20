#include "tcpclientsocket.h"
#include <boost/bind.hpp>
#include "kloglib.h"

CTcpClientSocket::CTcpClientSocket() : m_ioservice(), m_socket(m_ioservice)
{
}


CTcpClientSocket::~CTcpClientSocket()
{
}

int CTcpClientSocket::RegisterHandler(f_connect connect_callback, f_disconnect disconnect_callback, f_read read_callback, f_write write_callback)
{
	m_connect_callback = connect_callback;
	m_disconnect_callback = disconnect_callback;
	m_read_callback = read_callback;
	m_write_callback = write_callback;
	return 0;
}

int CTcpClientSocket::SetReconnectTime(__int64 reconnect_second)
{
	m_reconnect_second = reconnect_second;
	return 0;
}

int CTcpClientSocket::Connect(const string& ip, int port)
{
	ip::tcp::endpoint ep(boost::asio::ip::address_v4::from_string(ip), port);
	boost::system::error_code ec;
	m_socket.connect(ep, ec);
	if (ec)
	{
		TraceErrorCout() << "tcp client sync connect error, error code:" << ec.value() << ", error message:" << ec.message();
		m_socket.close();
		//Disconnect(ec.value());
	}
	else
	{
		TraceOKCout() << "tcp client sync connect success";
		m_keep_connect = true;
		m_sync_connect = true;
		m_thread_client = thread(bind(&CTcpClientSocket::SocketClientRunThread, this, false));
	}
	return ec.value();
}

int CTcpClientSocket::AsyncConnect(const string& ip, int port)
{
	ip::tcp::endpoint ep(boost::asio::ip::address_v4::from_string(ip), port);
	m_socket.async_connect(ep, [this](boost::system::error_code ec)
	{
		if (m_connect_callback)
		{
			m_connect_callback(ec.value());
		}
		if (ec)
		{
			TraceErrorCout() << "tcp client async connect error, error code:" << ec.value() << ", error message:" << ec.message();
			m_socket.close();
			//Disconnect(ec.value());
		}
		else
		{
			TraceOKCout() << "tcp client async connect success";
			m_keep_connect = true;
			m_sync_connect = false;
			ReadHeader();
		}
	});
	m_thread_client = thread(bind(&CTcpClientSocket::SocketClientRunThread, this, true));
	return 0;
}

int CTcpClientSocket::Disconnect()
{
	TrackCout();
	TraceInfoCout() << "tcp client disconnect, server ip:" << m_socket.remote_endpoint().address().to_string();
	TraceInfoCout() << "tcp client socket will close";
	boost::system::error_code ec;
	m_keep_connect = false;
	if (m_socket.is_open())
	{
		m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
		m_socket.close(ec); // excute this statement occure crash
	}
	TraceInfoCout() << "tcp client ioservice will stop";
	m_ioservice.stop();
	if (m_thread_client.joinable())
	{
		TraceInfoCout() << "waiting SocketClientRunThread over";
		m_thread_client.join();
	}
	if (m_disconnect_callback)
	{
		m_disconnect_callback(ec.value());
	}
	return 0;
}

int CTcpClientSocket::Write(const char* data, size_t size)
{
	int error_code = 1;
	if (m_socket.is_open() && data && size > 0)
	{
		boost::system::error_code ec;
		size_t writed_size = 0;
		NetDataPackage write_package(data, size);
		writed_size = boost::asio::write(m_socket, buffer(write_package.data(), size), ec);
		error_code = WriteErrorCheck(ec, writed_size, size);
	}
	return error_code;
}

int CTcpClientSocket::AsyncWrite(const char* data, size_t size)
{
	if (m_socket.is_open() && data && size > 0)
	{
		shared_ptr<NetDataPackage> write_package = shared_ptr<NetDataPackage>(new NetDataPackage(data, size));
		m_write_packages.push_back(write_package);
		DoWrite();
	}
	return 0;
}

int CTcpClientSocket::SocketClientRunThread(bool async)
{
	TrackCout();
	if (async)
	{
		TraceInfoCout() << "async tcp client ioservice runing";
		m_ioservice.run();
		TraceInfoCout() << "async tcp client ioservice run over";
	}
	else
	{
		while (true)
		{
			if (!m_keep_connect)
			{
				break;
			}
			boost::system::error_code ec;
			m_read_package = shared_ptr<NetDataPackage>(new NetDataPackage());
			size_t readed_size = boost::asio::read(m_socket, boost::asio::buffer(m_read_package->header(), NetDataPackage::HEADER_SIZE), ec);
			int error_code = ReadErrorCheck(ec, readed_size, NetDataPackage::HEADER_SIZE);
			if (error_code)
			{
				if (m_read_callback)
				{
					m_read_callback(nullptr, 0, error_code);
				}
				continue;
			}
			if (m_read_package->decode_header())
			{
				TraceErrorCout() << "tcp client decode header error";
				if (m_read_callback)
				{
					m_read_callback(nullptr, 0, error_code);
				}
				continue;
			}

			readed_size = boost::asio::read(m_socket, boost::asio::buffer(m_read_package->body(), m_read_package->header()->body_size), ec);
			error_code = ReadErrorCheck(ec, readed_size, m_read_package->header()->body_size);
			if (error_code)
			{
				if (m_read_callback)
				{
					m_read_callback(nullptr, 0, error_code);
				}
				continue;
			}
			if (m_read_callback)
			{
				(TraceTempCout() << "tcp client readed payload data size=" << m_read_package->header()->body_size << ", payload data:").write(m_read_package->body(), m_read_package->header()->body_size);
				m_read_callback(m_read_package->body(), readed_size, ec.value());
			}

		}
	}
	return 0;
}
 
//void CTcpClientSocket::DisconnectHandler()
//{
//	m_ioservice.stop();
//	// ²»ÒªµÈÏß³Ì½áÊø»á×èÈû
//	//if (m_thread_client.joinable())
//	//{
//	//	cout << "waiting SocketClientRunThread end" << endl;
//	//	m_thread_client.join();
//	//}
//	if (m_disconnect_callback)
//	{
//		m_disconnect_callback(0);
//	}
//}

void CTcpClientSocket::DoWrite()
{
	if (!m_write_packages.empty())
	{
		boost::asio::async_write(m_socket, boost::asio::buffer(m_write_packages.front()->data(),
			m_write_packages.front()->data_size()),
			[this](boost::system::error_code ec, std::size_t length)
		{
			int error_code = WriteErrorCheck(ec, length, m_write_packages.front()->data_size());
			if (error_code)
			{
				if (m_write_callback)
				{
					m_write_callback(nullptr, 0, error_code);
				}
			}
			else
			{
				if (m_write_callback)
				{
					TraceTempCout() << "tcp client writed payload data size=" << m_write_packages.front()->header()->body_size << ", payload data:" << m_write_packages.front()->body();
					m_write_callback(m_write_packages.front()->body(), m_write_packages.front()->header()->body_size, ec.value());
				}
				m_write_packages.pop_front();
				if (!m_write_packages.empty())
				{
					DoWrite();
				}
			}
		});
	}
}

void CTcpClientSocket::ReadHeader()
{
	m_read_package = shared_ptr<NetDataPackage>(new NetDataPackage());
	boost::asio::async_read(m_socket,
		boost::asio::buffer(m_read_package->header(), NetDataPackage::HEADER_SIZE),
		[this](boost::system::error_code ec, std::size_t length)
	{
		int error_code = ReadErrorCheck(ec, length, NetDataPackage::HEADER_SIZE);
		if (error_code)
		{
			if (m_read_callback)
			{
				m_read_callback(nullptr, 0, error_code);
			}
		}
		else if (m_read_package->decode_header())
		{
			TraceErrorCout() << "tcp client decode header error";
			if (m_read_callback)
			{
				m_read_callback(nullptr, 0, error_code);
			}
		}
		else
		{
			ReadBody();
		}
	});
}

void CTcpClientSocket::ReadBody()
{
	boost::asio::async_read(m_socket,
		boost::asio::buffer(m_read_package->body(), m_read_package->header()->body_size),
		[this](boost::system::error_code ec, std::size_t length)
	{
		int error_code = ReadErrorCheck(ec, length, m_read_package->header()->body_size);
		if (error_code)
		{
			if (m_read_callback)
			{
				m_read_callback(nullptr, 0, error_code);
			}
		}
		else
		{
			//std::cout.write(m_read_package->body(), m_read_package->header()->body_size);
			if (m_read_callback)
			{
				TraceTempCout() << "tcp client readed payload data size=" << m_read_package->header()->body_size << ", payload data:" << m_read_package->body();
				m_read_callback(m_read_package->body(), m_read_package->header()->body_size, ec.value());
			}
			ReadHeader();
		}
	});
}

int CTcpClientSocket::ReadErrorCheck(boost::system::error_code ec, size_t readed_size, size_t require_read_size)
{
	int error_code = 1;
	do
	{
		if (ec == boost::asio::error::eof)
		{
			TraceInfoCout() << "tcp client read eof";
			break;
		}
		if (ec)
		{
			TraceErrorCout() << "tcp client read error, error code:" << ec.value() << ", error message:" << ec.message();
			//std::thread th = std::thread([this, ec]() {Disconnect(ec.value()); });
			//th.detach();
			break;
		}
		if (readed_size != require_read_size)
		{
			TraceErrorCout() << "tcp client read error, readed_size != require_read_size, readed_size=" << readed_size << ", require_read_size=" << require_read_size;
			break;
		}
		error_code = 0;
	} while (false);
	return error_code;
}

int CTcpClientSocket::WriteErrorCheck(boost::system::error_code ec, size_t writed_size, size_t require_write_size)
{
	int error_code = 1;
	do
	{		
		if (ec)
		{
			TraceErrorCout() << "tcp client write error, error code:" << ec.value() << ", error message:" << ec.message();
			//std::thread th = std::thread([this, ec]() {Disconnect(ec.value()); });
			//th.detach();
			break;
		}
		if (writed_size != require_write_size)
		{
			TraceErrorCout() << "tcp client write error, writed_size != require_write_size, writed_size=" << writed_size << ", require_write_size=" << require_write_size;
			break;
		}
		error_code = 0;
	} while (false);
	return error_code;
}