#include "config.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <stdexcept>

namespace {
std::string GetRequiredEnv(const char* key) {
  const char* value = std::getenv(key);
  if (value == nullptr || std::string(value).empty()) {
    throw std::runtime_error(std::string("Missing required environment variable: ") + key);
  }
  return value;
}

std::string GetEnvOrDefault(const char* key, const std::string& default_value) {
  const char* value = std::getenv(key);
  if (value == nullptr || std::string(value).empty()) {
    return default_value;
  }
  return value;
}

std::size_t GetEnvSizeOrDefault(const char* key, std::size_t default_value) {
  const char* value = std::getenv(key);
  if (value == nullptr || std::string(value).empty()) {
    return default_value;
  }
  return static_cast<std::size_t>(std::stoull(value));
}

bool GetEnvBoolOrDefault(const char* key, bool default_value) {
  const char* value = std::getenv(key);
  if (value == nullptr || std::string(value).empty()) {
    return default_value;
  }
  std::string normalized(value);
  std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return normalized == "1" || normalized == "true" || normalized == "yes" || normalized == "y";
}
}  // namespace

PublisherConfig LoadConfigFromEnv() {
  PublisherConfig config;
  config.dry_run = GetEnvBoolOrDefault("DRY_RUN", false);
  if (config.dry_run) {
    config.project_id = GetEnvOrDefault("GCP_PROJECT_ID", "dry-run-project");
    config.topic_id = GetEnvOrDefault("PUBSUB_TOPIC_ID", "dry-run-topic");
  } else {
    config.project_id = GetRequiredEnv("GCP_PROJECT_ID");
    config.topic_id = GetRequiredEnv("PUBSUB_TOPIC_ID");
  }
  config.scene_id = GetEnvOrDefault("SCENE_ID", "urban-night-circuit");
  config.dry_run_frames = GetEnvSizeOrDefault("DRY_RUN_FRAMES", 3);
  config.publish_mode = GetEnvOrDefault("PUBLISH_MODE", "fixed");
  config.frame_limit = GetEnvSizeOrDefault(
      "FRAME_LIMIT", GetEnvSizeOrDefault("TARGET_MESSAGES", 20000));
  config.tick_hz = GetEnvSizeOrDefault("TICK_HZ", 10);
  config.lane_count = GetEnvSizeOrDefault("LANE_COUNT", 4);
  config.lane_waypoint_count = GetEnvSizeOrDefault("LANE_WAYPOINT_COUNT", 120);
  config.vehicle_count = GetEnvSizeOrDefault("VEHICLE_COUNT", 120);
  config.road_length_meters = GetEnvSizeOrDefault("ROAD_LENGTH_METERS", 1800);
  config.road_object_count = GetEnvSizeOrDefault("ROAD_OBJECT_COUNT", 30);
  config.road_object_spread_meters = GetEnvSizeOrDefault("ROAD_OBJECT_SPREAD_METERS", 900);
  config.road_object_lane_occupancy_percent =
      GetEnvSizeOrDefault("ROAD_OBJECT_LANE_OCCUPANCY_PERCENT", 35);
  config.occupancy_grid_width = GetEnvSizeOrDefault("OCCUPANCY_GRID_WIDTH", 80);
  config.occupancy_grid_height = GetEnvSizeOrDefault("OCCUPANCY_GRID_HEIGHT", 80);
  config.inflight_limit = GetEnvSizeOrDefault("INFLIGHT_LIMIT", 2000);
  config.max_batch_messages = GetEnvSizeOrDefault("MAX_BATCH_MESSAGES", 500);
  config.max_batch_bytes = GetEnvSizeOrDefault("MAX_BATCH_BYTES", 4 * 1024 * 1024);
  config.max_hold_time = std::chrono::milliseconds(
      static_cast<int>(GetEnvSizeOrDefault("MAX_HOLD_TIME_MS", 20)));
  return config;
}
