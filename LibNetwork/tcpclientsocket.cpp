#include "tcpclientsocket.h"
//#include <boost/bind.hpp>
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
	lock_guard<mutex> lock(m_mutex_client);
	ip::tcp::endpoint ep(asio::ip::address_v4::from_string(ip), port);
	asio::error_code ec;
	m_socket.connect(ep, ec);
	if (ec)
	{
		TraceErrorCout << "tcp client sync connect error, error code:" << ec.value() << ", error message:" << ec.message();
		m_socket.close();
		//Disconnect(ec.value());
	}
	else
	{
		TraceOKCout << "tcp client sync connect success";
		m_keep_connect = true;
		m_sync_connect = false;
		if (m_thread_client.joinable())
		{
			m_thread_client.join();
		}
		m_thread_client = std::thread(std::bind(&CTcpClientSocket::SocketClientRunThread, this, false));
	}
	return ec.value();
}

int CTcpClientSocket::AsyncConnect(const string& ip, int port)
{
	lock_guard<mutex> lock(m_mutex_client);
	ip::tcp::endpoint ep(asio::ip::address_v4::from_string(ip), port);
	m_socket.async_connect(ep, [this](asio::error_code ec)
	{
		if (ec)
		{
			TraceErrorCout << "tcp client async connect error, error code:" << ec.value() << ", error message:" << ec.message();
			m_socket.close();
			if (m_connect_callback)
			{
				m_connect_callback(ec.value());
			}
			//Disconnect(ec.value());
		}
		else
		{
			TraceOKCout << "tcp client async connect success";
			m_keep_connect = true;
			m_sync_connect = true;
			if (m_connect_callback)
			{
				m_connect_callback(ec.value());
			}
			AsyncRead();
		}
	});
	if (m_thread_client.joinable())
	{
		m_thread_client.join();
	}
	m_thread_client = std::thread(std::bind(&CTcpClientSocket::SocketClientRunThread, this, true));
	return 0;
}

int CTcpClientSocket::Disconnect()
{
	TrackCout;
	lock_guard<mutex> lock(m_mutex_client);
	asio::error_code ec;
	m_keep_connect = false;
	if (m_socket.is_open())
	{		
		try {
			asio::ip::tcp::endpoint remote_ep = m_socket.remote_endpoint();
			asio::ip::address remote_address = remote_ep.address();
			std::cout << "Remote address: " << remote_address.to_string() << std::endl;
			TraceInfoCout << "tcp client close socket, server ip:" << m_socket.remote_endpoint().address().to_string();
		}
		catch (const asio::system_error& e) 
		{
			std::cerr << "Error: " << e.what() << std::endl;
		}
		m_socket.close(ec); // excute this statement occure crash
	}
	m_ioservice.stop();
	if (m_thread_client.joinable())
	{
		m_thread_client.join();
	}
	return 0;
}

int CTcpClientSocket::Write(const char* data, size_t size)
{
	int error_code = 1;
	if (m_socket.is_open() && data && size > 0)
	{
		asio::error_code ec;
		size_t writed_size = asio::write(m_socket, asio::buffer(data, size), ec);
		error_code = WriteErrorCheck(ec, writed_size, size);
	}
	return error_code;
}

int CTcpClientSocket::AsyncWrite(const char* data, size_t size)
{
	if (m_socket.is_open() && data && size > 0)
	{
		auto write_buffer = std::make_shared<std::vector<char>>(data, data + size);  // 使用智能指针管理缓冲区
		asio::async_write(m_socket, asio::buffer(*write_buffer),
			[this, write_buffer, size](asio::error_code ec, std::size_t bytes_transferred)
			{
				int error_code = WriteErrorCheck(ec, bytes_transferred, size);
				if (error_code)
				{
					if (m_write_callback)
					{
						m_write_callback(nullptr, 0, error_code);
					}
				}
				else
				{
					if (m_write_callback && !write_buffer->empty())
					{
						TraceTempCout << "tcp client writed payload data size=" << bytes_transferred;
						m_write_callback(write_buffer->data(), write_buffer->size(), ec.value());
					}
				}
			});
	}
	return 0;
}

int CTcpClientSocket::SocketClientRunThread(bool async)
{
	//TraceInfoCout << "tcp client thread runing";
	TrackCout;
	if (async)
	{
		TraceInfoCout << "async tcp client ioservice runing";
		m_ioservice.reset();
		m_ioservice.restart();
		m_ioservice.run();
		m_ioservice.reset();
		TraceInfoCout << "async tcp client ioservice run over";
	}
	else
	{
		while (true)
		{
			if (!m_keep_connect)
			{
				break;
			}
			Read();
		}
	}
	//TraceInfoCout << "tcp client thread over";
	return 0;
}

void CTcpClientSocket::Read()
{
	asio::error_code ec;
	auto read_buffer         = std::make_shared<std::vector<char>>(1024);  // 使用智能指针管理缓冲区
	size_t bytes_transferred = m_socket.read_some(asio::buffer(*read_buffer), ec);
	int error_code           = ReadErrorCheck(ec, bytes_transferred, bytes_transferred);
	if (error_code)
	{
		if (m_read_callback)
		{
			m_read_callback(nullptr, 0, error_code);
		}
	}
	else if (m_read_callback)
	{
		// 获取缓冲区中的数据			
		std::string read_data(read_buffer->data(), bytes_transferred);
		//std::cout << "Received: " << read_data << std::endl;
		//(std::cout << "Received: ").write(read_data.c_str(), read_data.size());
		(TraceTempCout << "tcp client readed payload data size=" << bytes_transferred << ", payload data:").write(read_data.c_str(), read_data.size());
		m_read_callback(read_data.c_str(), read_data.size(), ec.value());
	}
}

void CTcpClientSocket::AsyncRead()
{
	auto read_buffer = std::make_shared<std::vector<char>>(1024);  // 使用智能指针管理缓冲区
	m_socket.async_read_some(asio::buffer(*read_buffer), [this, read_buffer](asio::error_code ec, std::size_t bytes_transferred)
	{
		int error_code = ReadErrorCheck(ec, bytes_transferred, bytes_transferred);
		if (error_code)
		{
			if (m_read_callback)
			{
				m_read_callback(nullptr, 0, error_code);
			}
		}
		else
		{
			if (m_read_callback)
			{
				// 获取缓冲区中的数据					
				std::string read_data(read_buffer->data(), bytes_transferred);
				//(std::cout << "Received: ").write(read_data.c_str(), read_data.size());
				(TraceTempCout << "tcp client readed payload data size=" << bytes_transferred << ", payload data:").write(read_data.c_str(), read_data.size());
				m_read_callback(read_data.c_str(), read_data.size(), ec.value());
			}
			AsyncRead();
		}
	});
}

int CTcpClientSocket::ReadErrorCheck(asio::error_code ec, size_t readed_size, size_t require_read_size)
{
	int error_code = 1;
	do
	{
		if (ec == asio::error::eof)
		{
			TraceErrorCout << "tcp client read eof";
			break;
		}
		if (ec)
		{
			TraceErrorCout << "tcp client read error, error code:" << ec.value() << ", error message:" << ec.message();
			break;
		}
		if (readed_size != require_read_size)
		{
			TraceErrorCout << "tcp client read error, readed_size != require_read_size, readed_size=" << readed_size << ", require_read_size=" << require_read_size;
			break;
		}
		error_code = 0;
	} while (false);
	if (error_code)
	{
		ProcessSocketError(ec.value());
	}
	return error_code;
}

int CTcpClientSocket::WriteErrorCheck(asio::error_code ec, size_t writed_size, size_t require_write_size)
{
	int error_code = 1;
	do
	{
		if (ec)
		{
			TraceErrorCout << "tcp client write error, error code:" << ec.value() << ", error message:" << ec.message();
			break;
		}
		if (writed_size != require_write_size)
		{
			TraceErrorCout << "tcp client write error, writed_size != require_write_size, writed_size=" << writed_size << ", require_write_size=" << require_write_size;
			break;
		}
		error_code = 0;
	} while (false);
	if (error_code)
	{
		ProcessSocketError(ec.value());
	}
	return error_code;
}

int CTcpClientSocket::ProcessSocketError(int error_code)
{
	std::thread th = std::thread([this, error_code]() {
		TraceErrorCout << "tcp client socket occur error, will close socket";
		asio::error_code ec;
		m_keep_connect = false;
		if (m_socket.is_open())
		{
			TraceErrorCout << "tcp client close socket, server ip:" << m_socket.remote_endpoint().address().to_string();
			m_socket.close(ec); // excute this statement occure crash
		}
		if (m_disconnect_callback)
		{
			m_disconnect_callback(error_code);
		}
	});
	th.detach();
	return 0;
}