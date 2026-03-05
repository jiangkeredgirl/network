#pragma once
#include <vector>
#include <mutex>

class RingBuffer
{
public:
    RingBuffer(size_t capacity = 8192);

    size_t push(const char* data, size_t len);
    size_t pop(std::vector<char>& out, size_t len);
    size_t size() const;

private:
    std::vector<char> buffer_;
    size_t head_, tail_;
    mutable std::mutex mtx_;
};