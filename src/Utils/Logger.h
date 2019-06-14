#ifndef _LOGGER_H

#include <spdlog/spdlog.h>

#include <string>

namespace NXE
{
namespace Utils
{

namespace Logger
{

  void init(std::string filename);

#define LOG_TRACE SPDLOG_TRACE
#define LOG_DEBUG SPDLOG_DEBUG
#define LOG_INFO SPDLOG_INFO
#define LOG_WARN SPDLOG_WARN
#define LOG_ERROR SPDLOG_ERROR
#define LOG_CRITICAL SPDLOG_CRITICAL

} // namespace Logger
} // namespace Utils
} // namespace NXE

#endif //_LOGGER_H