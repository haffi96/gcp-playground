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

double SafeDouble(std::size_t value, double fallback) {
  return value == 0 ? fallback : static_cast<double>(value);
}

std::string EpochMillisecondsString() {
  const auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::system_clock::now().time_since_epoch())
                       .count();
  return std::to_string(now);
}
}  // namespace

GameStateManager::GameStateManager(const PublisherConfig& config,
                                   SharedMemoryStateBuffer& shared_buffer,
                                   InputCommandBus* input_command_bus)
    : config_(config),
      shared_buffer_(shared_buffer),
      input_command_bus_(input_command_bus),
      diagnostics_counter_(0) {}

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
    ApplyMovementCommand(command, player);
    auto* applied = frame->add_applied_commands();
    *applied = command;
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
      player_vehicle->mutable_position()->set_x(player_vehicle->position().x() - 0.55);
      player_vehicle->set_heading_rad(player_vehicle->heading_rad() - kTurnStep);
      break;
    case pubsub3d::TURN_RIGHT:
      player_vehicle->mutable_position()->set_x(player_vehicle->position().x() + 0.55);
      player_vehicle->set_heading_rad(player_vehicle->heading_rad() + kTurnStep);
      break;
    case pubsub3d::STOP:
      player_vehicle->set_speed_mps(0.0);
      break;
    default:
      break;
  }
}

void GameStateManager::PopulateStaticTrack(pubsub3d::Track* track) const {
  const std::size_t lane_count = std::max<std::size_t>(config_.lane_count, 1);
  const std::size_t lane_waypoints = std::max<std::size_t>(config_.lane_waypoint_count, 10);
  const double lane_width_meters = 3.9;
  const double shoulder_width_meters = 2.4;
  const double road_width_meters =
      lane_width_meters * static_cast<double>(lane_count) + shoulder_width_meters * 2.0;
  const double road_length = static_cast<double>(std::max<std::size_t>(config_.road_length_meters, 500));
  const double half_road_length = road_length * 0.5;
  const std::size_t one_way_count = lane_waypoints / 2;

  track->set_name(config_.scene_id);
  track->set_length_meters(road_length);
  track->set_width_meters(road_width_meters);
  track->clear_lanes();

  for (std::size_t lane_idx = 0; lane_idx < lane_count; ++lane_idx) {
    auto* lane = track->add_lanes();
    const double lane_center_x =
        (static_cast<double>(lane_idx) + 0.5 - (static_cast<double>(lane_count) * 0.5)) *
        lane_width_meters;
    lane->set_id("lane-" + std::to_string(lane_idx));
    lane->set_index(static_cast<std::uint32_t>(lane_idx));
    lane->set_center_offset_meters(lane_center_x);
    lane->set_width_meters(lane_width_meters);
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
                                        std::uint64_t frame_id) const {
  frame->clear_vehicles();
  const std::size_t lane_count = std::max<std::size_t>(config_.lane_count, 1);
  const std::size_t vehicle_count = std::max<std::size_t>(config_.vehicle_count, 1);
  const double lane_width_meters = 3.9;
  const double road_length = static_cast<double>(std::max<std::size_t>(config_.road_length_meters, 500));
  const double half_road_length = road_length * 0.5;
  const double seconds = static_cast<double>(frame_id) / static_cast<double>(std::max<std::size_t>(config_.state_tick_hz, 1));
  constexpr const char* kPalette[] = {"#f43f5e", "#22d3ee", "#f59e0b", "#84cc16",
                                      "#e879f9", "#60a5fa", "#fb7185", "#34d399"};
  constexpr std::size_t kPaletteSize = sizeof(kPalette) / sizeof(kPalette[0]);

  for (std::size_t i = 0; i < vehicle_count; ++i) {
    auto* vehicle = frame->add_vehicles();
    const std::size_t lane_idx = i == 0 ? lane_count / 2 : i % lane_count;
    const double lane_center_x =
        (static_cast<double>(lane_idx) + 0.5 - (static_cast<double>(lane_count) * 0.5)) *
        lane_width_meters;
    const double lateral_wobble = std::sin(seconds * 0.7 + static_cast<double>(i) * 0.35) * 0.28;
    const double x = lane_center_x + lateral_wobble;
    const double base_speed = i == 0 ? 44.0 : (17.0 + static_cast<double>(i % 8) * 1.35);
    const double travelled = seconds * base_speed + (i == 0 ? 0.0 : static_cast<double>(i) * 16.0);
    const std::uint64_t lap = static_cast<std::uint64_t>(travelled / SafeDouble(config_.road_length_meters, 1.0));
    const double progress = std::fmod(travelled / SafeDouble(config_.road_length_meters, 1.0), 1.0);
    const double z = std::fmod(travelled, road_length) - half_road_length;

    vehicle->set_id("car-" + std::to_string(i));
    vehicle->set_lane_id("lane-" + std::to_string(lane_idx));
    vehicle->set_lap(lap);
    vehicle->set_progress(progress);
    vehicle->set_speed_mps(base_speed);
    vehicle->set_heading_rad(0.0);
    vehicle->mutable_position()->set_x(x);
    vehicle->mutable_position()->set_y(0.6);
    vehicle->mutable_position()->set_z(z);
    vehicle->mutable_dimensions()->set_length(4.4);
    vehicle->mutable_dimensions()->set_width(2.0);
    vehicle->mutable_dimensions()->set_height(1.4);
    vehicle->set_color(kPalette[i % kPaletteSize]);
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
