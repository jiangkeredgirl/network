#pragma once

#include "kutility.h"


struct NetDataPackage
{
	struct Header
	{
		static constexpr char FLAG[2] = { '#','k' };
		char flag[2] = { '#','k' };
		size_t body_size = 0;
	};
	static const size_t HEADER_SIZE = sizeof(Header);
	//static const size_t MAX_NET_DATA_PACKAGE_SIZE = 512;	
	//static const size_t MAX_BODY_SIZE = MAX_NET_DATA_PACKAGE_SIZE - HEADER_SIZE;

public:
	NetDataPackage(const NetDataPackage& other_object) = delete;
	NetDataPackage &operator=(const NetDataPackage &other_object) = delete;

	NetDataPackage()
	{

	}

	NetDataPackage(const char* _body, size_t _body_size)
	{
		m_header.body_size = _body_size;
		size_t data_size = HEADER_SIZE + m_header.body_size;
		m_data = shared_ptr<char>(new char[data_size]);
		memset(&(*m_data), 0, data_size);
		memcpy(&(*m_data), &m_header, HEADER_SIZE);
		memcpy(&(*m_data) + HEADER_SIZE, _body, m_header.body_size);
	}

	int decode_header()
	{
		int error_code = 1;
		do
		{
			if (strncmp(m_header.flag, Header::FLAG, sizeof(Header::FLAG)))
			{
				break;
			}
			size_t data_size = HEADER_SIZE + m_header.body_size;
			m_data = shared_ptr<char>(new char[data_size]);
			memset(&(*m_data), 0, data_size);
			memcpy(&(*m_data), &m_header, HEADER_SIZE);
			error_code = 0;
		} while (false);
		return error_code;
	}

	Header* header()
	{
		return &m_header;
	}

	char* body()
	{
		return &(*m_data) + HEADER_SIZE;
	}

	char* data()
	{
		return &(*m_data);
	}
	size_t data_size()
	{
		return HEADER_SIZE + m_header.body_size;
	}

public:
	Header m_header;
	shared_ptr<char> m_data = nullptr;

};