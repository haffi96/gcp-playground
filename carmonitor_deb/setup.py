from setuptools import setup, find_packages

setup(
    name="carmonitor",
    version="1.0.0",
    packages=find_packages(),
    install_requires=[
        "pyyaml>=6.0",
        # other dependencies
    ],
    entry_points={
        "console_scripts": [
            "car-monitor=carmonitor.main:main",
        ],
    },
)
