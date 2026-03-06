#pragma once
#include <system_error>

namespace serial {

    // 与原接口兼容：返回 int 类型错误码
    enum class SerialError {
        SUCCESS = 0,
        ERROR_INVALID_ARGUMENT = -1,
        ERROR_CONNECT_FAILED = -2,
        ERROR_DISCONNECT_FAILED = -3,
        ERROR_WRITE_FAILED = -4,
        ERROR_READ_FAILED = -5,
        ERROR_TIME_OUT = -6,
        ERROR_SERIAL_CLOSED = -7,
        ERROR_UNKNOWN = -999
    };

    inline const char* to_string(SerialError ec) {
        switch (ec) {
        case SerialError::SUCCESS:                return "Success";
        case SerialError::ERROR_INVALID_ARGUMENT: return "Invalid argument";
        case SerialError::ERROR_CONNECT_FAILED:   return "Connect failed";
        case SerialError::ERROR_DISCONNECT_FAILED:return "Disconnect failed";
        case SerialError::ERROR_WRITE_FAILED:     return "Write failed";
        case SerialError::ERROR_READ_FAILED:      return "Read failed";
        case SerialError::ERROR_TIME_OUT:         return "Operation timed out";
        case SerialError::ERROR_SERIAL_CLOSED:    return "Serial port closed";
        case SerialError::ERROR_UNKNOWN:          return "Unknown error";
        default:                                  return "Unknown error code";
        }
    }


    enum class SerialErrc
    {
        None = 0,
        NotConnected,
        Timeout,
        WriteFailed,
        ReadFailed,
        CRCError
    };

    class SerialErrorCategory : public std::error_category
    {
    public:
        const char* name() const noexcept override { return "SerialError"; }
        std::string message(int ev) const override
        {
            switch (static_cast<SerialErrc>(ev))
            {
            case SerialErrc::None: return "No Error";
            case SerialErrc::NotConnected: return "Not connected";
            case SerialErrc::Timeout: return "Timeout";
            case SerialErrc::WriteFailed: return "Write failed";
            case SerialErrc::ReadFailed: return "Read failed";
            case SerialErrc::CRCError: return "CRC error";
            default: return "Unknown error";
            }
        }
    };

    inline std::error_code make_error_code(SerialErrc e)
    {
        static SerialErrorCategory cat;
        return { static_cast<int>(e), cat };
    }
}