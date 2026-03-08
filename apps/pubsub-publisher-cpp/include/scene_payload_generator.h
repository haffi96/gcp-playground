#pragma once

#include "frame_source.h"

#include <cstdint>
#include <string>

class ScenePayloadGenerator {
 public:
  explicit ScenePayloadGenerator(std::string scene_id, std::size_t lane_count,
                                 std::size_t lane_waypoint_count,
                                 std::size_t vehicle_count,
                                 std::size_t road_length_meters,
                                 std::size_t road_object_count,
                                 std::size_t road_object_spread_meters,
                                 std::size_t road_object_lane_occupancy_percent,
                                 std::size_t occupancy_grid_width,
                                 std::size_t occupancy_grid_height);
  std::string Generate(std::uint64_t frame_id) const;

 private:
  std::string scene_id_;
  std::size_t lane_count_;
  std::size_t lane_waypoint_count_;
  std::size_t vehicle_count_;
  std::size_t road_length_meters_;
  std::size_t road_object_count_;
  std::size_t road_object_spread_meters_;
  std::size_t road_object_lane_occupancy_percent_;
  std::size_t occupancy_grid_width_;
  std::size_t occupancy_grid_height_;
};

class SyntheticFrameSource : public IFrameSource {
 public:
  explicit SyntheticFrameSource(ScenePayloadGenerator generator);
  std::string NextPayload(std::uint64_t frame_id) override;

 private:
  ScenePayloadGenerator generator_;
};
