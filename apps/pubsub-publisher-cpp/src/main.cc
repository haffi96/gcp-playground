#include "benchmark_runner.h"
#include "config.h"
#include "gcp_pubsub_publisher.h"
#include "scene_payload_generator.h"

#include <exception>
#include <iostream>

int main() {
  try {
    auto config = LoadConfigFromEnv();
    ScenePayloadGenerator generator(config.scene_id, config.entity_count);
    if (config.dry_run) {
      for (std::size_t i = 0; i < config.dry_run_frames; ++i) {
        std::cout << generator.Generate(i) << std::endl;
      }
      return 0;
    }
    GcpPubSubPublisher publisher(config);
    BenchmarkRunner runner(config, publisher, generator);
    return runner.Run();
  } catch (const std::exception& ex) {
    std::cerr << "publisher_failed error=" << ex.what() << std::endl;
    return 1;
  }
}
