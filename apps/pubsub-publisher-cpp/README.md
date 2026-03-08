# Pub/Sub 3D Publisher (C++)

Local high-throughput publisher for Pub/Sub large JSON 3D scene payloads.

## Install dependencies on macOS (vcpkg)

`google-cloud-cpp` Pub/Sub is not currently available via Homebrew formula in this environment, so use vcpkg.

```bash
git clone https://github.com/microsoft/vcpkg.git "$HOME/vcpkg"
"$HOME/vcpkg/bootstrap-vcpkg.sh"
"$HOME/vcpkg/vcpkg" install google-cloud-cpp[core,pubsub] nlohmann-json
```

Then configure CMake with the toolchain file:

```bash
cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE="$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake"
cmake --build build -j
```

To build with Ninja (matching your arm64 vcpkg setup):

```bash
cmake -S . -B build-ninja -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake" \
  -DVCPKG_TARGET_TRIPLET=arm64-osx
cmake --build build-ninja --target pubsub_3d_publisher -j
```

Or use the included CMake presets:

```bash
cmake --preset ninja-vcpkg-arm64-osx
cmake --build --preset build-ninja-publisher -j
```

## Runtime architecture

The binary now supports two scene sources:

- `SCENE_SOURCE=synthetic`: old direct JSON generation path.
- `SCENE_SOURCE=shared_memory`: game-state manager writes protobuf state to shared buffer, local client and publisher read from it.

Runtime modes:

- `RUNTIME_MODE=server` (default): game-state manager + publisher.
- `RUNTIME_MODE=dev`: same as server plus local client logging thread.

## Required environment variables

For real Pub/Sub publishing:

- `GCP_PROJECT_ID`
- `PUBSUB_TOPIC_ID`
- `GOOGLE_APPLICATION_CREDENTIALS` (path to service-account JSON, required by Google auth libs)

For dry-run (`DRY_RUN=true`), `GCP_PROJECT_ID` and `PUBSUB_TOPIC_ID` are optional.

## Optional tuning variables

Core runtime:

- `SCENE_SOURCE` (default `synthetic`, use `shared_memory` for new pipeline)
- `RUNTIME_MODE` (default `server`, use `dev` to run local client thread)
- `ENABLE_LOCAL_CLIENT` (default `false`, forces local client in any mode)
- `ENABLE_LOCAL_INPUT_SCRIPT` (default `false`, emits synthetic local movement commands)
- `ENABLE_REMOTE_INPUT` (default `false`, consumes input commands from Pub/Sub)
- `INPUT_SUBSCRIPTION_ID` (required when `ENABLE_REMOTE_INPUT=true`)
- `PLAYER_ACTOR_ID` (default `car-0`, actor controlled by input commands)
- `PLAYER_AUTOPILOT_ENABLED` (default `false`, keeps `car-0` stationary until input unless enabled)
- `PLAYER_AUTOPILOT_MODE` (default `legacy`, supported values: `legacy`, `input_only`)
- `DRY_RUN` (default `false`)
- `DRY_RUN_FRAMES` (default `3`)

Rates and publish:

- `PUBLISH_MODE` (default `fixed`, `continuous` supported)
- `FRAME_LIMIT` (default `20000`)
- `TICK_HZ` (default `10`, publisher tick)
- `STATE_TICK_HZ` (default same as `TICK_HZ`, state manager tick)
- `LOCAL_CLIENT_TICK_HZ` (default same as `TICK_HZ`)
- `INFLIGHT_LIMIT` (default `2000`)
- `MAX_BATCH_MESSAGES` (default `500`)
- `MAX_BATCH_BYTES` (default `4194304`)
- `MAX_HOLD_TIME_MS` (default `20`)
- `INPUT_QUEUE_CAPACITY` (default `4096`)
- `INPUT_STALE_MS` (default `1000`)
- `INPUT_MAX_PER_TICK` (default `128`)
- `DIAGNOSTICS_LOG_INTERVAL_FRAMES` (default `120`)

Scene generation parameters:

- `SCENE_ID` (default `urban-night-circuit`)
- `LANE_COUNT` (default `4`)
- `LANE_WAYPOINT_COUNT` (default `120`)
- `VEHICLE_COUNT` (default `120`)
- `ROAD_LENGTH_METERS` (default `1800`)
- `ROAD_OBJECT_COUNT` (default `30`)
- `ROAD_OBJECT_SPREAD_METERS` (default `900`)
- `ROAD_OBJECT_LANE_OCCUPANCY_PERCENT` (default `35`)
- `OCCUPANCY_GRID_WIDTH` (default `80`)
- `OCCUPANCY_GRID_HEIGHT` (default `80`)

## Build

```bash
cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE="$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake"
cmake --build build -j
```

### Build with Ninja target

```bash
cmake --preset ninja-vcpkg-arm64-osx
cmake --build --preset build-ninja-publisher -j
```

## Run locally

Build:

```bash
cmake --preset ninja-vcpkg-arm64-osx
cmake --build --preset build-ninja-publisher -j
```

### 1) Dry-run synthetic mode (no Pub/Sub required)

```bash
DRY_RUN=true \
DRY_RUN_FRAMES=2 \
SCENE_SOURCE=synthetic \
./build-ninja/pubsub_3d_publisher
```

### 2) Dry-run shared-memory mode with local client

```bash
DRY_RUN=true \
DRY_RUN_FRAMES=2 \
SCENE_SOURCE=shared_memory \
RUNTIME_MODE=dev \
ENABLE_LOCAL_CLIENT=1 \
STATE_TICK_HZ=30 \
LOCAL_CLIENT_TICK_HZ=30 \
./build-ninja/pubsub_3d_publisher
```

### 3) Real Pub/Sub publishing from shared-memory pipeline

```bash
export GOOGLE_APPLICATION_CREDENTIALS="/absolute/path/to/service-account.json"
export GCP_PROJECT_ID="your-gcp-project"
export PUBSUB_TOPIC_ID="your-scene-topic"

SCENE_SOURCE=shared_memory \
RUNTIME_MODE=server \
PUBLISH_MODE=fixed \
FRAME_LIMIT=500 \
TICK_HZ=20 \
STATE_TICK_HZ=20 \
./build-ninja/pubsub_3d_publisher
```

### 4) Real Pub/Sub publishing with local debug client

```bash
SCENE_SOURCE=shared_memory \
RUNTIME_MODE=dev \
ENABLE_LOCAL_CLIENT=1 \
PUBLISH_MODE=continuous \
TICK_HZ=20 \
STATE_TICK_HZ=20 \
LOCAL_CLIENT_TICK_HZ=20 \
./build-ninja/pubsub_3d_publisher
```

### 5) Real Pub/Sub publishing with remote input subscription enabled

```bash
export GOOGLE_APPLICATION_CREDENTIALS="/absolute/path/to/service-account.json"
export GCP_PROJECT_ID="your-gcp-project"
export PUBSUB_TOPIC_ID="pubsub-3d-scene-topic"
export INPUT_SUBSCRIPTION_ID="pubsub-3d-input-subscription"

SCENE_SOURCE=shared_memory \
RUNTIME_MODE=dev \
ENABLE_LOCAL_CLIENT=1 \
ENABLE_REMOTE_INPUT=1 \
ENABLE_LOCAL_INPUT_SCRIPT=0 \
PLAYER_ACTOR_ID=car-0 \
PLAYER_AUTOPILOT_ENABLED=0 \
PLAYER_AUTOPILOT_MODE=input_only \
STATE_TICK_HZ=30 \
TICK_HZ=30 \
LOCAL_CLIENT_TICK_HZ=30 \
./build-ninja/pubsub_3d_publisher
```

To restore the original demo movement for `car-0`, set `PLAYER_AUTOPILOT_ENABLED=1`.

## Local input-command validation against relay/webapp schemas

Generate a fixture from the C++ shared-memory pipeline and validate with both TS schema guards:

```bash
DRY_RUN=1 DRY_RUN_FRAMES=1 SCENE_SOURCE=shared_memory ./build-ninja/pubsub_3d_publisher > /tmp/scene_fixture.json
(cd ../pubsub-relay && bun run validate-scene-fixture /tmp/scene_fixture.json)
(cd ../pubsub-3d-webapp && bun run validate-scene-fixture /tmp/scene_fixture.json)
```
