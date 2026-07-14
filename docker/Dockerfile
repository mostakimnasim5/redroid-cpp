# =============================================================================
# ReDroidCPP - Android Emulator Dockerfile
# Base: Ubuntu 22.04 with OpenJDK 17, Android SDK, and Emulator
# =============================================================================

FROM ubuntu:22.04

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# =============================================================================
# Install System Dependencies
# =============================================================================
RUN apt-get update && apt-get install -y \
    openjdk-17-jdk \
    wget \
    unzip \
    curl \
    python3 \
    libx11-6 \
    libxext6 \
    libxrender1 \
    libxrandr2 \
    libxi6 \
    libgl1-mesa-glx \
    libegl1-mesa \
    libopengl0 \
    libpulse0 \
    libglu1-mesa \
    && rm -rf /var/lib/apt/lists/* \
    && apt-get clean

# =============================================================================
# Set Environment Variables
# =============================================================================
ENV ANDROID_HOME=/opt/android-sdk
ENV ANDROID_SDK_ROOT=/opt/android-sdk
ENV ANDROID_AVD_HOME=/root/.android/avd
ENV JAVA_HOME=/usr/lib/jvm/java-17-openjdk-amd64
ENV PATH="${PATH}:${ANDROID_HOME}/cmdline-tools/latest/bin:${ANDROID_HOME}/platform-tools:${ANDROID_HOME}/emulator"

# Create AVD home directory
RUN mkdir -p ${ANDROID_AVD_HOME}

# =============================================================================
# Download and Install Android Command Line Tools
# =============================================================================
RUN mkdir -p ${ANDROID_HOME}/cmdline-tools \
    && cd ${ANDROID_HOME}/cmdline-tools \
    && wget -q https://dl.google.com/android/repository/commandlinetools-linux-11076708_latest.zip -O cmdline-tools.zip \
    && unzip -q cmdline-tools.zip \
    && mv cmdline-tools latest \
    && rm cmdline-tools.zip

# =============================================================================
# Accept All SDK Licenses
# =============================================================================
RUN yes | sdkmanager --licenses --sdk_root=${ANDROID_SDK_ROOT}

# =============================================================================
# Install Platform Tools and Build Tools
# =============================================================================
RUN sdkmanager --sdk_root=${ANDROID_SDK_ROOT} \
    "platform-tools" \
    "platforms;android-34" \
    "build-tools;34.0.0"

# =============================================================================
# Install System Image
# =============================================================================
RUN sdkmanager --sdk_root=${ANDROID_SDK_ROOT} \
    "system-images;android-34;google_apis;x86_64"

# =============================================================================
# Create AVD (Android Virtual Device)
# =============================================================================
RUN echo "no" | avdmanager create avd \
    --sdk_root=${ANDROID_SDK_ROOT} \
    --name AndroidEmulator \
    --package "system-images;android-34;google_apis;x86_64" \
    --path ${ANDROID_AVD_HOME}/AndroidEmulator.avd \
    --force

# Create config.ini for the AVD with proper paths
RUN echo "avd.ini.displayname=AndroidEmulator" > ${ANDROID_AVD_HOME}/AndroidEmulator.avd/config.ini && \
    echo "avd.ini.usesystempartitions=yes" >> ${ANDROID_AVD_HOME}/AndroidEmulator.avd/config.ini

# =============================================================================
# Download Android Emulator
# =============================================================================
RUN sdkmanager --sdk_root=${ANDROID_SDK_ROOT} \
    "emulator"

# =============================================================================
# Copy entrypoint script
# =============================================================================
COPY docker/entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

# =============================================================================
# Cleanup - Remove apt cache to keep image small
# =============================================================================
RUN rm -rf /var/cache/apt/archives/* \
    && rm -rf /tmp/* \
    && rm -rf /var/tmp/*

# =============================================================================
# Working Directory
# =============================================================================
WORKDIR /workspace

# =============================================================================
# Default Command
# =============================================================================
CMD ["/bin/bash"]
