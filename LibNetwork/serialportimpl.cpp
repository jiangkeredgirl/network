/*********************************************************************
 * \file   SerialPortImpl.cpp
 * \brief  工业级串口类.
 * \author 蒋珂
 * \date   2026.03.04
 *********************************************************************/
#include "SerialPortImpl.h"


LIBSERIALPORT_API ISerialPort* NewSerialPort(void)
{
    return new serial::AsioSerialPortImpl();
}

LIBSERIALPORT_API void DeleteSerialPort(ISerialPort* serial_port)
{
    if (serial_port)
    {
        delete dynamic_cast<serial::AsioSerialPortImpl*>(serial_port);
        serial_port = nullptr;
    }
}
