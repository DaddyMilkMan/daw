#!/bin/bash
# Launch the native Zenith DAW build. Builds the project if missing.

set -euo pipefail

BUILD_DIR="build"
CONFIG="${1:-Release}"

if [ ! -d "$BUILD_DIR" ]; then
    echo "Build directory missing. Building now..."
    ./build.sh "$CONFIG"
fi

APP_PATH=""
if [ -f "$BUILD_DIR/zenith-core/ZenithDAW_artefacts/$CONFIG/ZenithDAW" ]; then
    APP_PATH="$BUILD_DIR/zenith-core/ZenithDAW_artefacts/$CONFIG/ZenithDAW"
elif [ -f "$BUILD_DIR/zenith-core/ZenithDAW_artefacts/$CONFIG/ZenithDAW.exe" ]; then
    APP_PATH="$BUILD_DIR/zenith-core/ZenithDAW_artefacts/$CONFIG/ZenithDAW.exe"
fi

if [ -z "$APP_PATH" ]; then
    echo "Unable to locate ZenithDAW binary. Rebuilding..."
    ./build.sh "$CONFIG"
    if [ -f "$BUILD_DIR/zenith-core/ZenithDAW_artefacts/$CONFIG/ZenithDAW" ]; then
        APP_PATH="$BUILD_DIR/zenith-core/ZenithDAW_artefacts/$CONFIG/ZenithDAW"
    elif [ -f "$BUILD_DIR/zenith-core/ZenithDAW_artefacts/$CONFIG/ZenithDAW.exe" ]; then
        APP_PATH="$BUILD_DIR/zenith-core/ZenithDAW_artefacts/$CONFIG/ZenithDAW.exe"
    fi
fi

if [ -z "$APP_PATH" ]; then
    echo "❌ Unable to find ZenithDAW executable after rebuild."
    exit 1
fi

"$APP_PATH"
