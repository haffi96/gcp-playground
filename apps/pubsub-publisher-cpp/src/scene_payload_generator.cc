#include "scene_payload_generator.h"

#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "nlohmann/json.hpp"

namespace {
constexpr double kPi = 3.14159265358979323846;

double PositiveOr(std::size_t value, double fallback) {
  return value == 0 ? fallback : static_cast<double>(value);
}

std::uint64_t Hash64(std::uint64_t value) {
  value ^= value >> 33U;
  value *= 0xff51afd7ed558ccdULL;
  value ^= value >> 33U;
  value *= 0xc4ceb9fe1a85ec53ULL;
  value ^= value >> 33U;
  return value;
}

double UnitFloat(std::uint64_t value) {
  return static_cast<double>(Hash64(value) & 0xFFFFFFULL) /
         static_cast<double>(0xFFFFFFULL);
}

void IncrementCell(nlohmann::json::array_t &cell_array, std::size_t grid_width,
                   std::size_t grid_height, double grid_origin_x,
                   double grid_origin_z, double cell_size, double x, double z) {
  const int cell_x =
      static_cast<int>(std::floor((x - grid_origin_x) / cell_size));
  const int cell_z =
      static_cast<int>(std::floor((z - grid_origin_z) / cell_size));
  if (cell_x >= 0 && cell_x < static_cast<int>(grid_width) && cell_z >= 0 &&
      cell_z < static_cast<int>(grid_height)) {
    const std::size_t idx = static_cast<std::size_t>(cell_z) * grid_width +
                            static_cast<std::size_t>(cell_x);
    const int current = cell_array[idx].get<int>();
    cell_array[idx] = current + 1;
  }
}
} // namespace

ScenePayloadGenerator::ScenePayloadGenerator(
    std::string scene_id, std::size_t lane_count,
    std::size_t lane_waypoint_count, std::size_t vehicle_count,
    std::size_t road_length_meters, std::size_t road_object_count,
    std::size_t road_object_spread_meters,
    std::size_t road_object_lane_occupancy_percent,
    std::size_t occupancy_grid_width, std::size_t occupancy_grid_height)
    : scene_id_(std::move(scene_id)), lane_count_(lane_count),
      lane_waypoint_count_(lane_waypoint_count), vehicle_count_(vehicle_count),
      road_length_meters_(road_length_meters),
      road_object_count_(road_object_count),
      road_object_spread_meters_(road_object_spread_meters),
      road_object_lane_occupancy_percent_(road_object_lane_occupancy_percent),
      occupancy_grid_width_(occupancy_grid_width),
      occupancy_grid_height_(occupancy_grid_height) {}

std::string ScenePayloadGenerator::Generate(std::uint64_t frame_id) const {
  nlohmann::json payload;
  payload["schemaVersion"] = "1.0.0";
  payload["timestamp"] =
      std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::system_clock::now().time_since_epoch())
                         .count());
  payload["sceneId"] = scene_id_;
  payload["frameId"] = frame_id;

  const std::size_t lane_count = lane_count_ == 0 ? 1 : lane_count_;
  const std::size_t lane_waypoints =
      lane_waypoint_count_ < 10 ? 10 : lane_waypoint_count_;
  const std::size_t vehicle_count = vehicle_count_ == 0 ? 1 : vehicle_count_;
  const std::size_t road_object_count = road_object_count_;
  const std::size_t road_length_meters =
      road_length_meters_ < 500 ? 500 : road_length_meters_;
  const std::size_t road_object_spread =
      road_object_spread_meters_ < 200 ? 200 : road_object_spread_meters_;
  const std::size_t object_lane_occupancy_percent =
      road_object_lane_occupancy_percent_ > 100
          ? 100
          : road_object_lane_occupancy_percent_;
  const std::size_t grid_width =
      occupancy_grid_width_ == 0 ? 1 : occupancy_grid_width_;
  const std::size_t grid_height =
      occupancy_grid_height_ == 0 ? 1 : occupancy_grid_height_;

  const double lane_width_meters = 3.9;
  const double shoulder_width_meters = 2.4;
  const double road_width_meters =
      lane_width_meters * static_cast<double>(lane_count) +
      shoulder_width_meters * 2.0;
  const double road_length = static_cast<double>(road_length_meters);
  const double half_road_length = road_length * 0.5;

  payload["track"] = {
      {"name", "urban-night-circuit"},
      {"lengthMeters", road_length},
      {"widthMeters", road_width_meters},
      {"lanes", nlohmann::json::array()},
  };

  auto &lanes = payload["track"]["lanes"];
  auto &lanes_array = lanes.get_ref<nlohmann::json::array_t &>();
  lanes_array.reserve(lane_count);

  for (std::size_t lane_idx = 0; lane_idx < lane_count; ++lane_idx) {
    const double lane_center_x = (static_cast<double>(lane_idx) + 0.5 -
                                  (static_cast<double>(lane_count) * 0.5)) *
                                 lane_width_meters;
    nlohmann::json lane = {
        {"id", "lane-" + std::to_string(lane_idx)},
        {"index", lane_idx},
        {"centerOffsetMeters", lane_center_x},
        {"widthMeters", lane_width_meters},
        {"speedLimitKph", 120.0 - static_cast<double>(lane_idx * 4)},
        {"waypoints", nlohmann::json::array()},
    };

    auto &waypoints = lane["waypoints"];
    auto &waypoint_array = waypoints.get_ref<nlohmann::json::array_t &>();
    waypoint_array.reserve(lane_waypoints);
    const std::size_t one_way_count = lane_waypoints / 2;
    for (std::size_t wp = 0; wp < one_way_count; ++wp) {
      const double t =
          static_cast<double>(wp) / PositiveOr(one_way_count - 1, 1.0);
      waypoint_array.push_back({
          {"x", lane_center_x},
          {"y", 0.0},
          {"z", (-half_road_length) + (road_length * t)},
      });
    }
    for (std::size_t wp = 0; wp < one_way_count; ++wp) {
      const double t =
          static_cast<double>(wp) / PositiveOr(one_way_count - 1, 1.0);
      waypoint_array.push_back({
          {"x", lane_center_x + 0.05},
          {"y", 0.0},
          {"z", half_road_length - (road_length * t)},
      });
    }
    lanes.push_back(std::move(lane));
  }

  const double cell_size_meters =
      std::max(2.0, road_width_meters /
                        std::max(2.0, static_cast<double>(lane_count * 2)));
  const double grid_span_x = static_cast<double>(grid_width) * cell_size_meters;
  const double grid_span_z =
      static_cast<double>(grid_height) * cell_size_meters;
  const double grid_origin_x = -grid_span_x * 0.5;
  const double grid_origin_z = -grid_span_z * 0.5;

  payload["occupancyGrid"] = {
      {"origin", {{"x", grid_origin_x}, {"y", 0.1}, {"z", grid_origin_z}}},
      {"cellSizeMeters", cell_size_meters},
      {"width", grid_width},
      {"height", grid_height},
      {"cells", nlohmann::json::array()},
  };

  auto &cells = payload["occupancyGrid"]["cells"];
  auto &cell_array = cells.get_ref<nlohmann::json::array_t &>();
  cell_array.assign(grid_width * grid_height, 0);

  payload["vehicles"] = nlohmann::json::array();
  auto &vehicles = payload["vehicles"];
  auto &vehicle_array = vehicles.get_ref<nlohmann::json::array_t &>();
  vehicle_array.reserve(vehicle_count);

  payload["roadObjects"] = nlohmann::json::array();
  auto &road_objects = payload["roadObjects"];
  auto &object_array = road_objects.get_ref<nlohmann::json::array_t &>();
  object_array.reserve(road_object_count);

  const std::vector<std::string> palette = {"#f43f5e", "#22d3ee", "#f59e0b",
                                            "#84cc16", "#e879f9", "#60a5fa",
                                            "#fb7185", "#34d399"};
  const std::vector<std::string> object_types = {"cone", "barrel", "sign"};
  const std::vector<std::string> object_palette = {
      "#fb7185", "#f59e0b", "#facc15", "#60a5fa", "#34d399"};

  const double seconds = static_cast<double>(frame_id) * 0.1;
  const double focus_speed_mps = 44.0;
  const double focus_distance = seconds * focus_speed_mps;
  const double half_object_spread =
      static_cast<double>(road_object_spread) * 0.5;

  for (std::size_t obj_idx = 0; obj_idx < road_object_count; ++obj_idx) {
    const double lane_roll = UnitFloat(obj_idx * 17ULL + 5ULL) * 100.0;
    const bool on_lane =
        lane_roll < static_cast<double>(object_lane_occupancy_percent);
    const std::size_t lane_idx =
        static_cast<std::size_t>(UnitFloat(obj_idx * 29ULL + 11ULL) *
                                 static_cast<double>(lane_count)) %
        lane_count;
    const double lane_center_x = (static_cast<double>(lane_idx) + 0.5 -
                                  (static_cast<double>(lane_count) * 0.5)) *
                                 lane_width_meters;
    const double side_sign =
        UnitFloat(obj_idx * 37ULL + 13ULL) > 0.5 ? 1.0 : -1.0;
    const double roadside_x =
        side_sign * (road_width_meters * 0.5 + 2.0 +
                     UnitFloat(obj_idx * 41ULL + 23ULL) * 6.0);
    const double local_jitter_x =
        (UnitFloat(obj_idx * 59ULL + 31ULL) - 0.5) * 0.6;
    const double x = on_lane ? (lane_center_x + local_jitter_x) : roadside_x;

    const double object_anchor =
        UnitFloat(obj_idx * 71ULL + 7ULL) * road_length;
    double z =
        std::fmod(object_anchor - focus_distance + road_length, road_length) -
        half_road_length;
    z = std::fmod(z + half_object_spread,
                  static_cast<double>(road_object_spread)) -
        half_object_spread;

    const std::size_t type_idx =
        static_cast<std::size_t>(UnitFloat(obj_idx * 73ULL + 47ULL) *
                                 static_cast<double>(object_types.size())) %
        object_types.size();
    const std::string object_type = object_types[type_idx];
    double length = 0.8;
    double width = 0.8;
    double height = 1.0;
    if (object_type == "barrel") {
      length = 0.9;
      width = 0.9;
      height = 1.15;
    } else if (object_type == "sign") {
      length = 0.45;
      width = 1.4;
      height = 1.9;
    }

    object_array.push_back({
        {"id", "obj-" + std::to_string(obj_idx)},
        {"type", object_type},
        {"position", {{"x", x}, {"y", height * 0.5}, {"z", z}}},
        {"rotation",
         {{"x", 0.0},
          {"y", UnitFloat(obj_idx * 97ULL + 53ULL) * (2.0 * kPi)},
          {"z", 0.0}}},
        {"size", {{"length", length}, {"width", width}, {"height", height}}},
        {"color", object_palette[obj_idx % object_palette.size()]},
    });

    IncrementCell(cell_array, grid_width, grid_height, grid_origin_x,
                  grid_origin_z, cell_size_meters, x, z);
  }

  for (std::size_t i = 0; i < vehicle_count; ++i) {
    const std::size_t lane_idx = i == 0 ? (lane_count / 2) : (i % lane_count);
    const double lane_center_x = (static_cast<double>(lane_idx) + 0.5 -
                                  (static_cast<double>(lane_count) * 0.5)) *
                                 lane_width_meters;
    const double lateral_wobble =
        std::sin(seconds * 0.7 + static_cast<double>(i) * 0.35) * 0.28;
    const double x = lane_center_x + lateral_wobble;

    const double base_speed =
        i == 0 ? 44.0 : (17.0 + static_cast<double>(i % 8) * 1.35);
    const double travelled =
        seconds * base_speed + (i == 0 ? 0.0 : static_cast<double>(i) * 16.0);
    const std::uint64_t lap = static_cast<std::uint64_t>(
        travelled / PositiveOr(road_length_meters, 1.0));
    const double progress =
        std::fmod(travelled / PositiveOr(road_length_meters, 1.0), 1.0);
    const double z = std::fmod(travelled, road_length) - half_road_length;
    const double heading = 0.0;

    IncrementCell(cell_array, grid_width, grid_height, grid_origin_x,
                  grid_origin_z, cell_size_meters, x, z);

    vehicle_array.push_back({
        {"id", "car-" + std::to_string(i)},
        {"laneId", "lane-" + std::to_string(lane_idx)},
        {"lap", lap},
        {"progress", progress},
        {"speedMps", base_speed},
        {"headingRad", heading},
        {"position", {{"x", x}, {"y", 0.6}, {"z", z}}},
        {"dimensions", {{"length", 4.4}, {"width", 2.0}, {"height", 1.4}}},
        {"color", palette[i % palette.size()]},
    });
  }

  return payload.dump();
}

SyntheticFrameSource::SyntheticFrameSource(ScenePayloadGenerator generator)
    : generator_(std::move(generator)) {}

std::string SyntheticFrameSource::NextPayload(std::uint64_t frame_id) {
  return generator_.Generate(frame_id);
}
