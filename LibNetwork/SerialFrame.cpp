#include "SerialFrame.h"
#include <algorithm>

bool SerialFrame::Parse(const std::vector<char>& buffer, std::vector<char>& frame, const FrameConfig& cfg)
{
    frame.clear();
    if (!cfg.enable)
    {
        frame = buffer;
        return !frame.empty();
    }

    auto head_it = std::find(buffer.begin(), buffer.end(), cfg.header);
    if (head_it == buffer.end()) return false;

    auto tail_it = std::find(head_it, buffer.end(), cfg.tail);
    if (tail_it == buffer.end()) return false;

    frame.assign(head_it, tail_it + 1);
    return true;
}