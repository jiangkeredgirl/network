#pragma once
#include <iostream>
#include <mutex>
class SerialLogger
{
public:
    template<typename T>
    static void Info(const T& msg)
    {
        std::lock_guard<std::mutex> lock(mtx_);
        std::cout << "[Serial][INFO] " << msg << std::endl;
    }

private:
    static std::mutex mtx_;
};