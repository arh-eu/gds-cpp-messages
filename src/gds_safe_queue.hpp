#ifndef GDS_SAFE_QUEUE_HPP
#define GDS_SAFE_QUEUE_HPP

#include <condition_variable>
#include <mutex>
#include <queue>

namespace gds_lib {

template <typename T> class SafeQueue {
public:
  SafeQueue() = default;

  SafeQueue &operator=(const SafeQueue &) = delete;
  SafeQueue &operator=(const SafeQueue &&) = delete;

  SafeQueue(const SafeQueue &) = delete;
  SafeQueue(const SafeQueue &&) = delete;

  void push(const T &d) {
    std::unique_lock<std::mutex> lk(_m);
    _q.push(d);
    _cv.notify_all();
  }

  T pop() {
    std::unique_lock<std::mutex> lk(_m);
    if (_q.empty()) {
      _cv.wait(lk, [=] { return !_q.empty(); });
    }

    T data = _q.front();
    _q.pop();
    return data;
  }

  bool empty() {
    std::unique_lock<std::mutex> lk(_m);
    return _q.empty();
  }

private:
  std::queue<T>           _q;
  std::mutex              _m;
  std::condition_variable _cv;
};
} // namespace gds_lib

#endif // GDS_SAFE_QUEUE_HPP