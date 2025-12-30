# Known Issues

**Last Updated**: December 11, 2025  
**Version**: 0.1.0-alpha

This document lists all known bugs, limitations, and unfinished features in Zenith DAW.

---

## 🔴 Critical Issues (Blocking Release)

### 1. Debug Build Compilation Failure
**Location**: `apps/desktop/Source/engine/Track.h:426`  
**Severity**: Critical  
**Status**: Fixed (Resolved by refactoring/cleanup)  

---

### 2. AI Assistant Uses Mock Responses
**Location**: `apps/desktop/Source/network/AIBridgeClient.cpp:106`  
**Severity**: High  
**Status**: Not Fixed  

**Problem:**
```cpp
// Line 106 - Not using real Grok API
juce::String responseBody = MockAIProvider::processRequest(request.jsonPayload);
```

**Details:**
- `GrokAPIClient` class exists and is implemented
- `AIBridgeClient` is wired to use `MockAIProvider` instead
- All AI responses are simulated/canned
- Users think they're getting real AI assistance but they're not

**Workaround**: None. Feature is non-functional.

**Fix Required**: 
1. Replace `MockAIProvider::processRequest()` with `GrokAPIClient::sendChat()`
2. Add API key configuration UI
3. Handle network errors gracefully
4. Update README to reflect actual AI capabilities

---

### 3. Stem Separation Not Functional
**Location**: `apps/desktop/Source/dsp/ONNXStemSeparator.cpp`  
**Severity**: High  
**Status**: Not Fixed  

**Problem:**
```cpp
#ifdef ZENITH_USE_ONNX_RUNTIME
  // Real ONNX implementation
#else
  DBG("ONNXStemSeparator: ONNX Runtime not linked - DSP fallback only");
  return false;  // Feature disabled
#endif
```

**Details:**
- ONNX Runtime not included in default build
- CMake flag `ZENITH_USE_ONNX_RUNTIME` not set
- Falls back to basic DSP filtering (not ML-based separation)
- README claims stem separation is implemented (it's not)

**Workaround**: Compile with ONNX Runtime manually.

**Fix Required**:
1. Add ONNX Runtime to vcpkg dependencies
2. Enable `ZENITH_USE_ONNX_RUNTIME` in CMake
3. Test with actual Demucs model
4. Update README to reflect actual capabilities

---

### 4. Zero Test Coverage
**Location**: `apps/desktop/Source/tests/`  
**Severity**: High  
**Status**: Partially Fixed (Initial real tests added in `AudioEngineTests.cpp`)  

**Problem:**
```cpp
// EngineTests.cpp - Not a real test
void testLockFreeQueue()
{
    bool success = engine.queueEvent(e);
    jassert(success);
    
    // In a real test, we would check if the event was processed, 
    // but processEvents() consumes it internally.
    // This just verifies compilation and basic API.
}
```

**Details:**
- Test files exist but contain no real assertions
- Tests only verify code compiles, not behavior
- **Update (Dec 23, 2025)**: Added `BasicAudioTest` to `AudioEngineTests.cpp` which validates audio buffer content for NaN/Inf.
- Much more coverage is still needed.

**Workaround**: Manual testing only.

**Fix Required**:
1. Add real assertions to existing test files
2. Test actual behavior (audio output, MIDI scheduling, etc.)
3. Add CI/CD that runs tests on every commit
4. Aim for 80% code coverage

---

## 🟠 High Priority Issues

### 5. VST3 Scanner Crashes on Some Plugins
**Location**: `apps/desktop/Source/engine/PluginHost.cpp`  
**Severity**: High  
**Status**: Not Fixed  

**Symptoms:**
- Application hangs during plugin scan
- Some plugins cause immediate crash
- No crash recovery mechanism
- Plugin list sometimes incomplete

**Known Problematic Plugins:**
- Serum (hangs on scan)
- Native Instruments plugins (some crash)
- Older VST3 plugins with bad state save/load

**Workaround**: 
- Manually delete problematic plugins from scan folders
- Clear plugin cache: Delete `AppData/Roaming/Zenith/PluginCache.xml`

**Fix Required**:
1. Implement out-of-process plugin scanning
2. Add timeout mechanism (5 seconds max per plugin)
3. Blacklist crashing plugins
4. Log failed scans for debugging

---

### 6. Track Class is Too Large (God Class)
**Location**: `apps/desktop/Source/engine/Track.h`  
**Severity**: Medium  
**Status**: Not Fixed  

**Problem:**
- Track.h is 450+ lines
- Mixes concerns (audio, MIDI, plugins, automation, freeze, monitoring)
- Has `friend class AudioRenderer` (broken encapsulation)
- Three redundant `public:` sections (lines 60-62)

**Impact:**
- Hard to maintain
- Difficult to test in isolation
- Changes risk breaking unrelated features

**Fix Required**:
1. Split into:
   - `AudioTrack` (audio-specific logic)
   - `MIDITrack` (MIDI scheduling)
   - `InstrumentTrack` (instrument hosting)
   - `BusTrack` (aux buses)
2. Remove `friend class AudioRenderer`
3. Refactor to use interfaces/abstractions

---

### 7. Thread Safety Not Validated
**Location**: Multiple files  
**Severity**: High  
**Status**: Not Fixed  

**Problem:**
- `Track.h:432` uses `juce::CriticalSection` (mutex) for MIDI notes
- Claims to be "lock-free" but uses locks in audio thread
- AddressSanitizer and ThreadSanitizer disabled in CMake
- No thread safety tests

**Known Unsafe Patterns:**
```cpp
// Track.h - Claims lock-free but uses mutex
juce::CriticalSection activeNotesLock;  // NOT LOCK-FREE!
std::vector<ActiveNote> activeNotes;
```

**Fix Required**:
1. Enable ASAN/TSAN in debug builds
2. Replace `CriticalSection` with lock-free alternatives
3. Audit all audio thread code paths
4. Add thread safety tests

---

## 🟡 Medium Priority Issues

### 8. Memory Leaks Likely
**Location**: Multiple files  
**Severity**: Medium  
**Status**: Not Confirmed  

**Problem:**
- Sanitizers disabled by default in CMakeLists.txt
- No regular leak detection during development
- Complex pointer management (unique_ptr, shared_ptr, raw pointers mixed)

**Fix Required**:
1. Enable sanitizers: `cmake -DENABLE_SANITIZERS=ON`
2. Run leak detection tools regularly
3. Add RAII wrappers for all resources
4. Document ownership patterns

---

### 9. No Offline Audio Export
**Location**: Not implemented  
**Severity**: Medium  
**Status**: Not Started  

**Problem:**
- Can't bounce/export projects to audio files
- Real-time playback only
- No stems export
- No region export

**Fix Required**:
1. Implement non-realtime rendering pipeline
2. Add export dialog (format, sample rate, bit depth)
3. Support WAV, AIFF, MP3, FLAC
4. Multi-track/stems export option

---

### 10. Session View Unclear Status
**Location**: `apps/desktop/Source/ui/skia/views/SessionViewComponent.h`  
**Severity**: Medium  
**Status**: Unknown  

**Problem:**
- File exists but functionality unclear
- Not mentioned in build logs
- README lists as "In Development"
- No documentation

**Fix Required**:
1. Document current state
2. Either finish implementation or remove placeholder
3. Update README with accurate status

---

## 🟢 Low Priority Issues

### 11. Documentation Comments Inconsistent
**Severity**: Low  
**Status**: Acknowledged  

**Problem:**
- "Phase 1", "Phase 2A", "ROAST FIX #2" comments throughout code
- Implementation history mixed with actual documentation
- Doxygen comments incomplete

**Fix Required**:
1. Move phase/history comments to commit messages
2. Keep only technical documentation in code
3. Complete Doxygen comments for all public APIs

---

### 12. Hardening Features Disabled
**Location**: `CMakeLists.txt`  
**Severity**: Low  
**Status**: Acknowledged  

**Problem:**
```cmake
option(ENABLE_HARDENING "Enable Security Hardening Flags" OFF)
option(ENABLE_SANITIZERS "Enable Address and UB Sanitizers" OFF)
```

**Fix Required**:
Enable by default in debug builds:
```cmake
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(ENABLE_SANITIZERS ON)
    set(ENABLE_HARDENING ON)
endif()
```

---

### 13. Skia Rendering May Require GPU Updates
**Severity**: Low  
**Status**: Acknowledged  

**Problem**:
- Hardware acceleration requires recent GPU drivers
- Older GPUs may not support required OpenGL/Direct3D features
- No fallback message if Skia init fails

**Workaround**: Update GPU drivers.

**Fix Required**:
1. Add graceful fallback to JUCE rendering
2. Detect GPU capabilities at startup
3. Show user-friendly error message

---

### 14. Skia Instability in Debug Builds
**Severity**: Medium
**Status**: Noted

**Problem**: Skia rendering can be extremely slow or crash when compiled in Debug mode due to heavy assertion checking and lack of optimizations.

**Workaround**: Use **Release** builds for all UI-related work.

---

## 📊 Summary

| Priority | Count | Fixed | Remaining |
|----------|-------|-------|-----------|
| Critical | 4     | 1     | 3         |
| High     | 3     | 0.5   | 2.5       |
| Medium   | 3     | 0     | 3         |
| Low      | 3     | 0     | 3         |
| **Total**| **13**| **1.5** | **11.5**  |

---

## 🎯 Recommended Fix Order

1. ✅ **Fix `Track.h:426` compilation error**
2. 🔄 **Add more real tests with assertions** (Initial tests added)
3. **Enable sanitizers in debug builds** (30 minutes)
4. **Remove broken build badge** (5 minutes)
5. **Fix AI assistant or remove feature** (4 hours)
6. **Implement plugin crash recovery** (8 hours)
7. **Split Track class** (16 hours)
8. **Add offline export** (24 hours)
9. **Everything else** (weeks)

---

## 📝 Reporting New Issues

When reporting issues:
1. Check this document first
2. Include steps to reproduce
3. Attach crash logs if applicable
4. Specify build configuration (Debug/Release)
5. List your system specs (OS, CPU, GPU, RAM)

---

**Last Reviewed**: December 11, 2025  
**Next Review**: Weekly until critical issues resolved
