resource "google_artifact_registry_repository" "docker_repo" {
  depends_on = [google_project_service.artifactregistry_api]

  location      = var.region
  repository_id = var.docker_repository_id
  description   = "Docker repository for pubsub 3d stack"
  format        = "DOCKER"
  mode          = "STANDARD_REPOSITORY"
}
