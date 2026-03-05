/*********************************************************************
 * \file   SerialPortImpl.cpp
 * \brief  工业级串口类.
 * \author 蒋珂
 * \date   2026.03.04
 *********************************************************************/
#include "SerialPortImpl.h"


LIBSERIALPORT_API ISerialPort* NewSerialPort(void)
{
    return new SerialPortImpl();
}

LIBSERIALPORT_API void DeleteSerialPort(ISerialPort* serial_port)
{
    if (serial_port)
    {
        delete dynamic_cast<SerialPortImpl*>(serial_port);
        serial_port = nullptr;
    }
}
