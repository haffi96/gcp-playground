#pragma once

#include <cstdint>
#include <string>

class IFrameSource {
 public:
  virtual ~IFrameSource() = default;
  virtual std::string NextPayload(std::uint64_t frame_id) = 0;
};
