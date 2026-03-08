#include "proto/game_state.pb.h"
#include "proto/input_command.pb.h"
#include "proto_to_scene_json.h"

#include "nlohmann/json.hpp"

#include <iostream>
#include <string>

namespace {
bool Check(bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "scene_adapter_invariant_test_failed reason=" << message
              << std::endl;
    return false;
  }
  return true;
}
}  // namespace

int main() {
  pubsub3d::GameStateFrame frame;
  frame.set_schema_version("1.0.0");
  frame.set_timestamp("1700000000000");
  frame.set_scene_id("urban-night-circuit");
  frame.set_frame_id(42);
  frame.set_server_tick(42);

  auto* lane = frame.mutable_track()->add_lanes();
  frame.mutable_track()->set_name("urban-night-circuit");
  frame.mutable_track()->set_length_meters(1800.0);
  frame.mutable_track()->set_width_meters(24.0);
  lane->set_id("lane-0");
  lane->set_index(0);
  lane->set_center_offset_meters(0.0);
  lane->set_width_meters(3.9);
  lane->set_speed_limit_kph(120.0);
  for (int i = 0; i < 4; ++i) {
    auto* wp = lane->add_waypoints();
    wp->set_x(0.0);
    wp->set_y(0.0);
    wp->set_z(static_cast<double>(i) * 2.0);
  }

  auto* grid = frame.mutable_occupancy_grid();
  grid->mutable_origin()->set_x(-2.0);
  grid->mutable_origin()->set_y(0.1);
  grid->mutable_origin()->set_z(-2.0);
  grid->set_cell_size_meters(2.0);
  grid->set_width(2);
  grid->set_height(2);
  grid->add_cells(1);
  grid->add_cells(0);
  grid->add_cells(0);
  grid->add_cells(2);

  auto* vehicle = frame.add_vehicles();
  vehicle->set_id("car-0");
  vehicle->set_lane_id("lane-0");
  vehicle->set_lap(0);
  vehicle->set_progress(0.2);
  vehicle->set_speed_mps(22.5);
  vehicle->set_heading_rad(0.0);
  vehicle->mutable_position()->set_x(0.0);
  vehicle->mutable_position()->set_y(0.6);
  vehicle->mutable_position()->set_z(12.0);
  vehicle->mutable_dimensions()->set_length(4.4);
  vehicle->mutable_dimensions()->set_width(2.0);
  vehicle->mutable_dimensions()->set_height(1.4);
  vehicle->set_color("#f43f5e");

  auto* object = frame.add_road_objects();
  object->set_id("obj-0");
  object->set_type("cone");
  object->mutable_position()->set_x(2.0);
  object->mutable_position()->set_y(0.5);
  object->mutable_position()->set_z(8.0);
  object->mutable_rotation()->set_x(0.0);
  object->mutable_rotation()->set_y(0.3);
  object->mutable_rotation()->set_z(0.0);
  object->mutable_size()->set_length(0.8);
  object->mutable_size()->set_width(0.8);
  object->mutable_size()->set_height(1.0);
  object->set_color("#f59e0b");

  const auto json_payload = ProtoFrameToSceneJson(frame);
  const auto parsed = nlohmann::json::parse(json_payload);

  bool ok = true;
  ok &= Check(parsed["schemaVersion"].is_string(), "schemaVersion must be string");
  ok &= Check(parsed["timestamp"].is_string(), "timestamp must be string");
  ok &= Check(parsed["sceneId"].is_string(), "sceneId must be string");
  ok &= Check(parsed["track"]["lanes"].is_array(), "track.lanes must be array");
  ok &= Check(parsed["track"]["lanes"].size() >= 1, "at least one lane required");
  ok &= Check(parsed["track"]["lanes"][0]["waypoints"].size() >= 4,
              "lane waypoints must be >= 4");
  ok &= Check(parsed["occupancyGrid"]["cells"].size() ==
                  parsed["occupancyGrid"]["width"].get<std::size_t>() *
                      parsed["occupancyGrid"]["height"].get<std::size_t>(),
              "occupancy grid cardinality mismatch");
  ok &= Check(parsed["vehicles"].is_array() && !parsed["vehicles"].empty(),
              "vehicles must be non-empty");
  ok &= Check(parsed["roadObjects"].is_array(), "roadObjects must be array");
  return ok ? 0 : 1;
}
