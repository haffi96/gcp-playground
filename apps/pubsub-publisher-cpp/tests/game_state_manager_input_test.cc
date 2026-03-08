#include "config.h"
#include "game_state_manager.h"
#include "input_command_bus.h"
#include "proto/game_state.pb.h"
#include "shared_memory_state_buffer.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <thread>

namespace {
bool Check(bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "game_state_manager_input_test_failed reason=" << message
              << std::endl;
    return false;
  }
  return true;
}

PublisherConfig TestConfig() {
  PublisherConfig config;
  config.project_id = "dry-run";
  config.topic_id = "dry-run";
  config.scene_id = "urban-night-circuit";
  config.runtime_mode = "dev";
  config.scene_source = "shared_memory";
  config.player_actor_id = "car-0";
  config.input_subscription_id = "";
  config.dry_run = true;
  config.enable_local_client = false;
  config.enable_remote_input = false;
  config.enable_local_input_script = false;
  config.dry_run_frames = 1;
  config.publish_mode = "fixed";
  config.frame_limit = 1;
  config.tick_hz = 20;
  config.state_tick_hz = 20;
  config.local_client_tick_hz = 20;
  config.input_queue_capacity = 100;
  config.input_stale_ms = 1000;
  config.input_max_per_tick = 100;
  config.diagnostics_log_interval_frames = 10;
  config.lane_count = 4;
  config.lane_waypoint_count = 20;
  config.vehicle_count = 4;
  config.road_length_meters = 1000;
  config.road_object_count = 0;
  config.road_object_spread_meters = 500;
  config.road_object_lane_occupancy_percent = 20;
  config.occupancy_grid_width = 10;
  config.occupancy_grid_height = 10;
  config.inflight_limit = 10;
  config.max_batch_messages = 10;
  config.max_batch_bytes = 1024 * 1024;
  config.max_hold_time = std::chrono::milliseconds(20);
  return config;
}

pubsub3d::InputCommand MakeCommand(std::uint64_t sequence, pubsub3d::InputAction action) {
  pubsub3d::InputCommand command;
  command.set_schema_version("1.0.0");
  command.set_command_id("cmd-" + std::to_string(sequence));
  command.set_client_id("test-client");
  command.set_scene_id("urban-night-circuit");
  command.set_actor_id("car-0");
  command.set_action(action);
  command.set_timestamp_ms(1700000000000ULL);
  command.set_sequence(sequence);
  return command;
}
}  // namespace

int main() {
  auto config = TestConfig();
  SharedMemoryStateBuffer buffer;
  InputCommandBus bus(32);
  GameStateManager manager(config, buffer, &bus);

  std::atomic<bool> running{true};
  std::thread manager_thread([&]() { manager.Run(running); });
  std::this_thread::sleep_for(std::chrono::milliseconds(30));

  double baseline_x = 0.0;
  double baseline_z = 0.0;
  {
    const auto baseline_snapshot = buffer.ReadLatest();
    pubsub3d::GameStateFrame baseline_frame;
    if (baseline_frame.ParseFromString(baseline_snapshot.serialized_state) &&
        baseline_frame.vehicles_size() > 0) {
      baseline_x = baseline_frame.vehicles(0).position().x();
      baseline_z = baseline_frame.vehicles(0).position().z();
    }
  }

  bus.Push(MakeCommand(2, pubsub3d::TURN_RIGHT));
  bus.Push(MakeCommand(1, pubsub3d::MOVE_FORWARD));

  pubsub3d::GameStateFrame frame;
  bool saw_applied_commands = false;
  for (int attempt = 0; attempt < 20; ++attempt) {
    const auto snapshot = buffer.ReadLatest();
    if (!snapshot.serialized_state.empty() &&
        frame.ParseFromString(snapshot.serialized_state) &&
        frame.applied_commands_size() >= 2) {
      saw_applied_commands = true;
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }

  running.store(false);
  manager_thread.join();

  bool ok = true;
  ok &= Check(saw_applied_commands, "commands should be applied");
  ok &= Check(frame.applied_commands_size() >= 2, "commands should exist");
  ok &= Check(frame.applied_commands(0).sequence() == 1,
              "commands should be ordered by sequence");
  ok &= Check(frame.applied_commands(1).sequence() == 2,
              "commands should be ordered by sequence");
  ok &= Check(frame.vehicles_size() > 0, "must have player vehicle");
  ok &= Check(frame.vehicles(0).position().z() > baseline_z,
              "forward command should move player z");
  ok &= Check(frame.vehicles(0).position().x() > baseline_x,
              "turn right command should move player x");
  return ok ? 0 : 1;
}
