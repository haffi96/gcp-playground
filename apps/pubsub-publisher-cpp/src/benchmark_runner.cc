#include "benchmark_runner.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <deque>
#include <future>
#include <iostream>
#include <thread>
#include <utility>

BenchmarkRunner::BenchmarkRunner(PublisherConfig config, IPubSubPublisher& publisher,
                                 IFrameSource& frame_source)
    : config_(std::move(config)),
      publisher_(publisher),
      frame_source_(frame_source) {}

int BenchmarkRunner::Run() {
  std::string publish_mode = config_.publish_mode;
  std::transform(
      publish_mode.begin(), publish_mode.end(), publish_mode.begin(),
      [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  const bool run_continuously = publish_mode == "continuous";
  const std::size_t frame_limit = std::max<std::size_t>(config_.frame_limit, 1);
  const std::size_t inflight_limit = std::max<std::size_t>(config_.inflight_limit, 1);
  const std::size_t tick_hz = std::max<std::size_t>(config_.tick_hz, 1);
  const auto tick_interval = std::chrono::microseconds(1000000 / tick_hz);

  std::size_t sent = 0;
  std::size_t acked = 0;
  std::size_t failed = 0;
  std::size_t payload_bytes_total = 0;
  std::deque<std::future<bool>> inflight;

  const auto start = std::chrono::steady_clock::now();
  auto next_tick = start;
  while (run_continuously || sent < frame_limit) {
    while (inflight.size() >= inflight_limit) {
      const bool ok = inflight.front().get();
      inflight.pop_front();
      ok ? ++acked : ++failed;
    }

    auto payload = frame_source_.NextPayload(sent);
    payload_bytes_total += payload.size();
    inflight.push_back(publisher_.Publish(std::move(payload)));
    ++sent;

    next_tick += tick_interval;
    std::this_thread::sleep_until(next_tick);
  }

  while (!inflight.empty()) {
    const bool ok = inflight.front().get();
    inflight.pop_front();
    ok ? ++acked : ++failed;
  }

  const auto end = std::chrono::steady_clock::now();
  const double seconds =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / 1000.0;
  const double messages_per_second = static_cast<double>(acked) / std::max(seconds, 0.001);
  const double mb_per_second =
      (static_cast<double>(payload_bytes_total) / (1024.0 * 1024.0)) / std::max(seconds, 0.001);

  std::cout << "publish_summary messages_sent=" << sent << " messages_acked=" << acked
            << " messages_failed=" << failed << " mode=" << publish_mode
            << " tick_hz=" << tick_hz << " elapsed_seconds=" << seconds
            << " msg_per_sec=" << messages_per_second << " mb_per_sec=" << mb_per_second
            << std::endl;

  return failed == 0 ? 0 : 1;
}
