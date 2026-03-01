#pragma once

#include "config.h"
#include "pubsub_publisher.h"

#include "google/cloud/pubsub/publisher.h"

class GcpPubSubPublisher : public IPubSubPublisher {
 public:
  explicit GcpPubSubPublisher(const PublisherConfig& config);
  std::future<bool> Publish(std::string payload) override;

 private:
  google::cloud::pubsub::Publisher publisher_;
};
