#pragma once

#include "proto/input_command.pb.h"

#include <cstddef>
#include <deque>
#include <mutex>
#include <vector>

class InputCommandBus {
 public:
  explicit InputCommandBus(std::size_t capacity);

  bool Push(pubsub3d::InputCommand command);
  std::vector<pubsub3d::InputCommand> Drain(std::size_t max_count);
  std::size_t Size() const;
  std::size_t DroppedCount() const;

 private:
  const std::size_t capacity_;
  mutable std::mutex mutex_;
  std::deque<pubsub3d::InputCommand> queue_;
  std::size_t dropped_count_;
};
