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

## Required environment variables

- `GCP_PROJECT_ID`
- `PUBSUB_TOPIC_ID`

## Optional tuning variables

- `SCENE_ID` (default `urban-night-circuit`)
- `DRY_RUN` (default `false`)
- `DRY_RUN_FRAMES` (default `3`)
- `PUBLISH_MODE` (default `fixed`)
- `FRAME_LIMIT` (default `50000`)
- `TICK_HZ` (default `10`)
- `LANE_COUNT` (default `4`)
- `LANE_WAYPOINT_COUNT` (default `120`)
- `VEHICLE_COUNT` (default `120`)
- `OCCUPANCY_GRID_WIDTH` (default `80`)
- `OCCUPANCY_GRID_HEIGHT` (default `80`)
- `INFLIGHT_LIMIT` (default `2000`)
- `MAX_BATCH_MESSAGES` (default `500`)
- `MAX_BATCH_BYTES` (default `4194304`)

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

## Run publish benchmark

```bash
./build/pubsub_3d_publisher
```

## Run dry mode (prints JSON only)

```bash
export DRY_RUN=true
export DRY_RUN_FRAMES=2
./build/pubsub_3d_publisher
```
