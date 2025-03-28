#include <iostream>

#pragma once

#include <cstddef>
#include <vector>

class RingBuffer {
 private:
  std::vector<int> queue_;
  size_t max_count_ = 0;

 public:
  size_t count = 0;
  explicit RingBuffer(size_t capacity) : max_count_(capacity) {}
  size_t Size() const { return count; }
  bool Empty() const { return count == 0; }
  bool TryPush(int element) {
    if (count == max_count_) {
      return false;
    }
    queue_.push_back(element);
    count++;
    return true;
  }
  bool TryPop(int* element) {
    if (!Empty()) {
      *element = *queue_.begin();
      queue_.erase(queue_.begin());
      count--;
      return true;
    }
    return false;
  }
};
