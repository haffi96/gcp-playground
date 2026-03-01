cmake_minimum_required(VERSION 3.20)
project(pubsub_3d_publisher LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(google_cloud_cpp_pubsub REQUIRED)
find_package(nlohmann_json REQUIRED)

add_executable(pubsub_3d_publisher
  src/main.cc
  src/config.cc
  src/scene_payload_generator.cc
  src/gcp_pubsub_publisher.cc
  src/benchmark_runner.cc
)

target_include_directories(pubsub_3d_publisher PRIVATE include)
target_link_libraries(pubsub_3d_publisher
  PRIVATE
    google-cloud-cpp::pubsub
    nlohmann_json::nlohmann_json
)
