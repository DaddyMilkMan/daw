# Roast Fixes Progress

## Completed Fixes
- [x] **Roast #9: Hardcoded Input Routing**
  - Added `PROP_INPUT_CHANNEL` to `ProjectState`.
  - Updated `TrackStateSynchronizer` to sync input channel.
  - Implemented `setTrackInputChannel` in `Engine`.
  - Verified `processAudioRecording` uses dynamic routing.

- [x] **Roast #4: Blocking I/O on Record Start**
  - Implemented async recording preparation (`prepareRecordingForTrack`).
  - Updated `setTrackArmed` to trigger async prep.
  - Updated `record()` to use pre-prepared sessions.

- [x] **Roast #1: Shared Ptr Cycles**
  - **Audit Complete**: No circular shared_ptr references found in codebase.
  - **Ownership Model Verified**:
    - Engine → Track: `std::shared_ptr<Track>` (correct parent→child)
    - ClipTrack → Clip: `std::unique_ptr<Clip>` (correct parent→child)
    - Back-references: Raw pointers/references (`Engine&`, `Track*`) - no cycles
    - BrowserItem: Uses `std::weak_ptr<BrowserItem>` for parent - correct
  - **Documentation Added**: Ownership model comments added to:
    - `Engine.h` - Full ownership hierarchy documented
    - `Track.h` - Engine→Track→Clip relationships documented
    - `ClipTrack.h` - Clip ownership documented
    - `Clip.h` - No back-reference policy documented
  - **Conclusion**: Codebase already follows correct ownership patterns.

- [x] **Roast #5: Buffer Size Mismatch**
  - Added buffer size validation in `Track::prepareToPlay`, `Clip::prepareToPlay`, `AudioRenderer::prepare`
  - Added change detection to avoid unnecessary reallocations
  - Fixed `ZenithPolySynth::prepareToPlay` to use `samplesPerBlock` (was ignored)
  - Added `currentBlockSize_` tracking and `setBlockSize` propagation

- [x] **Roast #3: Audio Thread Safety**
  - Removed `juce::SpinLock` from `TempoMap` and replaced with atomic snapshot load/store.
  - Removed redundant `juce::CriticalSection` locks from `Clip` (`audioLock`, `midiLock`) in audio path methods.
  - Optimized `AudioRenderer` to pre-allocate `auxBufferPtrsVector_` in `prepare()` to avoid RT-allocations.
  - Verified lock-free RCU patterns in `AudioRecorder`, `RoutingGraph`, and `PluginChain`.
  - Added assertions to ensure message-thread-only methods are not called from audio thread.

## Pending Fixes
- [x] **Roast #2: UI Thread Safety**
  - **Audit**: `MixerView` was accessing `engine_.tracks()` directly (polling). Audio thread access was safe via snapshot, but UI architectural coupling was high.
  - **Fix**: Refactored `MixerView` to listen to `ProjectState` (Source of Truth).
  - **Implementation**: Used `juce::ValueTree::Listener` + `callAsync` to trigger rebuilds only when persistent state changes.
  - **Result**: Decoupled UI from Engine internals. UI only displays tracks that exist in the project model.

