# Zenith DAW - Production Readiness Roadmap

**Status**: Verified Assessment  
**Date**: February 4, 2026  
**Current State**: ~6/10 feature complete, ~3.5/10 production ready  

---

## Executive Summary

This roadmap addresses the critical gaps identified in the comprehensive codebase review. The assessment findings have been **verified** and this document provides actionable, production-quality implementation tasks.

### Critical Findings Confirmed

| Issue | Status | Evidence |
|-------|--------|----------|
| Duplicate Transport UI | ✅ Confirmed | `TransportBar` (legacy) + `SkiaTransportBar` (new) both active |
| AI Jam View Unwired | ✅ Confirmed | `setGrokController()` is empty stub |
| Preset Browser Stub | ✅ Confirmed | Header explicitly labeled "STUB" |
| Clip Editor Waveform TODO | ✅ Confirmed | Line 115 in ClipEditorWindow.cpp |
| WingmanSynthBridge | ⚠️ Partial | Instantiated but CommandAPI handlers missing |
| ACTUAL_STATUS.md Stale | ✅ Confirmed | Claims bridge not instantiated, but it IS |

---

## Phase 1: UI Stack Consolidation (Week 1)

### 1.1 Remove Legacy Transport Bar Duplication

**Problem**: Both `TransportBar` (legacy) and `SkiaTransportBar` (new) are rendered simultaneously.

**Files Affected**:
- `apps/desktop/Source/ui/common/MainWindow.cpp` (lines 89-158, 467-472)
- `apps/desktop/Source/ui/views2/ZenithMainLayout.cpp` (lines 29-31)

**Implementation**:

```cpp
// In MainWindow.cpp - Remove legacy transport instantiation
MainComponent::MainComponent(...) {
    // REMOVE: Lines 89-158 (legacy transportBar creation)
    // REMOVE: Lines 467-472 (legacy transportBar drawing in drawSkiaContent)
    
    // KEEP: newUILayout which contains SkiaTransportBar
}
```

**Migration Requirements**:
1. Port all callbacks from legacy `TransportBar` to `SkiaTransportBar`:
   - `onWingmanClicked` → Wire to toggleWingman()
   - `onSettingsClicked` → Wire to settings panel
   - `onExportAudio` → Wire to export dialog
   - Update check integration

2. Ensure `SkiaTransportBar` has feature parity:
   - CPU meter display
   - Update available indicator
   - MIDI activity indicator

**Acceptance Criteria**:
- [ ] Single transport bar rendered
- [ ] All transport callbacks functional
- [ ] No visual duplication
- [ ] Hub mode still works correctly

---

### 1.2 Unify UI Stack Decision

**Decision Required**: Full migration to `views2/` stack OR gradual cutover

**Recommended Approach**: Full migration (cleaner long-term)

**Implementation Steps**:

1. **Audit Legacy UI Components**:
   ```bash
   find apps/desktop/Source/ui -type f \( -name "*.cpp" -o -name "*.h" \) \
     ! -path "*/views2/*" \
     ! -path "*/framework/*" \
     ! -path "*/design-system/*" \
     ! -path "*/common/*" \
     ! -path "*/dialogs/*" \
     ! -path "*/controls/*" \
     | sort
   ```

2. **Migration Priority**:
   | Component | Status | Action |
   |-----------|--------|--------|
   | TransportBar | ✅ Replacement exists | Remove after 1.1 |
   | MainLayoutComponent | ✅ Replacement exists | Remove after views2 stable |
   | PianoRollComponent | ⚠️ No replacement | Create SkiaPianoRoll |
   | MixerPanel | ⚠️ No replacement | Create SkiaMixer |
   | ArrangerClip | ⚠️ Partial | Complete SkiaArrangementView |

3. **File Cleanup Plan**:
   - Move legacy UI to `ui/legacy/` archive
   - Update CMakeLists.txt
   - Update include paths

---

## Phase 2: Complete AI Jam Integration (Week 2)

### 2.1 Wire AI Jam View to Grok Controller

**Problem**: `SkiaAIJamView::setGrokController()` stores pointer but never uses it.

**Files**:
- `apps/desktop/Source/ui/views2/ai-jam/SkiaAIJamView.cpp` (lines 294-296)
- `apps/desktop/Source/ui/views2/ai-jam/SkiaAIJamView.h`

**Implementation**:

```cpp
// SkiaAIJamView.h - Add proper integration
class SkiaAIJamView : public SkiaComponent {
    // ... existing code ...
    
private:
    zenith::GrokDAWController* grokController_ = nullptr;
    
    // Add async request handling
    std::unique_ptr<juce::ThreadPool> aiThreadPool_;
    std::atomic<bool> hasPendingRequest_{false};
    
    // Add proper callback wiring
    void onSubmitPromptInternal(const juce::String& prompt);
    void handleAIResponse(const juce::var& response);
};

// SkiaAIJamView.cpp - Implement real integration
void SkiaAIJamView::submitPrompt(const juce::String& prompt) {
    if (!grokController_) {
        // Fallback to demo mode
        submitPromptDemoMode(prompt);
        return;
    }
    
    // Real implementation
    setThinking(true);
    
    // Use ThreadPool for async - NOT audio thread
    aiThreadPool_->addJob([this, prompt]() {
        auto response = grokController_->generateStems(prompt);
        
        // Message thread callback
        juce::MessageManager::callAsync([this, response]() {
            handleAIResponse(response);
        });
    });
}
```

### 2.2 Enable AI Jam View Switching

**Problem**: ViewSwitcher has AIJam enum but switching is disabled.

**Files**:
- `apps/desktop/Source/ui/views2/ZenithMainLayout.cpp` (lines 57-66)
- `apps/desktop/Source/ui/views2/core/ViewSwitcher.h` (lines 46-50)

**Implementation**:

1. Add AIJam view instance to ViewSwitcher:
```cpp
// ViewSwitcher.h
private:
    std::unique_ptr<SkiaArrangementView> arrangementView_;
    std::unique_ptr<SkiaSessionView> sessionView_;
    std::unique_ptr<SkiaAIJamView> aiJamView_;  // ADD
```

2. Enable view switching in ZenithMainLayout:
```cpp
// ZenithMainLayout.cpp - setupCallbacks()
transportBar_->onViewChange = [this](SkiaTransportBar::ActiveView view) {
    switch (view) {
        case SkiaTransportBar::ActiveView::Arrangement:
            viewSwitcher_->setActiveView(ViewType::Arrangement);
            break;
        case SkiaTransportBar::ActiveView::Session:
            viewSwitcher_->setActiveView(ViewType::Session);
            break;
        case SkiaTransportBar::ActiveView::AIJam:  // ADD
            viewSwitcher_->setActiveView(ViewType::AIJam);
            break;
    }
};
```

**Acceptance Criteria**:
- [ ] AI Jam view accessible via transport bar
- [ ] Real Grok API calls functional
- [ ] Async response handling (no UI blocking)
- [ ] Proper error handling for API failures

---

## Phase 3: Preset Browser Implementation (Week 3)

### 3.1 Full Skia Preset Browser

**Problem**: `PresetBrowserComponent.h` is an explicit stub.

**Implementation**:

Create new files:
- `apps/desktop/Source/ui/views2/browser/SkiaPresetBrowser.h`
- `apps/desktop/Source/ui/views2/browser/SkiaPresetBrowser.cpp`

**Requirements**:

```cpp
class SkiaPresetBrowser : public SkiaComponent {
public:
    // Core functionality
    void loadPresetLibrary(const juce::File& libraryPath);
    void searchPresets(const juce::String& query);
    void filterByCategory(const juce::String& category);
    void filterByTags(const juce::StringArray& tags);
    
    // Preview
    void previewPreset(const Preset& preset);
    void stopPreview();
    
    // Callbacks
    std::function<void(const Preset&)> onPresetSelected;
    std::function<void(const Preset&)> onPresetLoadRequest;
    
    // Drawing
    void drawSkia(SkCanvas* canvas) override;
    
private:
    struct PresetListItem {
        Preset preset;
        SkRect bounds;
        bool isHovered = false;
        bool isSelected = false;
    };
    
    std::vector<PresetListItem> items_;
    std::unique_ptr<zenith::ZenithPresetManager> presetManager_;
    
    // Virtual scrolling
    float scrollOffset_ = 0.0f;
    float contentHeight_ = 0.0f;
    
    void drawPresetItem(SkCanvas* canvas, const PresetListItem& item);
    void drawSearchBar(SkCanvas* canvas, const SkRect& bounds);
    void drawCategorySidebar(SkCanvas* canvas, const SkRect& bounds);
};
```

**Acceptance Criteria**:
- [ ] Browse presets by category
- [ ] Real-time search filtering
- [ ] Tag-based filtering
- [ ] Preview on hover/select
- [ ] Proper integration with ZenithPolySynth

---

## Phase 4: Clip Editor Waveform (Week 4)

### 4.1 Implement Waveform Rendering

**Problem**: `ClipEditorWindow.cpp` line 115 has TODO for waveform loading.

**Implementation**:

```cpp
// ClipEditorWindow.cpp - drawAudioClipEditor()
void ClipEditorWindow::ContentComponent::drawAudioClipEditor(SkCanvas* canvas, 
                                                              float width, 
                                                              float height) {
    using namespace design;
    
    // Header
    // ... existing code ...
    
    // Waveform display area
    float waveformTop = 60.0f;
    float waveformHeight = height - 120.0f;
    SkRect waveRect = SkRect::MakeXYWH(10.0f, waveformTop, width - 20.0f, waveformHeight);
    
    // Draw waveform background
    SkPaint bgPaint;
    bgPaint.setColor(toSkColor(colors::surface::elevated));
    canvas->drawRect(waveRect, bgPaint);
    
    // Load waveform if needed
    if (!audioFilePath_.isEmpty() && !waveformLoaded_) {
        loadWaveformAsync(audioFilePath_);
    }
    
    // Draw cached waveform
    if (waveformCache_.isValid()) {
        SkPaint wavePaint;
        wavePaint.setColor(toSkColor(colors::accent::primary));
        
        // Draw cached SkPicture or regenerate from samples
        canvas->save();
        canvas->clipRect(waveRect);
        canvas->translate(waveRect.left(), waveRect.top());
        canvas->drawPicture(waveformCache_);
        canvas->restore();
    } else {
        // Draw loading indicator
        drawLoadingIndicator(canvas, waveRect.centerX(), waveRect.centerY());
    }
    
    // Draw playhead
    drawPlayhead(canvas, waveRect, currentPlayPosition_);
    
    // Draw selection
    if (hasSelection_) {
        drawSelectionOverlay(canvas, waveRect, selectionStart_, selectionEnd_);
    }
}

// New method - async waveform generation
void ClipEditorWindow::ContentComponent::loadWaveformAsync(const juce::File& audioFile) {
    waveformLoading_ = true;
    
    juce::Thread::launch([this, audioFile]() {
        // Use juce::AudioFormatReader to load samples
        auto* formatReader = formatManager_.createReaderFor(audioFile);
        if (!formatReader) {
            waveformLoading_ = false;
            return;
        }
        
        // Generate overview (reduced sample set for display)
        const int numSamples = static_cast<int>(formatReader->lengthInSamples);
        const int targetPoints = 2000;  // Resolution
        const int samplesPerPoint = numSamples / targetPoints;
        
        std::vector<float> minValues, maxValues;
        minValues.reserve(targetPoints);
        maxValues.reserve(targetPoints);
        
        // Read in chunks for performance
        juce::AudioBuffer<float> tempBuffer(1, samplesPerPoint);
        
        for (int i = 0; i < targetPoints; ++i) {
            formatReader->read(&tempBuffer, 0, samplesPerPoint, 
                              i * samplesPerPoint, true, false);
            
            float min = 0.0f, max = 0.0f;
            auto* samples = tempBuffer.getReadPointer(0);
            
            for (int s = 0; s < samplesPerPoint; ++s) {
                min = std::min(min, samples[s]);
                max = std::max(max, samples[s]);
            }
            
            minValues.push_back(min);
            maxValues.push_back(max);
        }
        
        delete formatReader;
        
        // Create SkPicture on message thread
        juce::MessageManager::callAsync([this, minValues, maxValues]() {
            createWaveformPicture(minValues, maxValues);
            waveformLoaded_ = true;
            waveformLoading_ = false;
            repaint();
        });
    });
}
```

**Acceptance Criteria**:
- [ ] Async waveform generation (no UI blocking)
- [ ] Configurable resolution
- [ ] Stereo waveform display
- [ ] Zoom and scroll
- [ ] Selection highlighting

---

## Phase 5: CommandAPI Synth Handlers (Week 5)

### 5.1 Implement Missing Synth Commands

**Problem**: WingmanSynthBridge exists but CommandAPI has no synth command handlers.

**Files to Create/Modify**:
- `apps/desktop/Source/commands/CommandAPI_SynthHandlers.cpp` (extend)
- `apps/desktop/Source/commands/CommandAPI.h` (add declarations)

**Implementation**:

```cpp
// CommandAPI.h - Add to CommandID enum
enum class CommandID {
    // ... existing commands ...
    
    // Synth Commands
    SetSynthOscillatorWave,
    SetSynthOscillatorDetune,
    SetSynthOscillatorMix,
    SetSynthFilterCutoff,
    SetSynthFilterResonance,
    SetSynthAmpEnvelope,
    SetSynthFilterEnvelope,
    SetSynthLFORate,
    SetSynthLFOAmount,
    SetSynthUnisonVoices,
    SetSynthUnisonDetune,
    RandomizeSynthPatch,
    AnalyzeCurrentPatch
};

// CommandAPI.h - Add handler declarations
class CommandAPI {
    // ... existing ...
    
private:
    // Synth Handlers
    juce::var handleSetSynthOscillatorWave(const juce::var& params);
    juce::var handleSetSynthOscillatorDetune(const juce::var& params);
    juce::var handleSetSynthFilterCutoff(const juce::var& params);
    juce::var handleSetSynthFilterResonance(const juce::var& params);
    juce::var handleSetSynthAmpEnvelope(const juce::var& params);
    juce::var handleSetSynthUnisonVoices(const juce::var& params);
    juce::var handleRandomizeSynthPatch(const juce::var& params);
    
    // Helper
    WingmanSynthBridge* getSynthBridgeForTrack(const juce::String& trackId);
};
```

```cpp
// CommandAPI_SynthHandlers.cpp
juce::var CommandAPI::handleSetSynthOscillatorWave(const juce::var& params) {
    auto trackId = params.getProperty("trackId", "").toString();
    int oscIndex = params.getProperty("oscillator", 1);
    juce::String waveType = params.getProperty("waveform", "saw").toString();
    
    auto* bridge = getSynthBridgeForTrack(trackId);
    if (!bridge) {
        return makeError("No synth found on track " + trackId);
    }
    
    OscillatorWaveform wave = parseWaveform(waveType);
    bridge->setOscillatorWaveform(oscIndex, wave);
    
    return makeSuccess("Set oscillator " + juce::String(oscIndex) + 
                      " to " + waveType);
}

WingmanSynthBridge* CommandAPI::getSynthBridgeForTrack(const juce::String& trackId) {
    auto* track = engine_.getTrack(trackId);
    if (!track) return nullptr;
    
    auto* instrument = track->getInstrument();
    if (!instrument) return nullptr;
    
    // Try to cast to ZenithPolySynth
    if (auto* synth = dynamic_cast<ZenithPolySynthProcessor*>(instrument)) {
        return synth->getWingmanBridge();
    }
    
    return nullptr;
}
```

**Acceptance Criteria**:
- [ ] All 18 synth commands implemented
- [ ] Proper error handling
- [ ] JSON command parsing validated
- [ ] Thread-safe parameter changes

---

## Phase 6: Documentation Cleanup (Week 6)

### 6.1 Archive Stale Documentation

**Stale Files**:
```
ACTUAL_STATUS.md                    → Archive (outdated, contradicts code)
docs/ZENITH_DAW_BRUTAL_ASSESSMENT.md → Review for accuracy
docs/IMPLEMENTATION_COMPLETE.md      → Archive (misleading)
```

### 6.2 Create Single Source of Truth

**New File**: `docs/PRODUCTION_STATUS.md`

```markdown
# Production Status Dashboard

**Last Updated**: [Date of last commit]
**Version**: [Git SHA]

## Completion by Module

| Module | Status | Notes |
|--------|--------|-------|
| UI Framework | 85% | Skia rendering stable |
| Transport | 90% | Consolidated to SkiaTransportBar |
| Arrangement | 70% | Core features working |
| Session | 60% | Grid functional, scenes pending |
| AI Jam | 50% | Grok wired, async pending |
| Synth | 80% | Bridge complete, UI animations pending |
| Plugin Host | 65% | Scanning stable, blacklist UI pending |
| Export | 50% | Basic export works, stems pending |

## Known Blockers

1. **Thread Safety Audit** - Not started
2. **Plugin Sandboxing** - Partial
3. **Crash Recovery** - Not implemented
```

---

## Phase 7: Testing Infrastructure (Ongoing)

### 7.1 Test Coverage Goals

| Category | Current | Target |
|----------|---------|--------|
| Unit Tests | ~40% | 80% |
| Integration | ~20% | 60% |
| UI Tests | ~10% | 40% |
| Thread Safety | 0% | 50% |

### 7.2 Critical Test Additions

1. **UI Integration Tests**:
```cpp
// Test: Transport bar play button triggers engine
TEST_F(TransportIntegrationTest, PlayButtonStartsEngine) {
    auto transport = std::make_unique<SkiaTransportBar>();
    
    bool playCalled = false;
    transport->onPlay = [&playCalled]() { playCalled = true; };
    
    // Simulate click on play button
    simulateMouseClick(transport.get(), kPlayButtonX, kPlayButtonY);
    
    EXPECT_TRUE(playCalled);
}
```

2. **Thread Safety Tests**:
```cpp
// Test: Audio thread never allocates
TEST_F(ThreadSafetyTest, AudioCallbackNoAllocation) {
    AllocationTracker tracker;
    
    // Run audio callback 1000 times
    for (int i = 0; i < 1000; ++i) {
        tracker.reset();
        engine_.processBlock(audioBuffer, midiBuffer);
        EXPECT_EQ(tracker.getAllocationCount(), 0);
    }
}
```

---

## Phase 8: Production Hardening (Weeks 7-8)

### 8.1 Crash Recovery

```cpp
// apps/desktop/Source/utils/CrashRecovery.h
class CrashRecovery {
public:
    static void installHandlers();
    static void saveEmergencyBackup();
    static juce::File getLastCrashReport();
    
private:
    static void signalHandler(int sig);
    static void exceptionHandler();
    
    static std::atomic<bool> handlingCrash_{false};
};
```

### 8.2 Plugin Sandboxing

- Complete out-of-process plugin host
- IPC for audio/MIDI
- Crash isolation

### 8.3 Auto-Update System

- Delta updates
- Rollback capability
- Signature verification

---

## Implementation Schedule

| Week | Phase | Deliverables |
|------|-------|--------------|
| 1 | UI Consolidation | Single transport bar, no duplication |
| 2 | AI Jam | Full Grok integration, view switching |
| 3 | Preset Browser | Full Skia implementation |
| 4 | Clip Editor | Waveform display, selection |
| 5 | Synth Commands | Complete CommandAPI handlers |
| 6 | Documentation | Single source of truth |
| 7-8 | Hardening | Crash recovery, sandboxing |

---

## Success Metrics

### Technical Metrics
- [ ] Zero UI duplication
- [ ] < 16ms UI frame time (60fps)
- [ ] < 5ms audio latency
- [ ] Zero memory leaks (ASAN clean)
- [ ] 80% test coverage

### User Metrics
- [ ] AI Jam generates real stems via Grok
- [ ] Preset browser browses 1000+ presets smoothly
- [ ] Clip editor displays waveforms for 10min+ audio
- [ ] Transport controls respond in < 50ms

---

## Risk Assessment

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Skia performance issues | Low | High | Profiling, fallback to JUCE |
| Grok API rate limits | Medium | Medium | Caching, local fallback |
| Thread safety bugs | Medium | Critical | TSAN, code review |
| Plugin compatibility | High | Medium | Blacklist, sandboxing |

---

**Document Owner**: Zenith DAW Team  
**Review Cycle**: Weekly until production  
**Next Review**: [Current Date + 7 days]
