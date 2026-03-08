#pragma once

#include "config.h"
#include "input_command_bus.h"

#include <atomic>

class LocalInputAdapter {
 public:
  LocalInputAdapter(const PublisherConfig& config, InputCommandBus& command_bus);
  void Run(const std::atomic<bool>& running);

 private:
  const PublisherConfig& config_;
  InputCommandBus& command_bus_;
};
