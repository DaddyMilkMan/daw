# Known Issues

**Last Updated**: February 3, 2026  
**Version**: 0.1.0-alpha

**HONEST STATUS: This DAW is 18+ months from production-ready. See ZENITH_DAW_BRUTAL_ASSESSMENT.md for details.**

This document lists all known bugs, limitations, and unfinished features in Zenith DAW.

---

## 🔴 Critical Issues (Blocking Release)

### 1. Debug Build Compilation Failure
**Location**: `modules/zenith_core/engine/Track.h:426`  
**Severity**: Critical  
**Status**: Fixed (Resolved by refactoring/cleanup)  

---

### 2. AI Assistant Uses Mock Responses
**Location**: `apps/desktop/Source/network/GrokDAWClient.cpp`  
**Severity**: High  
**Status**: ✅ Fixed (January 2026)  

**Solution:**
- WingmanPanel now uses real Grok API via GrokDAWController → GrokDAWClient
- Models used:
  - **Default**: `grok-4.1-fast` (non-reasoning, low latency)
  - **Brain icon ON**: `grok-4.1-fast-reasoning` (fast with reasoning/thinking)
- API key loaded from environment variable `GROK_API_KEY` or `XAI_API_KEY`
- Fallback to SecureKeyStore for persistent storage

**Setup:**
1. Set environment variable: `export GROK_API_KEY=xai-your-key-here`
2. Or configure via Settings UI in the app

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

### 4. Test Coverage Improved
**Location**: `apps/desktop/Source/tests/`  
**Severity**: Medium  
**Status**: Improved  

**Details:**
- 37 test categories now exist with real assertions
- Audio engine tests validate actual audio processing
- Project state tests verify ValueTree operations
- CRDT sync tests (1 known failure in LWW resolution)
- Plugin automation tests
- Accessibility tests

**Remaining Work:**
- Fix CRDT concurrent edit test
- Add more integration tests
- Increase coverage to 80%

---

## 🟠 High Priority Issues

### 5. VST3 Scanner Crashes on Some Plugins
**Location**: `modules/zenith_core/engine/PluginHost.cpp`  
**Severity**: High  
**Status**: Partially Mitigated  

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

**Fix Applied**:
1. ✅ Out-of-process scanning via `ZenithPluginScanner`
2. ✅ Per-plugin timeout (5 seconds) with kill-on-timeout

**Remaining Work**:
1. Add persistent blacklist of crashing plugins
2. Improve failure logging and surface in UI
3. Add retry policy for transient scan failures

---

### 6. Track Class is Too Large (God Class)
**Location**: `modules/zenith_core/engine/Track.h`  
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

### 7. Thread Safety Not Fully Validated
**Location**: Multiple files  
**Severity**: High  
**Status**: Not Fixed  

**Problem:**
- RT-safety audit is incomplete across engine modules
- Some locks still exist (e.g., `juce::SpinLock` for sidechain routing)
- AddressSanitizer and ThreadSanitizer disabled in CMake
- No thread safety tests

**Fix Required**:
1. Enable ASAN/TSAN in debug builds
2. Eliminate remaining locks in audio-thread paths
3. Audit all audio thread code paths
4. Add thread safety tests

---

## 🟡 Medium Priority Issues

### 8. Memory Leaks - FIXED (2026-01-21)
**Location**: RealTimeGarbageCollector.cpp, cmake/CompilerFlags.cmake  
**Severity**: High (was causing 301+ leaked objects in tests)  
**Status**: ✅ RESOLVED  

**Problem (Found):**
- RealTimeGarbageCollector::ensureClean() had critical bug: cleared pending deleters without executing them
- Caused 301 AudioPluginInstance, 602 OwnedArray, and 1 PluginAutomationBinding leaks in tests
- Sanitizers were disabled by default, preventing early detection

**Fix Applied (2026-01-21)**:
1. ✅ Fixed RealTimeGarbageCollector::ensureClean() to properly execute all pending deleters
2. ✅ Enabled LeakSanitizer by default in Debug builds (cmake/CompilerFlags.cmake)
3. ✅ Added lsan.supp suppression file for known ONNX Runtime false positives
4. ✅ Configured CMakeLists.txt to use suppression file

**Verification**:
- Run tests with: `LSAN_OPTIONS=suppressions=lsan.supp ./ZenithDAWTests`
- Should now report zero memory leaks from application code
- ONNX Runtime allocations suppressed (known library behavior)

---

### 9. Offline Audio Export is Limited
**Location**: `modules/zenith_core/engine/EngineExport.cpp`  
**Severity**: Medium  
**Status**: Partially Implemented  

**Problem:**
- Offline export exists but UI polish and workflows are incomplete
- Stems export and batch region export are still missing

**Fix Required**:
1. Finish export UI (format, sample rate, bit depth)
2. Add stems and region export options
3. Improve progress reporting/cancellation

---

### 10. Session View Unclear Status
**Location**: `modules/zenith_ui/ui/skia/views/SessionViewComponent.h`  
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
| Critical | 4     | 3     | 1         |
| High     | 3     | 2     | 1         |
| Medium   | 3     | 0     | 3         |
| Low      | 3     | 0     | 3         |
| **Total**| **13**| **5** | **8**     |

---

## 🎯 Recommended Fix Order

1. ✅ **Fix `Track.h:426` compilation error** - DONE
2. ✅ **Add real tests with assertions** - DONE (37 test categories)
3. ✅ **Documentation cleanup** - DONE (Jan 2026)
4. ✅ **Fix AI assistant** - DONE (Jan 2026) - Now uses real grok-4.1-fast API
5. **Enable sanitizers in debug builds** (30 minutes)
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

**Last Reviewed**: January 3, 2026  
**Next Review**: Weekly until critical issues resolved
