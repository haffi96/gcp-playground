#include "gcp_pubsub_publisher.h"

#include <utility>

#include "google/cloud/pubsub/message.h"
#include "google/cloud/pubsub/publisher_options.h"

using google::cloud::future;
using google::cloud::pubsub::MakePublisherConnection;
using google::cloud::pubsub::MessageBuilder;
using google::cloud::pubsub::Publisher;
using google::cloud::pubsub::Topic;

GcpPubSubPublisher::GcpPubSubPublisher(const PublisherConfig& config)
    : publisher_([&config] {
        auto options = google::cloud::Options{}
                           .set<google::cloud::pubsub::MaxBatchMessagesOption>(
                               config.max_batch_messages)
                           .set<google::cloud::pubsub::MaxBatchBytesOption>(
                               config.max_batch_bytes)
                           .set<google::cloud::pubsub::MaxHoldTimeOption>(config.max_hold_time);

        return Publisher(MakePublisherConnection(
            Topic(config.project_id, config.topic_id), std::move(options)));
      }()) {}

std::future<bool> GcpPubSubPublisher::Publish(std::string payload) {
  auto publish_future =
      publisher_.Publish(MessageBuilder{}.SetData(std::move(payload)).Build());
  return std::async(std::launch::deferred, [f = std::move(publish_future)]() mutable {
    auto result = f.get();
    return result.ok();
  });
}
