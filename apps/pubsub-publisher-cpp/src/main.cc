#include "benchmark_runner.h"
#include "config.h"
#include "gcp_pubsub_publisher.h"
#include "scene_payload_generator.h"

#include <exception>
#include <iostream>

int main() {
  try {
    auto config = LoadConfigFromEnv();
    ScenePayloadGenerator generator(
        config.scene_id, config.lane_count, config.lane_waypoint_count,
        config.vehicle_count, config.road_length_meters,
        config.road_object_count, config.road_object_spread_meters,
        config.road_object_lane_occupancy_percent, config.occupancy_grid_width,
        config.occupancy_grid_height);
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
