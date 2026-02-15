resource "google_service_account" "test_secrets_reader" {
  account_id   = "test-secrets-reader"
  display_name = "Test Secrets Reader Service Account"
}

resource "google_project_iam_member" "test_secrets_reader_secret_accessor" {
  project = var.project_id
  role    = "roles/secretmanager.secretAccessor"
  member  = "serviceAccount:${google_service_account.test_secrets_reader.email}"
}

# Manual key workflow (keys are NOT managed by Terraform):
# 1) terraform apply
# 2) gcloud iam service-accounts keys create ./test-secrets-reader-key.json --iam-account test-secrets-reader@${var.project_id}.iam.gserviceaccount.com
# 3) export GOOGLE_APPLICATION_CREDENTIALS=./test-secrets-reader-key.json
# 4) Optional cleanup:
#    gcloud iam service-accounts keys list --iam-account test-secrets-reader@${var.project_id}.iam.gserviceaccount.com
#    gcloud iam service-accounts keys delete KEY_ID --iam-account test-secrets-reader@${var.project_id}.iam.gserviceaccount.com

output "test_secrets_reader_service_account_email" {
  value = google_service_account.test_secrets_reader.email
}

output "test_secrets_reader_service_account_unique_id" {
  value = google_service_account.test_secrets_reader.unique_id
}
