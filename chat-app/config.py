import os


class Config:
    REDIS_URL = os.getenv("REDIS_URL", "redis://10.89.162.227:6379")

    AUTH_SERVICE_URL = os.getenv("AUTH_SERVICE_URL", "http://127.0.0.1:9000/auth")


APP_CONFIG = Config()
