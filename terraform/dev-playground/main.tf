locals {
  services = {
    for filename in fileset(path.module, "services/*.yaml") :
    filename => yamldecode(file(filename))
  }
}

resource "google_service_account" "service" {
  for_each = local.services

  account_id   = "${each.value.name}-cloudrun"
  display_name = "${each.value.name} CloudRun Service Account"
}

resource "google_cloud_run_service" "default" {
  for_each = local.services

  name     = each.value.name
  location = each.value.region

  template {
    spec {
      service_account_name = google_service_account.service[each.key].email
      containers {
        image = lookup(each.value, "dockerImage", "us-docker.pkg.dev/cloudrun/container/hello:latest")
      }
    }

    metadata {
      # annotations = {
      #   "run.googleapis.com/ingress" = "all"
      # }

      # labels = {
      #   "run.googleapis.come/startupProbeType" = "Default"
      # }
    }
  }

  metadata {
    annotations = {
      "run.googleapis.com/ingress" : "all"
      "run.googleapi.com/launch-stage" : "BETA"
    }
  }

  lifecycle {
    ignore_changes = [
      metadata,
      template,
    ]
  }
}


resource "google_cloud_run_service_iam_binding" "invoker" {
  for_each = local.services


  location = google_cloud_run_service.default[each.key].location
  project  = google_cloud_run_service.default[each.key].project
  service  = google_cloud_run_service.default[each.key].name

  role    = "roles/run.invoker"
  members = ["allUsers"]
}

resource "google_cloud_run_service_iam_binding" "developer" {
  for_each = local.services


  location = google_cloud_run_service.default[each.key].location
  project  = google_cloud_run_service.default[each.key].project
  service  = google_cloud_run_service.default[each.key].name

  role    = "roles/run.developer"
  members = lookup(each.value, "admin_members", [])
}


resource "google_service_account_iam_binding" "admin" {
  for_each = local.services

  service_account_id = google_service_account.service[each.key].name
  role               = "roles/iam.serviceAccountUser"

  members = lookup(each.value, "admin_members", [])
}
