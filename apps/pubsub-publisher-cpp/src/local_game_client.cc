#include "local_game_client.h"

#include "proto/game_state.pb.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <thread>

LocalGameClient::LocalGameClient(const PublisherConfig& config,
                                 SharedMemoryStateBuffer& shared_buffer)
    : config_(config), shared_buffer_(shared_buffer) {}

void LocalGameClient::Run(const std::atomic<bool>& running) {
  const std::size_t tick_hz = std::max<std::size_t>(config_.local_client_tick_hz, 1);
  const auto tick_interval = std::chrono::microseconds(1000000 / tick_hz);
  auto next_tick = std::chrono::steady_clock::now();
  std::uint64_t last_sequence = 0;
  std::size_t local_tick = 0;
  auto loop_start = std::chrono::steady_clock::now();

  while (running.load()) {
    const auto snapshot = shared_buffer_.ReadLatest();
    if (snapshot.sequence != 0 && snapshot.sequence != last_sequence) {
      pubsub3d::GameStateFrame frame;
      if (frame.ParseFromString(snapshot.serialized_state)) {
        const std::size_t vehicles = static_cast<std::size_t>(frame.vehicles_size());
        const std::size_t objects = static_cast<std::size_t>(frame.road_objects_size());
        const std::int64_t frame_latency_ms =
            static_cast<std::int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                          std::chrono::system_clock::now().time_since_epoch())
                                          .count()) -
            std::stoll(frame.timestamp());
        const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                    std::chrono::steady_clock::now() - loop_start)
                                    .count();
        const double observed_hz = elapsed_ms > 0
                                       ? static_cast<double>(local_tick + 1) /
                                             (static_cast<double>(elapsed_ms) / 1000.0)
                                       : 0.0;
        std::cout << "local_client_update frame_id=" << frame.frame_id()
                  << " vehicles=" << vehicles << " road_objects=" << objects
                  << " sequence=" << snapshot.sequence
                  << " sequence_lag=" << (snapshot.sequence - frame.frame_id())
                  << " frame_latency_ms=" << frame_latency_ms
                  << " observed_tick_hz=" << observed_hz << std::endl;
      }
      last_sequence = snapshot.sequence;
    }

    ++local_tick;
    next_tick += tick_interval;
    std::this_thread::sleep_until(next_tick);
  }
}
