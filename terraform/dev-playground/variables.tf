variable "project_id" {
  type        = string
  description = "project id"
  default     = "playground-442622"
}


variable "region" {
  type        = string
  description = "region"
  default     = "europe-west4"
}

variable "livekit_domain" {
  type        = string
  description = "Domain for LiveKit server"
  default     = "livekit-gke.haffi.dev"
}

variable "enable_livekit" {
  type        = bool
  description = "Enable the full LiveKit infrastructure stack (GKE, Redis, cert-manager, ingress, firewall, and LiveKit)"
  default     = false
}

variable "gke_node_pool_min_count" {
  type        = number
  description = "Minimum number of nodes in the GKE node pool (0 for auto-scaling to zero)"
  default     = 0
}

variable "gke_node_pool_max_count" {
  type        = number
  description = "Maximum number of nodes in the GKE node pool"
  default     = 1
}

variable "gke_machine_type" {
  type        = string
  description = "Machine type for GKE nodes"
  default     = "e2-standard-4"
}

variable "redis_version" {
  type        = string
  description = "Redis image version"
  default     = "7.2-alpine"
}

variable "livekit_version" {
  type        = string
  description = "LiveKit server version"
  default     = "1.9.0"
}

variable "docker_repository_id" {
  type        = string
  description = "Docker repository for Cloud Run images"
  default     = "playground-docker-repo"
}

variable "pubsub_3d_topic_name" {
  type        = string
  description = "Pub/Sub topic for 3D scene messages"
  default     = "pubsub-3d-scene-topic"
}

variable "pubsub_3d_subscription_name" {
  type        = string
  description = "Pub/Sub subscription consumed by relay service"
  default     = "pubsub-3d-relay-subscription"
}

variable "pubsub_3d_relay_service_name" {
  type        = string
  description = "Cloud Run service name for relay"
  default     = "pubsub-3d-relay"
}

variable "pubsub_3d_webapp_service_name" {
  type        = string
  description = "Cloud Run service name for webapp"
  default     = "pubsub-3d-webapp"
}

variable "pubsub_3d_relay_image" {
  type        = string
  description = "Container image for Pub/Sub relay service"
  default     = "europe-west4-docker.pkg.dev/playground-442622/playground-docker-repo/pubsub-3d-relay:latest"
}

variable "pubsub_3d_webapp_image" {
  type        = string
  description = "Container image for 3D webapp service"
  default     = "europe-west4-docker.pkg.dev/playground-442622/playground-docker-repo/pubsub-3d-webapp:latest"
}

variable "pubsub_3d_stream_protocol" {
  type        = string
  description = "Relay protocol for browser streaming, ws or sse"
  default     = "ws"
}

variable "pubsub_3d_max_ws_clients" {
  type        = number
  description = "Max connected websocket clients per relay instance"
  default     = 1000
}

variable "pubsub_3d_flow_control_max_messages" {
  type        = number
  description = "Pub/Sub subscriber flow control max outstanding messages"
  default     = 1000
}

variable "pubsub_3d_flow_control_max_bytes" {
  type        = number
  description = "Pub/Sub subscriber flow control max outstanding bytes"
  default     = 10485760
}

variable "pubsub_3d_relay_min_instances" {
  type        = number
  description = "Minimum relay Cloud Run instances"
  default     = 0
}

variable "pubsub_3d_relay_max_instances" {
  type        = number
  description = "Maximum relay Cloud Run instances"
  default     = 3
}
