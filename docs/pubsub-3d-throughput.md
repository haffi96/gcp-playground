# Pub/Sub Throughput + 3D Web Viewer

This stack adds:

- a local C++ publisher for high-throughput large JSON scene messages,
- a Cloud Run relay service that subscribes to Pub/Sub and streams to clients,
- a relay input ingress path (`POST /input`) that publishes movement commands to a dedicated Pub/Sub input topic,
- a Cloud Run-hosted React Three Fiber viewer.

## 1) Build and push images

```bash
./scripts/deploy.sh --build-only
```

Optional overrides:

```bash
PROJECT_ID=playground-442622 \
REGION=europe-west4 \
REPOSITORY=playground-docker-repo \
TAG=latest \
VITE_RELAY_URL=https://relay.example.run.app \
VITE_STREAM_PROTOCOL=ws \
./scripts/deploy.sh --build-only
```

Deploy selected services and run Terraform apply:

```bash
./scripts/deploy.sh --services pubsub-relay,pubsub-3d-webapp
```

## 2) Provision infrastructure with Terraform (manual)

```bash
cd terraform/dev-playground
terraform init
terraform plan
terraform apply
```

Outputs to use:

```bash
terraform output pubsub_3d_topic
terraform output pubsub_3d_subscription
terraform output pubsub_3d_input_topic
terraform output pubsub_3d_input_subscription
terraform output pubsub_3d_relay_url
terraform output pubsub_3d_webapp_url
terraform output pubsub_3d_publisher_service_account_email
```

## 3) Run C++ publisher locally

Install C++ dependencies on macOS (vcpkg):

```bash
git clone https://github.com/microsoft/vcpkg.git "$HOME/vcpkg"
"$HOME/vcpkg/bootstrap-vcpkg.sh"
"$HOME/vcpkg/vcpkg" install google-cloud-cpp[core,pubsub] nlohmann-json
```

Homebrew does not currently provide a `google-cloud-cpp` formula in this environment.

Build with vcpkg toolchain:

```bash
cd apps/pubsub-publisher-cpp
cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE="$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake"
cmake --build build -j
```

Use manual SA key from `./test-credentials`:

```bash
export GOOGLE_APPLICATION_CREDENTIALS=/Users/haff/code/gcp-playground/test-credentials/pubsub-publisher-key.json
export GCP_PROJECT_ID=playground-442622
export PUBSUB_TOPIC_ID=pubsub-3d-scene-topic
export INPUT_SUBSCRIPTION_ID=pubsub-3d-input-subscription
export SCENE_ID=urban-night-circuit
export PUBLISH_MODE=fixed
export FRAME_LIMIT=50000
export TICK_HZ=10
export STATE_TICK_HZ=10
export ENABLE_REMOTE_INPUT=true
export ENABLE_LOCAL_CLIENT=true
export ENABLE_LOCAL_INPUT_SCRIPT=false
export PLAYER_ACTOR_ID=car-0
export PLAYER_AUTOPILOT_ENABLED=0
export PLAYER_AUTOPILOT_MODE=input_only
export LANE_COUNT=4
export LANE_WAYPOINT_COUNT=120
export VEHICLE_COUNT=120
export OCCUPANCY_GRID_WIDTH=80
export OCCUPANCY_GRID_HEIGHT=80
export INFLIGHT_LIMIT=2000
export MAX_BATCH_MESSAGES=500
export MAX_BATCH_BYTES=4194304
export MAX_HOLD_TIME_MS=20
./build-ninja/pubsub_3d_publisher
```

Set `PLAYER_AUTOPILOT_ENABLED=1` to restore the old synthetic `car-0` autopilot path for demos or baseline throughput runs.

The publisher prints one summary line including `msg_per_sec` and `mb_per_sec`.

Dry run (no publish, prints JSON payloads to stdout):

```bash
export DRY_RUN=true
export DRY_RUN_FRAMES=2
./build-ninja/pubsub_3d_publisher
```

Validate generated scene payload against relay/webapp schema guards:

```bash
DRY_RUN=1 DRY_RUN_FRAMES=1 SCENE_SOURCE=shared_memory ./build-ninja/pubsub_3d_publisher > /tmp/scene_fixture.json
(cd ../pubsub-relay && bun run validate-scene-fixture /tmp/scene_fixture.json)
(cd ../pubsub-3d-webapp && bun run validate-scene-fixture /tmp/scene_fixture.json)
```

## 4) Validate end-to-end behavior

- Relay health endpoint returns config: `GET <relay_url>/healthz`
- Browser websocket endpoint: `<relay_url>/ws`
- Browser SSE fallback endpoint: `<relay_url>/stream`
- Input command endpoint: `POST <relay_url>/input`
- Viewer runs at: `<webapp_url>`
- Viewer controls: `W/A/S/D` or arrow keys for `car-0` movement.

## 5) Throughput test checklist

- Increase `SCENE_ENTITY_COUNT` and verify relay/webapp stability.
- Increase `TARGET_MESSAGES` for longer sustained runs.
- Tune `MAX_BATCH_MESSAGES`, `MAX_BATCH_BYTES`, `MAX_HOLD_TIME_MS`.
- Tune relay flow control envs in Terraform:
  - `pubsub_3d_flow_control_max_messages`
  - `pubsub_3d_flow_control_max_bytes`
- Track:
  - publisher `msg_per_sec` and `mb_per_sec`
  - relay CPU/memory in Cloud Run
  - browser latency and dropped-frame behavior in viewer HUD.
