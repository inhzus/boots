#pragma once
#include <cstdint>
#include <string_view>

namespace boots {
class DnsResolver;
class EventLoop;

class TcpRelay {
  TcpRelay(EventLoop *loop, uint16_t port);

 private:
  EventLoop *loop_;
  int fd_;
};
}  // namespace boots