/*********************************************************************
 * \file   SerialPortImpl.cpp
 * \brief  串口类.
 * \author 蒋珂
 * \date   2024.08.29
 *********************************************************************/

#include "serialportimpl.h"


LIBSERIALPORT_API ISerialPort* NewSerialPort(void)
{
    return new CSerialPortImpl();
}

LIBSERIALPORT_API void DeleteSerialPort(ISerialPort* serial_port)
{
    if (serial_port)
    {
        delete dynamic_cast<CSerialPortImpl*>(serial_port);
        serial_port = nullptr;
    }
}
