#!/bin/bash
# Build Script for Zenith DAW Qt/QML Application
#
# This script builds the Qt/QML + JUCE hybrid DAW application
# Usage: ./build.sh [Debug|Release]

set -e  # Exit on error

# Configuration
BUILD_TYPE="${1:-Release}"
BUILD_DIR="build"
JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo "================================="
echo "Zenith DAW - Qt/QML + JUCE Build"
echo "================================="
echo "Build type: $BUILD_TYPE"
echo "Jobs: $JOBS"
echo ""

# Check for Qt
echo "Checking for Qt..."
if ! command -v qmake &> /dev/null; then
    echo "❌ ERROR: Qt not found!"
    echo ""
    echo "Please install Qt 6.5+ from: https://www.qt.io/download-qt-installer"
    echo ""
    echo "Or set Qt path:"
    echo "  export CMAKE_PREFIX_PATH=/path/to/Qt/6.x/gcc_64"
    exit 1
fi

QT_VERSION=$(qmake -query QT_VERSION)
echo "✅ Found Qt version: $QT_VERSION"
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

# Check for JUCE (optional - will use placeholders if not available)
if [ -d "JUCE" ]; then
    echo "✅ Found JUCE framework"
else
    echo "⚠️  JUCE not found - using placeholder implementation"
    echo "   To add JUCE: git submodule add https://github.com/juce-framework/JUCE.git"
fi
echo ""

# Create build directory
echo "Creating build directory..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake
echo "Configuring CMake..."
cmake .. \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Build
echo ""
echo "Building..."
cmake --build . --config "$BUILD_TYPE" -j"$JOBS"

# Success
echo ""
echo "================================="
echo "✅ Build completed successfully!"
echo "================================="
echo ""
echo "To run the application:"
echo "  ./build/bin/ZenithDAW"
echo ""
echo "Or use: ./run.sh"
echo ""
