import os


class Config:
    REDIS_URL = os.getenv("REDIS_URL", "redis://localhost:6379")

    RABBITMQ_URL = os.getenv("RABBITMQ_URL", "amqp://guest:guest@localhost:5672/")

    AUTH_SERVICE_URL = os.getenv("AUTH_SERVICE_URL", "http://127.0.0.1:9000/auth")


APP_CONFIG = Config()
