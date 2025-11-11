#!/bin/bash
# Run Script for Vexel DAW Qt/QML Application
#
# This script runs the Qt/QML + JUCE hybrid DAW application
# It will build first if needed

set -e

# Check if built
if [ ! -f "build/bin/VexelDAW" ] && [ ! -f "build/VexelDAW" ]; then
    echo "Application not built yet. Building now..."
    ./build.sh Release
fi

# Find executable
EXECUTABLE=""
if [ -f "build/bin/VexelDAW" ]; then
    EXECUTABLE="build/bin/VexelDAW"
elif [ -f "build/VexelDAW" ]; then
    EXECUTABLE="build/VexelDAW"
fi

if [ -z "$EXECUTABLE" ]; then
    echo "❌ ERROR: VexelDAW executable not found"
    exit 1
fi

echo "================================="
echo "Starting Vexel DAW..."
echo "================================="
echo ""

# Run application
$EXECUTABLE "$@"
