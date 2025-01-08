#!/usr/bin/env python3
import logging
import yaml
import os
import sys
from pathlib import Path
import time


logging.basicConfig(level=logging.INFO)
logger = logging.getLogger("carmonitor.main")


def load_config():
    config_path = os.environ.get("CONFIG_PATH", "/etc/carmonitor/config.yaml")
    secrets_path = os.environ.get("SECRETS_PATH", "/etc/carmonitor/secrets")

    with open(config_path) as f:
        config = yaml.safe_load(f)

    # Load secrets
    secrets_dir = Path(secrets_path)
    secrets = {}
    for secret_file in secrets_dir.glob("*"):
        with open(secret_file) as f:
            secrets[secret_file.name] = f.read().strip()

    return config, secrets


def main():
    config, secrets = load_config()

    logger.info("carmonitor is running...")
    logger.info(f"Config: {config}")
    logger.info(f"Secrets: {secrets}")
    # Your main program logic here
    while True:
        try:
            logger.info("Monitoring car status...")
            time.sleep(5)
        except Exception as e:
            print(f"Error: {e}", file=sys.stderr)
            sys.exit(1)


if __name__ == "__main__":
    main()
