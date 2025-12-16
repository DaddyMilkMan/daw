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

## Pending Fixes
- [ ] **Roast #1: Shared Ptr Cycles**
- [ ] **Roast #2: UI Thread Safety**
- [ ] **Roast #3: Audio Thread Safety**
- [ ] **Roast #5: Buffer Size Mismatch** (Partially fixed in Engine.cpp?)
