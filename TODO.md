# TODO - Zenith DAW Development Roadmap

**Last Updated**: December 11, 2025  
**Version**: 0.1.0-alpha

This document tracks planned work organized by priority and timeline.

---

## 🔥 Critical - Do This Week

### 1. Fix Debug Build Compilation ✅
**File**: `apps/desktop/Source/engine/Track.h`  
**Status**: Fixed by refactoring and cleanup.

---

### 2. Add One Real Test ✅
**File**: `apps/desktop/Source/tests/AudioEngineTests.cpp`
**Status**: Added `BasicAudioTest` with real assertions.

---

### 3. Enable Sanitizers in Debug Builds ✅
**File**: `CMakeLists.txt`
**Status**: Sanitizer flags added to CMake configuration.

---

### 4. Update README to Match Reality ✅
**Files**: `README.md`, `docs/KNOWN_ISSUES.md`
**Status**: Updated to reflect alpha status, mocked AI, and current focus.

---

## 🔴 High Priority - Do This Month

### 5. Wire Up Real Grok API ⏱️ 4 hours
**Files**: 
- `apps/desktop/Source/network/AIBridgeClient.cpp`
- `apps/desktop/Source/ui/WingmanPanel.cpp`

**Current State**: Uses `MockAIProvider`

**Goal**: Connect to real Grok API

**Steps**:
1. **Add API Key Configuration**:
   - Create `Settings → AI → Grok API Key` field
   - Store in `SecureKeyStore` or config file
   - UI to paste key from https://console.x.ai/

2. **Replace Mock Provider**:
```cpp
// AIBridgeClient.cpp - Replace line 106
// OLD:
juce::String responseBody = MockAIProvider::processRequest(request.jsonPayload);

// NEW:
if (!grokClient_) {
    grokClient_ = std::make_unique<GrokAPIClient>();
    grokClient_->setAPIKey(getAPIKeyFromSettings());
}

grokClient_->sendChat(
    naturalLanguage,
    GrokMode::Fast,
    commandFunctions_,
    "You are Wingman, a DAW assistant...",
    [this](juce::String response) { handleGrokResponse(response); },
    [this](GrokFunctionCall call) { handleFunctionCall(call); },
    [this](juce::String error) { handleGrokError(error); }
);
```

3. **Handle Errors**:
   - Network timeout (show "Connecting..." message)
   - Invalid API key (show "Check API key" error)
   - Rate limiting (queue requests)

4. **Test**:
   - Get Grok API key from https://console.x.ai/
   - Send test message: "Create a new audio track"
   - Verify real response vs mock

**Acceptance Criteria**:
- [ ] Real Grok API responses shown in Wingman panel
- [ ] Error messages when network unavailable
- [ ] API key configurable in UI
- [ ] No crashes if API key invalid

---

### 6. Implement Plugin Crash Recovery ⏱️ 8 hours
**File**: `apps/desktop/Source/engine/PluginHost.cpp`

**Current State**: Plugin scanner hangs/crashes app

**Goal**: Isolate plugin scanning to prevent crashes

**Approach**: Out-of-process scanning

**Steps**:
1. **Create Scanner Subprocess**:
```cpp
// PluginScanner.exe - separate process
int main(int argc, char* argv[])
{
    if (argc < 2) return 1;
    
    juce::String pluginPath = argv[1];
    
    // Try to load plugin
    juce::PluginDescription desc;
    if (scanPlugin(pluginPath, desc)) {
        // Write to stdout as JSON
        std::cout << descriptionToJSON(desc) << std::endl;
        return 0;
    }
    return 1;
}
```

2. **Update PluginHost to Use Subprocess**:
```cpp
void PluginHost::scanPlugin(const juce::File& pluginFile)
{
    juce::ChildProcess scanner;
    juce::StringArray args;
    args.add("PluginScanner.exe");
    args.add(pluginFile.getFullPathName());
    
    if (scanner.start(args, juce::ChildProcess::wantStdOut))
    {
        // Wait max 5 seconds
        if (scanner.waitForProcessToFinish(5000))
        {
            juce::String output = scanner.readAllProcessOutput();
            if (!output.isEmpty()) {
                // Parse JSON and add to known plugins
                addPluginFromJSON(output);
            }
        }
        else
        {
            // Timeout - kill and blacklist
            scanner.kill();
            blacklistPlugin(pluginFile);
        }
    }
}
```

3. **Add Blacklist**:
- Store crashed plugins in `PluginBlacklist.xml`
- Skip blacklisted plugins on next scan
- UI to view/clear blacklist

4. **Test**:
- Scan folder with problematic plugins
- Verify no hang/crash
- Check blacklist populated

**Acceptance Criteria**:
- [ ] Plugin scan never hangs app
- [ ] Crashed plugins blacklisted
- [ ] Can scan 1000+ plugins safely
- [ ] Progress bar shows scan status

---

### 7. Fix Thread Safety Issues ⏱️ 16 hours
**Files**: Multiple

**Current State**: 
- Uses mutexes in audio thread (not lock-free)
- No validation of thread safety claims

**Goal**: Make audio thread truly lock-free

**Steps**:
1. **Audit Audio Thread Code**:
   - Find all `CriticalSection` usage in audio callbacks
   - Find all heap allocations in audio thread
   - Find all blocking operations

2. **Fix Track MIDI Notes**:
```cpp
// Track.h - Replace mutex with lock-free structure
// OLD:
juce::CriticalSection activeNotesLock;
std::vector<ActiveNote> activeNotes;

// NEW: Use lock-free queue
juce::AbstractFifo noteFifo{128};  // Fixed-size FIFO
std::array<ActiveNote, 128> noteBuffer;  // Pre-allocated
```

3. **Add Thread Safety Assertions**:
```cpp
void Track::getNextAudioBlock(...)
{
    // Add assertion to catch mistakes
    jassert(juce::MessageManager::getInstance()->currentThreadHasLockedMessageManager() == false);
    // This ensures we're NOT on the message thread
}
```

4. **Enable TSAN**:
```cmake
# CMakeLists.txt
if(ENABLE_SANITIZERS AND NOT MSVC)  # TSAN not on Windows yet
    add_compile_options(-fsanitize=thread)
    add_link_options(-fsanitize=thread)
endif()
```

5. **Run Under Load**:
- 100 tracks with plugins
- Rapid parameter automation
- Plugin loading during playback
- Check for data races

**Acceptance Criteria**:
- [ ] No mutexes in audio callback
- [ ] No heap allocations in audio callback
- [ ] TSAN reports no data races
- [ ] Audio thread performance stable

---

## 🟡 Medium Priority - Next Quarter

### 8. Offline Audio Export ⏱️ 24 hours
**Files**: Create new `apps/desktop/Source/engine/AudioExporter.cpp`

**Goal**: Bounce projects to audio files

**Features**:
- Export entire project
- Export selected tracks
- Export region/selection
- Export stems (one file per track)

**Implementation**:
```cpp
class AudioExporter
{
public:
    struct ExportSettings {
        juce::File outputFile;
        int sampleRate = 48000;
        int bitDepth = 24;
        Format format = Format::WAV;  // WAV, AIFF, FLAC, MP3
        bool normalize = true;
        bool exportStems = false;
        double startTime = 0.0;
        double endTime = -1.0;  // -1 = end of project
    };
    
    void exportProject(ProjectState& state, Engine& engine, 
                      const ExportSettings& settings,
                      std::function<void(float progress)> progressCallback);
};
```

**Steps**:
1. Create non-realtime rendering pipeline
2. Add export dialog UI
3. Support multiple formats (WAV, AIFF, FLAC)
4. Add normalization option
5. Progress bar + cancel button

---

### 9. Split Track Class ⏱️ 16 hours
**File**: `apps/desktop/Source/engine/Track.h`

**Current**: 450+ lines, god class

**Goal**: Separate concerns

**New Structure**:
```
Track (base class - common interface)
├── AudioTrack (audio clip playback)
├── MIDITrack (MIDI scheduling)
├── InstrumentTrack (instrument hosting)
└── BusTrack (aux/submix)
```

**Steps**:
1. Create `TrackBase` abstract class
2. Move audio-specific code to `AudioTrack`
3. Move MIDI-specific code to `MIDITrack`
4. Move instrument code to `InstrumentTrack`
5. Update Engine to handle polymorphic tracks
6. Update UI components

---

### 10. Enable ONNX Stem Separation ⏱️ 8 hours
**File**: `CMakeLists.txt`, `apps/desktop/Source/dsp/ONNXStemSeparator.cpp`

**Goal**: Get real ML-based stem separation working

**Steps**:
1. Add ONNX Runtime to vcpkg:
```bash
vcpkg install onnxruntime:x64-windows
```

2. Update CMake:
```cmake
option(ZENITH_USE_ONNX_RUNTIME "Enable ONNX Stem Separation" ON)

if(ZENITH_USE_ONNX_RUNTIME)
    find_package(onnxruntime REQUIRED)
    target_link_libraries(ZenithDAW PRIVATE onnxruntime::onnxruntime)
    target_compile_definitions(ZenithDAW PRIVATE ZENITH_USE_ONNX_RUNTIME=1)
endif()
```

3. Test with Demucs model (already in repo)
4. Add UI for stem separation:
   - Right-click track → "Separate Stems"
   - Progress dialog
   - Creates 4 new tracks (vocals, drums, bass, other)

---

## 🟢 Low Priority - Backlog

### 11. Complete Session View ⏱️ 40 hours
**File**: `apps/desktop/Source/ui/skia/views/SessionViewComponent.cpp`

**Goal**: Ableton-style clip launcher

**Features**:
- Clip slots (8x8 grid per track)
- Launch clips from UI
- Scene launch (all clips in row)
- Follow actions (clip looping, next clip, stop)

---

### 12. Cross-Platform Support ⏱️ 80 hours
**Goal**: macOS and Linux builds

**Challenges**:
- Windows-specific APIs (WASAPI, ASIO)
- Skia dependencies differ per platform
- File paths (Windows backslash vs Unix forward slash)

**Steps**:
1. Audit Windows-specific code
2. Add macOS CMake configuration
3. Test on macOS (CoreAudio)
4. Add Linux support (ALSA/JACK)

---

### 13. Performance Profiling ⏱️ 16 hours
**Goal**: Find and fix performance bottlenecks

**Tools**:
- Visual Studio Profiler
- Tracy Profiler
- Intel VTune

**Metrics to Track**:
- Audio callback time (must stay under buffer duration)
- Plugin processing time
- UI frame rate
- Memory allocations in hot paths

---

### 14. Comprehensive Test Suite ⏱️ 80 hours
**Goal**: 80% code coverage

**Test Categories**:
- Unit tests (individual classes)
- Integration tests (components together)
- Regression tests (bugs don't come back)
- Performance tests (benchmarks)
- Stress tests (100+ tracks, 1000+ clips)

---

### 15. Documentation Overhaul ⏱️ 24 hours
**Goal**: Professional-quality docs

**Deliverables**:
- API reference (Doxygen)
- User manual (getting started, tutorials)
- Developer guide (architecture, contributing)
- Video tutorials (YouTube)

---

## 📊 Effort Summary

| Priority | Tasks | Hours | Status |
|----------|-------|-------|--------|
| Critical | 4     | ~5h   | 0% complete |
| High     | 3     | ~28h  | 0% complete |
| Medium   | 3     | ~48h  | 0% complete |
| Low      | 6     | ~240h | 0% complete |
| **Total**| **16**| **~321h** | **0% complete** |

**Estimated Timeline**: 8 weeks full-time work

---

## 🎯 Recommended Order

**Week 1-2**: Critical items (fix build, add tests, sanitizers)  
**Week 3-4**: High priority (real AI, plugin safety, thread safety)  
**Week 5-6**: Medium priority (export, refactor, stem separation)  
**Week 7-8**: Low priority (session view, docs, polish)

---

## 📝 Notes

- Estimates are rough - expect 1.5x-2x actual time
- Some tasks block others (e.g., must fix build before adding tests)
- Don't start new features until critical issues fixed
- Test each feature thoroughly before moving to next

---

**Last Updated**: December 11, 2025  
**Next Review**: Weekly until critical path complete
