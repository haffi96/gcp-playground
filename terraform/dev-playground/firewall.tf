# Firewall rules for LiveKit WebRTC traffic

# Allow WebRTC media (UDP)
resource "google_compute_firewall" "livekit_webrtc_udp" {
  name    = "livekit-webrtc-udp"
  network = "default"
  project = var.project_id

  allow {
    protocol = "udp"
    ports    = ["50000-50200", "7882", "3478"]
  }

  source_ranges = ["0.0.0.0/0"]
  # Target all GKE nodes - use network tag pattern
  target_tags = ["gke-dev-playground-29713660-node"]

  description = "Allow LiveKit WebRTC UDP traffic to GKE nodes"
}

# Allow LiveKit HTTP/WebSocket and RTC TCP
resource "google_compute_firewall" "livekit_tcp" {
  name    = "livekit-tcp"
  network = "default"
  project = var.project_id

  allow {
    protocol = "tcp"
    ports    = ["80", "443", "7880", "7881", "5349"]
  }

  source_ranges = ["0.0.0.0/0"]
  # Target all GKE nodes - use network tag pattern
  target_tags = ["gke-dev-playground-29713660-node"]

  description = "Allow LiveKit HTTP/WebSocket and RTC TCP traffic to GKE nodes"
}

