#pragma once

#include "config.h"
#include "input_command_bus.h"

#include "google/cloud/pubsub/subscriber.h"

#include <atomic>
#include <cstdint>
#include <string>
#include <unordered_map>

class RemoteInputSubscriber {
 public:
  RemoteInputSubscriber(const PublisherConfig& config, InputCommandBus& command_bus);
  void Run(const std::atomic<bool>& running);

 private:
  bool IsStale(const pubsub3d::InputCommand& command) const;
  bool IsOutOfOrder(const pubsub3d::InputCommand& command);
  static std::uint64_t EpochMillis();

  PublisherConfig config_;
  InputCommandBus& command_bus_;
  std::unordered_map<std::string, std::uint64_t> last_sequence_by_client_;
};
