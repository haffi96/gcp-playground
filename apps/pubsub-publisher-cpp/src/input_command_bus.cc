#include "input_command_bus.h"

#include <algorithm>
#include <utility>

InputCommandBus::InputCommandBus(std::size_t capacity)
    : capacity_(std::max<std::size_t>(capacity, 1)), dropped_count_(0) {}

bool InputCommandBus::Push(pubsub3d::InputCommand command) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (queue_.size() >= capacity_) {
    ++dropped_count_;
    return false;
  }
  queue_.push_back(std::move(command));
  return true;
}

std::vector<pubsub3d::InputCommand> InputCommandBus::Drain(std::size_t max_count) {
  std::lock_guard<std::mutex> lock(mutex_);
  const std::size_t count = std::min(max_count, queue_.size());
  std::vector<pubsub3d::InputCommand> drained;
  drained.reserve(count);
  for (std::size_t i = 0; i < count; ++i) {
    drained.push_back(std::move(queue_.front()));
    queue_.pop_front();
  }
  return drained;
}

std::size_t InputCommandBus::Size() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return queue_.size();
}

std::size_t InputCommandBus::DroppedCount() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return dropped_count_;
}
