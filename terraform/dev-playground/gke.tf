resource "google_container_cluster" "default" {
  name     = "dev-playground"
  location = "europe-west2"

  # Allow cluster deletion for playground/dev environments
  deletion_protection = false

  # Standard mode (required for LiveKit host networking)
  # Remove default node pool immediately after cluster creation
  remove_default_node_pool = true
  initial_node_count       = 1

  # Network configuration
  network    = "default"
  subnetwork = "default"

  # IP allocation for pods and services
  ip_allocation_policy {
    cluster_ipv4_cidr_block  = ""
    services_ipv4_cidr_block = ""
  }

  # Enable Dataplane V2 for SCTP support (required for WebRTC data channels)
  datapath_provider = "ADVANCED_DATAPATH"

  # Enable Workload Identity for better security
  workload_identity_config {
    workload_pool = "${var.project_id}.svc.id.goog"
  }

  # Maintenance window
  maintenance_policy {
    daily_maintenance_window {
      start_time = "03:00"
    }
  }
}

resource "google_container_node_pool" "livekit_nodes" {
  name     = "livekit-node-pool"
  location = google_container_cluster.default.location
  cluster  = google_container_cluster.default.name

  # Auto-scaling configuration (0 to N nodes)
  autoscaling {
    min_node_count = var.gke_node_pool_min_count
    max_node_count = var.gke_node_pool_max_count
  }

  # Node configuration
  node_config {
    machine_type = var.gke_machine_type

    # Use Spot VMs for cost savings (60-91% cheaper)
    spot = true

    # OAuth scopes for node access
    oauth_scopes = [
      "https://www.googleapis.com/auth/cloud-platform"
    ]

    # Labels for node selection
    labels = {
      workload = "livekit"
    }

    # Metadata
    metadata = {
      disable-legacy-endpoints = "true"
    }

    # Workload Identity
    workload_metadata_config {
      mode = "GKE_METADATA"
    }

    # Disk configuration
    disk_size_gb = 50
    disk_type    = "pd-standard"
  }

  # Node management
  management {
    auto_repair  = true
    auto_upgrade = true
  }

  # Lifecycle configuration to handle updates gracefully
  lifecycle {
    ignore_changes = [
      node_config[0].labels,
      node_config[0].taint,
      node_config[0].resource_labels,
      node_config[0].kubelet_config,
    ]
  }
}
