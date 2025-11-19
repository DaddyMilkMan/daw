#!/bin/bash
# Run Script for Zenith DAW Qt/QML Application
#
# This script runs the Qt/QML + JUCE hybrid DAW application
# It will build first if needed

set -e

# Check if built
if [ ! -f "build/bin/ZenithDAW" ] && [ ! -f "build/ZenithDAW" ]; then
    echo "Application not built yet. Building now..."
    ./build.sh Release
fi

# Find executable
EXECUTABLE=""
if [ -f "build/bin/ZenithDAW" ]; then
    EXECUTABLE="build/bin/ZenithDAW"
elif [ -f "build/ZenithDAW" ]; then
    EXECUTABLE="build/ZenithDAW"
fi

if [ -z "$EXECUTABLE" ]; then
    echo "❌ ERROR: ZenithDAW executable not found"
    exit 1
fi

echo "================================="
echo "Starting Zenith DAW..."
echo "================================="
echo ""

# Run application
$EXECUTABLE "$@"
