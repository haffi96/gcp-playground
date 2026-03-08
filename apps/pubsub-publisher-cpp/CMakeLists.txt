cmake_minimum_required(VERSION 3.20)
project(pubsub_3d_publisher LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(CTest)

find_package(google_cloud_cpp_pubsub REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(Protobuf REQUIRED)

add_executable(pubsub_3d_publisher
  src/main.cc
  src/config.cc
  src/scene_payload_generator.cc
  src/gcp_pubsub_publisher.cc
  src/benchmark_runner.cc
  src/shared_memory_state_buffer.cc
  src/shared_memory_frame_source.cc
  src/proto_to_scene_json.cc
  src/game_state_manager.cc
  src/local_game_client.cc
  src/input_command_bus.cc
  src/local_input_adapter.cc
  src/remote_input_subscriber.cc
)

protobuf_generate(
  TARGET pubsub_3d_publisher
  LANGUAGE cpp
  PROTOS proto/input_command.proto proto/game_state.proto
)

target_include_directories(pubsub_3d_publisher PRIVATE include ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(pubsub_3d_publisher
  PRIVATE
    google-cloud-cpp::pubsub
    nlohmann_json::nlohmann_json
    protobuf::libprotobuf
)

if(BUILD_TESTING)
  add_executable(scene_adapter_invariant_test
    tests/scene_adapter_invariant_test.cc
    src/proto_to_scene_json.cc
  )
  protobuf_generate(
    TARGET scene_adapter_invariant_test
    LANGUAGE cpp
    PROTOS proto/input_command.proto proto/game_state.proto
  )
  target_include_directories(scene_adapter_invariant_test PRIVATE include ${CMAKE_CURRENT_BINARY_DIR})
  target_link_libraries(scene_adapter_invariant_test PRIVATE nlohmann_json::nlohmann_json protobuf::libprotobuf)

  add_executable(input_command_bus_test
    tests/input_command_bus_test.cc
    src/input_command_bus.cc
  )
  protobuf_generate(
    TARGET input_command_bus_test
    LANGUAGE cpp
    PROTOS proto/input_command.proto
  )
  target_include_directories(input_command_bus_test PRIVATE include ${CMAKE_CURRENT_BINARY_DIR})
  target_link_libraries(input_command_bus_test PRIVATE protobuf::libprotobuf)

  add_executable(game_state_manager_input_test
    tests/game_state_manager_input_test.cc
    src/game_state_manager.cc
    src/input_command_bus.cc
    src/shared_memory_state_buffer.cc
  )
  protobuf_generate(
    TARGET game_state_manager_input_test
    LANGUAGE cpp
    PROTOS proto/input_command.proto proto/game_state.proto
  )
  target_include_directories(game_state_manager_input_test PRIVATE include ${CMAKE_CURRENT_BINARY_DIR})
  target_link_libraries(game_state_manager_input_test PRIVATE protobuf::libprotobuf)

  add_test(NAME publisher_dry_run_synthetic
           COMMAND pubsub_3d_publisher)
  set_tests_properties(publisher_dry_run_synthetic PROPERTIES
    ENVIRONMENT "DRY_RUN=1;DRY_RUN_FRAMES=2;SCENE_SOURCE=synthetic")

  add_test(NAME publisher_dry_run_shared_memory
           COMMAND pubsub_3d_publisher)
  set_tests_properties(publisher_dry_run_shared_memory PROPERTIES
    ENVIRONMENT "DRY_RUN=1;DRY_RUN_FRAMES=2;SCENE_SOURCE=shared_memory;RUNTIME_MODE=dev;ENABLE_LOCAL_CLIENT=1;ENABLE_LOCAL_INPUT_SCRIPT=1;STATE_TICK_HZ=30;LOCAL_CLIENT_TICK_HZ=30")
  add_test(NAME scene_adapter_invariants COMMAND scene_adapter_invariant_test)
  add_test(NAME input_command_bus COMMAND input_command_bus_test)
  add_test(NAME game_state_manager_input COMMAND game_state_manager_input_test)
endif()
