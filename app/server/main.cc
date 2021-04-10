#include "boots/dns_resolver.h"
#include "boots/event_loop.h"

int main() {
  boots::EventLoop loop{};
  auto r = std::make_shared<boots::DnsResolver>(&loop);
  r->Init();
  auto callback = [](const auto &, const auto &ip, const auto &) {
    printf("ip: %s\n", ip.data());
  };
  for (int i = 0; i < 3; ++i) {
    r->Resolve("lws.dingtalk.com", callback);
  }
  r->Resolve("google.com", callback);
  r->Resolve("baidu.com", callback);
  loop.Run();
  return 0;
}
