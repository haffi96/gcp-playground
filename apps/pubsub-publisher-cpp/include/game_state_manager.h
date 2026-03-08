#pragma once

#include "config.h"
#include "input_command_bus.h"
#include "proto/game_state.pb.h"
#include "shared_memory_state_buffer.h"

#include <atomic>
#include <cstdint>

class GameStateManager {
 public:
  GameStateManager(const PublisherConfig& config, SharedMemoryStateBuffer& shared_buffer,
                   InputCommandBus* input_command_bus);
  void Run(const std::atomic<bool>& running);

 private:
  pubsub3d::GameStateFrame BuildFrame(std::uint64_t frame_id);
  void ApplyCommands(pubsub3d::GameStateFrame* frame);
  void ApplyMovementCommand(const pubsub3d::InputCommand& command,
                            pubsub3d::Vehicle* player_vehicle);
  void ApplyInputStateCommand(const pubsub3d::InputCommand& command);
  void IntegrateInputOnlyPlayer(pubsub3d::Vehicle* player_vehicle) const;
  bool UseLegacyAutopilot() const;
  void InitializePlayerState();
  void SyncPlayerStateFromVehicle(const pubsub3d::Vehicle& player_vehicle);
  pubsub3d::Vehicle BuildLegacyAutopilotPlayer(std::uint64_t frame_id) const;
  pubsub3d::Vehicle BuildInputOnlyPlayer() const;
  pubsub3d::Vehicle BuildNpcVehicle(std::size_t vehicle_index,
                                    std::uint64_t frame_id) const;
  void PopulateStaticTrack(pubsub3d::Track* track) const;
  void PopulateOccupancyGrid(pubsub3d::OccupancyGrid* grid) const;
  void PopulateVehicles(pubsub3d::GameStateFrame* frame, std::uint64_t frame_id);
  void PopulateRoadObjects(pubsub3d::GameStateFrame* frame, std::uint64_t frame_id) const;

  const PublisherConfig& config_;
  SharedMemoryStateBuffer& shared_buffer_;
  InputCommandBus* input_command_bus_;
  std::size_t diagnostics_counter_;
  pubsub3d::Vehicle player_state_;
  bool player_state_initialized_;
  bool move_forward_active_;
  bool move_back_active_;
  bool turn_left_active_;
  bool turn_right_active_;
};
