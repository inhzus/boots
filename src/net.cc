#include "net.h"

namespace boots {
namespace net {

bool IsIpv4(std::string_view s) {
  struct sockaddr_in sa;
  return 1 == inet_pton(AF_INET, s.data(), &sa.sin_addr);
}

bool IsIpv6(std::string_view s) {
  struct sockaddr_in sa;
  return 1 == inet_pton(AF_INET6, s.data(), &sa.sin_addr);
}

bool IsIp(std::string_view s) { 
  if (IsIpv4(s)) {
    return true;
  }
  return IsIpv6(s);
 }

} // namespace net
} // namespace boots
