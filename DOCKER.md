# Docker Development Guide for Zenith DAW

This guide explains how to use Docker for building and developing Zenith DAW in containerized environments.

## Table of Contents

1. [Getting Started](#getting-started)
2. [Basic Usage](#basic-usage)
3. [Development Workflow](#development-workflow)
4. [CI/CD Integration](#cicd-integration)
5. [Performance Considerations](#performance-considerations)
6. [Troubleshooting](#troubleshooting)

## Getting Started

### Prerequisites

- Docker 20.10+ or Docker Desktop 4.0+
- Docker Compose 2.0+
- At least 8GB RAM for the build container
- 20GB free disk space

### Quick Start

```bash
# Build the development image
docker-compose build

# Start a development shell
docker-compose up development

# Build the project
docker-compose up build

# Run the application
docker-compose up runtime
```

## Basic Usage

### Building the Project

```bash
# Build the project in a container
docker-compose build
docker-compose up build

# Or use a single command
docker run --rm -v $(pwd):/workspace zenith-daw-builder
```

### Development Environment

```bash
# Start development shell
docker-compose -f docker-compose.dev.yml up dev

# Build in development mode
docker-compose -f docker-compose.dev.yml up build-runner
```

### Running the Application

```bash
# Run the built application
docker-compose up runtime

# With audio forwarding (Linux)
PULSE_SERVER=host.docker.internal:4713 docker-compose up runtime
```

## Development Workflow

### 1. Project Setup

```bash
# Clone the repository
git clone https://github.com/zenith-daw/zenith.git
cd zenith

# Build development image
docker-compose -f docker-compose.dev.yml build
```

### 2. Development Session

```bash
# Start development container
docker-compose -f docker-compose.dev.yml up -d dev

# Connect to the container
docker-compose -f docker-compose.dev.yml exec dev bash

# Inside the container:
# - Code files are mounted at /workspace
# - Build directory at /workspace/build
# - Development tools are available
```

### 3. Making Changes

```bash
# Edit source code
vim apps/desktop/Source/Main.cpp

# Build changes
cd /workspace/build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .

# Run tests
ctest
```

### 4. Testing

```bash
# Run tests in CI mode
docker-compose -f docker-compose.yml up ci

# Run specific test
docker-compose -f docker-compose.yml run --rm ci bash -c "cd /workspace/build && ctest -R TestName"
```

## CI/CD Integration

### GitHub Actions Example

```yaml
name: Build with Docker
on: [push, pull_request]
jobs:
  build:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v3
    - name: Build Docker image
      run: docker-compose build
    - name: Build project
      run: docker-compose up build
    - name: Run tests
      run: docker-compose up ci
```

### Jenkins Pipeline

```groovy
pipeline {
    agent any
    stages {
        stage('Build') {
            steps {
                sh 'docker-compose build'
                sh 'docker-compose up build'
            }
        }
        stage('Test') {
            steps {
                sh 'docker-compose up ci'
            }
        }
    }
}
```

## Performance Considerations

### Build Performance

1. **Docker Daemon Storage**: Use fast storage (SSD) for Docker daemon
2. **Build Cache**: Docker layers provide caching for unchanged dependencies
3. **Memory**: Allocate at least 4GB RAM to containers for fast builds
4. **Parallel Jobs**: Use `-j$(nproc)` for optimal parallel builds

### File Performance

1. **Bind Mounts**: Use `consistent: cached` for source code mounts
2. **Network Storage**: Avoid network-mounted source code when possible
3. **Build Cache**: Keep build artifacts in a Docker volume for reuse

### Image Size

- **Base Image**: Ubuntu 22.04 provides good balance of features and size
- **Multi-Stage Builds**: Separate build and runtime images
- **Layer Optimization**: Combine related RUN commands
- **Cleanup**: Remove unnecessary files after package installation

## Troubleshooting

### Common Issues

#### Build Failures

```bash
# Check Docker resources
docker system df

# Clean up unused images
docker image prune -f

# Rebuild image
docker-compose build --no-cache
```

#### Volume Mount Issues

```bash
# Check volume permissions
ls -la build/

# Reset build directory
rm -rf build/
docker-compose build
```

#### Network Issues

```bash
# Check network connectivity
docker run --rm busybox ping google.com

# Restart Docker daemon
sudo systemctl restart docker
```

### Performance Issues

1. **Slow Builds**: Increase Docker memory limit in Docker Desktop
2. **File Access**: Use local filesystem instead of network shares
3. **Build Cache**: Keep intermediate builds for faster incremental builds

### Debug Mode

```bash
# Run interactive build
docker run --rm -it -v $(pwd):/workspace zenith-daw-builder bash

# Inspect image
docker run --rm -it zenith-daw-builder bash

# Check build logs
docker-compose logs build
```

## Advanced Usage

### Custom Build Arguments

```bash
# Pass custom build arguments
docker-compose build --build-arg CMAKE_BUILD_TYPE=Debug

# Or modify docker-compose.yml
services:
  build:
    build_args:
      CMAKE_BUILD_TYPE: Debug
```

### Multi-Architecture Builds

```bash
# Build for multiple platforms
docker buildx build --platform linux/amd64,linux/arm64 -t zenith-daw-builder .

# Build and push
docker buildx build --platform linux/amd64,linux/arm64 -t your-org/zenith-daw-builder .
```

### GPU Acceleration

```bash
# Enable GPU for Skia rendering
docker run --rm -it --gpus all \
  -v $(pwd):/workspace \
  zenith-daw-builder
```

### Development with GUI

```bash
# For Linux with X11 forwarding
docker run -it --rm \
  -e DISPLAY=$DISPLAY \
  -v /tmp/.X11-unix:/tmp/.X11-unix \
  zenith-daw-builder

# For macOS with XQuartz
xhost +local:docker
docker run -it --rm \
  -e DISPLAY=host.docker.internal:0 \
  zenith-daw-builder
```

## Contributing

When improving Docker support:

1. Test on multiple platforms (Linux, macOS, Windows)
2. Consider Windows-specific needs (WSL, Docker Desktop)
3. Update this documentation with any changes
4. Add CI tests for Docker builds

## References

- [Docker Documentation](https://docs.docker.com/)
- [Docker Compose Documentation](https://docs.docker.com/compose/)
- [Docker Best Practices](https://docs.docker.com/develop/dev-best-practices/)