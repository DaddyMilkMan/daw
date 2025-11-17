#!/bin/bash
# Build Script for Zenith DAW (Pure JUCE Native)
#
# This script builds the native JUCE DAW application
# Usage: ./build.sh [Debug|Release]

set -e  # Exit on error

# Configuration
BUILD_TYPE="${1:-Release}"
BUILD_DIR="zenith-core/build"
JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo "================================="
echo "Zenith DAW - Pure JUCE Native Build"
echo "================================="
echo "Build type: $BUILD_TYPE"
echo "Jobs: $JOBS"
echo ""

# Check for CMake
echo "Checking for CMake..."
if ! command -v cmake &> /dev/null; then
    echo "❌ ERROR: CMake not found!"
    echo "Install: sudo apt install cmake  (or brew install cmake)"
    exit 1
fi

CMAKE_VERSION=$(cmake --version | head -n1)
echo "✅ Found $CMAKE_VERSION"
echo ""

# Create build directory
echo "Creating build directory..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake
echo "Configuring CMake..."
echo "  - Fetching JUCE 8.0.9 (if needed)..."
cmake .. \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Build
echo ""
echo "Building Zenith DAW..."
cmake --build . --config "$BUILD_TYPE" -j"$JOBS"

# Success
echo ""
echo "================================="
echo "✅ Build completed successfully!"
echo "================================="
echo ""
echo "Architecture: Pure C++ JUCE Native"
echo "  - No Electron"
echo "  - No React/TypeScript"
echo "  - No Qt/QML"
echo "  - Pure JUCE GUI"
echo ""
echo "To run the application:"
echo "  ./zenith-core/build/ZenithDAW_artefacts/$BUILD_TYPE/ZenithDAW"
echo ""
echo "Or use: ./run.sh"
echo ""
