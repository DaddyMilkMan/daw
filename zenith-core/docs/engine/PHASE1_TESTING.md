# Phase 1 Audio Testing

Comprehensive headless regression tests for **W10+W10.2+W10.3** (mixer + scheduler + voices + fades).

## Overview

`zenith-core-phase1-tests` is a console application that validates the Phase 1 audio engine **offline**, without requiring:
- Audio device or sound card
- GUI or window manager
- Manual listening tests
- Real-time timing constraints

Tests run at full CPU speed and fail loudly if any timing/fade regressions occur.

## What It Tests

### Test 1: `Scheduler_ClipStartsExactlyAtScheduledSample`
**Validates**: Sample-accurate scheduler timing

- Schedules a StartClip event at sample 1000
- Verifies all samples < 1000 are exactly 0.0
- Verifies sample 1000 is non-zero (≈1.0)
- Verifies no pre-ring at sample 999

**Guarantees**: Events trigger at exact sample offsets, not "close enough" or quantized to block boundaries.

### Test 2: `Fades_FadeInAndFadeOutShapeIsLinear`
**Validates**: W13 fade curve implementation

- Clip with 100-sample fade-in, 100-sample fade-out
- Checks fade-in: 0.0 → 0.5 → 1.0 (linear)
- Checks sustain: ≈1.0 throughout middle
- Checks fade-out: 1.0 → 0.5 → 0.0 (linear)

**Guarantees**: Fade curves are smooth and linear, no clicks or discontinuities.

### Test 3: `StopEvents_FadeOutThenSilence`
**Validates**: StopClip event behavior

- Long clip (10000 samples), StopClip at sample 2000
- Verifies level ≈1.0 before stop
- Verifies smooth fade-out over 100 samples
- Verifies complete silence after fade-out

**Guarantees**: StopClip events trigger fade-out, not immediate cutoff.

## Building Locally

### Requirements
- `ZENITH_ENABLE_PHASE1_AUDIO=ON` (required)
- `ZENITH_ENABLE_PHASE1_TESTS=ON` (optional, default: ON)

### Build Steps

**Windows (MSVC):**
```cmd
cd zenith-core
cmake --preset dev-debug
cmake --build --preset dev-debug --target zenith-core-phase1-tests --config Debug
```

**Linux/macOS:**
```bash
cd zenith-core
cmake --preset dev-debug
cmake --build --preset dev-debug --target zenith-core-phase1-tests
```

### Run Tests

**Windows:**
```cmd
cd zenith-core\out\build\dev-debug
zenith-core-phase1-tests_artefacts\Debug\zenith-core-phase1-tests.exe
```

**Linux/macOS:**
```bash
cd zenith-core/out/build/dev-debug
./zenith-core-phase1-tests_artefacts/Debug/zenith-core-phase1-tests
```

**Expected Output:**
```
========================================
Phase 1 Audio Tests (W10+W10.2+W10.3)
Headless regression tests for scheduler + voices + fades
========================================

=== Test 1: Scheduler_ClipStartsExactlyAtScheduledSample ===
✓ Test 1 PASSED: Clip starts exactly at scheduled sample

=== Test 2: Fades_FadeInAndFadeOutShapeIsLinear ===
✓ Test 2 PASSED: Fade-in and fade-out shapes are linear

=== Test 3: StopEvents_FadeOutThenSilence ===
✓ Test 3 PASSED: StopClip triggers fade-out then silence

========================================
ALL TESTS PASSED ✓
========================================
```

**Exit Codes:**
- `0` = All tests passed
- `1` = Test failure (assertion failed)

## CI Integration

### Automated Testing
Tests run automatically in CI on every PR where `dev-debug` preset is built.

**GitHub Actions Workflow**: `.github/workflows/windows.yml`
- **dev-debug job**: Builds and runs `zenith-core-phase1-tests`
- **release-safe job**: Verifies tests are NOT built (Phase 1 OFF)

### Failure Behavior
If any test fails, the CI job exits with code 1, failing the entire build:
- Scheduler timing regression → PR blocked
- Fade curve change → PR blocked
- Voice management bug → PR blocked

This prevents regressions from sneaking into main/release branches.

## Tolerance Values

Tests use floating-point epsilon comparisons to account for rounding:

| Test | Tolerance | Purpose |
|------|-----------|---------|
| Scheduler timing | 1e-3f (0.001) | Verify exact sample offsets |
| Fade shapes | 0.1f | Allow for quantization in fade curves |
| Sustain level | 0.05f | Tight tolerance for constant gain |
| Silence detection | 1e-3f (0.001) | Verify complete fade-out |

## Test Architecture

### No Audio Device
Tests directly invoke `Engine::audioDeviceIOCallbackWithContext()` with fake buffers:
```cpp
juce::AudioBuffer<float> testBuffer(2, blockSize);
engine.audioDeviceIOCallbackWithContext(nullptr, 0,
    testBuffer.getArrayOfWritePointers(), 2, blockSize, {});
```

### Same Code Path
Tests exercise the **identical W10.3 voice rendering path** as real playback:
- `Engine::drainScheduledEvents()` (segment loop)
- `Mixer::processSegment()` (track mixing)
- `AudioTrack::processSegment()` (voice rendering)
- `AudioTrack::renderVoiceIntoSegment()` (fade application)

No mocks, no stubs—just the real engine with fake I/O.

## Adding New Tests

### Test Template
```cpp
void test4_MyNewTest()
{
    DBG("\n=== Test 4: MyNewTest ===");

    const int sampleRate = 48000;
    const int blockSize = 256;

    Phase1TestHarness harness(sampleRate, blockSize);

    // 1. Create test clip
    auto clipAudio = TestUtil::createTestClip(5000, 1.0f);

    // 2. Add track and clip
    int trackIndex = harness.addTrack("Test Track");
    const int32_t clipId = 42;
    harness.addClipDef(trackIndex, clipId, &clipAudio, 0, 5000, 1.0f, 0, 0);
    harness.populateClipDefsForTrack(trackIndex);

    // 3. Schedule events
    harness.scheduleStartClip(trackIndex, clipId, 0);

    // 4. Render audio
    harness.seek(0);
    harness.play();
    auto timeline = harness.renderBlocks(20);

    // 5. Assert expectations
    TestUtil::assertWithMessage(timeline[100] > 0.9f,
        "Sample 100 should be ≈1.0, got " + juce::String(timeline[100]));

    DBG("✓ Test 4 PASSED: MyNewTest");
}
```

### Register Test
In `main()`:
```cpp
test1_SchedulerClipStartsExactly();
test2_FadeShapesAreLinear();
test3_StopEventTriggersFadeOut();
test4_MyNewTest();  // Add here
```

## Debugging Failed Tests

### Viewing Timeline Data
Modify test to dump samples:
```cpp
for (int i = 0; i < 200; ++i) {
    DBG("Sample " + juce::String(i) + ": " + juce::String(timeline[i]));
}
```

### Tolerance Adjustments
If tests fail due to expected floating-point drift:
```cpp
// Looser tolerance for rough checks
TestUtil::approxEqual(value, expected, 0.1f);

// Tighter tolerance for critical timing
TestUtil::approxEqual(value, expected, 1e-4f);
```

### Disabling Specific Tests
Comment out test call in `main()` for bisection:
```cpp
// test1_SchedulerClipStartsExactly();  // Skip this one
test2_FadeShapesAreLinear();
test3_StopEventTriggersFadeOut();
```

## Performance

Tests complete in **< 1 second** on typical CI runners:
- No real-time constraints (runs at full CPU speed)
- No I/O waits (fake buffers)
- No GUI overhead (console app)

Typical execution time: ~50-200ms depending on CPU.

## Limitations

### What Tests DON'T Cover
❌ **Real audio device timing**: Tests use fake buffers, not WASAPI/CoreAudio
❌ **UI integration**: Drag-and-drop, timeline rendering, etc.
❌ **Multi-threading stress**: Single-threaded test harness
❌ **Long-duration stability**: Tests run for ~1 second max

### Future Enhancements
- Add tests for multi-track scenarios (voice stealing across tracks)
- Add tests for transport seek during playback
- Add tests for event queue overflow (> 64 events/block)
- Add benchmarks for voice rendering performance

## Support

**File location**: `zenith-core/tests/Phase1AudioTests.cpp`
**Test harness**: `Phase1TestHarness` class
**Utilities**: `TestUtil` namespace

For questions or test failures, check:
1. Test output (DBG messages)
2. CI logs (GitHub Actions)
3. This documentation

---

**Last Updated**: 2025-11-13
**Phase 1 Audio Tests**: W10+W10.2+W10.3
**Status**: Production-ready for CI integration
