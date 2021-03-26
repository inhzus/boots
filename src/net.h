#pragma once
#include <arpa/inet.h>
#include <string_view>

namespace boots {
namespace net {
bool IsIpv4(std::string_view s);
bool IsIpv6(std::string_view s);
bool IsIp(std::string_view s);

} // namespace net
} // namespace boots
