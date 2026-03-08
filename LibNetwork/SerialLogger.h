#pragma once
#include <string>
#include <mutex>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_sinks.h>

class SerialLogger
{
public:

    static void Info(const std::string& msg)
    {
        Log("INFO", msg);
    }

    static void Error(const std::string& msg)
    {
        Log("ERROR", msg);
    }

    static void Debug(const std::string& msg)
    {
        Log("DEBUG", msg);
    }

private:

    static void Log(const std::string& level, const std::string& msg)
    {
        static std::mutex m;

        std::lock_guard<std::mutex> lock(m);

        auto now = std::chrono::system_clock::now();
        auto t = std::chrono::system_clock::to_time_t(now);

        std::tm tm_time;

#ifdef _WIN32
        localtime_s(&tm_time, &t);     // Windows 安全版本
#else
        localtime_r(&t, &tm_time);     // Linux / Unix
#endif

        std::cout
            << "[" << level << "] "
            << std::put_time(&tm_time, "%H:%M:%S")
            << " "
            << msg
            << std::endl;
    }

    static std::shared_ptr<spdlog::logger> logger_;
	/**
     * @brief 创建一个带日志的 logger（静态）
    */
	static std::shared_ptr<spdlog::logger> create_logger(const std::string& name = "serial") {
		return spdlog::stdout_logger_mt(name);
	}

public:
    template<typename... Args>
    static void log(int level, const std::string& fmt, Args&&... args) {
        if (logger_) {
            logger_->log(static_cast<spdlog::level::level_enum>(level), fmt, std::forward<Args>(args)...);
        }
    }
};