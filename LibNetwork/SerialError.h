#pragma once
#include <system_error>

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