#!/usr/bin/env bash
set -euo pipefail

if [[ "${1:-}" == "--help" || "${1:-}" == "-h" ]]; then
  cat <<'EOF'
Build and push Pub/Sub 3D stack images.

Environment variables:
  PROJECT_ID            GCP project id (default: playground-442622)
  REGION                Artifact Registry region (default: europe-west4)
  REPOSITORY            Artifact Registry repository (default: playground-docker-repo)
  TAG                   Image tag (default: latest)
  PLATFORM              Docker platform for Cloud Run compatibility (default: linux/amd64)
  VITE_RELAY_URL        Relay URL baked into build stage (default: http://localhost:8080)
  VITE_STREAM_PROTOCOL  ws or sse (default: ws)

Example:
  PROJECT_ID=playground-442622 TAG=v1 ./scripts/build_push_pubsub_3d_images.sh
EOF
  exit 0
fi

PROJECT_ID="${PROJECT_ID:-playground-442622}"
REGION="${REGION:-europe-west4}"
REPOSITORY="${REPOSITORY:-playground-docker-repo}"
TAG="${TAG:-latest}"
PLATFORM="${PLATFORM:-linux/amd64}"
VITE_RELAY_URL="${VITE_RELAY_URL:-http://localhost:8080}"
VITE_STREAM_PROTOCOL="${VITE_STREAM_PROTOCOL:-ws}"

REGISTRY_HOST="${REGION}-docker.pkg.dev"
RELAY_IMAGE="${REGISTRY_HOST}/${PROJECT_ID}/${REPOSITORY}/pubsub-3d-relay:${TAG}"
WEBAPP_IMAGE="${REGISTRY_HOST}/${PROJECT_ID}/${REPOSITORY}/pubsub-3d-webapp:${TAG}"

echo "Configuring docker auth for ${REGISTRY_HOST}"
gcloud auth configure-docker "${REGISTRY_HOST}"

if ! docker buildx inspect >/dev/null 2>&1; then
  echo "No active buildx builder, creating one..."
  docker buildx create --use --name cloudrun-builder
fi

echo "Building relay image: ${RELAY_IMAGE}"
docker buildx build \
  --platform "${PLATFORM}" \
  -f Dockerfile.pubsub-relay \
  -t "${RELAY_IMAGE}" \
  --push .

echo "Building webapp image: ${WEBAPP_IMAGE}"
docker buildx build \
  --platform "${PLATFORM}" \
  -f Dockerfile.pubsub-3d-webapp \
  --build-arg VITE_RELAY_URL="${VITE_RELAY_URL}" \
  --build-arg VITE_STREAM_PROTOCOL="${VITE_STREAM_PROTOCOL}" \
  -t "${WEBAPP_IMAGE}" \
  --push .

cat <<EOF
Done.
Relay image:  ${RELAY_IMAGE}
Webapp image: ${WEBAPP_IMAGE}
EOF
