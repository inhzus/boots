#include "tcp_relay.h"

#include "boots/event_loop.h"
#include "log.h"
#include "net.h"
#include "util.h"

namespace boots {

TcpRelay::TcpRelay(EventLoop *loop, uint16_t port) : loop_{loop} {
  fd_ =
      socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
  LOG_ERRNO_IF(fd_ < 0);
  int val{1};
  auto n = setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
  LOG_ERRNO_IF(n < 0);
  n = setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &val, sizeof(val));
  LOG_ERRNO_IF(n < 0);
  sockaddr_in sa{AF_INET, port, str::Swap(INADDR_ANY)};
  n = bind(fd_, reinterpret_cast<const sockaddr *>(&sa), sizeof(sa));
  LOG_ERRNO_IF(n < 0);

  n = listen(fd_, SOMAXCONN);
  LOG_ERRNO_IF(n < 0);
}
}  // namespace boots
