#include "proto_to_scene_json.h"

#include "nlohmann/json.hpp"

#include <cstddef>

std::string ProtoFrameToSceneJson(const pubsub3d::GameStateFrame& frame) {
  nlohmann::json payload;
  payload["schemaVersion"] = frame.schema_version();
  payload["timestamp"] = frame.timestamp();
  payload["sceneId"] = frame.scene_id();
  payload["frameId"] = frame.frame_id();

  payload["track"] = {
      {"name", frame.track().name()},
      {"lengthMeters", frame.track().length_meters()},
      {"widthMeters", frame.track().width_meters()},
      {"lanes", nlohmann::json::array()},
  };

  for (const auto& lane : frame.track().lanes()) {
    nlohmann::json lane_json = {
        {"id", lane.id()},
        {"index", lane.index()},
        {"centerOffsetMeters", lane.center_offset_meters()},
        {"widthMeters", lane.width_meters()},
        {"speedLimitKph", lane.speed_limit_kph()},
        {"waypoints", nlohmann::json::array()},
    };
    for (const auto& wp : lane.waypoints()) {
      lane_json["waypoints"].push_back({
          {"x", wp.x()},
          {"y", wp.y()},
          {"z", wp.z()},
      });
    }
    payload["track"]["lanes"].push_back(std::move(lane_json));
  }

  payload["occupancyGrid"] = {
      {"origin",
       {{"x", frame.occupancy_grid().origin().x()},
        {"y", frame.occupancy_grid().origin().y()},
        {"z", frame.occupancy_grid().origin().z()}}},
      {"cellSizeMeters", frame.occupancy_grid().cell_size_meters()},
      {"width", frame.occupancy_grid().width()},
      {"height", frame.occupancy_grid().height()},
      {"cells", nlohmann::json::array()},
  };

  for (std::size_t i = 0; i < static_cast<std::size_t>(frame.occupancy_grid().cells_size());
       ++i) {
    payload["occupancyGrid"]["cells"].push_back(frame.occupancy_grid().cells(
        static_cast<int>(i)));
  }

  payload["vehicles"] = nlohmann::json::array();
  for (const auto& vehicle : frame.vehicles()) {
    payload["vehicles"].push_back({
        {"id", vehicle.id()},
        {"laneId", vehicle.lane_id()},
        {"lap", vehicle.lap()},
        {"progress", vehicle.progress()},
        {"speedMps", vehicle.speed_mps()},
        {"headingRad", vehicle.heading_rad()},
        {"position",
         {{"x", vehicle.position().x()},
          {"y", vehicle.position().y()},
          {"z", vehicle.position().z()}}},
        {"dimensions",
         {{"length", vehicle.dimensions().length()},
          {"width", vehicle.dimensions().width()},
          {"height", vehicle.dimensions().height()}}},
        {"color", vehicle.color()},
    });
  }

  payload["roadObjects"] = nlohmann::json::array();
  for (const auto& object : frame.road_objects()) {
    payload["roadObjects"].push_back({
        {"id", object.id()},
        {"type", object.type()},
        {"position",
         {{"x", object.position().x()},
          {"y", object.position().y()},
          {"z", object.position().z()}}},
        {"rotation",
         {{"x", object.rotation().x()},
          {"y", object.rotation().y()},
          {"z", object.rotation().z()}}},
        {"size",
         {{"length", object.size().length()},
          {"width", object.size().width()},
          {"height", object.size().height()}}},
        {"color", object.color()},
    });
  }

  return payload.dump();
}
