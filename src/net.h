#pragma once
#include <arpa/inet.h>

#include <string_view>

namespace boots::net {
bool ToIpv4(std::string_view s, sockaddr_storage *sa = nullptr);
bool ToIpv6(std::string_view s, sockaddr_storage *sa = nullptr);
bool ToIp(std::string_view s, sockaddr_storage *sa = nullptr);

}  // namespace boots::net
