#pragma once

#include "config.h"
#include "pubsub_publisher.h"
#include "scene_payload_generator.h"

class BenchmarkRunner {
 public:
  BenchmarkRunner(PublisherConfig config, IPubSubPublisher& publisher,
                  ScenePayloadGenerator& generator);
  int Run();

 private:
  PublisherConfig config_;
  IPubSubPublisher& publisher_;
  ScenePayloadGenerator& generator_;
};
