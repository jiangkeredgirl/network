#include "RingBuffer.h"
#include <algorithm>

RingBuffer::RingBuffer(size_t capacity)
    : buffer_(capacity), head_(0), tail_(0)
{
}

size_t RingBuffer::push(const char* data, size_t len)
{
    std::lock_guard<std::mutex> lock(mtx_);
    size_t space = buffer_.size() - size();
    size_t to_write = std::min(space, len);
    for (size_t i = 0; i < to_write; ++i)
    {
        buffer_[tail_] = data[i];
        tail_ = (tail_ + 1) % buffer_.size();
    }
    return to_write;
}

size_t RingBuffer::pop(std::vector<char>& out, size_t len)
{
    std::lock_guard<std::mutex> lock(mtx_);
    size_t avail = size();
    size_t to_read = std::min(avail, len);
    for (size_t i = 0; i < to_read; ++i)
    {
        out.push_back(buffer_[head_]);
        head_ = (head_ + 1) % buffer_.size();
    }
    return to_read;
}

size_t RingBuffer::size() const
{
    std::lock_guard<std::mutex> lock(mtx_);
    if (tail_ >= head_)
        return tail_ - head_;
    else
        return buffer_.size() - head_ + tail_;
}