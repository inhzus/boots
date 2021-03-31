#include "net.h"

namespace boots::net {
bool ToIpv4(std::string_view s, sockaddr_storage *sa) {
  sockaddr_storage temp_sa{};
  if (sa == nullptr) {
    sa = &temp_sa;
  }
  return 1 == inet_pton(AF_INET, s.data(),
                        &reinterpret_cast<sockaddr_in *>(sa)->sin_addr);
}

bool ToIpv6(std::string_view s, sockaddr_storage *sa) {
  sockaddr_storage temp_sa{};
  if (sa == nullptr) {
    sa = &temp_sa;
  }

  return 1 == inet_pton(AF_INET6, s.data(),
                        &reinterpret_cast<sockaddr_in6 *>(sa)->sin6_addr);
}

bool ToIp(std::string_view s, sockaddr_storage *sa) {
  return ToIpv4(s, sa) || ToIpv6(s, sa);
}

}  // namespace boots::net
