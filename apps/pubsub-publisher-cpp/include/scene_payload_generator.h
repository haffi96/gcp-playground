#pragma once

#include <cstdint>
#include <string>

class ScenePayloadGenerator {
 public:
  explicit ScenePayloadGenerator(std::string scene_id, std::size_t entity_count);
  std::string Generate(std::uint64_t frame_id) const;

 private:
  std::string scene_id_;
  std::size_t entity_count_;
};
