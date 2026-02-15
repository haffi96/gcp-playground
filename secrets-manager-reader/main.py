import os

import google.auth
from google.api_core import exceptions
from google.cloud import secretmanager_v1


def access_secret(project_id: str, secret_id: str, version: str) -> str:
    client = secretmanager_v1.SecretManagerServiceClient()
    name = client.secret_version_path(project_id, secret_id, version)
    response = client.access_secret_version(request={"name": name})
    return response.payload.data.decode("utf-8")


def main():
    credentials, detected_project = google.auth.default()
    env_project = os.getenv("GCP_PROJECT")
    project_id = env_project or detected_project
    secret_id = os.getenv("SECRET_ID", "chat-app-fastapi--static-config")
    version = os.getenv("SECRET_VERSION", "latest")

    if not project_id:
        raise ValueError("Set GCP_PROJECT (or configure ADC with a default project).")

    print("Using credential type:", credentials.__class__.__name__)
    print("Using project:", project_id)
    print("Accessing secret:", secret_id, "version:", version)

    try:
        payload = access_secret(
            project_id=project_id, secret_id=secret_id, version=version
        )
        print(payload)
    except exceptions.PermissionDenied as err:
        print("Permission denied while accessing secret version.")
        print(f"Hint: verify roles/secret IAM for project '{project_id}'.")
        raise err
    except exceptions.NotFound as err:
        print("Secret or version not found.")
        print(
            f"Hint: confirm project='{project_id}', secret='{secret_id}', version='{version}'."
        )
        raise err


if __name__ == "__main__":
    main()
