Default user:

```sh
gcloud auth application-default login --project dev-playground
```

```sh
GOOGLE_APPLICATION_CREDENTIALS="/Users/haff/.config/gcloud/application_default_credentials.json" uv run main.py
```


Test with SA and key file:

```sh
gcloud auth login --project dev-playground
```

```sh
GOOGLE_APPLICATION_CREDENTIALS="/Users/haff/code/gcp-playground/test-credentials/test-secrets-reader-key.json" uv run main.py
```
