# Phase 1 Sanity Checks

## Pre-Integration Checks (Before Phase 1)
- [x] `Engine::processAudio()` has zero allocations (C3 verified)
- [x] Track iteration uses pre-sized containers (C3: std::vector with reserve)
- [x] No donor type access in audio thread (C3 verified)
- [ ] `prepareToPlay` called for each track on device (re)start (Phase 1)
- [x] No exceptions/RTTI on RT thread (JUCE best practices)
- [ ] GUI meters read via lock-free FIFO only (Phase 1.1)
- [ ] Device change triggers re-prepare without leaks (Phase 1)

## Integration Verification (Phase 1)
- [ ] Grep audit: no `new`/`malloc` in `processAudio()` or `Track::getNextAudioBlock()`
- [ ] No `std::string`/`juce::String` in audio callback
- [ ] No `std::vector::push_back` in hot path
- [ ] Lock usage: only `juce::ScopedLock` for clip/plugin containers (message thread)
- [ ] Atomics for all cross-thread communication (volume, pan, mute, solo)
- [ ] Pre-allocated buffers: `pluginBuffer`, `clipBuffer` (Track.cpp:39, 115)

## Performance Targets
- [ ] CPU usage < 5% idle (test tone baseline: ~1%)
- [ ] CPU usage < 25% with 32 tracks + 4 clips each
- [ ] No glitches at 512 samples buffer (Windows WASAPI)
- [ ] No glitches at 128 samples buffer (ASIO)

## Memory Safety
- [ ] Valgrind/ASAN clean (Linux)
- [ ] No leaks on device change
- [ ] No leaks on track create/delete
- [ ] Proper cleanup in `Engine::shutdown()`

## Thread Safety
- [ ] ThreadSanitizer clean (if available)
- [ ] No data races (verified by TSan or manual audit)
- [ ] Message thread never blocks audio thread
- [ ] Audio thread never waits for message thread

## Code Quality
- [ ] No compiler warnings with `/W4` (MSVC) or `-Wall -Wextra` (GCC/Clang)
- [ ] No undefined behavior (UBSan if available)
- [ ] All donor classes in `namespace zenith`
- [ ] Forward declarations in public headers only

## Current Status (C4)
- ✅ Phase 0 complete (test tone functional)
- ✅ Donor classes ported (Track, Clip, MixerChannel)
- ✅ Engine adapter complete (compile-only)
- ✅ UI shows track count (read-only)
- ⚠️ Phase 1 not started (audio wiring pending)
