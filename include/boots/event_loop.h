#pragma once
#include <chrono>
#include <functional>
#include <unordered_map>
#include <utility>

namespace boots {
class EventLoop {
public:
  using CallbackFunc = std::function<void(int, uint32_t)>;
  static uint32_t kPollIn;
  static uint32_t kPollErr;
  EventLoop();
  ~EventLoop();

  EventLoop(const EventLoop &) = delete;
  EventLoop &operator=(const EventLoop &) = delete;

  void Add(int fd, uint32_t events, CallbackFunc);
  void Modify(int fd, uint32_t events);
  void Remove(int fd);
  void Stop();
  void Run();

private:
  bool stopping_{false};
  int epoll_fd_;
  std::unordered_map<int, std::pair<int, CallbackFunc>> fd_handlers_{};
};

} // namespace boots
