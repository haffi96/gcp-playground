#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

PROJECT_ID="${PROJECT_ID:-playground-442622}"
REGION="${REGION:-europe-west4}"
REPOSITORY="${REPOSITORY:-playground-docker-repo}"
PLATFORM="${PLATFORM:-linux/amd64}"
DOCKER_PROGRESS="${DOCKER_PROGRESS:-plain}"

default_tag() {
  local ts git_sha rand
  ts="$(date +%Y%m%d-%H%M%S)"
  git_sha="$(git -C "${REPO_ROOT}" rev-parse --short=8 HEAD 2>/dev/null || true)"
  rand="$(LC_ALL=C tr -dc 'a-z0-9' </dev/urandom | head -c 6)"
  if [[ -n "${git_sha}" ]]; then
    echo "${ts}-${git_sha}-${rand}"
  else
    echo "${ts}-${rand}"
  fi
}

TAG="${TAG:-$(default_tag)}"
BUILD_ONLY=false
NO_CACHE=false
SERVICES="pubsub-relay,pubsub-3d-webapp"

usage() {
  cat <<'EOF'
Build, push, and deploy Pub/Sub 3D services with Terraform.

Options:
  --build-only                  Build and push images only, skip terraform apply
  --services <csv>              Comma-separated service names to build/deploy.
                                Supported: pubsub-relay,pubsub-3d-webapp
                                Default: all
  --no-cache                    Build docker images with no cache
  --tag <value>                 Image tag (default: timestamp-gitsha-random)
  -h, --help                    Show this help

Environment variables:
  PROJECT_ID                    default: playground-442622
  REGION                        default: europe-west4
  REPOSITORY                    default: playground-docker-repo
  PLATFORM                      default: linux/amd64
  DOCKER_PROGRESS               default: plain (buildx progress output mode)
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --build-only)
      BUILD_ONLY=true
      shift
      ;;
    --services)
      SERVICES="${2:?missing value for --services}"
      shift 2
      ;;
    --no-cache)
      NO_CACHE=true
      shift
      ;;
    --tag)
      TAG="${2:?missing value for --tag}"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      usage
      exit 1
      ;;
  esac
done

is_supported_service() {
  case "$1" in
    pubsub-relay|pubsub-3d-webapp) return 0 ;;
    *) return 1 ;;
  esac
}

declare -a SELECTED_SERVICES=()
declare -a service_list=()
IFS=',' read -r -a service_list <<< "${SERVICES:-}" || true
for service in "${service_list[@]:-}"; do
  trimmed="$(echo "${service}" | xargs)"
  if [[ -z "${trimmed}" ]]; then
    continue
  fi
  if ! is_supported_service "${trimmed}"; then
    echo "Unsupported service '${trimmed}'. Valid values: pubsub-relay,pubsub-3d-webapp" >&2
    exit 1
  fi
  already_selected=false
  for selected in "${SELECTED_SERVICES[@]:-}"; do
    if [[ "${selected}" == "${trimmed}" ]]; then
      already_selected=true
      break
    fi
  done
  if [[ "${already_selected}" == "false" ]]; then
    SELECTED_SERVICES+=("${trimmed}")
  fi
done

if [[ ${#SELECTED_SERVICES[@]} -eq 0 ]]; then
  echo "No valid services selected." >&2
  exit 1
fi

REGISTRY_HOST="${REGION}-docker.pkg.dev"
RELAY_IMAGE="${REGISTRY_HOST}/${PROJECT_ID}/${REPOSITORY}/pubsub-3d-relay:${TAG}"
WEBAPP_IMAGE="${REGISTRY_HOST}/${PROJECT_ID}/${REPOSITORY}/pubsub-3d-webapp:${TAG}"

echo "Using tag: ${TAG}"
echo "Configuring docker auth for ${REGISTRY_HOST}"
gcloud auth configure-docker "${REGISTRY_HOST}" --quiet

BUILD_ARGS=()
if [[ "${NO_CACHE}" == "true" ]]; then
  BUILD_ARGS+=(--no-cache)
fi
BUILD_ARGS+=(--tag "${TAG}")

export PROJECT_ID REGION REPOSITORY PLATFORM DOCKER_PROGRESS TAG

for service in "${SELECTED_SERVICES[@]}"; do
  case "${service}" in
    pubsub-relay)
      "${REPO_ROOT}/apps/pubsub-relay/build.sh" "${BUILD_ARGS[@]}"
      ;;
    pubsub-3d-webapp)
      "${REPO_ROOT}/apps/pubsub-3d-webapp/build.sh" "${BUILD_ARGS[@]}"
      ;;
  esac
done

if [[ "${BUILD_ONLY}" == "true" ]]; then
  echo "Build-only mode complete."
  exit 0
fi

TF_ARGS=()
TARGET_ARGS=()

for service in "${SELECTED_SERVICES[@]}"; do
  case "${service}" in
    pubsub-relay)
      TF_ARGS+=(-var="pubsub_3d_relay_image=${RELAY_IMAGE}")
      TARGET_ARGS+=(-target=google_cloud_run_v2_service.pubsub_3d_relay)
      TARGET_ARGS+=(-target=google_cloud_run_v2_service_iam_binding.pubsub_3d_relay_invoker)
      ;;
    pubsub-3d-webapp)
      TF_ARGS+=(-var="pubsub_3d_webapp_image=${WEBAPP_IMAGE}")
      TARGET_ARGS+=(-target=google_cloud_run_v2_service.pubsub_3d_webapp)
      TARGET_ARGS+=(-target=google_cloud_run_v2_service_iam_binding.pubsub_3d_webapp_invoker)
      ;;
  esac
done

if [[ ${#SELECTED_SERVICES[@]} -gt 1 ]]; then
  TARGET_ARGS=()
fi

echo "Running terraform apply..."
cd "${REPO_ROOT}/terraform/dev-playground"
if [[ ${#TARGET_ARGS[@]} -gt 0 ]]; then
  terraform apply "${TF_ARGS[@]}" "${TARGET_ARGS[@]}"
else
  terraform apply "${TF_ARGS[@]}"
fi
