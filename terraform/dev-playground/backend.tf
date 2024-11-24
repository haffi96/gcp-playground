terraform {
  backend "gcs" {
    bucket = "dev-playground-tf-state"
    prefix = "terraform/state"
  }
}
