#include "SerialLogger.h"
//std::shared_ptr<spdlog::logger> SerialLogger::logger_ = spdlog::get("serial");
std::shared_ptr<spdlog::logger> SerialLogger::logger_ = SerialLogger::create_logger();
