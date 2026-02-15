terraform {
  required_providers {
    google = {
      source  = "hashicorp/google"
      version = "5.40.0"
    }
    kubernetes = {
      source  = "hashicorp/kubernetes"
      version = "~> 2.23"
    }
    helm = {
      source  = "hashicorp/helm"
      version = "~> 2.11"
    }
    kubectl = {
      source  = "gavinbunney/kubectl"
      version = "~> 1.14"
    }
    random = {
      source  = "hashicorp/random"
      version = "~> 3.5"
    }
  }
}

provider "google" {
  project = var.project_id
  region  = var.region
  zone    = "europe-west4-a"
}

# Get cluster credentials for kubernetes providers
data "google_client_config" "default" {}

locals {
  kubernetes_host = var.enable_livekit ? try("https://${google_container_cluster.default[0].endpoint}", "https://127.0.0.1") : "https://127.0.0.1"
  kubernetes_ca = var.enable_livekit ? try(base64decode(
    google_container_cluster.default[0].master_auth[0].cluster_ca_certificate,
  ), "") : ""
  kubernetes_token = var.enable_livekit ? data.google_client_config.default.access_token : ""
}

provider "kubernetes" {
  host                   = local.kubernetes_host
  token                  = local.kubernetes_token
  cluster_ca_certificate = local.kubernetes_ca
}

provider "helm" {
  kubernetes {
    host                   = local.kubernetes_host
    token                  = local.kubernetes_token
    cluster_ca_certificate = local.kubernetes_ca
  }
}

provider "kubectl" {
  host                   = local.kubernetes_host
  token                  = local.kubernetes_token
  cluster_ca_certificate = local.kubernetes_ca
  load_config_file = false
}
