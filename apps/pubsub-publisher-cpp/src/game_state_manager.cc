#include "game_state_manager.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <thread>
#include <utility>

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kLaneWidthMeters = 3.9;
constexpr double kPlayerBaseSpeedMps = 44.0;
constexpr double kPlayerHeightMeters = 0.6;
constexpr double kPlayerCruiseSpeedMps = 22.0;
constexpr double kPlayerReverseSpeedMps = 9.0;
constexpr double kPlayerLateralSpeedMps = 7.5;
constexpr double kPlayerMaxHeadingRad = 0.26;
constexpr const char* kPalette[] = {"#f43f5e", "#22d3ee", "#f59e0b", "#84cc16",
                                    "#e879f9", "#60a5fa", "#fb7185", "#34d399"};
constexpr std::size_t kPaletteSize = sizeof(kPalette) / sizeof(kPalette[0]);

double SafeDouble(std::size_t value, double fallback) {
  return value == 0 ? fallback : static_cast<double>(value);
}

std::string EpochMillisecondsString() {
  const auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::system_clock::now().time_since_epoch())
                       .count();
  return std::to_string(now);
}

double LaneCenterX(std::size_t lane_idx, std::size_t lane_count) {
  return (static_cast<double>(lane_idx) + 0.5 - (static_cast<double>(lane_count) * 0.5)) *
         kLaneWidthMeters;
}

double Clamp(double value, double min_value, double max_value) {
  return std::max(min_value, std::min(max_value, value));
}
}  // namespace

GameStateManager::GameStateManager(const PublisherConfig& config,
                                   SharedMemoryStateBuffer& shared_buffer,
                                   InputCommandBus* input_command_bus)
    : config_(config),
      shared_buffer_(shared_buffer),
      input_command_bus_(input_command_bus),
      diagnostics_counter_(0),
      player_state_initialized_(false),
      move_forward_active_(false),
      move_back_active_(false),
      turn_left_active_(false),
      turn_right_active_(false) {}

void GameStateManager::Run(const std::atomic<bool>& running) {
  std::uint64_t frame_id = 0;
  const std::size_t tick_hz = std::max<std::size_t>(config_.state_tick_hz, 1);
  const auto tick_interval = std::chrono::microseconds(1000000 / tick_hz);
  auto next_tick = std::chrono::steady_clock::now();

  while (running.load()) {
    auto frame = BuildFrame(frame_id);
    std::string serialized;
    frame.SerializeToString(&serialized);
    const std::uint64_t sequence = shared_buffer_.Write(std::move(serialized));

    ++frame_id;
    next_tick += tick_interval;
    const auto now = std::chrono::steady_clock::now();
    const auto drift_us = std::chrono::duration_cast<std::chrono::microseconds>(
                              now - next_tick)
                              .count();
    ++diagnostics_counter_;
    if (diagnostics_counter_ % std::max<std::size_t>(config_.diagnostics_log_interval_frames, 1) ==
        0) {
      const std::size_t queue_depth =
          input_command_bus_ != nullptr ? input_command_bus_->Size() : 0;
      std::cout << "game_state_tick frame_id=" << frame_id
                << " shared_sequence=" << sequence << " tick_drift_us="
                << drift_us << " command_queue_depth=" << queue_depth
                << std::endl;
    }
    std::this_thread::sleep_until(next_tick);
  }
}

pubsub3d::GameStateFrame GameStateManager::BuildFrame(std::uint64_t frame_id) {
  pubsub3d::GameStateFrame frame;
  frame.set_schema_version("1.0.0");
  frame.set_timestamp(EpochMillisecondsString());
  frame.set_scene_id(config_.scene_id);
  frame.set_frame_id(frame_id);
  frame.set_server_tick(frame_id);

  PopulateStaticTrack(frame.mutable_track());
  PopulateOccupancyGrid(frame.mutable_occupancy_grid());
  PopulateVehicles(&frame, frame_id);
  PopulateRoadObjects(&frame, frame_id);
  ApplyCommands(&frame);
  return frame;
}

bool GameStateManager::UseLegacyAutopilot() const {
  return config_.player_autopilot_enabled && config_.player_autopilot_mode == "legacy";
}

void GameStateManager::ApplyCommands(pubsub3d::GameStateFrame* frame) {
  if (input_command_bus_ == nullptr || frame->vehicles_size() == 0) {
    return;
  }

  auto commands =
      input_command_bus_->Drain(std::max<std::size_t>(config_.input_max_per_tick, 1));
  std::sort(commands.begin(), commands.end(),
            [](const pubsub3d::InputCommand& lhs,
               const pubsub3d::InputCommand& rhs) {
              if (lhs.timestamp_ms() != rhs.timestamp_ms()) {
                return lhs.timestamp_ms() < rhs.timestamp_ms();
              }
              if (lhs.sequence() != rhs.sequence()) {
                return lhs.sequence() < rhs.sequence();
              }
              return lhs.command_id() < rhs.command_id();
            });

  pubsub3d::Vehicle* player = frame->mutable_vehicles(0);
  for (const auto& command : commands) {
    if (!command.actor_id().empty() && command.actor_id() != config_.player_actor_id) {
      continue;
    }
    if (UseLegacyAutopilot()) {
      ApplyMovementCommand(command, player);
    } else {
      ApplyInputStateCommand(command);
    }
    auto* applied = frame->add_applied_commands();
    *applied = command;
  }

  if (!UseLegacyAutopilot()) {
    IntegrateInputOnlyPlayer(player);
    SyncPlayerStateFromVehicle(*player);
  }
}

void GameStateManager::ApplyMovementCommand(const pubsub3d::InputCommand& command,
                                            pubsub3d::Vehicle* player_vehicle) {
  constexpr double kStep = 1.1;
  constexpr double kTurnStep = 0.12;
  switch (command.action()) {
    case pubsub3d::MOVE_FORWARD:
      player_vehicle->mutable_position()->set_z(player_vehicle->position().z() + kStep);
      player_vehicle->set_speed_mps(std::max(1.0, player_vehicle->speed_mps() + 0.5));
      break;
    case pubsub3d::MOVE_BACK:
      player_vehicle->mutable_position()->set_z(player_vehicle->position().z() - kStep);
      player_vehicle->set_speed_mps(std::max(0.0, player_vehicle->speed_mps() - 0.5));
      break;
    case pubsub3d::TURN_LEFT:
      player_vehicle->mutable_position()->set_x(player_vehicle->position().x() + 0.55);
      player_vehicle->set_heading_rad(player_vehicle->heading_rad() + kTurnStep);
      break;
    case pubsub3d::TURN_RIGHT:
      player_vehicle->mutable_position()->set_x(player_vehicle->position().x() - 0.55);
      player_vehicle->set_heading_rad(player_vehicle->heading_rad() - kTurnStep);
      break;
    case pubsub3d::STOP:
      player_vehicle->set_speed_mps(0.0);
      break;
    default:
      break;
  }
}

void GameStateManager::ApplyInputStateCommand(
    const pubsub3d::InputCommand& command) {
  switch (command.action()) {
    case pubsub3d::MOVE_FORWARD:
      move_forward_active_ = true;
      move_back_active_ = false;
      break;
    case pubsub3d::MOVE_BACK:
      move_back_active_ = true;
      move_forward_active_ = false;
      break;
    case pubsub3d::TURN_LEFT:
      turn_left_active_ = true;
      turn_right_active_ = false;
      break;
    case pubsub3d::TURN_RIGHT:
      turn_right_active_ = true;
      turn_left_active_ = false;
      break;
    case pubsub3d::STOP:
      move_forward_active_ = false;
      move_back_active_ = false;
      turn_left_active_ = false;
      turn_right_active_ = false;
      break;
    default:
      break;
  }
}

void GameStateManager::IntegrateInputOnlyPlayer(
    pubsub3d::Vehicle* player_vehicle) const {
  const double tick_seconds =
      1.0 / static_cast<double>(std::max<std::size_t>(config_.state_tick_hz, 1));
  const double road_length =
      static_cast<double>(std::max<std::size_t>(config_.road_length_meters, 500));
  const double half_road_length = road_length * 0.5;
  const double road_width =
      kLaneWidthMeters * static_cast<double>(std::max<std::size_t>(config_.lane_count, 1)) +
      (2.4 * 2.0);
  const double max_lateral_x = (road_width * 0.5) - 1.0;

  const double forward_velocity =
      move_forward_active_ ? kPlayerCruiseSpeedMps
                           : (move_back_active_ ? -kPlayerReverseSpeedMps : 0.0);
  const double lateral_velocity =
      turn_left_active_ ? kPlayerLateralSpeedMps
                        : (turn_right_active_ ? -kPlayerLateralSpeedMps : 0.0);

  double next_x = player_vehicle->position().x() + (lateral_velocity * tick_seconds);
  double next_z = player_vehicle->position().z() + (forward_velocity * tick_seconds);
  std::uint64_t lap = player_vehicle->lap();

  if (next_z > half_road_length) {
    next_z -= road_length;
    ++lap;
  } else if (next_z < -half_road_length) {
    next_z += road_length;
    if (lap > 0) {
      --lap;
    }
  }

  next_x = Clamp(next_x, -max_lateral_x, max_lateral_x);
  player_vehicle->mutable_position()->set_x(next_x);
  player_vehicle->mutable_position()->set_y(kPlayerHeightMeters);
  player_vehicle->mutable_position()->set_z(next_z);
  player_vehicle->set_speed_mps(std::fabs(forward_velocity));
  player_vehicle->set_lap(lap);
  player_vehicle->set_progress((next_z + half_road_length) / road_length);

  if (turn_left_active_ == turn_right_active_) {
    player_vehicle->set_heading_rad(0.0);
  } else {
    player_vehicle->set_heading_rad(turn_left_active_ ? kPlayerMaxHeadingRad
                                                      : -kPlayerMaxHeadingRad);
  }
}

void GameStateManager::InitializePlayerState() {
  if (player_state_initialized_) {
    return;
  }

  const std::size_t lane_count = std::max<std::size_t>(config_.lane_count, 1);
  const std::size_t lane_idx = lane_count / 2;
  const double road_length =
      static_cast<double>(std::max<std::size_t>(config_.road_length_meters, 500));
  const double half_road_length = road_length * 0.5;

  player_state_.set_id(config_.player_actor_id);
  player_state_.set_lane_id("lane-" + std::to_string(lane_idx));
  player_state_.set_lap(0);
  player_state_.set_progress(0.0);
  player_state_.set_speed_mps(0.0);
  player_state_.set_heading_rad(0.0);
  player_state_.mutable_position()->set_x(LaneCenterX(lane_idx, lane_count));
  player_state_.mutable_position()->set_y(kPlayerHeightMeters);
  player_state_.mutable_position()->set_z(-half_road_length);
  player_state_.mutable_dimensions()->set_length(4.4);
  player_state_.mutable_dimensions()->set_width(2.0);
  player_state_.mutable_dimensions()->set_height(1.4);
  player_state_.set_color(kPalette[0]);
  player_state_initialized_ = true;
}

void GameStateManager::SyncPlayerStateFromVehicle(
    const pubsub3d::Vehicle& player_vehicle) {
  player_state_ = player_vehicle;
  player_state_initialized_ = true;
}

pubsub3d::Vehicle GameStateManager::BuildLegacyAutopilotPlayer(
    std::uint64_t frame_id) const {
  const std::size_t lane_count = std::max<std::size_t>(config_.lane_count, 1);
  const std::size_t lane_idx = lane_count / 2;
  const double road_length =
      static_cast<double>(std::max<std::size_t>(config_.road_length_meters, 500));
  const double half_road_length = road_length * 0.5;
  const double seconds =
      static_cast<double>(frame_id) /
      static_cast<double>(std::max<std::size_t>(config_.state_tick_hz, 1));
  const double lateral_wobble = std::sin(seconds * 0.7) * 0.28;
  const double travelled = seconds * kPlayerBaseSpeedMps;
  const std::uint64_t lap = static_cast<std::uint64_t>(
      travelled / SafeDouble(config_.road_length_meters, 1.0));
  const double progress =
      std::fmod(travelled / SafeDouble(config_.road_length_meters, 1.0), 1.0);
  const double z = std::fmod(travelled, road_length) - half_road_length;

  pubsub3d::Vehicle vehicle;
  vehicle.set_id(config_.player_actor_id);
  vehicle.set_lane_id("lane-" + std::to_string(lane_idx));
  vehicle.set_lap(lap);
  vehicle.set_progress(progress);
  vehicle.set_speed_mps(kPlayerBaseSpeedMps);
  vehicle.set_heading_rad(0.0);
  vehicle.mutable_position()->set_x(LaneCenterX(lane_idx, lane_count) + lateral_wobble);
  vehicle.mutable_position()->set_y(kPlayerHeightMeters);
  vehicle.mutable_position()->set_z(z);
  vehicle.mutable_dimensions()->set_length(4.4);
  vehicle.mutable_dimensions()->set_width(2.0);
  vehicle.mutable_dimensions()->set_height(1.4);
  vehicle.set_color(kPalette[0]);
  return vehicle;
}

pubsub3d::Vehicle GameStateManager::BuildInputOnlyPlayer() const {
  return player_state_;
}

pubsub3d::Vehicle GameStateManager::BuildNpcVehicle(std::size_t vehicle_index,
                                                    std::uint64_t frame_id) const {
  const std::size_t lane_count = std::max<std::size_t>(config_.lane_count, 1);
  const double road_length =
      static_cast<double>(std::max<std::size_t>(config_.road_length_meters, 500));
  const double half_road_length = road_length * 0.5;
  const double seconds =
      static_cast<double>(frame_id) /
      static_cast<double>(std::max<std::size_t>(config_.state_tick_hz, 1));
  const std::size_t lane_idx = vehicle_index % lane_count;
  const double lateral_wobble =
      std::sin(seconds * 0.7 + static_cast<double>(vehicle_index) * 0.35) * 0.28;
  const double x = LaneCenterX(lane_idx, lane_count) + lateral_wobble;
  const double base_speed = 17.0 + static_cast<double>(vehicle_index % 8) * 1.35;
  const double travelled =
      seconds * base_speed + static_cast<double>(vehicle_index) * 16.0;
  const std::uint64_t lap = static_cast<std::uint64_t>(
      travelled / SafeDouble(config_.road_length_meters, 1.0));
  const double progress =
      std::fmod(travelled / SafeDouble(config_.road_length_meters, 1.0), 1.0);
  const double z = std::fmod(travelled, road_length) - half_road_length;

  pubsub3d::Vehicle vehicle;
  vehicle.set_id("car-" + std::to_string(vehicle_index));
  vehicle.set_lane_id("lane-" + std::to_string(lane_idx));
  vehicle.set_lap(lap);
  vehicle.set_progress(progress);
  vehicle.set_speed_mps(base_speed);
  vehicle.set_heading_rad(0.0);
  vehicle.mutable_position()->set_x(x);
  vehicle.mutable_position()->set_y(kPlayerHeightMeters);
  vehicle.mutable_position()->set_z(z);
  vehicle.mutable_dimensions()->set_length(4.4);
  vehicle.mutable_dimensions()->set_width(2.0);
  vehicle.mutable_dimensions()->set_height(1.4);
  vehicle.set_color(kPalette[vehicle_index % kPaletteSize]);
  return vehicle;
}

void GameStateManager::PopulateStaticTrack(pubsub3d::Track* track) const {
  const std::size_t lane_count = std::max<std::size_t>(config_.lane_count, 1);
  const std::size_t lane_waypoints = std::max<std::size_t>(config_.lane_waypoint_count, 10);
  const double shoulder_width_meters = 2.4;
  const double road_width_meters =
      kLaneWidthMeters * static_cast<double>(lane_count) + shoulder_width_meters * 2.0;
  const double road_length = static_cast<double>(std::max<std::size_t>(config_.road_length_meters, 500));
  const double half_road_length = road_length * 0.5;
  const std::size_t one_way_count = lane_waypoints / 2;

  track->set_name(config_.scene_id);
  track->set_length_meters(road_length);
  track->set_width_meters(road_width_meters);
  track->clear_lanes();

  for (std::size_t lane_idx = 0; lane_idx < lane_count; ++lane_idx) {
    auto* lane = track->add_lanes();
    const double lane_center_x = LaneCenterX(lane_idx, lane_count);
    lane->set_id("lane-" + std::to_string(lane_idx));
    lane->set_index(static_cast<std::uint32_t>(lane_idx));
    lane->set_center_offset_meters(lane_center_x);
    lane->set_width_meters(kLaneWidthMeters);
    lane->set_speed_limit_kph(120.0 - static_cast<double>(lane_idx * 4));

    for (std::size_t wp = 0; wp < one_way_count; ++wp) {
      const double t = static_cast<double>(wp) / SafeDouble(one_way_count - 1, 1.0);
      auto* waypoint = lane->add_waypoints();
      waypoint->set_x(lane_center_x);
      waypoint->set_y(0.0);
      waypoint->set_z((-half_road_length) + (road_length * t));
    }
    for (std::size_t wp = 0; wp < one_way_count; ++wp) {
      const double t = static_cast<double>(wp) / SafeDouble(one_way_count - 1, 1.0);
      auto* waypoint = lane->add_waypoints();
      waypoint->set_x(lane_center_x + 0.05);
      waypoint->set_y(0.0);
      waypoint->set_z(half_road_length - (road_length * t));
    }
  }
}

void GameStateManager::PopulateOccupancyGrid(pubsub3d::OccupancyGrid* grid) const {
  const std::size_t lane_count = std::max<std::size_t>(config_.lane_count, 1);
  const std::size_t grid_width = std::max<std::size_t>(config_.occupancy_grid_width, 1);
  const std::size_t grid_height = std::max<std::size_t>(config_.occupancy_grid_height, 1);
  const double road_width = 3.9 * static_cast<double>(lane_count) + (2.4 * 2.0);
  const double cell_size_meters = std::max(2.0, road_width / std::max(2.0, static_cast<double>(lane_count * 2)));
  const double grid_span_x = static_cast<double>(grid_width) * cell_size_meters;
  const double grid_span_z = static_cast<double>(grid_height) * cell_size_meters;

  grid->mutable_origin()->set_x(-grid_span_x * 0.5);
  grid->mutable_origin()->set_y(0.1);
  grid->mutable_origin()->set_z(-grid_span_z * 0.5);
  grid->set_cell_size_meters(cell_size_meters);
  grid->set_width(static_cast<std::uint32_t>(grid_width));
  grid->set_height(static_cast<std::uint32_t>(grid_height));
  grid->clear_cells();
  for (std::size_t i = 0; i < grid_width * grid_height; ++i) {
    grid->add_cells(0);
  }
}

void GameStateManager::PopulateVehicles(pubsub3d::GameStateFrame* frame,
                                        std::uint64_t frame_id) {
  frame->clear_vehicles();
  const std::size_t vehicle_count = std::max<std::size_t>(config_.vehicle_count, 1);
  if (UseLegacyAutopilot()) {
    *frame->add_vehicles() = BuildLegacyAutopilotPlayer(frame_id);
  } else {
    InitializePlayerState();
    *frame->add_vehicles() = BuildInputOnlyPlayer();
  }

  for (std::size_t i = 1; i < vehicle_count; ++i) {
    *frame->add_vehicles() = BuildNpcVehicle(i, frame_id);
  }
}

void GameStateManager::PopulateRoadObjects(pubsub3d::GameStateFrame* frame,
                                           std::uint64_t frame_id) const {
  frame->clear_road_objects();
  const std::size_t object_count = config_.road_object_count;
  const std::size_t lane_count = std::max<std::size_t>(config_.lane_count, 1);
  const double lane_width_meters = 3.9;
  const double road_width = lane_width_meters * static_cast<double>(lane_count) + (2.4 * 2.0);
  const double road_length = static_cast<double>(std::max<std::size_t>(config_.road_length_meters, 500));
  const double half_road_length = road_length * 0.5;
  const double seconds = static_cast<double>(frame_id) / static_cast<double>(std::max<std::size_t>(config_.state_tick_hz, 1));
  constexpr const char* kObjectTypes[] = {"cone", "barrel", "sign"};
  constexpr const char* kObjectPalette[] = {"#fb7185", "#f59e0b", "#facc15", "#60a5fa", "#34d399"};
  constexpr std::size_t kTypeSize = sizeof(kObjectTypes) / sizeof(kObjectTypes[0]);
  constexpr std::size_t kColorSize = sizeof(kObjectPalette) / sizeof(kObjectPalette[0]);

  for (std::size_t i = 0; i < object_count; ++i) {
    auto* object = frame->add_road_objects();
    const std::size_t lane_idx = i % lane_count;
    const double lane_center_x =
        (static_cast<double>(lane_idx) + 0.5 - (static_cast<double>(lane_count) * 0.5)) *
        lane_width_meters;
    const double x = lane_center_x + std::sin(seconds + static_cast<double>(i) * 0.33) * 0.2;
    const double anchor = static_cast<double>((i * 53) % std::max<std::size_t>(config_.road_length_meters, 500));
    const double z = std::fmod(anchor - (seconds * 18.0) + road_length, road_length) - half_road_length;
    const std::string type = kObjectTypes[i % kTypeSize];

    object->set_id("obj-" + std::to_string(i));
    object->set_type(type);
    object->mutable_position()->set_x(x);
    object->mutable_position()->set_y(type == "sign" ? 0.95 : 0.5);
    object->mutable_position()->set_z(z);
    object->mutable_rotation()->set_x(0.0);
    object->mutable_rotation()->set_y(std::fmod((seconds + i) * 0.2, 2.0 * kPi));
    object->mutable_rotation()->set_z(0.0);
    object->mutable_size()->set_length(type == "sign" ? 0.45 : 0.8);
    object->mutable_size()->set_width(type == "sign" ? 1.4 : 0.8);
    object->mutable_size()->set_height(type == "sign" ? 1.9 : 1.0);
    object->set_color(kObjectPalette[i % kColorSize]);
  }
}
