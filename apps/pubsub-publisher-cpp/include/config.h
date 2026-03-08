#pragma once

#include <chrono>
#include <cstddef>
#include <string>

struct PublisherConfig {
  std::string project_id;
  std::string topic_id;
  std::string scene_id;
  std::string runtime_mode;
  std::string scene_source;
  std::string player_actor_id;
  std::string input_subscription_id;
  bool dry_run;
  bool enable_local_client;
  bool enable_remote_input;
  bool enable_local_input_script;
  std::size_t dry_run_frames;
  std::string publish_mode;
  std::size_t frame_limit;
  std::size_t tick_hz;
  std::size_t state_tick_hz;
  std::size_t local_client_tick_hz;
  std::size_t input_queue_capacity;
  std::size_t input_stale_ms;
  std::size_t input_max_per_tick;
  std::size_t diagnostics_log_interval_frames;
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
