resource "google_secret_manager_secret" "static_config" {
  secret_id = "chat-app-fastapi--static-config"

  replication {
    user_managed {
      replicas {
        location = var.region
      }
    }
  }
}

resource "google_secret_manager_secret_version" "static_config" {
  secret = google_secret_manager_secret.static_config.id

  secret_data     = file("./config/static_config.yaml")
  deletion_policy = "ABANDON"
}



resource "google_secret_manager_secret_iam_binding" "binding" {
  for_each = toset(
    [
      google_secret_manager_secret.static_config.secret_id,
    ]
  )
  secret_id = each.value
  role      = "roles/secretmanager.secretAccessor"
  members = [
    "serviceAccount:chat-app-fastapi-cloudrun@playground-442622.iam.gserviceaccount.com",
  ]
}
