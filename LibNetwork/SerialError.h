#pragma once
#include <system_error>

enum class SerialErrc
{
    Success = 0,

    NotConnected,
    OpenFailed,
    AlreadyConnected,
    WriteFailed,
    ReadFailed,

    Timeout,
    Cancelled,

    ReconnectFailed,

    InvalidParam,

    InternalError
};

class SerialErrorCategory : public std::error_category
{
public:
    const char* name() const noexcept override
    {
        return "SerialPort";
    }

    std::string message(int ev) const override
    {
        switch ((SerialErrc)ev)
        {
        case SerialErrc::Success: return "Success";
        case SerialErrc::NotConnected: return "Not connected";
        case SerialErrc::OpenFailed: return "Open serial port failed";
        case SerialErrc::AlreadyConnected: return "Already connected";
        case SerialErrc::WriteFailed: return "Write failed";
        case SerialErrc::ReadFailed: return "Read failed";
        case SerialErrc::Timeout: return "Timeout";
        case SerialErrc::Cancelled: return "Cancelled";
        case SerialErrc::ReconnectFailed: return "Reconnect failed";
        default: return "Unknown serial error";
        }
    }
};

inline const std::error_category& SerialCategory()
{
    static SerialErrorCategory instance;
    return instance;
}

inline std::error_code make_error_code(SerialErrc e)
{
    return { (int)e, SerialCategory() };
}

namespace std
{
    template<>
    struct is_error_code_enum<SerialErrc> : true_type {};
}