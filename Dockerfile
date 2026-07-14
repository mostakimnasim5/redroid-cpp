# ==============================================================================
# RedroidCPP - Docker-based Android Emulator
# Professional Android Emulator with Device Profile Integration
# ==============================================================================

# Stage 1: Build the C++ CLI tool
FROM alpine:3.19 AS builder

# Install build dependencies
RUN apk add --no-cache \
    cmake \
    make \
    g++ \
    linux-headers \
    boost-dev

WORKDIR /build

# Copy source files
COPY CMakeLists.txt ./
COPY include/ ./include/
COPY src/ ./src/

# Build
RUN cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=17 . && \
    make -j$(nproc)

# ==============================================================================
# Stage 2: Redroid Base Image with Device Profile Support
# ==============================================================================
FROM ghcr.io/redroid/redroid:15.0.0_google_64only AS redroid-base

# Install runtime dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    bash \
    coreutils \
    util-linux \
    procps \
    nano \
    curl \
    wget \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# ==============================================================================
# Stage 3: Final Emulator Image
# ==============================================================================
FROM debian:12-slim

# Labels
LABEL maintainer="mostakimnasim5"
LABEL description="RedroidCPP - Docker-based Android Emulator with Device Profile"
LABEL version="3.0.0"

# Environment variables
ENV ANDROID_HOME=/system
ENV PATH=$PATH:$ANDROID_HOME/bin:/opt/redroid-cli
ENV REDROID_VERSION=15.0.0

# Install runtime dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    bash \
    coreutils \
    util-linux \
    procps \
    ca-certificates \
    openssl \
    udev \
    && rm -rf /var/lib/apt/lists/*

# Copy redroid init script
COPY docker/init.sh /opt/redroid-init.sh
RUN chmod +x /opt/redroid-init.sh

# Copy compiled CLI tool from builder
COPY --from=builder /build/redroid-cli /opt/redroid-cli

# Copy device profile directory
COPY profiles/ /opt/profiles/

# Create device directories
RUN mkdir -p /data/profiles /data/devices /var/log

# Copy rootfs from redroid base (requires multi-stage from redroid image)
# Note: In production, use the redroid container as base directly

# Entrypoint script
COPY docker/entrypoint.sh /opt/entrypoint.sh
RUN chmod +x /opt/entrypoint.sh

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=60s --retries=3 \
    CMD /opt/redroid-cli status || exit 1

WORKDIR /data

ENTRYPOINT ["/opt/entrypoint.sh"]
CMD ["start"]

# Exposed ports
# 15555: ADB
# 5555: ADB over network
# 5900: VNC (optional)
EXPOSE 15555 5555 5900

# Volume for persistent data
VOLUME ["/data"]
