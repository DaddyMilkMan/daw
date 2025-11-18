# Zenith DAW Test Suite

## Overview

This directory contains non-GUI integration tests for Zenith DAW's core functionality.

## Test Files

### 1. ProjectStateTests.cpp
Tests the ProjectState data model:
- Project save/load cycle
- Track ID generation
- Duplicate ID detection

### 2. RecordingAutomationTests.cpp
Tests recording and automation integration (non-GUI):

**Test 1: Recording Simulation - Clip Length Calculations**
- Verifies recording duration calculations
- Tests clip length in samples matches expected values
- Validates conversion between samples and seconds
- Simulates audio buffer creation and filling

**Test 2: Automation Envelope Sampling**
- Tests automation point storage in ProjectState
- Verifies linear interpolation between automation points
- Tests automation sampling at different beat positions
- Validates volume automation from 0.0 to 1.0 over 4 beats

**Test 3: Tempo Change - Automation Beat Mapping**
- Verifies automation points stay anchored to beat positions
- Tests that tempo changes don't affect beat-based automation
- Validates that time duration changes correctly with tempo
- Ensures musical synchronization is maintained

**Test 4: ProjectState Change Propagation**
- Tests automation data model supports change notifications
- Verifies volume, pan, and mute automation parameters
- Tests undo/redo functionality for automation points
- Ensures changes can propagate to engine

## Building Tests

### Configure and Build

```bash
cd zenith-core
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --target RecordingAutomationTests -j4
cmake --build . --target ProjectStateTests -j4
```

### Build All Tests

```bash
cmake --build . -j4
```

## Running Tests

### Run Specific Test

```bash
# Run RecordingAutomationTests directly
./RecordingAutomationTests_artefacts/Release/RecordingAutomationTests

# Run ProjectStateTests directly
./ProjectStateTests_artefacts/Release/ProjectStateTests
```

### Run All Tests via CTest

```bash
cd build
ctest                      # Run all tests
ctest -V                   # Run with verbose output
ctest --output-on-failure  # Show output only if tests fail
```

## Test Results

Both tests pass successfully:

```
Test project /home/user/daw/zenith-core/build
    Start 1: ProjectStateTests
1/2 Test #1: ProjectStateTests ................   Passed    0.02 sec
    Start 2: RecordingAutomationTests
2/2 Test #2: RecordingAutomationTests .........   Passed    0.01 sec

100% tests passed, 0 tests failed out of 2

Total Test time (real) =   0.04 sec
```

## Test Invariants

### Recording Tests Assert:
- Clip length in samples = numBlocks * blockSize
- Clip duration in seconds = samples / sampleRate (within tolerance)
- Audio buffer creation with correct channels and samples
- Sample data integrity (sine wave generation)

### Automation Tests Assert:
- Linear interpolation between automation points
- Volume interpolation: beat 0→0.0, beat 1→0.25, beat 2→0.5, beat 3→0.75, beat 4→1.0
- Automation points remain at same beat positions across tempo changes
- Time duration changes correctly with tempo (slower = longer)
- Undo/redo preserves automation data integrity

## Adding New Tests

Follow the existing pattern in `ProjectStateTests.cpp` and `RecordingAutomationTests.cpp`:

1. Create a new `.cpp` file in `tests/` directory
2. Use standard `int main()` entry point
3. Return 0 for success, 1 for failure
4. Add test to `CMakeLists.txt`:

```cmake
juce_add_console_app(YourNewTest
    PRODUCT_NAME "YourNewTest"
)

target_sources(YourNewTest PRIVATE
    tests/YourNewTest.cpp
    src/RequiredSource1.cpp
    src/RequiredSource2.cpp
)

target_include_directories(YourNewTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(YourNewTest PRIVATE
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    # Add other required JUCE modules
)

target_compile_definitions(YourNewTest PRIVATE
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
)

juce_generate_juce_header(YourNewTest)

add_test(NAME YourNewTest COMMAND YourNewTest)
```

## Test Framework

- **Framework**: Custom (simple main() with return codes)
- **Pattern**: JUCE console application
- **Success**: Return 0
- **Failure**: Return non-zero
- **Output**: std::cout for info, std::cerr for errors
- **Integration**: CMake CTest

## Dependencies

Tests link only necessary JUCE modules to minimize build time:
- `juce::juce_core` - Core utilities
- `juce::juce_data_structures` - ValueTree, etc.
- `juce::juce_events` - Event handling
- `juce::juce_audio_basics` - Audio buffers

## Test Coverage

Current test coverage focuses on:
- ✅ Project state management (save/load, tracks, IDs)
- ✅ Recording data structures (clip length calculations, audio buffers)
- ✅ Automation data model (envelope sampling, interpolation)
- ✅ Tempo changes (beat-based automation mapping)
- ✅ Undo/redo functionality

Future test areas:
- 🚧 Actual audio recording integration with Engine
- 🚧 TrackAutomationSynchronizer real-time synchronization
- 🚧 Multi-track recording scenarios
- 🚧 Automation edge cases (overlapping points, extreme values)
- 🚧 Performance tests (large buffers, many automation points)
