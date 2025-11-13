#!/bin/bash
# Zenith DAW - Native JUCE Build Script

set -euo pipefail

BUILD_TYPE="${1:-Release}"
BUILD_DIR="build"
JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

printf '\n=================================\n'
printf 'Zenith DAW Native Build\n'
printf '=================================\n'
printf 'Build type: %s\n' "$BUILD_TYPE"
printf 'Parallel jobs: %s\n\n' "$JOBS"

mkdir -p "$BUILD_DIR"

cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
cmake --build "$BUILD_DIR" --config "$BUILD_TYPE" -j"$JOBS"

printf '\nBuild artifacts are in %s\n' "$BUILD_DIR"
