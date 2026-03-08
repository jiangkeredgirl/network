#pragma once
#include <vector>
#include <future>

struct SerialRequest
{
    std::vector<char> request;

    std::vector<char> response;

    std::promise<int> promise;

    int timeout_ms;

    int retry;
};