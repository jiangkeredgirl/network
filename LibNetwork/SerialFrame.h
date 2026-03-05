#pragma once
#include <vector>

struct FrameConfig
{
    bool enable = true;
    char header = 0xAA;
    char tail = 0x55;
};

class SerialFrame
{
public:
    static bool Parse(const std::vector<char>& buffer, std::vector<char>& frame, const FrameConfig& cfg);
};