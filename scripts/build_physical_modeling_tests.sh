#!/bin/bash

# Build script for physical modeling tests
# This script builds and runs the test suite for the physical modeling engine

set -e

echo "Building ZenithUltraSynth Physical Modeling Tests..."

# Create build directory
mkdir -p build/tests/physical_modeling
cd build/tests/physical_modeling

# Run CMake configuration
cmake ../../../../tests/physical_modeling

# Build the test executable
make -j4

# Run the tests
echo "Running Physical Modeling Tests..."
./TestRunner

# Copy test results to project root
if [ -f "PhysicalModelingTestReport.txt" ]; then
    cp PhysicalModelingTestReport.txt ../../../../
    echo "Test report copied to project root"
fi

if [ -f "StringModelTest.wav" ]; then
    cp StringModelTest.wav ../../../../
    cp WindModelTest.wav ../../../../
    cp PercussionModelTest.wav ../../../../
    echo "Test audio files copied to project root"
fi

echo "Physical Modeling Tests completed successfully!"
