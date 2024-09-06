#include "tcpserversocket.h"
#include "kloglib.h"

CTcpServerSocket::CTcpServerSocket(io_service& service) :m_socket(service)
{

}

CTcpServerSocket::~CTcpServerSocket()
{
}

int CTcpServerSocket::RegisterHandler(f_connect connect_callback, f_disconnect disconnect_callback, f_read read_callback, f_write write_callback)
{
	m_connect_callback = connect_callback;
	m_disconnect_callback = disconnect_callback;
	m_read_callback = read_callback;
	m_write_callback = write_callback;
	return 0;
}

int CTcpServerSocket::Write(const char* data, size_t size)
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

int CTcpServerSocket::AsyncWrite(const char* data, size_t size)
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
						m_write_callback(shared_from_this(), nullptr, 0, error_code);
					}
				}
				else
				{
					if (m_write_callback && !write_buffer->empty())
					{
						TraceTempCout << "tcp client writed payload data size=" << bytes_transferred;
						m_write_callback(shared_from_this(), write_buffer->data(), write_buffer->size(), ec.value());
					}
				}
			});
	}
	return 0;
}

string CTcpServerSocket::RemoteIP()
{
	return m_socket.remote_endpoint().address().to_string();
}

int CTcpServerSocket::RemotePort()
{
	return m_socket.remote_endpoint().port();
}

void CTcpServerSocket::StartRead()
{
	TraceInfoCout << "tcp server accept a connet and start sync read data, client ip:" << m_socket.remote_endpoint().address().to_string();
	if (m_connect_callback)
	{
		m_connect_callback(shared_from_this(), 0);
	}
	do
	{
		Read();
	} while (false);
}

void CTcpServerSocket::AsyncStartRead()
{
	TraceInfoCout << "tcp server accept a connet and start async read data, client ip:" << m_socket.remote_endpoint().address().to_string();
	if (m_connect_callback)
	{
		m_connect_callback(shared_from_this(), 0);
	}
	AsyncRead();
}

void CTcpServerSocket::Disconnect()
{
	TraceInfoCout << "tcp server will disconnect a connect" << endl;
	asio::error_code ec;
	if (m_socket.is_open())
	{
		TraceInfoCout << "tcp server close a connect, client ip:" << m_socket.remote_endpoint().address().to_string() << endl;
		//m_socket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
		m_socket.close(ec); // excute this statement occure crash
	}
}

void CTcpServerSocket::Read()
{	
	asio::error_code ec;
	auto read_buffer         = std::make_shared<std::vector<char>>(1024);  // 使用智能指针管理缓冲区
	size_t bytes_transferred = m_socket.read_some(asio::buffer(*read_buffer), ec);
	int error_code           = ReadErrorCheck(ec, bytes_transferred, bytes_transferred);
	if (error_code)
	{
		if (m_read_callback)
		{
			m_read_callback(shared_from_this(), nullptr, 0, error_code);
		}
	}
	else if (m_read_callback)
	{
		// 获取缓冲区中的数据
		std::string read_data(read_buffer->data(), bytes_transferred);
		//std::cout << "Received: " << read_data << std::endl;
		(std::cout << "Received: ").write(read_data.c_str(), read_data.size());
		(TraceTempCout << "tcp client readed payload data size=" << bytes_transferred << ", payload data:").write(read_data.c_str(), read_data.size());
		m_read_callback(shared_from_this(), read_data.c_str(), read_data.size(), ec.value());
	}
}

void CTcpServerSocket::AsyncRead()
{
	auto read_buffer = std::make_shared<std::vector<char>>(1024);  // 使用智能指针管理缓冲区
	m_socket.async_read_some(asio::buffer(*read_buffer), [this, read_buffer](asio::error_code ec, std::size_t bytes_transferred)
		{
			int error_code = ReadErrorCheck(ec, bytes_transferred, bytes_transferred);
			if (error_code)
			{
				if (m_read_callback)
				{
					m_read_callback(shared_from_this(), nullptr, 0, error_code);
				}
			}
			else
			{
				if (m_read_callback)
				{
					// 获取缓冲区中的数据
					std::string read_data(read_buffer->data(), bytes_transferred);
					//std::cout << "Received: " << read_data << std::endl;
					(std::cout << "Received: ").write(read_data.c_str(), read_data.size());
					(TraceTempCout << "tcp client readed payload data size=" << bytes_transferred << ", payload data:").write(read_data.c_str(), read_data.size());
					m_read_callback(shared_from_this(), read_data.c_str(), read_data.size(), ec.value());
				}
				AsyncRead();
			}
		});
}

int CTcpServerSocket::ReadErrorCheck(asio::error_code ec, size_t readed_size, size_t require_read_size)
{
	int error_code = 1;
	do
	{
		if (ec == asio::error::eof)
		{
			TraceErrorCout << "tcp server read eof";
			break;
		}
		if (ec)
		{
			TraceErrorCout << "tcp server read error, error code:" << ec.value() << ", error message:" << ec.message();
			break;
		}
		if (readed_size != require_read_size)
		{
			TraceErrorCout << "tcp server read error, readed_size != require_read_size, readed_size=" << readed_size << ", require_read_size=" << require_read_size;
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

int CTcpServerSocket::WriteErrorCheck(asio::error_code ec, size_t writed_size, size_t require_write_size)
{
	int error_code = 1;
	do
	{
		if (ec)
		{
			TraceErrorCout << "tcp server write error, error code:" << ec.value() << ", error message:" << ec.message();
			break;
		}
		if (writed_size != require_write_size)
		{
			TraceErrorCout << "tcp server write error, writed_size != require_write_size, writed_size=" << writed_size << ", require_write_size=" << require_write_size;
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

int CTcpServerSocket::ProcessSocketError(int error_code)
{
	std::thread th = std::thread([this, error_code]() {
		TraceInfoCout << "socket occur error, will close socket" << endl;
		asio::error_code ec;
		if (m_socket.is_open())
		{
			TraceInfoCout << "tcp server close a connect, client ip:" << m_socket.remote_endpoint().address().to_string();
			//m_socket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
			m_socket.close(ec); // excute this statement occure crash
		}
		shared_ptr<CTcpServerSocket> connect = shared_from_this();
		if (m_disconnect_callback)
		{
			m_disconnect_callback(connect, error_code);
		}
	});
	th.detach();
	return 0;
}