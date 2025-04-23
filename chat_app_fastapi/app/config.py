import os


class Config:
    AUTH_SERVICE_URL = os.getenv("AUTH_SERVICE_URL", "http://127.0.0.1:8000")

    STATIC_CONFIG_PATH = os.getenv(
        "STATIC_CONFIG_PATH", "/etc/chat_app/static_config.yaml"
    )


APP_CONFIG = Config()
