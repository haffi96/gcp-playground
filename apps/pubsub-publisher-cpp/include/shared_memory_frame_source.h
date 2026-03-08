#pragma once

#include "frame_source.h"
#include "shared_memory_state_buffer.h"

#include <cstdint>

class SharedMemoryFrameSource : public IFrameSource {
 public:
  explicit SharedMemoryFrameSource(SharedMemoryStateBuffer& shared_buffer);
  std::string NextPayload(std::uint64_t frame_id) override;

 private:
  SharedMemoryStateBuffer& shared_buffer_;
  std::uint64_t last_sequence_;
};
