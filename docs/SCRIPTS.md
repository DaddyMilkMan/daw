# Zenith DAW Build Scripts Documentation

This document provides comprehensive documentation for all build scripts and automation tools included in the Zenith DAW project.

## Table of Contents

1. [Overview](#overview)
2. [Build Scripts](#build-scripts)
3. [Prerequisites Checkers](#prerequisites-checkers)
4. [Docker Scripts](#docker-scripts)
5. [Utility Scripts](#utility-scripts)
6. [CI/CD Scripts](#cicd-scripts)
7. [Custom Build Profiles](#custom-build-profiles)
8. [Advanced Usage](#advanced-usage)

## Overview

The Zenith DAW project includes a comprehensive set of build scripts to make the build process as smooth as possible for contributors. These scripts are located in the `scripts/` directory and cover:

- Prerequisites checking
- Automated building
- Docker support
- Development workflows
- CI/CD integration

### Script Structure

```
scripts/
├── build.sh              # Linux/macOS build script
├── build.bat              # Windows build script
├── check-prerequisites.sh # Linux/macOS prerequisites checker
├── check-prerequisites.ps1 # Windows prerequisites checker
├── docker/               # Docker-related scripts
│   ├── build-and-test.sh
│   ├── run-dev.sh
│   └── ci-build.sh
├── ci/                   # CI/CD scripts
│   ├── build.sh
│   ├── test.sh
│   ├── deploy.sh
│   └── cleanup.sh
├── utils/               # Utility scripts
│   ├── clean.sh
│   ├── update-deps.sh
│   ├── format-code.sh
│   └── run-tests.sh
└── templates/            # Build templates
    ├── minimal-build.sh
    ├── debug-build.sh
    └── ci-build.sh
```

## Build Scripts

### `scripts/build.sh` (Linux/macOS)

The main build script for Linux and macOS systems.

#### Features

- **Automated dependency checking**
- **Parallel build support**
- **Multiple build profiles**
- **Clean build option**
- **Verbose mode**
- **Docker support**

#### Usage

```bash
# Basic build (Release)
./scripts/build.sh

# Debug build with tests
./scripts/build.sh --debug --tests

# Clean build with 8 parallel jobs
./scripts/build.sh --clean --jobs 8

# Install missing dependencies automatically
./scripts/build.sh --install

# Verbose build
./scripts/build.sh --verbose

# Build in Docker container
./scripts/build.sh --docker
```

#### Options

| Option | Description | Default |
|--------|-------------|---------|
| `-h, --help` | Show help message | - |
| `-c, --clean` | Clean build directory first | OFF |
| `-r, --release` | Build release version | ON |
| `-d, --debug` | Build debug version | OFF |
| `-t, --tests` | Build with tests | OFF |
| `-j, --jobs N` | Use N parallel jobs | All cores |
| `-v, --verbose` | Verbose build output | OFF |
| `--skip-prerequisites` | Skip prerequisites check | OFF |
| `--skip-deps` | Skip dependency fetching | OFF |
| `--install-deps` | Install missing dependencies | OFF |
| `--docker` | Build in Docker container | OFF |

#### Examples

```bash
# Development workflow
./scripts/build.sh --debug --tests --verbose

# Production build
./scripts/build.sh --clean --release --jobs 16

 CI build with Docker
./scripts/build.sh --docker --tests --jobs 8
```

#### Environment Variables

```bash
# Custom C++ compiler
export CXX=clang++

# Custom build directory
export BUILD_DIR=/custom/build/path

# Additional CMake paths
export CMAKE_PREFIX_PATH=/opt/special/lib
```

### `scripts/build.bat` (Windows)

The main build script for Windows systems.

#### Features

- **Visual Studio integration**
- **Ninja build support**
- **Automatic dependency checking**
- **Multiple build profiles**
- **Clean build option**

#### Usage

```cmd
:: Basic build
scripts\build.bat

:: Debug build with tests
scripts\build.bat --debug --tests

:: Clean build
scripts\build.bat --clean

:: Install dependencies (requires vcpkg)
scripts\build.bat --install-deps

:: Verbose build
scripts\build.bat --verbose
```

#### Requirements

- Visual Studio 2022 with C++ workload
- CMake 3.25+
- Ninja build system
- vcpkg (optional, for dependency management)

## Prerequisites Checkers

### `scripts/check-prerequisites.sh` (Linux/macOS)

Comprehensive checker for Linux and macOS systems.

#### Features

- **Dependency version checking**
- **Automated installation**
- **Platform-specific checks**
- **Detailed error reporting**
- **Helpful installation instructions**

#### Usage

```bash
# Check prerequisites
./scripts/check-prerequisites.sh

# Check and install missing dependencies
./scripts/check-prerequisites.sh --install

# Skip version compatibility tests
./scripts/check-prerequisites.sh --skip-tests

# Show detailed version information
./scripts/check-prerequisites.sh --verbose
```

#### Checks Performed

| Check | Minimum Version | Notes |
|-------|----------------|-------|
| CMake | 3.25 | Build system |
| Ninja | 1.10 | Build generator |
| GCC | 12.0 | C++20 support |
| Clang | 14.0 | C++20 support |
| Git | - | Version control |
| ALSA | - | Audio (Linux) |
| JACK | - | Audio (Linux) |
| X11 | - | Graphics (Linux) |
| Xcode CLI | - | macOS toolchain |

### `scripts/check-prerequisites.ps1` (Windows)

PowerScript-based checker for Windows systems.

#### Features

- **Visual Studio integration**
- **Windows SDK checking**
- **PowerShell-based checks**
- **Chocolatey/Winget integration**
- **Detailed error reporting**

#### Usage

```powershell
# Check prerequisites
.\scripts\check-prerequisites.ps1

# Check and install missing dependencies
.\scripts\check-prerequisites.ps1 -Install

# Show detailed information
.\scripts\check-prerequisites.ps1 -Verbose
```

#### Requirements

- PowerShell 5.1+
- Administrator privileges for installation
- Windows 10 or later

## Docker Scripts

### Docker Compose Configuration

#### `docker-compose.yml`

Main Docker configuration for production builds.

**Services:**
- `build`: Build service for the project
- `development`: Development environment
- `runtime`: Runtime environment
- `ci`: CI/CD pipeline service
- `cache`: Dependency caching service

**Usage:**
```bash
# Build project
docker-compose up build

# Development shell
docker-compose up development

# Run application
docker-compose up runtime

# CI build with tests
docker-compose up ci
```

#### `docker-compose.dev.yml`

Development-focused Docker configuration.

**Features:**
- Hot-reloading support
- Development tools pre-installed
- Persistent build cache
- Network optimization

**Usage:**
```bash
# Development workflow
docker-compose -f docker-compose.dev.yml up dev

# Build in development mode
docker-compose -f docker-compose.dev.yml up build-runner
```

### Docker Build Profiles

#### Build Service
```yaml
services:
  build:
    build:
      context: .
      dockerfile: Dockerfile
      target: build
    volumes:
      - type: bind
        source: .
        target: /workspace
    command: cmake . && cmake --build . -j$(nproc)
```

#### Development Service
```yaml
services:
  development:
    build:
      context: .
      dockerfile: Dockerfile
      target: development
    volumes:
      - type: bind
        source: .
        target: /workspace
        consistency: cached
    command: bash
    stdin_open: true
    tty: true
```

## CI/CD Scripts

### `ci/build.sh`

CI build script for automated builds.

**Features:**
- Matrix builds for multiple configurations
- Automated dependency management
- Build caching
- Error handling and reporting

```bash
# Basic CI build
./ci/build.sh

# Build with specific configuration
./ci/build.sh --config Debug --tests

# Build with caching
./ci/build.sh --cache
```

### `ci/test.sh`

CI test runner script.

**Features:**
- Parallel test execution
- Test result formatting
- Coverage reporting
- JUnit XML output

```bash
# Run all tests
./ci/test.sh

# Run specific test suite
./ci/test.sh --suite Core

# Run with coverage
./ci/test.sh --coverage
```

### `ci/deploy.sh`

CI deployment script.

**Features:**
- Artifact management
- Release tagging
- Package creation
- Upload to repositories

```bash
# Create packages
./ci/deploy.sh --package

# Release version
./ci/deploy.sh --release 1.0.0

# Upload artifacts
./ci/deploy.sh --upload
```

## Utility Scripts

### `utils/clean.sh`

Clean build artifacts.

```bash
# Clean build directory
./utils/clean.sh

# Clean all build artifacts
./utils/clean.sh --all

# Clean dependencies
./utils/clean.sh --deps
```

### `utils/update-deps.sh`

Update project dependencies.

```bash
# Update all dependencies
./utils/update-deps.sh

# Update specific dependency
./utils/update-deps.sh --dep juce

# Update and test
./utils/update-deps.sh --test
```

### `utils/format-code.sh`

Format code according to project standards.

```bash
# Format all code
./utils/format-code.sh

# Format specific files
./utils/format-code.sh --file apps/desktop/Source/*.cpp

# Check formatting
./utils/format-code.sh --check
```

### `utils/run-tests.sh`

Run tests with custom options.

```bash
# Run unit tests
./utils/run-tests.sh --unit

# Run integration tests
./utils/run-tests.sh --integration

# Run with specific filter
./utils/run-tests.sh --filter "Audio*"
```

## Build Templates

### `templates/minimal-build.sh`

Minimal build for CI systems.

**Features:**
- Only essential components
- Fast build times
- Minimal dependencies

```bash
#!/bin/bash
./scripts/build.sh --clean --release --jobs 2
```

### `templates/debug-build.sh`

Debug build with full instrumentation.

**Features:**
- Debug symbols
- AddressSanitizer
- LeakSanitizer
- Full test suite

```bash
#!/bin/bash
./scripts/build.sh --debug --tests --jobs 4
```

### `templates/ci-build.sh`

CI build matrix script.

**Features:**
- Multiple build configurations
- Parallel builds
- Test matrix
- Coverage reports

```bash
#!/bin/bash
# Build matrix for CI
builds=(
    "Debug --tests"
    "Release --release"
    "RelWithDebInfo --debug-symbols"
)

for build in "${builds[@]}"; do
    IFS=' ' read -ra config <<< "$build"
    ./scripts/build.sh "${config[@]}"
done
```

## Advanced Usage

### Custom Build Profiles

Create custom build profiles by combining options:

```bash
# Audio-focused build
./scripts/build.sh \
    --debug \
    --tests \
    --jobs 8 \
    --verbose

# Performance testing build
./scripts/build.sh \
    --release \
    --jobs $(nproc) \
    --no-symbols \
    --optimize

# Development build with hot-reloading
./scripts/build.sh \
    --debug \
    --deps \
    --verbose
```

### Docker Development Workflow

```bash
# 1. Start development container
docker-compose -f docker-compose.dev.yml up -d dev

# 2. Connect to container
docker-compose -f docker-compose.dev.yml exec dev bash

# 3. Make changes and build
vim apps/desktop/Source/Main.cpp
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .

# 4. Run tests
ctest

# 5. Stop container
docker-compose -f docker-compose.dev.yml down
```

### CI/CD Pipeline Integration

```yaml
# .github/workflows/build.yml
name: Build Zenith DAW

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    strategy:
      matrix:
        config:
          - {build_type: Release, tests: false}
          - {build_type: Debug, tests: true}

    steps:
    - uses: actions/checkout@v3
    - name: Check prerequisites
      run: ./scripts/check-prerequisites.sh --skip-tests
    - name: Build
      run: ./scripts/build.sh --${{ matrix.config.build_type }} --tests=${{ matrix.config.tests }}
    - name: Test
      if: matrix.config.tests
      run: ctest --output-on-failure
```

### Build Automation

#### Git Hooks

```bash
# .git/hooks/pre-commit
#!/bin/bash
# Run build before commit
./scripts/build.sh --debug --tests
if [ $? -ne 0 ]; then
    echo "Build failed. Commit aborted."
    exit 1
fi
```

#### Makefile Integration

```makefile
# Makefile
.PHONY: build test clean debug release install-deps

build:
    ./scripts/build.sh

test:
    ./scripts/build.sh --tests
    ctest --output-on-failure

debug:
    ./scripts/build.sh --debug --tests

release:
    ./scripts/build.sh --release --clean

clean:
    ./utils/clean.sh --all

install-deps:
    ./scripts/check-prerequisites.sh --install
```

## Troubleshooting

### Common Issues

1. **Script permission denied**
   ```bash
   chmod +x scripts/*.sh
   ```

2. **Docker build fails**
   ```bash
   docker-compose down -v --remove-orphans
   docker-compose build --no-cache
   ```

3. **CI build timeout**
   ```bash
   # Use parallel jobs
   ./scripts/build.sh --jobs 4
   ```

4. **Missing tools**
   ```bash
   ./scripts/check-prerequisites.sh --install
   ```

### Debug Mode

Enable debug mode for detailed logging:

```bash
# Enable bash debug mode
bash -x ./scripts/build.sh

# Enable script debug mode
./scripts/build.sh --verbose
```

### Logging

All scripts provide detailed logging:

```bash
# Save build log
./scripts/build.sh > build.log 2>&1

# Save prerequisites check
./scripts/check-prerequisites.sh > check.log 2>&1
```

## Contributing

When adding new scripts:

1. **Follow naming conventions**
   - Use kebab-case for shell scripts
   - Use PascalCase for PowerShell scripts
   - Use snake_case for internal functions

2. **Include documentation**
   - Add usage examples
   - Document all options
   - Include exit codes

3. **Add error handling**
   - Check for required tools
   - Provide meaningful error messages
   - Handle edge cases

4. **Test thoroughly**
   - Test on all target platforms
   - Test with different configurations
   - Test in CI environments

## Security Considerations

1. **Script Permissions**
   - Only execute scripts from trusted sources
   - Regular audit of script permissions

2. **Dependency Verification**
   - Verify checksums of downloaded dependencies
   - Use trusted sources for package repositories

3. **Build Security**
   - Build in isolated containers when possible
   - Scan build artifacts for security issues

4. **Environment Variables**
   - Validate environment variables before use
   - Don't log sensitive information

## Performance Optimization

1. **Build Cache**
   - Use Docker layers for dependency caching
   - Keep build artifacts between runs

2. **Parallel Processing**
   - Use all available CPU cores
   - Balance parallel jobs with memory constraints

3. **Dependency Management**
   - Use system packages when possible
   - Minimize dependency fetching during builds

---

For more information about building Zenith DAW, see:

- [BUILDING.md](BUILDING.md) - Comprehensive build guide
- [BUILD_ISSUES.md](BUILD_ISSUES.md) - Troubleshooting guide
- [DOCKER.md](DOCKER.md) - Docker development guide