#include "SerialLogger.h"
std::shared_ptr<spdlog::logger> SerialLogger::logger_ = spdlog::get("serial");