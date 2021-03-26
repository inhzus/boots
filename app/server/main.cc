#include "boots/dns_resolver.h"
#include "boots/event_loop.h"

int main() {
  boots::EventLoop loop{};
  auto r = std::make_shared<boots::DnsResolver>(&loop);
  r->Init();
  r->Resolve("baidu.com", [](const auto &, const auto &ip, const auto &) {
    printf("ip: %s\n", ip.data());
  });
  loop.Run();
  return 0;
}
