#include "local_input_adapter.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>

namespace {
std::uint64_t EpochMillis() {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count());
}
}  // namespace

LocalInputAdapter::LocalInputAdapter(const PublisherConfig& config,
                                     InputCommandBus& command_bus)
    : config_(config), command_bus_(command_bus) {}

void LocalInputAdapter::Run(const std::atomic<bool>& running) {
  const std::size_t tick_hz = std::max<std::size_t>(config_.local_client_tick_hz, 1);
  const auto tick_interval = std::chrono::microseconds(1000000 / tick_hz);
  auto next_tick = std::chrono::steady_clock::now();
  std::uint64_t sequence = 0;
  std::size_t action_index = 0;
  static constexpr std::array<pubsub3d::InputAction, 5> kActions = {
      pubsub3d::MOVE_FORWARD, pubsub3d::TURN_LEFT, pubsub3d::MOVE_FORWARD,
      pubsub3d::TURN_RIGHT, pubsub3d::MOVE_BACK};

  while (running.load()) {
    pubsub3d::InputCommand command;
    command.set_schema_version("1.0.0");
    command.set_command_id("local-" + std::to_string(sequence));
    command.set_client_id("local-dev-client");
    command.set_scene_id(config_.scene_id);
    command.set_actor_id(config_.player_actor_id);
    command.set_action(kActions[action_index % kActions.size()]);
    command.set_timestamp_ms(EpochMillis());
    command.set_sequence(sequence);
    const bool accepted = command_bus_.Push(std::move(command));
    if (!accepted && (sequence % 120 == 0)) {
      std::cout << "local_input_drop reason=queue_full queue_depth="
                << command_bus_.Size() << std::endl;
    }

    ++sequence;
    ++action_index;
    next_tick += tick_interval;
    std::this_thread::sleep_until(next_tick);
  }
}
