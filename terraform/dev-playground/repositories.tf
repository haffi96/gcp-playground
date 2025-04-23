
variable "repository_id" {
  description = "ID of the repository"
  type        = string
  default     = "playground-apt-repo"
}

# Enable required APIs
resource "google_project_service" "artifactregistry_api" {
  service            = "artifactregistry.googleapis.com"
  disable_on_destroy = false
}

# Create the Artifact Registry repository
resource "google_artifact_registry_repository" "apt_repo" {
  depends_on = [google_project_service.artifactregistry_api]

  location      = var.region
  repository_id = var.repository_id
  description   = "APT repository for Debian packages"
  format        = "APT"

  # Configure debian specific settings
  mode = "STANDARD_REPOSITORY"
}

# IAM binding for repository access
resource "google_artifact_registry_repository_iam_binding" "repo_iam" {
  location   = google_artifact_registry_repository.apt_repo.location
  repository = google_artifact_registry_repository.apt_repo.name
  role       = "roles/artifactregistry.admin"
  members = [
    "serviceAccount:${var.project_id}@appspot.gserviceaccount.com",
    "serviceAccount:apt-reader@playground-442622.iam.gserviceaccount.com",
    "user:haffimazhar96@gmail.com"
  ]
}

# Outputs
output "repository_id" {
  value = google_artifact_registry_repository.apt_repo.repository_id
}

output "repository_location" {
  value = google_artifact_registry_repository.apt_repo.location
}

output "repository_url" {
  value = "${var.region}-apt.pkg.dev/${var.project_id}/${var.repository_id}"
}
