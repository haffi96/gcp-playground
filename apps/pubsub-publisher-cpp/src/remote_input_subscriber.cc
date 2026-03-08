#include "remote_input_subscriber.h"

#include "google/cloud/pubsub/subscriber.h"
#include "nlohmann/json.hpp"
#include "proto/input_command.pb.h"

#include <chrono>
#include <iostream>
#include <thread>

using google::cloud::pubsub::AckHandler;
using google::cloud::pubsub::MakeSubscriberConnection;
using google::cloud::pubsub::Message;
using google::cloud::pubsub::Subscriber;
using google::cloud::pubsub::Subscription;

RemoteInputSubscriber::RemoteInputSubscriber(const PublisherConfig& config,
                                             InputCommandBus& command_bus)
    : config_(config), command_bus_(command_bus) {}

void RemoteInputSubscriber::Run(const std::atomic<bool>& running) {
  if (!config_.enable_remote_input) {
    return;
  }

  Subscriber subscriber(MakeSubscriberConnection(
      Subscription(config_.project_id, config_.input_subscription_id)));

  auto session = subscriber.Subscribe(
      [this](Message const& message, AckHandler handler) {
        const std::string raw(message.data().begin(), message.data().end());
        nlohmann::json payload;
        try {
          payload = nlohmann::json::parse(raw);
        } catch (...) {
          std::move(handler).ack();
          return;
        }

        pubsub3d::InputCommand command;
        if (!payload.contains("schemaVersion") || !payload.contains("clientId") ||
            !payload.contains("sceneId") || !payload.contains("actorId") ||
            !payload.contains("action") || !payload.contains("timestampMs") ||
            !payload.contains("sequence")) {
          std::move(handler).ack();
          return;
        }
        command.set_schema_version(payload.value("schemaVersion", "1.0.0"));
        command.set_command_id(payload.value("commandId", ""));
        command.set_client_id(payload.value("clientId", ""));
        command.set_scene_id(payload.value("sceneId", ""));
        command.set_actor_id(payload.value("actorId", ""));
        command.set_timestamp_ms(payload.value("timestampMs", 0ULL));
        command.set_sequence(payload.value("sequence", 0ULL));
        const std::string action = payload.value("action", "");
        if (action == "MOVE_FORWARD") {
          command.set_action(pubsub3d::MOVE_FORWARD);
        } else if (action == "MOVE_BACK") {
          command.set_action(pubsub3d::MOVE_BACK);
        } else if (action == "TURN_LEFT") {
          command.set_action(pubsub3d::TURN_LEFT);
        } else if (action == "TURN_RIGHT") {
          command.set_action(pubsub3d::TURN_RIGHT);
        } else if (action == "STOP") {
          command.set_action(pubsub3d::STOP);
        } else {
          command.set_action(pubsub3d::INPUT_ACTION_UNSPECIFIED);
        }

        if (IsStale(command) || IsOutOfOrder(command)) {
          std::move(handler).ack();
          return;
        }
        if (!command_bus_.Push(std::move(command))) {
          std::cout << "remote_input_drop reason=queue_full queue_depth="
                    << command_bus_.Size() << std::endl;
        }
        std::move(handler).ack();
      });

  while (running.load()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }
  session.cancel();
  auto status = session.get();
  if (!status.ok()) {
    std::cerr << "remote_input_subscriber_stopped error=" << status.message()
              << std::endl;
  }
}

bool RemoteInputSubscriber::IsStale(const pubsub3d::InputCommand& command) const {
  const std::uint64_t now_ms = EpochMillis();
  if (command.timestamp_ms() > now_ms) {
    return true;
  }
  return (now_ms - command.timestamp_ms()) > config_.input_stale_ms;
}

bool RemoteInputSubscriber::IsOutOfOrder(const pubsub3d::InputCommand& command) {
  auto& last = last_sequence_by_client_[command.client_id()];
  if (last != 0 && command.sequence() <= last) {
    return true;
  }
  last = command.sequence();
  return false;
}

std::uint64_t RemoteInputSubscriber::EpochMillis() {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count());
}
