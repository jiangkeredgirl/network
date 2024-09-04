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
		size_t writed_size = 0;
		NetDataPackage write_package(data, size);
		writed_size = asio::write(m_socket, buffer(write_package.data(), size), ec);
		error_code = WriteErrorCheck(ec, writed_size, size);
	}
	return error_code;
}

int CTcpServerSocket::AsyncWrite(const char* data, size_t size)
{
	if (m_socket.is_open() && data && size > 0)
	{
		shared_ptr<NetDataPackage> write_package = shared_ptr<NetDataPackage>(new NetDataPackage(data, size));
		m_write_packages.push_back(write_package);
		DoWrite();
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
		asio::error_code ec;
		m_read_package = shared_ptr<NetDataPackage>(new NetDataPackage());
		size_t readed_size = asio::read(m_socket, asio::buffer(m_read_package->header(), NetDataPackage::HEADER_SIZE), ec);
		int error_code = ReadErrorCheck(ec, readed_size, NetDataPackage::HEADER_SIZE);
		if (error_code)
		{
			if (m_read_callback)
			{
				m_read_callback(shared_from_this(), nullptr, 0, error_code);
			}
			break;
		}
		if (m_read_package->decode_header())
		{
			TraceErrorCout << "tcp client decode header error";
			if (m_read_callback)
			{
				m_read_callback(shared_from_this(), nullptr, 0, error_code);
			}
			break;
		}

		readed_size = asio::read(m_socket, asio::buffer(m_read_package->body(), m_read_package->header()->body_size), ec);
		error_code = ReadErrorCheck(ec, readed_size, m_read_package->header()->body_size);
		if (error_code)
		{
			if (m_read_callback)
			{
				m_read_callback(shared_from_this(), nullptr, 0, error_code);
			}
			break;
		}
		if (m_read_callback)
		{
			(TraceTempCout << "tcp client readed payload data size=" << m_read_package->header()->body_size << ", payload data:").write(m_read_package->body(), m_read_package->header()->body_size);
			m_read_callback(shared_from_this(), m_read_package->body(), readed_size, ec.value());
		}

	} while (false);
}

void CTcpServerSocket::AsyncStartRead()
{
	TraceInfoCout << "tcp server accept a connet and start async read data, client ip:" << m_socket.remote_endpoint().address().to_string();
	if (m_connect_callback)
	{
		m_connect_callback(shared_from_this(), 0);
	}
	ReadHeader();
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

void CTcpServerSocket::DoWrite()
{
	if (!m_write_packages.empty())
	{
		asio::async_write(m_socket, asio::buffer(m_write_packages.front()->data(),
			m_write_packages.front()->data_size()),
			[this](asio::error_code ec, std::size_t length)
		{
			int error_code = WriteErrorCheck(ec, length, m_write_packages.front()->data_size());
			if (error_code)
			{
				if (m_write_callback)
				{
					m_write_callback(shared_from_this(), nullptr, 0, error_code);
				}
			}
			else
			{
				if (m_write_callback)
				{
					(TraceTempCout << "tcp server writed payload data size=" << m_write_packages.front()->header()->body_size << ", payload data:").write(m_write_packages.front()->body(), m_write_packages.front()->header()->body_size);
					m_write_callback(shared_from_this(), m_write_packages.front()->body(), m_write_packages.front()->header()->body_size, ec.value());
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

void CTcpServerSocket::ReadHeader()
{
	m_read_package = shared_ptr<NetDataPackage>(new NetDataPackage());
	asio::async_read(m_socket,
		asio::buffer(m_read_package->header(), NetDataPackage::HEADER_SIZE),
		[this](asio::error_code ec, std::size_t length)
	{
		int error_code = ReadErrorCheck(ec, length, NetDataPackage::HEADER_SIZE);
		if (error_code)
		{
			if (m_read_callback)
			{
				m_read_callback(shared_from_this(), nullptr, 0, error_code);
			}
		}
		else if (m_read_package->decode_header())
		{
			TraceErrorCout << "tcp server decode header error";
			if (m_read_callback)
			{
				m_read_callback(shared_from_this(), nullptr, 0, error_code);
			}
		}
		else
		{
			ReadBody();
		}
	});
}

void CTcpServerSocket::ReadBody()
{
	asio::async_read(m_socket,
		asio::buffer(m_read_package->body(), m_read_package->header()->body_size),
		[this](asio::error_code ec, std::size_t length)
	{
		int error_code = ReadErrorCheck(ec, length, m_read_package->header()->body_size);
		if (error_code)
		{
			if (m_read_callback)
			{
				m_read_callback(shared_from_this(), nullptr, 0, error_code);
			}
		}
		else
		{
			//std::cout.write(m_read_package->body(), m_read_package->header()->body_size);
			if (m_read_callback)
			{
				(TraceTempCout << "tcp server readed payload data size=" << m_read_package->header()->body_size << ", payload data:").write(m_read_package->body(), m_read_package->header()->body_size);
				m_read_callback(shared_from_this(), m_read_package->body(), m_read_package->header()->body_size, ec.value());
			}
			ReadHeader();
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