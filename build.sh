#!/bin/bash
# Zenith Synth - Professional Build Script
# Copyright (C)2025 Micah Cooley <micahcooley@protonmail.com>

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}================================================${NC}"
echo -e "${GREEN}  Zenith Synth - Professional Build${NC}"
echo -e "${GREEN}================================================${NC}"

# Detect OS
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    PLATFORM="linux"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    PLATFORM="macos"
else
    PLATFORM="windows"
fi

echo -e "${YELLOW}Platform: $PLATFORM${NC}"

# Build directory
BUILD_DIR="build"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure
echo -e "\n${YELLOW}[1/5] Configuring...${NC}"
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_CXX_FLAGS="-march=native -O3" \
      ..

# Build
echo -e "${YELLOW}[2/5] Building ($(nproc) cores)...${NC}"
make -j$(nproc)

# Run tests
echo -e "${YELLOW}[3/5] Running unit tests...${NC}"
if [ -f "tests/run_zenith_tests" ]; then
    ./tests/run_zenith_tests
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✓ Tests passed${NC}"
    else
        echo -e "${RED}✗ Tests failed${NC}"
        exit 1
    fi
fi

# CPU Profiling
echo -e "${YELLOW}[4/5] CPU Profiling...${NC}"
if [ -f "tests/CPUProfiler" ]; then
    ./tests/CPUProfiler > profile_results.txt 2>&1
    echo -e "${GREEN}✓ Profiling complete${NC}"
    cat profile_results.txt
fi

# Package
echo -e "${YELLOW}[5/5] Creating distribution...${NC}"
DIST_DIR="ZenithSynth-$PLATFORM"
mkdir -p "$DIST_DIR"

# Copy binary
cp -r apps/Standalone "$DIST_DIR/" 2>/dev/null || true
cp -r presets "$DIST_DIR/" 2>/dev/null || true
cp README.md "$DIST_DIR/" 2>/dev/null || true
cp SHIPPING_CHECKLIST.md "$DIST_DIR/" 2>/dev/null || true

# Create tarball
tar -czf "ZenithSynth-Source.tar.gz" \
    --exclude=build --exclude='.git' \
    CMakeLists.txt README.md \
    modules presets tests \
    SHIPPING_CHECKLIST.md

echo -e "\n${GREEN}================================================${NC}"
echo -e "${GREEN}  Build Complete!${NC}"
echo -e "${GREEN}================================================${NC}"
echo -e "Binary: ${GREEN}$BUILD_DIR/apps/Standalone/ZenithSynth${NC}"
echo -e "Source: ${GREEN}ZenithSynth-Source.tar.gz${NC}"
echo -e "\n${YELLOW}Run with: $BUILD_DIR/apps/Standalone/ZenithSynth${NC}"
