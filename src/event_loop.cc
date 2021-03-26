#include "boots/event_loop.h"
#include <spdlog/spdlog.h>
#include <sys/epoll.h>
#include <unistd.h>

namespace boots {

static int kPollTime = 10;
uint32_t EventLoop::kPollIn = EPOLLIN;
uint32_t EventLoop::kPollErr = EPOLLERR;

EventLoop::EventLoop() : epoll_fd_{} { epoll_fd_ = epoll_create1(0); }
EventLoop::~EventLoop() { close(epoll_fd_); };

void EventLoop::Add(int fd, uint32_t events, CallbackFunc callback) {
  fd_handlers_.insert({fd, {fd, callback}});
  epoll_event ev{};
  ev.events = events;
  ev.data.fd = fd;
  if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev) != 0) {
    spdlog::error("[EventLoop] add failed, fd={}, errno={}", fd,
                  strerror(errno));
  }
}

void EventLoop::Modify(int fd, uint32_t events) {
  epoll_event ev{.events = events, .data = {.fd = fd}};
  if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev) != 0) {
    spdlog::error("[EventLoop] modify failed, errno={}", errno);
  }
}

void EventLoop::Remove(int fd) {
  fd_handlers_.erase(fd);
  epoll_event ev{.data = {.fd = fd}};
  if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, &ev) != 0) {
    spdlog::error("[EventLoop] delete failed, errno={}", errno);
  }
}

void EventLoop::Stop() { stopping_ = true; }

void EventLoop::Run() {
  std::array<epoll_event, 1> evs{};
  while (!stopping_) {
    int cnt{epoll_wait(epoll_fd_, evs.data(), 1, kPollTime)};
    if (cnt == -1) {
      spdlog::error("[EventLoop] poll, errno={}", errno);
      continue;
    }

    for (int i = 0; i < cnt; ++i) {
      auto it = fd_handlers_.find(evs[i].data.fd);
      if (it == fd_handlers_.end()) {
        continue;
      }
      it->second.second(evs[i].data.fd, evs[i].events);
    }
  }
}

} // namespace boots
