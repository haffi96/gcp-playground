#include "input_command_bus.h"

#include "proto/input_command.pb.h"

#include <iostream>
#include <string>

namespace {
bool Check(bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "input_command_bus_test_failed reason=" << message << std::endl;
    return false;
  }
  return true;
}

pubsub3d::InputCommand MakeCommand(std::uint64_t sequence) {
  pubsub3d::InputCommand command;
  command.set_schema_version("1.0.0");
  command.set_command_id("cmd-" + std::to_string(sequence));
  command.set_client_id("client-a");
  command.set_scene_id("urban-night-circuit");
  command.set_actor_id("car-0");
  command.set_action(pubsub3d::MOVE_FORWARD);
  command.set_timestamp_ms(1700000000000ULL + sequence);
  command.set_sequence(sequence);
  return command;
}
}  // namespace

int main() {
  InputCommandBus bus(2);
  bool ok = true;
  ok &= Check(bus.Push(MakeCommand(1)), "first push should succeed");
  ok &= Check(bus.Push(MakeCommand(2)), "second push should succeed");
  ok &= Check(!bus.Push(MakeCommand(3)), "third push should be dropped");
  ok &= Check(bus.DroppedCount() == 1, "dropped_count must be 1");
  auto drained = bus.Drain(10);
  ok &= Check(drained.size() == 2, "drain should return two commands");
  ok &= Check(drained[0].sequence() == 1, "first drained sequence should be 1");
  ok &= Check(drained[1].sequence() == 2, "second drained sequence should be 2");
  ok &= Check(bus.Size() == 0, "queue must be empty after drain");
  return ok ? 0 : 1;
}
