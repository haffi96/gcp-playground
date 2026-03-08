#include "config.h"
#include "game_state_manager.h"
#include "input_command_bus.h"
#include "proto/game_state.pb.h"
#include "shared_memory_state_buffer.h"

#include <atomic>
#include <cmath>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <thread>

namespace {
constexpr double kEpsilon = 0.0001;

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
  config.player_autopilot_mode = "legacy";
  config.input_subscription_id = "";
  config.dry_run = true;
  config.enable_local_client = false;
  config.enable_remote_input = false;
  config.enable_local_input_script = false;
  config.player_autopilot_enabled = false;
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

bool NearlyEqual(double lhs, double rhs) {
  return std::fabs(lhs - rhs) < kEpsilon;
}

pubsub3d::GameStateFrame WaitForFrame(SharedMemoryStateBuffer& buffer, int attempts = 20) {
  pubsub3d::GameStateFrame frame;
  for (int attempt = 0; attempt < attempts; ++attempt) {
    const auto snapshot = buffer.ReadLatest();
    if (!snapshot.serialized_state.empty() &&
        frame.ParseFromString(snapshot.serialized_state) &&
        frame.vehicles_size() > 0) {
      return frame;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }
  return frame;
}

bool RunInputOnlyStationaryScenario() {
  auto config = TestConfig();
  config.player_autopilot_enabled = false;
  config.player_autopilot_mode = "legacy";
  SharedMemoryStateBuffer buffer;
  InputCommandBus bus(32);
  GameStateManager manager(config, buffer, &bus);

  std::atomic<bool> running{true};
  std::thread manager_thread([&]() { manager.Run(running); });
  std::this_thread::sleep_for(std::chrono::milliseconds(30));

  const auto baseline = WaitForFrame(buffer);
  std::this_thread::sleep_for(std::chrono::milliseconds(80));
  const auto later = WaitForFrame(buffer);

  running.store(false);
  manager_thread.join();

  bool ok = true;
  ok &= Check(baseline.vehicles_size() > 0, "stationary baseline frame must exist");
  ok &= Check(later.vehicles_size() > 0, "stationary later frame must exist");
  ok &= Check(NearlyEqual(baseline.vehicles(0).position().x(), later.vehicles(0).position().x()),
              "input_only player x should remain stationary");
  ok &= Check(NearlyEqual(baseline.vehicles(0).position().z(), later.vehicles(0).position().z()),
              "input_only player z should remain stationary");
  return ok;
}

bool RunInputOnlyMovementScenario() {
  auto config = TestConfig();
  config.player_autopilot_enabled = false;
  config.player_autopilot_mode = "input_only";
  SharedMemoryStateBuffer buffer;
  InputCommandBus bus(32);
  GameStateManager manager(config, buffer, &bus);

  std::atomic<bool> running{true};
  std::thread manager_thread([&]() { manager.Run(running); });
  std::this_thread::sleep_for(std::chrono::milliseconds(30));

  const auto baseline = WaitForFrame(buffer);

  bus.Push(MakeCommand(2, pubsub3d::TURN_RIGHT));
  bus.Push(MakeCommand(1, pubsub3d::MOVE_FORWARD));

  pubsub3d::GameStateFrame applied_frame;
  bool saw_applied_commands = false;
  for (int attempt = 0; attempt < 20; ++attempt) {
    const auto snapshot = buffer.ReadLatest();
    if (!snapshot.serialized_state.empty() &&
        applied_frame.ParseFromString(snapshot.serialized_state) &&
        applied_frame.applied_commands_size() >= 2) {
      saw_applied_commands = true;
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(80));
  const auto persisted_frame = WaitForFrame(buffer);

  running.store(false);
  manager_thread.join();

  bool ok = true;
  ok &= Check(saw_applied_commands, "commands should be applied");
  ok &= Check(applied_frame.applied_commands_size() >= 2, "commands should exist");
  ok &= Check(applied_frame.applied_commands(0).sequence() == 1,
              "commands should be ordered by sequence");
  ok &= Check(applied_frame.applied_commands(1).sequence() == 2,
              "commands should be ordered by sequence");
  ok &= Check(applied_frame.vehicles_size() > 0, "must have player vehicle");
  ok &= Check(applied_frame.vehicles(0).position().z() > baseline.vehicles(0).position().z(),
              "forward command should move player z");
  ok &= Check(applied_frame.vehicles(0).position().x() < baseline.vehicles(0).position().x(),
              "turn right command should move player toward negative x");
  ok &= Check(persisted_frame.vehicles(0).position().z() > applied_frame.vehicles(0).position().z(),
              "input_only player should keep moving forward until stop");
  ok &= Check(persisted_frame.vehicles(0).position().x() < applied_frame.vehicles(0).position().x(),
              "input_only player should keep steering right until stop");

  bus.Push(MakeCommand(3, pubsub3d::STOP));
  std::this_thread::sleep_for(std::chrono::milliseconds(80));
  const auto stopped_frame = WaitForFrame(buffer);
  std::this_thread::sleep_for(std::chrono::milliseconds(80));
  const auto later_stopped_frame = WaitForFrame(buffer);

  ok &= Check(NearlyEqual(stopped_frame.vehicles(0).position().x(),
                          later_stopped_frame.vehicles(0).position().x()),
              "stop should halt lateral movement");
  ok &= Check(NearlyEqual(stopped_frame.vehicles(0).position().z(),
                          later_stopped_frame.vehicles(0).position().z()),
              "stop should halt forward movement");
  return ok;
}

bool RunLegacyAutopilotScenario() {
  auto config = TestConfig();
  config.player_autopilot_enabled = true;
  config.player_autopilot_mode = "legacy";
  SharedMemoryStateBuffer buffer;
  InputCommandBus bus(32);
  GameStateManager manager(config, buffer, &bus);

  std::atomic<bool> running{true};
  std::thread manager_thread([&]() { manager.Run(running); });
  std::this_thread::sleep_for(std::chrono::milliseconds(30));

  const auto baseline = WaitForFrame(buffer);
  std::this_thread::sleep_for(std::chrono::milliseconds(80));
  const auto later = WaitForFrame(buffer);

  running.store(false);
  manager_thread.join();

  bool ok = true;
  ok &= Check(baseline.vehicles_size() > 0, "legacy baseline frame must exist");
  ok &= Check(later.vehicles_size() > 0, "legacy later frame must exist");
  ok &= Check(!NearlyEqual(baseline.vehicles(0).position().x(), later.vehicles(0).position().x()) ||
                  !NearlyEqual(baseline.vehicles(0).position().z(), later.vehicles(0).position().z()),
              "legacy autopilot player should move without input");
  return ok;
}
}  // namespace

int main() {
  bool ok = true;
  ok &= RunInputOnlyStationaryScenario();
  ok &= RunInputOnlyMovementScenario();
  ok &= RunLegacyAutopilotScenario();
  return ok ? 0 : 1;
}
