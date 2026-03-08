#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>

class SharedMemoryStateBuffer {
 public:
  struct Snapshot {
    std::uint64_t sequence;
    std::string serialized_state;
  };

  SharedMemoryStateBuffer();

  std::uint64_t Write(std::string serialized_state);
  Snapshot ReadLatest() const;

 private:
  mutable std::mutex mutex_;
  std::string serialized_state_;
  std::atomic<std::uint64_t> sequence_;
};
