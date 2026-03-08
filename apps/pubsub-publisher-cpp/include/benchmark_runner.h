#pragma once

#include "config.h"
#include "frame_source.h"
#include "pubsub_publisher.h"

class BenchmarkRunner {
 public:
  BenchmarkRunner(PublisherConfig config, IPubSubPublisher& publisher,
                  IFrameSource& frame_source);
  int Run();

 private:
  PublisherConfig config_;
  IPubSubPublisher& publisher_;
  IFrameSource& frame_source_;
};
