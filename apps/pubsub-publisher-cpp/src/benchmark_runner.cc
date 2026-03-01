#include "benchmark_runner.h"

#include <algorithm>
#include <chrono>
#include <deque>
#include <future>
#include <iostream>
#include <utility>

BenchmarkRunner::BenchmarkRunner(PublisherConfig config, IPubSubPublisher& publisher,
                                 ScenePayloadGenerator& generator)
    : config_(std::move(config)), publisher_(publisher), generator_(generator) {}

int BenchmarkRunner::Run() {
  std::size_t sent = 0;
  std::size_t acked = 0;
  std::size_t failed = 0;
  std::size_t payload_bytes_total = 0;
  std::deque<std::future<bool>> inflight;

  const auto start = std::chrono::steady_clock::now();
  while (sent < config_.target_messages) {
    while (inflight.size() >= config_.inflight_limit) {
      const bool ok = inflight.front().get();
      inflight.pop_front();
      ok ? ++acked : ++failed;
    }

    auto payload = generator_.Generate(sent);
    payload_bytes_total += payload.size();
    inflight.push_back(publisher_.Publish(std::move(payload)));
    ++sent;
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
            << " messages_failed=" << failed << " elapsed_seconds=" << seconds
            << " msg_per_sec=" << messages_per_second << " mb_per_sec=" << mb_per_second
            << std::endl;

  return failed == 0 ? 0 : 1;
}
