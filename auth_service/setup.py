from setuptools import setup, find_packages
from pathlib import Path


# Read requirements.txt
def read_requirements(filename: str):
    return [
        line.strip()
        for line in Path(filename).read_text().split("\n")
        if line.strip() and not line.startswith("#")
    ]


setup(
    name="app",
    version="0.1.0",
    author="Haffi Mazhar",
    author_email="haffimazhar96@gmail.com",
    description="Simple Auth Service",
    url="https://github.com/haffi96/gcp-playground",
    packages=find_packages(),
    classifiers=[
        "Programming Language :: Python :: 3.11",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
    ],
    python_requires=">=3.11",
    install_requires=read_requirements("requirements.txt"),
)
