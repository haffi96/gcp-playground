#include "scene_payload_generator.h"

#include <chrono>
#include <cmath>
#include <string>

#include "nlohmann/json.hpp"

ScenePayloadGenerator::ScenePayloadGenerator(std::string scene_id,
                                             std::size_t entity_count)
    : scene_id_(std::move(scene_id)), entity_count_(entity_count) {}

std::string ScenePayloadGenerator::Generate(std::uint64_t frame_id) const {
  nlohmann::json payload;
  payload["schemaVersion"] = "1.0.0";
  payload["timestamp"] =
      std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::system_clock::now().time_since_epoch())
                         .count());
  payload["sceneId"] = scene_id_;
  payload["frameId"] = frame_id;
  payload["environment"] = {
      {"style", "retro-cyberpunk-western-duel"},
      {"entities", nlohmann::json::array()},
  };

  auto &entities = payload["environment"]["entities"];
  auto &entities_array = entities.get_ref<nlohmann::json::array_t &>();
  entities_array.reserve(entity_count_);

  for (std::size_t i = 0; i < entity_count_; ++i) {
    const double t = static_cast<double>(frame_id + i);
    const double x = std::sin(t * 0.003) * 45.0;
    const double z = std::cos(t * 0.005) * 45.0;
    const double y = (i % 5 == 0) ? 1.0 : 0.0;

    nlohmann::json entity;
    entity["id"] = "entity-" + std::to_string(i);
    entity["type"] = (i % 8 == 0) ? "duelist" : "set-piece";
    entity["position"] = {{"x", x}, {"y", y}, {"z", z}};
    entity["rotation"] = {
        {"x", 0.0}, {"y", std::fmod(t * 0.02, 6.28)}, {"z", 0.0}};
    entity["scale"] = {{"x", 1.0}, {"y", 1.0}, {"z", 1.0}};
    entity["material"] = {
        {"color", (i % 3 == 0) ? "#0ea5e9" : "#f97316"},
        {"emissive", (i % 11 == 0) ? "#22d3ee" : "#111827"},
        {"roughness", 0.55},
        {"metalness", 0.7},
    };
    entities.push_back(std::move(entity));
  }

  return payload.dump();
}
