#pragma once

#include <chrono>
#include <cstddef>
#include <string>

struct PublisherConfig {
  std::string project_id;
  std::string topic_id;
  std::string scene_id;
  bool dry_run;
  std::size_t dry_run_frames;
  std::string publish_mode;
  std::size_t frame_limit;
  std::size_t tick_hz;
  std::size_t lane_count;
  std::size_t lane_waypoint_count;
  std::size_t vehicle_count;
  std::size_t road_length_meters;
  std::size_t road_object_count;
  std::size_t road_object_spread_meters;
  std::size_t road_object_lane_occupancy_percent;
  std::size_t occupancy_grid_width;
  std::size_t occupancy_grid_height;
  std::size_t inflight_limit;
  std::size_t max_batch_messages;
  std::size_t max_batch_bytes;
  std::chrono::milliseconds max_hold_time;
};

PublisherConfig LoadConfigFromEnv();
