#!/bin/bash
echo "🧹 Cleaning up..."

# Stop container if running
if docker ps -q -f name=carmonitor-container; then
    echo "Stopping container..."
    docker stop carmonitor-container
fi

# Remove container if exists
if docker ps -aq -f name=carmonitor-container; then
    echo "Removing container..."
    docker rm carmonitor-container
fi

# Remove files
echo "Removing test files..."
rm -rf secrets/
rm -f carmonitor_1.0.0_all.deb

echo "✅ Cleanup complete!"
