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

std::string ToLower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return value;
}

void ValidateEnum(const std::string& value, const char* key,
                  std::initializer_list<const char*> allowed) {
  for (const auto* option : allowed) {
    if (value == option) {
      return;
    }
  }
  std::string message = std::string("Invalid value for ") + key + ": " + value +
                        ". Allowed values: ";
  bool first = true;
  for (const auto* option : allowed) {
    if (!first) {
      message += ", ";
    }
    message += option;
    first = false;
  }
  throw std::runtime_error(message);
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
  config.runtime_mode = ToLower(GetEnvOrDefault("RUNTIME_MODE", "server"));
  config.scene_source = ToLower(GetEnvOrDefault("SCENE_SOURCE", "synthetic"));
  ValidateEnum(config.runtime_mode, "RUNTIME_MODE", {"server", "dev"});
  ValidateEnum(config.scene_source, "SCENE_SOURCE", {"synthetic", "shared_memory"});
  config.player_actor_id = GetEnvOrDefault("PLAYER_ACTOR_ID", "car-0");
  config.input_subscription_id = GetEnvOrDefault("INPUT_SUBSCRIPTION_ID", "");
  config.enable_local_client = GetEnvBoolOrDefault("ENABLE_LOCAL_CLIENT", false);
  config.enable_remote_input = GetEnvBoolOrDefault("ENABLE_REMOTE_INPUT", false);
  config.enable_local_input_script = GetEnvBoolOrDefault("ENABLE_LOCAL_INPUT_SCRIPT", false);
  config.dry_run_frames = GetEnvSizeOrDefault("DRY_RUN_FRAMES", 3);
  config.publish_mode = ToLower(GetEnvOrDefault("PUBLISH_MODE", "fixed"));
  ValidateEnum(config.publish_mode, "PUBLISH_MODE", {"fixed", "continuous"});
  config.frame_limit = GetEnvSizeOrDefault(
      "FRAME_LIMIT", GetEnvSizeOrDefault("TARGET_MESSAGES", 20000));
  config.tick_hz = GetEnvSizeOrDefault("TICK_HZ", 10);
  config.state_tick_hz = GetEnvSizeOrDefault("STATE_TICK_HZ", config.tick_hz);
  config.local_client_tick_hz =
      GetEnvSizeOrDefault("LOCAL_CLIENT_TICK_HZ", config.tick_hz);
  config.input_queue_capacity = GetEnvSizeOrDefault("INPUT_QUEUE_CAPACITY", 4096);
  config.input_stale_ms = GetEnvSizeOrDefault("INPUT_STALE_MS", 1000);
  config.input_max_per_tick = GetEnvSizeOrDefault("INPUT_MAX_PER_TICK", 128);
  config.diagnostics_log_interval_frames =
      GetEnvSizeOrDefault("DIAGNOSTICS_LOG_INTERVAL_FRAMES", 120);
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
  if (config.enable_remote_input && config.input_subscription_id.empty()) {
    throw std::runtime_error(
        "INPUT_SUBSCRIPTION_ID is required when ENABLE_REMOTE_INPUT is enabled");
  }
  return config;
}
