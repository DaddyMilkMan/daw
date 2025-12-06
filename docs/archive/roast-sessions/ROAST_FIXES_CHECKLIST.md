# Roast Fixes - Completion Checklist

## ✅ COMPLETED

- [x] **Roast #1**: Use-After-Free in Track Management → FIXED ✅
- [x] **Roast #2**: Thread Safety Violation in Plugin Chain → FIXED ✅
- [x] **Roast #3**: "Nuke and Pave" UI Updates → FIXED ✅
- [x] **Roast #5**: Silent Audio Dropouts → FIXED ✅
- [x] **Roast #6**: O(N²) MIDI Note Insertion → FIXED ✅
- [x] **Roast #10**: Heavy Math in Parameter Setters → FIXED ✅

---

## 📋 REMAINING WORK

### 🟡 Medium Priority (Do Next)

- [ ] **Roast #4**: Blocking I/O on Record Start
  - Status: **NOT STARTED**
  - Location: `Engine.cpp` `record()` method
  - Problem: Blocks message thread for file creation
  - Fix: Async file preparation
  - Estimated Time: 1 hour

- [ ] **Roast #9**: Hardcoded Input Routing
  - Status: **NOT STARTED**
  - Location: `Engine.cpp` `processAudioRecording()`
  - Problem: Only records inputs 1&2, no multi-channel support
  - Fix: Add `InputRouting` struct to Track
  - Estimated Time: 1 hour

- [ ] **Roast #7**: Toy Compressor Implementation
  - Status: **NOT STARTED**
  - Location: `MixerChannel.cpp` compressor DSP
  - Problem: No RMS, no lookahead, no oversampling
  - Fix: Implement proper DSP compressor
  - Estimated Time: 2-3 hours (requires DSP knowledge)

### 🟢 Low Priority (Polish)

- [ ] **Roast #8**: Global ID Scanning
  - Status: **NOT STARTED**
  - Location: `ProjectState.cpp` `rebuildIdCounter()`
  - Problem: O(N) tree walk on project load just to find max ID
  - Fix: Store `nextId` in PROJECT root node
  - Estimated Time: 20 minutes

---

## 📊 Progress Tracking

**Completed**: 6/10 (60%)  
**Remaining**: 4/10 (40%)

**Critical Bugs Fixed**: 2/2 ✅
**Performance Fixes**: 4/4 ✅ (UI, Audio, MIDI, DSP)

---

## 🎯 Recommended Fix Order

**Session 3 (Next):**
1. Roast #8 - Stored ID Counter (20 min) ⭐ **Quick win**
2. Roast #9 - Input Routing (1 hour)
3. Roast #4 - Async Recording Prep (1 hour)

**Session 4:**
4. Roast #7 - Proper Compressor (2-3 hours) 

---

## 🧪 Testing Status

- [x] Compile tests (passed)
- [ ] Unit tests for shared_ptr snapshots
- [ ] Integration test: Delete tracks during playback
- [ ] Integration test: Add/remove plugins during playback
- [ ] Stress test: 100 tracks with plugins
- [ ] Thread Sanitizer run
- [ ] Address Sanitizer run
- [ ] Valgrind memory leak check
