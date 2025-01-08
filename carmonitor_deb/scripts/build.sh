!/bin/bash
set -e

echo "🚀 Starting build process..."

# Create package build structure
echo "📁 Creating package structure..."
mkdir -p ./usr/share/carmonitor/

# Copy development files to package structure
echo "📋 Copying files to package structure..."
cp setup.py ./usr/share/carmonitor/
cp -r carmonitor/ ./usr/share/carmonitor/

# Create necessary directories for test secrets
echo "📁 Setting up test secrets..."
mkdir -p secrets
echo "test_api_key" >secrets/api_key
echo "test_secret" >secrets/other_secret
chmod 600 secrets/*

# Verify project structure before building
echo "🔍 Verifying project structure..."
if [ ! -d "./usr/share/carmonitor" ]; then
    echo "Error: Required directory structure not found!"
    echo "Expected: ./usr/share/carmonitor/"
    echo "Please set up the directory structure correctly."
    exit 1
fi

if [ ! -f "./usr/share/carmonitor/setup.py" ]; then
    echo "Error: setup.py not found in the correct location!"
    exit 1
fi

# Build the Debian package
echo "📦 Building Debian package..."
dpkg-deb --build . carmonitor_1.0.0_all.deb

# Verify package was created
if [ ! -f "carmonitor_1.0.0_all.deb" ]; then
    echo "Error: Package build failed!"
    exit 1
fi

# Build Docker image
echo "🐳 Building Docker image..."
docker build -t carmonitor-test .

# Run container
echo "🚀 Starting test container..."
# docker run --privileged -d --name carmonitor-container carmonitor-test
docker run --privileged -d --name carmonitor-container carmonitor-test /lib/systemd/systemd

# Wait a moment for container to start
sleep 2

# Install the Debian package
echo "📦 Installing Debian package..."
docker exec carmonitor-container dpkg -i /app/carmonitor_1.0.0_all.deb || apt-get install -f -y

# Wait a moment for package installation and service to start
sleep 2

# Check installation and service status
echo "📋 Checking installation status..."
docker exec carmonitor-container dpkg -l | grep carmonitor
echo "🔍 Checking Python package installation..."
docker exec carmonitor-container python3 -c "import carmonitor" || echo "Warning: Package import failed!"
echo "📊 Checking service status..."
docker exec carmonitor-container systemctl status carmonitor || true

echo "✅ Build and deployment complete!"
echo "📝 To view systemd service logs:"
echo "  docker exec carmonitor-container journalctl -u carmonitor"
echo "🔧 To debug:"
echo "  docker exec -it carmonitor-container bash"
