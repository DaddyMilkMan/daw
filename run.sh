#!/bin/bash
# Run Script for Zenith DAW (Pure JUCE Native)
#
# This script runs the native JUCE DAW application
# It will build first if needed

set -e

# Find executable in common build locations
EXECUTABLE=""
BUILD_TYPE="${1:-Release}"

# Check Release build
if [ -f "zenith-core/build/ZenithDAW_artefacts/Release/ZenithDAW" ]; then
    EXECUTABLE="zenith-core/build/ZenithDAW_artefacts/Release/ZenithDAW"
# Check Debug build
elif [ -f "zenith-core/build/ZenithDAW_artefacts/Debug/ZenithDAW" ]; then
    EXECUTABLE="zenith-core/build/ZenithDAW_artefacts/Debug/ZenithDAW"
# Check macOS app bundle
elif [ -d "zenith-core/build/ZenithDAW_artefacts/Release/ZenithDAW.app" ]; then
    EXECUTABLE="zenith-core/build/ZenithDAW_artefacts/Release/ZenithDAW.app/Contents/MacOS/ZenithDAW"
elif [ -d "zenith-core/build/ZenithDAW_artefacts/Debug/ZenithDAW.app" ]; then
    EXECUTABLE="zenith-core/build/ZenithDAW_artefacts/Debug/ZenithDAW.app/Contents/MacOS/ZenithDAW"
fi

if [ -z "$EXECUTABLE" ]; then
    echo "Application not built yet. Building now..."
    ./build.sh "$BUILD_TYPE"

    # Try to find executable again
    if [ -f "zenith-core/build/ZenithDAW_artefacts/$BUILD_TYPE/ZenithDAW" ]; then
        EXECUTABLE="zenith-core/build/ZenithDAW_artefacts/$BUILD_TYPE/ZenithDAW"
    elif [ -d "zenith-core/build/ZenithDAW_artefacts/$BUILD_TYPE/ZenithDAW.app" ]; then
        EXECUTABLE="zenith-core/build/ZenithDAW_artefacts/$BUILD_TYPE/ZenithDAW.app/Contents/MacOS/ZenithDAW"
    else
        echo "❌ ERROR: ZenithDAW executable not found after build"
        exit 1
    fi
fi

echo "================================="
echo "Starting Zenith DAW (Native JUCE)"
echo "================================="
echo ""

# Run application
$EXECUTABLE
