import os


class Config:
    AUTH_SERVICE_URL = os.getenv("AUTH_SERVICE_URL", "http://127.0.0.1:9000/auth")


APP_CONFIG = Config()
