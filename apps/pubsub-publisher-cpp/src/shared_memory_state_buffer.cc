#include "shared_memory_state_buffer.h"

#include <utility>

SharedMemoryStateBuffer::SharedMemoryStateBuffer() : sequence_(0) {}

std::uint64_t SharedMemoryStateBuffer::Write(std::string serialized_state) {
  std::lock_guard<std::mutex> lock(mutex_);
  serialized_state_ = std::move(serialized_state);
  const std::uint64_t next = sequence_.fetch_add(1, std::memory_order_release) + 1;
  return next;
}

SharedMemoryStateBuffer::Snapshot SharedMemoryStateBuffer::ReadLatest() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return Snapshot{
      .sequence = sequence_.load(std::memory_order_acquire),
      .serialized_state = serialized_state_,
  };
}
