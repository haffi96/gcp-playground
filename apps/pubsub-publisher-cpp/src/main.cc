#include "benchmark_runner.h"
#include "config.h"
#include "game_state_manager.h"
#include "gcp_pubsub_publisher.h"
#include "input_command_bus.h"
#include "local_input_adapter.h"
#include "local_game_client.h"
#include "remote_input_subscriber.h"
#include "scene_payload_generator.h"
#include "shared_memory_frame_source.h"
#include "shared_memory_state_buffer.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <exception>
#include <iostream>
#include <memory>
#include <thread>

namespace {
std::string ToLower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return value;
}
}  // namespace

int main() {
  try {
    auto config = LoadConfigFromEnv();
    const std::string scene_source = ToLower(config.scene_source);
    const std::string runtime_mode = ToLower(config.runtime_mode);
    const bool run_local_client = config.enable_local_client || runtime_mode == "dev";
    const bool run_local_script = config.enable_local_input_script;

    std::cerr << "startup_config runtime_mode=" << runtime_mode
              << " scene_source=" << scene_source
              << " enable_remote_input=" << (config.enable_remote_input ? "true" : "false")
              << " enable_local_input_script="
              << (run_local_script ? "true" : "false")
              << " player_actor_id=" << config.player_actor_id << std::endl;

    std::unique_ptr<IFrameSource> frame_source;
    std::unique_ptr<SharedMemoryStateBuffer> shared_buffer;
    std::unique_ptr<InputCommandBus> input_command_bus;
    std::unique_ptr<GameStateManager> game_state_manager;
    std::unique_ptr<LocalGameClient> local_game_client;
    std::unique_ptr<LocalInputAdapter> local_input_adapter;
    std::unique_ptr<RemoteInputSubscriber> remote_input_subscriber;
    std::unique_ptr<std::thread> manager_thread;
    std::unique_ptr<std::thread> local_client_thread;
    std::unique_ptr<std::thread> local_input_thread;
    std::unique_ptr<std::thread> remote_input_thread;
    std::atomic<bool> running{true};

    if (scene_source == "shared_memory") {
      shared_buffer = std::make_unique<SharedMemoryStateBuffer>();
      input_command_bus =
          std::make_unique<InputCommandBus>(config.input_queue_capacity);
      game_state_manager =
          std::make_unique<GameStateManager>(config, *shared_buffer,
                                             input_command_bus.get());
      manager_thread = std::make_unique<std::thread>(
          [&]() { game_state_manager->Run(running); });

      if (run_local_script) {
        local_input_adapter =
            std::make_unique<LocalInputAdapter>(config, *input_command_bus);
        local_input_thread = std::make_unique<std::thread>(
            [&]() { local_input_adapter->Run(running); });
      }

      if (config.enable_remote_input) {
        remote_input_subscriber =
            std::make_unique<RemoteInputSubscriber>(config, *input_command_bus);
        remote_input_thread = std::make_unique<std::thread>(
            [&]() { remote_input_subscriber->Run(running); });
      }

      if (run_local_client) {
        local_game_client =
            std::make_unique<LocalGameClient>(config, *shared_buffer);
        local_client_thread = std::make_unique<std::thread>(
            [&]() { local_game_client->Run(running); });
      }
      frame_source = std::make_unique<SharedMemoryFrameSource>(*shared_buffer);
    } else {
      ScenePayloadGenerator generator(
          config.scene_id, config.lane_count, config.lane_waypoint_count,
          config.vehicle_count, config.road_length_meters,
          config.road_object_count, config.road_object_spread_meters,
          config.road_object_lane_occupancy_percent, config.occupancy_grid_width,
          config.occupancy_grid_height);
      frame_source = std::make_unique<SyntheticFrameSource>(std::move(generator));
    }

    if (config.dry_run) {
      for (std::size_t i = 0; i < config.dry_run_frames; ++i) {
        std::cout << frame_source->NextPayload(i) << std::endl;
      }
      running.store(false);
      if (local_client_thread && local_client_thread->joinable()) {
        local_client_thread->join();
      }
      if (local_input_thread && local_input_thread->joinable()) {
        local_input_thread->join();
      }
      if (remote_input_thread && remote_input_thread->joinable()) {
        remote_input_thread->join();
      }
      if (manager_thread && manager_thread->joinable()) {
        manager_thread->join();
      }
      return 0;
    }

    GcpPubSubPublisher publisher(config);
    BenchmarkRunner runner(config, publisher, *frame_source);
    const int exit_code = runner.Run();
    running.store(false);
    if (local_client_thread && local_client_thread->joinable()) {
      local_client_thread->join();
    }
    if (local_input_thread && local_input_thread->joinable()) {
      local_input_thread->join();
    }
    if (remote_input_thread && remote_input_thread->joinable()) {
      remote_input_thread->join();
    }
    if (manager_thread && manager_thread->joinable()) {
      manager_thread->join();
    }
    return exit_code;
  } catch (const std::exception& ex) {
    std::cerr << "publisher_failed error=" << ex.what() << std::endl;
    return 1;
  }
}
