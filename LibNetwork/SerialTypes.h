#pragma once
#include <vector>
#include <memory>
#include <chrono>
#include <future>

struct SerialRequest
{
    std::vector<char> tx_data;
    std::promise<std::vector<char>> promise;
    std::chrono::steady_clock::time_point expire;
};