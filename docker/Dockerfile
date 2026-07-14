# ==============================================================================
# RedroidCPP - Docker-based Android Emulator
# Professional Android Emulator with Device Profile Integration
# ==============================================================================

# Build stage for C++ CLI tool
FROM alpine:3.19 AS builder

WORKDIR /build

# Install build dependencies
RUN apk add --no-cache \
    cmake \
    make \
    g++ \
    linux-headers

# Copy project files
COPY ../CMakeLists.txt ./CMakeLists.txt
COPY ../include/ ./include/
COPY ../src/ ./src/

# Build CLI tool
RUN cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=17 . && \
    make -j$(nproc)

# ==============================================================================
# Final Emulator Image
# ==============================================================================
FROM ghcr.io/redroid/redroid:15.0.0_google_64only

# Labels
LABEL maintainer="mostakimnasim5"
LABEL description="RedroidCPP - Docker-based Android Emulator with Device Profile"
LABEL version="3.0.0"

# Environment variables
ENV ANDROID_HOME=/system
ENV PATH=$PATH:$ANDROID_HOME/bin:/opt/bin
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
    curl \
    && rm -rf /var/lib/apt/lists/* \
    && apt-get clean

# Copy provisioning scripts
COPY bin/ /opt/bin/
COPY configs/ /opt/configs/

# Make scripts executable
RUN chmod +x /opt/bin/*.sh

# Create directories
RUN mkdir -p /data/profiles /data/devices /var/log

# Copy CLI tool from builder
COPY --from=builder /build/redroid-cli /opt/bin/redroid-cli
RUN chmod +x /opt/bin/redroid-cli

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=60s --retries=3 \
    CMD /opt/bin/redroid-cli status 2>/dev/null || exit 1

WORKDIR /data

ENTRYPOINT ["/opt/bin/entrypoint.sh"]
CMD ["start"]

# Exposed ports
EXPOSE 15555 5555 5900

# Volume for persistent data
VOLUME ["/data"]
