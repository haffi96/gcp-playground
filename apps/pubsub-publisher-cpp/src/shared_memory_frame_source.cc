#include "shared_memory_frame_source.h"

#include "proto/game_state.pb.h"
#include "proto_to_scene_json.h"

#include <chrono>
#include <stdexcept>
#include <thread>

SharedMemoryFrameSource::SharedMemoryFrameSource(SharedMemoryStateBuffer& shared_buffer)
    : shared_buffer_(shared_buffer), last_sequence_(0) {}

std::string SharedMemoryFrameSource::NextPayload(std::uint64_t /*frame_id*/) {
  for (;;) {
    const auto snapshot = shared_buffer_.ReadLatest();
    if (snapshot.sequence == 0 || snapshot.sequence == last_sequence_) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      continue;
    }

    pubsub3d::GameStateFrame frame;
    if (!frame.ParseFromString(snapshot.serialized_state)) {
      throw std::runtime_error("Failed to parse game state protobuf from shared buffer");
    }

    last_sequence_ = snapshot.sequence;
    return ProtoFrameToSceneJson(frame);
  }
}
