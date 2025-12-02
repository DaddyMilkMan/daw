# 🎯 OPERATION POLISH - PHASE 2 EXECUTION PLAN

**Status**: IN PROGRESS  
**Lead**: Dream Team + User Direction

---

## ✅ Decisions Made

1. **Skia Rendering**: OPTION A - Commit to Skia fully, remove JUCE fallbacks
2. **Stubbed Functions**: IMPLEMENT critical ones, not remove
3. **Testing Priority**: Audio Engine tests → ProjectState tests

---

## 📋 Task Breakdown

### ✅ Task 1: Logging System (COMPLETE)
- [x] Create `ZenithLogger.h` - Centralized logger with levels
- [x] Create `ZenithLogger.cpp` - Thread-safe implementation
- [x] Provides macros to replace 120+ DBG() calls
- **Impact**: Proper production logging, no console spam

---

### 🔄 Task 2: Skia-Only Transition (IN PROGRESS)
**Goal**: Remove all `#ifdef ZENITH_USE_SKIA` fallback paths

**Sub-tasks**:
- [ ] Audit all files with `#ifdef ZENITH_USE_SKIA`
- [ ] Remove JUCE rendering fallbacks from:
  - MainComponent
  - ArrangerComponent  
  - All UI components in `Source/ui/`
- [ ] Make Skia the ONLY renderer
- [ ] Update build scripts to always build with Skia
- [ ] Test build and runtime

**Estimated**: ~20 files to modify

---

### 🔄 Task 3: Implement Critical Stubs (NEXT)
**Goal**: Replace `ignoreUnused()` with actual implementations

**Priority stubs to implement**:
1. **Export Engine** (`ExportEngine.cpp:281`)
   - Implement offline rendering
   - Connect to Engine::renderOffline()
   
2. **Plugin State** (`Track.cpp:523`)
   - Implement loadPluginState()
   - Ensure plugin recall works
   
3. **Instrument Parameters** (ZenithSampler, ZenithPolySynth)
   - Remove placeholder parameters
   - Ensure preset system works

**Lower priority** (document as "Coming Soon"):
- Osc 2/3 (already labeled in UI)
- Stem separation (ONNX integration)
- Async plugin scanning

---

### 🔄 Task 4: Audio Engine Tests (PRIORITY)
**Goal**: Create unit tests for core audio processing

**Test coverage**:
- [ ] Track audio processing (mix/solo/mute)
- [ ] Clip playback (start/stop/loop)
- [ ] MIDI routing
- [ ] Plugin hosting basics
- [ ] Mixer channel processing

**Framework**: JUCE UnitTest or Catch2

---

### 🔄 Task 5: ProjectState Tests
**Goal**: Test data integrity and undo/redo

**Test coverage**:
- [ ] ValueTree mutations
- [ ] Undo/Redo operations
- [ ] Track add/remove
- [ ] Clip manipulation
- [ ] Automation data

---

### ⏳ Task 6: Update CMakeLists.txt
**Goal**: Add new source files to build

**Files to add**:
- `Source/ui/ZenithTheme.cpp`
- `Source/engine/ZenithLogger.cpp`
- Test files (once created)

---

## 🎯 Success Metrics

- [ ] Zero `#ifdef ZENITH_USE_SKIA` in UI code
- [ ] All critical TODOs resolved or documented
- [ ] 80%+ reduction in `ignoreUnused()` calls
- [ ] 20+ passing audio engine tests
- [ ] 15+ passing ProjectState tests
- [ ] Clean build with Skia
- [ ] Application runs without crashes

---

## 📊 Progress Tracker

| Task | Status | Progress |
|------|--------|----------|
| Logging System | ✅ Done | 100% |
| Skia Transition | 🔄 In Progress | 5% |
| Implement Stubs | ⏳ Queued | 0% |
| Audio Tests | ⏳ Queued | 0% |
| ProjectState Tests | ⏳ Queued | 0% |
| CMakeLists Update | ⏳ Queued | 0% |

---

**Next Action**: Begin Skia-only transition by auditing all `#ifdef` usage
