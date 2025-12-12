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
