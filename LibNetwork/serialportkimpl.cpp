/*********************************************************************
 * \file   SerialPortImplk.cpp
 * \brief  串口类.
 * \author 蒋珂
 * \date   2024.08.29
 *********************************************************************/

#include "serialportkimpl.h"



LIBSERIALPORT_API ISerialPortk* NewSerialPortk(void)
{
    return new CSerialPortImplk();
}

LIBSERIALPORT_API void DeleteSerialPortk(ISerialPortk* serial_port)
{
    if (serial_port)
    {
        delete dynamic_cast<CSerialPortImplk*>(serial_port);
        serial_port = nullptr;
    }
}
