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
  std::size_t entity_count;
  std::size_t target_messages;
  std::size_t inflight_limit;
  std::size_t max_batch_messages;
  std::size_t max_batch_bytes;
  std::chrono::milliseconds max_hold_time;
};

PublisherConfig LoadConfigFromEnv();
