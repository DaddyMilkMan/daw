# =============================================================================
# Dockerfile for Zenith DAW Build Environment
# =============================================================================
# This Dockerfile provides a complete build environment for Zenith DAW
# with all necessary dependencies pre-installed.
#
# Usage:
#   docker build -t zenith-daw-builder -f Dockerfile .
#   docker run --rm -v $(pwd):/workspace zenith-daw-builder
#
# Or with docker-compose:
#   docker-compose build
#   docker-compose up build
# =============================================================================

# Base image with Ubuntu 22.04 and common build tools
FROM ubuntu:22.04 AS base

# Environment variables
ENV DEBIAN_FRONTEND=noninteractive
ENV LANG=C.UTF-8
ENV LC_ALL=C.UTF-8

# Install system dependencies
RUN apt-get update && apt-get install -y \
    # Build essentials
    build-essential \
    cmake \
    ninja-build \
    git \
    gcc \
    g++ \
    # Audio development libraries
    libasound2-dev \
    libjack-jackd2-dev \
    # Graphics and UI libraries
    libx11-dev \
    libxinerama-dev \
    libxext-dev \
    libxrandr-dev \
    libxcursor-dev \
    libwebkit2gtk-4.0-dev \
    libglu1-mesa-dev \
    mesa-common-dev \
    libfreetype6-dev \
    # Network and cryptography
    libcurl4-openssl-dev \
    libssl-dev \
    # Development tools
    pkg-config \
    autoconf \
    automake \
    libtool \
    # Clean up
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /workspace

# Create build directory
RUN mkdir -p build

# Multi-stage build for optimized final image
FROM base AS build

# Copy project files
COPY . /workspace/

# Build the project
RUN cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build -j$(nproc)

# Production image with minimal dependencies
FROM ubuntu:22.04 AS runtime

# Install minimal runtime dependencies
RUN apt-get update && apt-get install -y \
    libasound2 \
    libjack-jackd2 \
    libx11-6 \
    libxinerama1 \
    libxext6 \
    libxrandr2 \
    libxcursor1 \
    libwebkit2gtk-4.0-37 \
    libglu1-mesa \
    libgl1-mesa-glx \
    libfreetype6 \
    libcurl4 \
    libssl3 \
    # Clean up
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Copy built application from build stage
COPY --from=build /workspace/build/Zenith\ DAW /usr/local/bin/zenith-daw

# Create non-root user for security
RUN useradd -m -u 1000 zenith \
    && chown -R zenith:zenith /usr/local/bin/zenith-daw

# Switch to non-root user
USER zenith

# Set entrypoint
ENTRYPOINT ["/usr/local/bin/zenith-daw"]

# Development image with development tools
FROM base AS development

# Copy project files
COPY . /workspace/

# Set default command
CMD ["bash"]

# Development helper labels
LABEL maintainer="Zenith DAW Team"
LABEL description="Zenith DAW Development Environment"
LABEL version="1.0.0"

# Additional development tools
RUN apt-get update && apt-get install -y \
    gdb \
    valgrind \
    strace \
    ltrace \
    wget \
    curl \
    vim \
    less \
    # Clean up
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Development environment variables
ENV ENVIRONMENT=development
ENV PATH="/workspace:$PATH"

# Development commands
CMD ["bash"]