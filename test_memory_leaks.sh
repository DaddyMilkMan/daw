#!/usr/bin/env bash
# Test script for verifying memory leak fixes in Zenith DAW
# Run this script after building the project in Debug mode with sanitizers enabled

set -e

echo "=========================================="
echo "Memory Leak Validation Test Script"
echo "=========================================="
echo ""

# Check if build directory exists
if [ ! -d "build" ]; then
    echo "❌ Build directory not found. Please build the project first."
    echo ""
    echo "To build:"
    echo "  mkdir build && cd build"
    echo "  cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON"
    echo "  cmake --build . -j$(nproc)"
    exit 1
fi

cd build

# Check if test executable exists
if [ ! -f "ZenithDAWTests" ]; then
    echo "❌ ZenithDAWTests executable not found."
    echo "   Please build with -DBUILD_TESTS=ON"
    exit 1
fi

echo "✅ Found ZenithDAWTests executable"
echo ""

# Set LeakSanitizer options
export LSAN_OPTIONS="suppressions=../lsan.supp:verbosity=1:log_threads=1"
export ASAN_OPTIONS="detect_leaks=1"

echo "LeakSanitizer Configuration:"
echo "  LSAN_OPTIONS=$LSAN_OPTIONS"
echo "  ASAN_OPTIONS=$ASAN_OPTIONS"
echo ""

echo "=========================================="
echo "Running tests with LeakSanitizer enabled..."
echo "=========================================="
echo ""

# Run tests and capture output
./ZenithDAWTests 2>&1 | tee test_output.txt

# Check results
echo ""
echo "=========================================="
echo "Analyzing Results..."
echo "=========================================="
echo ""

# Check for leak summary
if grep -q "LeakSanitizer: detected memory leaks" test_output.txt; then
    echo "❌ FAILED: Memory leaks detected!"
    echo ""
    echo "Leak details:"
    grep -A 20 "LeakSanitizer: detected memory leaks" test_output.txt
    exit 1
elif grep -q "Leaked objects detected:" test_output.txt; then
    echo "❌ FAILED: JUCE leaked objects detected!"
    echo ""
    echo "Leak details:"
    grep "Leaked objects detected:" test_output.txt
    exit 1
else
    echo "✅ SUCCESS: No memory leaks detected!"
    echo ""
    echo "All tests passed with zero memory leaks."
fi

echo ""
echo "=========================================="
echo "Verification Complete"
echo "=========================================="
