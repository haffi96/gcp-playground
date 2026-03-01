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

## Required environment variables

- `GCP_PROJECT_ID`
- `PUBSUB_TOPIC_ID`

## Optional tuning variables

- `SCENE_ID` (default `retro-cyberpunk-duel`)
- `SCENE_ENTITY_COUNT` (default `800`)
- `TARGET_MESSAGES` (default `20000`)
- `INFLIGHT_LIMIT` (default `2000`)
- `MAX_BATCH_MESSAGES` (default `500`)
- `MAX_BATCH_BYTES` (default `4194304`)
- `MAX_HOLD_TIME_MS` (default `20`)
- `DRY_RUN` (default `false`)
- `DRY_RUN_FRAMES` (default `3`)

## Build

```bash
cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE="$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake"
cmake --build build -j
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
