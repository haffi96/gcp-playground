#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

PROJECT_ID="${PROJECT_ID:-playground-442622}"
REGION="${REGION:-europe-west4}"
REPOSITORY="${REPOSITORY:-playground-docker-repo}"
TAG="${TAG:-latest}"
PLATFORM="${PLATFORM:-linux/amd64}"
DOCKER_PROGRESS="${DOCKER_PROGRESS:-plain}"
VITE_RELAY_URL="${VITE_RELAY_URL:-http://localhost:8080}"
VITE_STREAM_PROTOCOL="${VITE_STREAM_PROTOCOL:-ws}"
NO_CACHE=false

while [[ $# -gt 0 ]]; do
  case "$1" in
    --tag)
      TAG="${2:?missing value for --tag}"
      shift 2
      ;;
    --no-cache)
      NO_CACHE=true
      shift
      ;;
    --help|-h)
      cat <<'EOF'
Build and push pubsub-3d-webapp image.

Options:
  --tag <value>    Image tag (default: latest)
  --no-cache       Build with no cache
EOF
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      exit 1
      ;;
  esac
done

REGISTRY_HOST="${REGION}-docker.pkg.dev"
IMAGE_URI="${REGISTRY_HOST}/${PROJECT_ID}/${REPOSITORY}/pubsub-3d-webapp:${TAG}"

if ! docker buildx inspect >/dev/null 2>&1; then
  docker buildx create --use --name cloudrun-builder >/dev/null
fi

BUILD_CMD=(
  docker buildx build
  --platform "${PLATFORM}"
  --progress "${DOCKER_PROGRESS}"
  -f apps/pubsub-3d-webapp/Dockerfile
  --build-arg VITE_RELAY_URL="${VITE_RELAY_URL}"
  --build-arg VITE_STREAM_PROTOCOL="${VITE_STREAM_PROTOCOL}"
  -t "${IMAGE_URI}"
  --push
  "${REPO_ROOT}"
)
if [[ "${NO_CACHE}" == "true" ]]; then
  BUILD_CMD=(docker buildx build --platform "${PLATFORM}" --progress "${DOCKER_PROGRESS}" --no-cache -f apps/pubsub-3d-webapp/Dockerfile --build-arg VITE_RELAY_URL="${VITE_RELAY_URL}" --build-arg VITE_STREAM_PROTOCOL="${VITE_STREAM_PROTOCOL}" -t "${IMAGE_URI}" --push "${REPO_ROOT}")
fi

echo "Building and pushing ${IMAGE_URI}"
"${BUILD_CMD[@]}" 2>&1

echo "${IMAGE_URI}"
