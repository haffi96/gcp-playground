FROM debian:bullseye-slim

# Install basic requirements
RUN apt-get update && apt-get install -y \
    python3 \
    python3-pip \
    python3-setuptools \
    systemd \
    && rm -rf /var/lib/apt/lists/*

# Create directories for the application
RUN mkdir -p /app /etc/carmonitor/secrets

# Copy the Debian package
COPY carmonitor_1.0.0_all.deb /app/

# Copy secrets (for testing only - in production use secure methods)
COPY secrets/ /etc/carmonitor/secrets/
RUN chmod 600 /etc/carmonitor/secrets/*


# # Install the package
# RUN dpkg -i /app/carmonitor_1.0.0_all.deb || apt-get install -f -y

# # For testing, we'll start the service manually
# CMD ["car-monitor"]