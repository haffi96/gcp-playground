#pragma once

#include "config.h"
#include "shared_memory_state_buffer.h"

#include <atomic>

class LocalGameClient {
 public:
  LocalGameClient(const PublisherConfig& config, SharedMemoryStateBuffer& shared_buffer);
  void Run(const std::atomic<bool>& running);

 private:
  const PublisherConfig& config_;
  SharedMemoryStateBuffer& shared_buffer_;
};
