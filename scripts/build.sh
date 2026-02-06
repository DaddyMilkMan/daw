#!/bin/bash

# =============================================================================
# Zenith DAW Build Script for Linux/macOS
# =============================================================================
# This script automates the build process for Zenith DAW with error checking
# and progress reporting.
#
# Usage:
#   ./build.sh [options]
#
# Options:
#   -h, --help          Show this help message
#   -c, --clean         Clean build directory before building
#   -r, --release       Build release version (default)
#   -d, --debug         Build debug version
#   -t, --tests         Build with tests
#   -j, --jobs N        Use N parallel jobs (default: all cores)
#   -v, --verbose       Verbose build output
#   --skip-prerequisites Skip prerequisites check
#   --skip-deps         Skip dependency fetching
#   --install-deps      Install missing dependencies
#   --docker            Build in Docker container
#
# Examples:
#   ./build.sh                    # Release build
#   ./build.sh --clean --debug    # Clean debug build
#   ./build.sh --tests --jobs 8   # Build with tests, 8 jobs
# =============================================================================

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Script configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../" && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
CMAKE_MIN_VERSION="3.25"
NINJA_MIN_VERSION="1.10"

# Default build options
BUILD_TYPE="Release"
BUILD_TESTS="OFF"
JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
CLEAN_BUILD="OFF"
VERBOSE="OFF"
SKIP_PREREQS="OFF"
SKIP_DEPS="OFF"
INSTALL_DEPS="OFF"
DOCKER_BUILD="OFF"

# Parse command line arguments
parse_args() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_help
                exit 0
                ;;
            -c|--clean)
                CLEAN_BUILD="ON"
                shift
                ;;
            -r|--release)
                BUILD_TYPE="Release"
                shift
                ;;
            -d|--debug)
                BUILD_TYPE="Debug"
                shift
                ;;
            -t|--tests)
                BUILD_TESTS="ON"
                shift
                ;;
            -j|--jobs)
                JOBS="$2"
                shift 2
                ;;
            -v|--verbose)
                VERBOSE="ON"
                shift
                ;;
            --skip-prerequisites)
                SKIP_PREREQS="ON"
                shift
                ;;
            --skip-deps)
                SKIP_DEPS="ON"
                shift
                ;;
            --install-deps)
                INSTALL_DEPS="ON"
                shift
                ;;
            --docker)
                DOCKER_BUILD="ON"
                shift
                ;;
            *)
                echo -e "${RED}Error: Unknown option '$1'${NC}"
                show_help
                exit 1
                ;;
        esac
    done
}

# Show help message
show_help() {
    cat << EOF
Zenith DAW Build Script for Linux/macOS

This script automates the build process for Zenith DAW with error checking
and progress reporting.

Usage:
    $0 [options]

Options:
    -h, --help              Show this help message
    -c, --clean             Clean build directory before building
    -r, --release           Build release version (default)
    -d, --debug             Build debug version
    -t, --tests             Build with tests
    -j, --jobs N            Use N parallel jobs (default: all cores)
    -v, --verbose           Verbose build output
    --skip-prerequisites    Skip prerequisites check
    --skip-deps            Skip dependency fetching
    --install-deps         Install missing dependencies
    --docker               Build in Docker container

Examples:
    $0                      # Release build
    $0 --clean --debug     # Clean debug build
    $0 --tests --jobs 8    # Build with tests, 8 jobs

Environment Variables:
    CC                      C compiler to use
    CXX                     C++ compiler to use
    CMAKE_PREFIX_PATH       Additional CMake prefix paths
    NINJA_PATH              Path to ninja executable (if not in PATH)

Prerequisites:
    - CMake $CMAKE_MIN_VERSION+
    - Ninja build system $NINJA_MIN_VERSION+
    - C++20 compatible compiler (GCC 12+, Clang 14+)
    - Build tools (make, gcc/clang)
    - Audio libraries (libasound2-dev, libjack-jackd2-dev for Linux)
EOF
}

# Print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Check version
check_version() {
    local version="$1"
    local required="$2"
    local program="$3"

    # Convert version strings to comparable format
    local version_clean=$(echo "$version" | sed 's/[^0-9.]//g' | cut -d. -f1,2)
    local required_clean=$(echo "$required" | sed 's/[^0-9.]//g' | cut -d. -f1,2)

    # Compare versions
    if [ "$(printf '%s\n' "$required_clean" "$version_clean" | sort -V | head -n1)" = "$required_clean" ]; then
        return 0
    else
        return 1
    fi
}

# Check prerequisites
check_prerequisites() {
    if [ "$SKIP_PREREQS" = "ON" ]; then
        print_status "Skipping prerequisites check"
        return 0
    fi

    print_status "Checking prerequisites..."

    # Check CMake
    if ! command_exists cmake; then
        print_error "CMake not found. Please install CMake $CMAKE_MIN_VERSION+"
        if [ "$INSTALL_DEPS" = "ON" ]; then
            install_dependencies
        else
            exit 1
        fi
    else
        cmake_version=$(cmake --version | head -n1 | cut -d" " -f3)
        if ! check_version "$cmake_version" "$CMAKE_MIN_VERSION" "CMake"; then
            print_error "CMake version $cmake_version found, but $CMAKE_MIN_VERSION+ is required"
            if [ "$INSTALL_DEPS" = "ON" ]; then
                install_dependencies
            else
                exit 1
            fi
        else
            print_success "CMake $cmake_version found"
        fi
    fi

    # Check Ninja
    if ! command_exists ninja; then
        if command_exists ninja-build; then
            # Create symlink for ninja-build to ninja
            if [ ! -L /usr/local/bin/ninja ]; then
                sudo ln -sf $(which ninja-build) /usr/local/bin/ninja
            fi
        else
            print_error "Ninja not found. Please install Ninja $NINJA_MIN_VERSION+"
            if [ "$INSTALL_DEPS" = "ON" ]; then
                install_dependencies
            else
                exit 1
            fi
        fi
    else
        ninja_version=$(ninja --version | head -n1)
        if ! check_version "$ninja_version" "$NINJA_MIN_VERSION" "Ninja"; then
            print_error "Ninja version $ninja_version found, but $NINJA_MIN_VERSION+ is required"
            exit 1
        else
            print_success "Ninja $ninja_version found"
        fi
    fi

    # Check C++ compiler
    if [ -n "$CXX" ]; then
        compiler="$CXX"
    else
        compiler="g++"
        if ! command_exists "$compiler"; then
            compiler="clang++"
        fi
    fi

    if ! command_exists "$compiler"; then
        print_error "C++ compiler not found. Please install a C++20 compatible compiler"
        exit 1
    else
        compiler_version=$($compiler --version | head -n1 | grep -oE '[0-9]+\.[0-9]+' | head -n1)
        print_success "C++ compiler ($compiler) version $compiler_version found"
    fi

    # Check platform-specific dependencies
    if [ "$(uname)" = "Linux" ]; then
        print_status "Checking Linux dependencies..."
        check_linux_dependencies
    fi
}

# Check Linux dependencies
check_linux_dependencies() {
    local missing=()

    # Essential audio libraries
    if ! dpkg -l libasound2-dev >/dev/null 2>&1; then
        missing+=("libasound2-dev")
    fi

    if ! dpkg -l libjack-jackd2-dev >/dev/null 2>&1; then
        missing+=("libjack-jackd2-dev")
    fi

    # Graphics libraries
    if ! dpkg -l libx11-dev >/dev/null 2>&1; then
        missing+=("libx11-dev")
    fi

    if ! dpkg -l libxinerama-dev >/dev/null 2>&1; then
        missing+=("libxinerama-dev")
    fi

    if ! dpkg -l libxext-dev >/dev/null 2>&1; then
        missing+=("libxext-dev")
    fi

    if ! dpkg -l libxrandr-dev >/dev/null 2>&1; then
        missing+=("libxrandr-dev")
    fi

    if ! dpkg -l libxcursor-dev >/dev/null 2>&1; then
        missing+=("libxcursor-dev")
    fi

    # Network and crypto
    if ! dpkg -l libcurl4-openssl-dev >/dev/null 2>&1; then
        missing+=("libcurl4-openssl-dev")
    fi

    if [ ${#missing[@]} -gt 0 ]; then
        print_warning "Missing Linux dependencies: ${missing[*]}"
        if [ "$INSTALL_DEPS" = "ON" ]; then
            print_status "Installing missing dependencies..."
            sudo apt update
            sudo apt install -y "${missing[@]}"
            print_success "Dependencies installed"
        else
            print_error "Missing required dependencies. Install with:"
            echo "    sudo apt install ${missing[*]}"
            echo "    Or run with --install-deps"
            exit 1
        fi
    else
        print_success "All Linux dependencies found"
    fi
}

# Install dependencies
install_dependencies() {
    print_status "Installing dependencies..."

    if [ "$(uname)" = "Linux" ]; then
        # Ubuntu/Debian
        if command_exists apt; then
            sudo apt update
            sudo apt install -y build-essential cmake ninja-build git \
                               libasound2-dev libjack-jackd2-dev libcurl4-openssl-dev \
                               libfreetype6-dev libx11-dev libxinerama-dev libxext-dev \
                               libxrandr-dev libxcursor-dev libwebkit2gtk-4.0-dev \
                               libglu1-mesa-dev mesa-common-dev libssl-dev
        elif command_exists dnf; then
            # Fedora
            sudo dnf install -y cmake ninja-build gcc-c++ git \
                               alsa-lib-devel jack-audio-connection-kit-devel \
                               libcurl-devel freetype-devel libX11-devel \
                               libXinerama-devel libXext-devel libXrandr-devel \
                               libXcursor-devel webkit2gtk4.0-devel mesa-libGL-devel \
                               openssl-devel
        elif command_exists brew; then
            # macOS (Homebrew)
            brew install cmake ninja
        fi
    elif [ "$(uname)" = "Darwin" ]; then
        # macOS with Homebrew
        if command_exists brew; then
            brew install cmake ninja
        else
            print_error "Homebrew not found. Please install Homebrew first."
            exit 1
        fi
    fi

    print_success "Dependencies installed"
}

# Clean build directory
clean_build() {
    if [ "$CLEAN_BUILD" = "ON" ] && [ -d "$BUILD_DIR" ]; then
        print_status "Cleaning build directory..."
        rm -rf "$BUILD_DIR"
        print_success "Build directory cleaned"
    fi
}

# Configure CMake
configure_cmake() {
    print_status "Configuring CMake for $BUILD_TYPE build..."

    local cmake_args=(
        -S "$PROJECT_ROOT"
        -B "$BUILD_DIR"
        -G Ninja
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
        -DBUILD_TESTS="$BUILD_TESTS"
    )

    # Add CMake prefix path if set
    if [ -n "$CMAKE_PREFIX_PATH" ]; then
        cmake_args+=(-DCMAKE_PREFIX_PATH="$CMAKE_PREFIX_PATH")
    fi

    # Add verbose flag
    if [ "$VERBOSE" = "ON" ]; then
        cmake_args+=(-DCMAKE_VERBOSE_MAKEFILE=ON)
    fi

    # Run CMake
    if "${cmake_args[@]}"; then
        print_success "CMake configuration successful"
    else
        print_error "CMake configuration failed"
        exit 1
    fi
}

# Build project
build_project() {
    print_status "Building project with $JOBS jobs..."

    local build_args=(
        --build "$BUILD_DIR"
        -j "$JOBS"
    )

    # Add config for Windows-style builds
    if [ "$(uname)" = "Darwin" ]; then
        build_args+=(--config "$BUILD_TYPE")
    fi

    # Add verbose flag
    if [ "$VERBOSE" = "ON" ]; then
        build_args+=(--verbose)
    fi

    # Run build
    if "${build_args[@]}"; then
        print_success "Build completed successfully"
    else
        print_error "Build failed"
        exit 1
    fi
}

# Run tests if enabled
run_tests() {
    if [ "$BUILD_TESTS" = "ON" ]; then
        print_status "Running tests..."

        if [ -f "$BUILD_DIR/ZenithDAWTests" ]; then
            cd "$BUILD_DIR"
            if ./ZenithDAWTests; then
                print_success "All tests passed"
            else
                print_error "Some tests failed"
                exit 1
            fi
        elif [ -f "$BUILD_DIR/ZenithDAWTests_artefacts/Release/ZenithDAWTests" ]; then
            cd "$BUILD_DIR"
            if ./ZenithDAWTests_artefacts/Release/ZenithDAWTests; then
                print_success "All tests passed"
            else
                print_error "Some tests failed"
                exit 1
            fi
        else
            print_warning "Test executable not found"
        fi
    fi
}

# Build in Docker
build_in_docker() {
    print_status "Building in Docker container..."

    # Build Docker image
    print_status "Building Docker image..."
    if ! docker build -t zenith-daw-builder -f "$PROJECT_ROOT/Dockerfile" "$PROJECT_ROOT"; then
        print_error "Docker image build failed"
        exit 1
    fi

    # Run build in container
    print_status "Running build in container..."
    if ! docker run --rm -v "$PROJECT_ROOT:/workspace" -w /workspace zenith-daw-builder; then
        print_error "Docker build failed"
        exit 1
    fi

    print_success "Docker build completed"
}

# Show build summary
show_summary() {
    print_status "Build Summary:"
    echo "  Build Type: $BUILD_TYPE"
    echo "  Tests: $BUILD_TESTS"
    echo "  Jobs: $JOBS"
    echo "  Clean Build: $CLEAN_BUILD"
    echo "  Build Directory: $BUILD_DIR"

    if [ -f "$BUILD_DIR/ZenithDAW" ]; then
        local size=$(du -h "$BUILD_DIR/ZenithDAW" | cut -f1)
        echo "  Executable: $BUILD_DIR/ZenithDAW ($size)"
        echo ""
        print_success "You can run the application with:"
        echo "  $BUILD_DIR/ZenithDAW"
    elif [ -f "$BUILD_dir/Release/Zenith DAW.exe" ]; then
        local size=$(du -h "$BUILD_DIR/Release/Zenith DAW.exe" | cut -f1)
        echo "  Executable: $BUILD_DIR/Release/Zenith DAW.exe ($size)"
        echo ""
        print_success "You can run the application with:"
        echo "  \"$BUILD_DIR/Release/Zenith DAW.exe\""
    else
        print_warning "Build completed but executable not found"
    fi
}

# Main function
main() {
    # Parse arguments
    parse_args "$@"

    # Change to project root
    cd "$PROJECT_ROOT"

    # Check prerequisites
    check_prerequisites

    # Build in Docker if requested
    if [ "$DOCKER_BUILD" = "ON" ]; then
        build_in_docker
        show_summary
        return 0
    fi

    # Clean build directory if requested
    clean_build

    # Fetch dependencies if not skipped
    if [ "$SKIP_DEPS" = "OFF" ]; then
        print_status "Fetching dependencies..."
        if ! cmake --build "$BUILD_DIR" --target deps; then
            print_warning "Dependency fetching failed, continuing with build..."
        fi
    fi

    # Configure CMake
    configure_cmake

    # Build project
    build_project

    # Run tests if enabled
    run_tests

    # Show summary
    show_summary
}

# Run main function with all arguments
main "$@"