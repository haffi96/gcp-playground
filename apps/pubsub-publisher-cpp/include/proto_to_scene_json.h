#pragma once

#include "proto/game_state.pb.h"

#include <string>

std::string ProtoFrameToSceneJson(const pubsub3d::GameStateFrame& frame);
