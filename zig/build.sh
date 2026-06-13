#!/usr/bin/env bash
# Build + run the Zenith Zig DSP spike (clean-room, no JUCE).
#   - compiles the Zig kernel to a C-ABI static lib
#   - links it into a C++ A/B harness
#   - runs the correctness + benchmark comparison vs the C++ reference
set -euo pipefail
cd "$(dirname "$0")"

echo ">> building Zig kernel -> libzenith_dsp.a"
zig build-lib zenith_dsp.zig -O ReleaseFast -femit-bin=libzenith_dsp.a

echo ">> building + linking C++ A/B harness"
zig c++ -O2 -std=c++17 -I. ab_harness.cpp libzenith_dsp.a -o ab_harness

echo ">> running"
./ab_harness
