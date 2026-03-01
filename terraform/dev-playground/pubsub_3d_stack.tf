resource "google_project_service" "run_api" {
  project            = var.project_id
  service            = "run.googleapis.com"
  disable_on_destroy = false
}

resource "google_service_account" "pubsub_3d_relay" {
  account_id   = "pubsub-3d-relay"
  display_name = "PubSub 3D Relay"
}

resource "google_service_account" "pubsub_3d_webapp" {
  account_id   = "pubsub-3d-webapp"
  display_name = "PubSub 3D Webapp"
}

resource "google_cloud_run_v2_service" "pubsub_3d_relay" {
  depends_on = [
    google_project_service.run_api,
    google_pubsub_subscription.relay_subscription,
  ]

  name     = var.pubsub_3d_relay_service_name
  location = var.region
  project  = var.project_id
  ingress  = "INGRESS_TRAFFIC_ALL"

  template {
    service_account = google_service_account.pubsub_3d_relay.email
    timeout         = "300s"

    containers {
      image = var.pubsub_3d_relay_image
      ports {
        container_port = 8080
      }

      env {
        name  = "PUBSUB_SUBSCRIPTION"
        value = google_pubsub_subscription.relay_subscription.name
      }
      env {
        name  = "GCP_PROJECT_ID"
        value = var.project_id
      }
      env {
        name  = "STREAM_PROTOCOL"
        value = var.pubsub_3d_stream_protocol
      }
      env {
        name  = "MAX_WS_CLIENTS"
        value = tostring(var.pubsub_3d_max_ws_clients)
      }
      env {
        name  = "PUBSUB_FLOW_CONTROL_MAX_MESSAGES"
        value = tostring(var.pubsub_3d_flow_control_max_messages)
      }
      env {
        name  = "PUBSUB_FLOW_CONTROL_MAX_BYTES"
        value = tostring(var.pubsub_3d_flow_control_max_bytes)
      }
    }

    scaling {
      min_instance_count = var.pubsub_3d_relay_min_instances
      max_instance_count = var.pubsub_3d_relay_max_instances
    }
  }
}

resource "google_cloud_run_v2_service_iam_binding" "pubsub_3d_relay_invoker" {
  project  = var.project_id
  location = google_cloud_run_v2_service.pubsub_3d_relay.location
  name     = google_cloud_run_v2_service.pubsub_3d_relay.name
  role     = "roles/run.invoker"
  members  = ["allUsers"]
}

resource "google_cloud_run_v2_service" "pubsub_3d_webapp" {
  depends_on = [google_project_service.run_api]

  name     = var.pubsub_3d_webapp_service_name
  location = var.region
  project  = var.project_id
  ingress  = "INGRESS_TRAFFIC_ALL"

  template {
    service_account = google_service_account.pubsub_3d_webapp.email

    containers {
      image = var.pubsub_3d_webapp_image
      ports {
        container_port = 8080
      }

      env {
        name  = "VITE_RELAY_URL"
        value = google_cloud_run_v2_service.pubsub_3d_relay.uri
      }
      env {
        name  = "VITE_STREAM_PROTOCOL"
        value = var.pubsub_3d_stream_protocol
      }
    }
  }
}

resource "google_cloud_run_v2_service_iam_binding" "pubsub_3d_webapp_invoker" {
  project  = var.project_id
  location = google_cloud_run_v2_service.pubsub_3d_webapp.location
  name     = google_cloud_run_v2_service.pubsub_3d_webapp.name
  role     = "roles/run.invoker"
  members  = ["allUsers"]
}
