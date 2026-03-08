locals {
  pubsub_3d_topic_name        = var.pubsub_3d_topic_name
  pubsub_3d_subscription_name = var.pubsub_3d_subscription_name
  pubsub_3d_input_topic_name  = var.pubsub_3d_input_topic_name
  pubsub_3d_input_subscription_name = var.pubsub_3d_input_subscription_name
}

resource "google_project_service" "pubsub_api" {
  project            = var.project_id
  service            = "pubsub.googleapis.com"
  disable_on_destroy = false
}

resource "google_pubsub_topic" "scene_topic" {
  depends_on = [google_project_service.pubsub_api]

  name    = local.pubsub_3d_topic_name
  project = var.project_id
}

resource "google_pubsub_subscription" "relay_subscription" {
  depends_on = [google_pubsub_topic.scene_topic]

  name    = local.pubsub_3d_subscription_name
  project = var.project_id
  topic   = google_pubsub_topic.scene_topic.name

  message_retention_duration = "1200s"
  ack_deadline_seconds       = 20

  retry_policy {
    minimum_backoff = "10s"
    maximum_backoff = "120s"
  }

  expiration_policy {
    ttl = ""
  }
}

resource "google_pubsub_topic" "input_topic" {
  depends_on = [google_project_service.pubsub_api]

  name    = local.pubsub_3d_input_topic_name
  project = var.project_id
}

resource "google_pubsub_subscription" "game_server_input_subscription" {
  depends_on = [google_pubsub_topic.input_topic]

  name    = local.pubsub_3d_input_subscription_name
  project = var.project_id
  topic   = google_pubsub_topic.input_topic.name

  message_retention_duration = "1200s"
  ack_deadline_seconds       = 20

  retry_policy {
    minimum_backoff = "5s"
    maximum_backoff = "60s"
  }

  expiration_policy {
    ttl = ""
  }
}

resource "google_service_account" "pubsub_3d_publisher" {
  account_id   = "pubsub-3d-publisher"
  display_name = "PubSub 3D Publisher"
}

resource "google_project_iam_member" "publisher_pubsub_publisher" {
  project = var.project_id
  role    = "roles/pubsub.publisher"
  member  = "serviceAccount:${google_service_account.pubsub_3d_publisher.email}"
}

resource "google_project_iam_member" "relay_pubsub_subscriber" {
  project = var.project_id
  role    = "roles/pubsub.subscriber"
  member  = "serviceAccount:${google_service_account.pubsub_3d_relay.email}"
}

resource "google_project_iam_member" "relay_pubsub_publisher" {
  project = var.project_id
  role    = "roles/pubsub.publisher"
  member  = "serviceAccount:${google_service_account.pubsub_3d_relay.email}"
}

resource "google_project_iam_member" "publisher_pubsub_subscriber" {
  project = var.project_id
  role    = "roles/pubsub.subscriber"
  member  = "serviceAccount:${google_service_account.pubsub_3d_publisher.email}"
}
