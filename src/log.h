#pragma once

#include <cassert>
#include <cstring>

#include "spdlog/spdlog.h"

#define LOG_ERRNO()                                                     \
  do {                                                                  \
    SPDLOG_ERROR("fail({}) to {}", std::strerror(errno), __FUNCTION__); \
  } while (0)

#define LOG_ERRNO_F(fmt, ...)                                                      \
  do {                                                                             \
    SPDLOG_ERROR(fmt ", errno={}({})", ##__VA_ARGS__, errno, std::strerror(errno); \
  } while (0)

#define LOG_ERRNO_L(LEVEL, fmt, ...)                                           \
  do {                                                                         \
    SPDLOG_##LEVEL("fail({}) to {}, " fmt, std::strerror(errno), __FUNCTION__, \
                   ##__VA_ARGS__);                                             \
  } while (0)

#define LOG_ERRNO_IF(condition) \
  do {                          \
    if (condition) LOG_ERRNO(); \
  } while (0)

#define LOG_ERRNO_IF_F(condition, fmt, ...)                                    \
  do {                                                                         \
    if (condition)                                                             \
      SPDLOG_ERROR("fail({}) to {}, " fmt, std::strerror(errno), __FUNCTION__, \
                   ##__VA_ARGS__);                                             \
  } while (0)
