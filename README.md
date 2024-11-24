Authentication gcloud cli

```sh
gcloud auth login --project dev-playground
```


## Terraform

```sh
cd terraform/dev-playground
```

```sh
terraform init
```

```sh
terraform plan
```

```sh
terraform apply
```

## Skaffold

- This builds the image and pushes it to the registry
- Deploys cloudrun service

```sh
gcloud auth configure-docker europe-west4-docker.pkg.dev
```

```sh
skaffold run --profile dev-playground  --default-repo europe-west4-docker.pkg.dev/playground-442622/playground
```