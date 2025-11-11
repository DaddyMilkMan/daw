# Zenith-Core JUCE Implementation Checklist

**Target:** Hybrid JUCE + React Architecture
**Date:** 2025-11-11
**Status:** Phase 0 → Phase 1 Transition

---

## Overview

This checklist outlines the required changes to `zenith-core/` to support the hybrid architecture where:
- **JUCE** handles audio engine, plugin hosting, and windowing
- **React UI** (vexel-daw) runs inside CEF/WebView embedded in JUCE window
- **WebSocket bridge** enables bidirectional communication

---

## Phase 1: WebSocket Bridge (Weeks 1-2)

### 1.1 Add WebSocket Library

**Options:**
- [ ] **Boost.Beast** (recommended - part of Boost.Asio)
- [ ] **websocketpp** (header-only)
- [ ] **uWebSockets** (high-performance)

**CMake Integration:**
```cmake
# zenith-core/CMakeLists.txt
find_package(Boost REQUIRED COMPONENTS system)
target_link_libraries(ZenithDAW PRIVATE Boost::system)
```

### 1.2 Create WebSocketBridge Class

**Files to Create:**
- [ ] `zenith-core/include/WebSocketBridge.h`
- [ ] `zenith-core/src/WebSocketBridge.cpp`

**Class Structure:**
```cpp
class WebSocketBridge : public Thread
{
public:
    WebSocketBridge(Engine& engine);
    ~WebSocketBridge() override;

    void run() override;
    void sendEvent(const String& eventType, const var& payload);
    void shutdown();

private:
    void handleMessage(const String& message);
    void processCommand(const String& command, const var& params);

    Engine& engine;
    WebSocketServer server;
    CriticalSection connectionLock;
    Array<Connection*> connections;
};
```

### 1.3 Define Command Protocol

**Commands (React → JUCE):**
- [ ] `transport.play`
- [ ] `transport.stop`
- [ ] `transport.record`
- [ ] `transport.set_tempo`
- [ ] `transport.set_time_signature`
- [ ] `track.create`
- [ ] `track.delete`
- [ ] `track.set_volume`
- [ ] `track.set_pan`
- [ ] `track.set_mute`
- [ ] `track.set_solo`
- [ ] `clip.create`
- [ ] `clip.delete`
- [ ] `clip.move`
- [ ] `clip.resize`
- [ ] `plugin.load`
- [ ] `plugin.remove`
- [ ] `plugin.set_parameter`
- [ ] `project.save`
- [ ] `project.load`

**Events (JUCE → React):**
- [ ] `playhead.update` (30 Hz)
- [ ] `meters.update` (30 Hz)
- [ ] `transport.state_changed`
- [ ] `track.created`
- [ ] `track.deleted`
- [ ] `track.updated`
- [ ] `clip.created`
- [ ] `clip.deleted`
- [ ] `clip.updated`
- [ ] `plugin.loaded`
- [ ] `project.saved`
- [ ] `error.occurred`

### 1.4 Integrate with Engine Class

**Files to Modify:**
- [ ] `zenith-core/include/Engine.h` - Add WebSocketBridge member
- [ ] `zenith-core/src/Engine.cpp` - Start bridge in constructor, shutdown in destructor

**Example:**
```cpp
// Engine.h
class Engine
{
    // ...
    std::unique_ptr<WebSocketBridge> wsBridge;
};

// Engine.cpp
Engine::Engine()
{
    wsBridge = std::make_unique<WebSocketBridge>(*this);
    wsBridge->startThread();
}
```

### 1.5 Implement Real-Time Data Updates

**Lock-Free FIFO for Audio Thread → WebSocket Thread:**
- [ ] Create `AudioThreadEventQueue` using `AbstractFifo`
- [ ] Push playhead position from audio callback
- [ ] Push meter levels from audio callback
- [ ] Consume in WebSocket thread, batch send to React

**Example:**
```cpp
// In audio callback (real-time thread)
struct PlayheadUpdate {
    double position;
    int bar;
    int beat;
};

PlayheadUpdate update = { currentPosition, currentBar, currentBeat };
playheadQueue.push(update);

// In WebSocket thread (message thread)
while (playheadQueue.getNumReady() > 0)
{
    PlayheadUpdate update;
    playheadQueue.pop(update);
    wsBridge->sendEvent("playhead.update", createPayload(update));
}
```

### 1.6 Testing

- [ ] Unit test: WebSocket connection establishment
- [ ] Unit test: Command parsing and routing
- [ ] Integration test: Send `transport.play` from test client, verify engine starts
- [ ] Integration test: Verify playhead updates are received

---

## Phase 2: CEF Integration (Weeks 3-4)

### 2.1 Add CEF to Build System

**Download CEF:**
- [ ] Visit https://cef-builds.spotifycdn.com/
- [ ] Download CEF Standard Distribution for your platform (e.g., `cef_binary_120.0.6099.129`)
- [ ] Extract to `zenith-core/external/cef/`

**CMake Integration:**
```cmake
# zenith-core/CMakeLists.txt
set(CEF_ROOT "${CMAKE_SOURCE_DIR}/external/cef")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CEF_ROOT}/cmake")

include("${CEF_ROOT}/cmake/cef_variables.cmake")
add_subdirectory(${CEF_ROOT}/libcef_dll_wrapper libcef_dll_wrapper)

target_link_libraries(ZenithDAW PRIVATE
    libcef_dll_wrapper
    ${CEF_STANDARD_LIBS}
)

# Copy CEF binaries to output
COPY_FILES("ZenithDAW" "${CEF_BINARY_FILES}" "${CEF_BINARY_DIR}" "$<TARGET_FILE_DIR:ZenithDAW>")
COPY_FILES("ZenithDAW" "${CEF_RESOURCE_FILES}" "${CEF_RESOURCE_DIR}" "$<TARGET_FILE_DIR:ZenithDAW>")
```

### 2.2 Create CEF Application Handler

**Files to Create:**
- [ ] `zenith-core/include/CEFApp.h`
- [ ] `zenith-core/src/CEFApp.cpp`

**Class Structure:**
```cpp
class CEFApp : public CefApp,
               public CefBrowserProcessHandler,
               public CefRenderProcessHandler
{
public:
    // CefApp methods
    CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override { return this; }
    CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override { return this; }

    // CefBrowserProcessHandler methods
    void OnContextInitialized() override;

    // CefRenderProcessHandler methods
    void OnContextCreated(CefRefPtr<CefBrowser> browser,
                         CefRefPtr<CefFrame> frame,
                         CefRefPtr<CefV8Context> context) override;

private:
    IMPLEMENT_REFCOUNTING(CEFApp);
};
```

### 2.3 Create UIPanel Component

**Files to Create:**
- [ ] `zenith-core/include/UIPanel.h`
- [ ] `zenith-core/src/UIPanel.cpp`

**Class Structure:**
```cpp
class UIPanel : public juce::Component,
                public CefClient,
                public CefLifeSpanHandler,
                public CefLoadHandler,
                public CefDisplayHandler
{
public:
    UIPanel();
    ~UIPanel() override;

    // JUCE Component
    void paint(juce::Graphics& g) override;
    void resized() override;

    // CEF Client
    CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }
    CefRefPtr<CefLoadHandler> GetLoadHandler() override { return this; }
    CefRefPtr<CefDisplayHandler> GetDisplayHandler() override { return this; }

    // CEF Lifecycle
    void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;
    void DoClose(CefRefPtr<CefBrowser> browser) override;
    void OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode) override;
    void OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, ErrorCode errorCode, const CefString& errorText, const CefString& failedUrl) override;

    // Commands
    void loadURL(const juce::String& url);
    void executeJavaScript(const juce::String& js);

private:
    CefRefPtr<CefBrowser> browser;

    IMPLEMENT_REFCOUNTING(UIPanel);
};
```

### 2.4 Initialize CEF in Main.cpp

**Files to Modify:**
- [ ] `zenith-core/src/Main.cpp`

**Changes:**
```cpp
// Before JUCEApplication initialization
int main(int argc, char* argv[])
{
    #if JUCE_WINDOWS
    CefMainArgs mainArgs(GetModuleHandle(nullptr));
    #else
    CefMainArgs mainArgs(argc, argv);
    #endif

    // Execute CEF sub-processes (if running as helper)
    int exitCode = CefExecuteProcess(mainArgs, nullptr, nullptr);
    if (exitCode >= 0)
        return exitCode;

    // Initialize CEF
    CefSettings settings;
    settings.no_sandbox = true;
    settings.windowless_rendering_enabled = false;
    CefString(&settings.cache_path).FromString(getCachePath());

    CefRefPtr<CEFApp> app(new CEFApp());
    if (!CefInitialize(mainArgs, settings, app.get(), nullptr))
        return -1;

    // Continue with JUCE initialization
    juce::JUCEApplicationBase::createInstance = &juce::createApplication<ZenithDAWApplication>;
    return juce::JUCEApplicationBase::main(argc, argv);
}

// Before shutdown
void shutdown()
{
    CefShutdown();
}
```

### 2.5 Embed UIPanel in MainWindow

**Files to Modify:**
- [ ] `zenith-core/include/MainWindow.h`
- [ ] `zenith-core/src/MainWindow.cpp`

**Changes:**
```cpp
// MainWindow.h
class MainWindow : public DocumentWindow
{
    // ...
    std::unique_ptr<UIPanel> uiPanel;
};

// MainWindow.cpp
MainWindow::MainWindow(const String& name)
    : DocumentWindow(name, Colours::darkgrey, DocumentWindow::allButtons)
{
    uiPanel = std::make_unique<UIPanel>();
    uiPanel->loadURL("http://127.0.0.1:3000"); // Dev mode

    setContentOwned(uiPanel.get(), true);
    setResizable(true, true);
    centreWithSize(getWidth(), getHeight());
    setVisible(true);
}
```

### 2.6 Custom Scheme Handler (Optional)

**For production builds, serve React UI from `app://` scheme:**

- [ ] Create `CustomSchemeHandler.h` and `.cpp`
- [ ] Register custom scheme in `CEFApp::OnRegisterCustomSchemes()`
- [ ] Serve files from embedded resources or local directory

**Example:**
```cpp
void CEFApp::OnRegisterCustomSchemes(CefRawPtr<CefSchemeRegistrar> registrar)
{
    registrar->AddCustomScheme("app", CEF_SCHEME_OPTION_STANDARD | CEF_SCHEME_OPTION_CORS_ENABLED);
}

// In OnContextInitialized()
CefRegisterSchemeHandlerFactory("app", "ui", new CustomSchemeHandlerFactory());
```

### 2.7 Testing

- [ ] Verify CEF initializes without errors
- [ ] Load `http://127.0.0.1:3000` in UIPanel
- [ ] Verify React UI renders inside JUCE window
- [ ] Test window resize (CEF browser should resize)
- [ ] Test custom scheme handler (if implemented)

---

## Phase 3: Audio Engine Features (Weeks 5-8)

### 3.1 Multi-Track Recording

**Files to Modify:**
- [ ] `zenith-core/include/AudioTrack.h` - Add recording state
- [ ] `zenith-core/src/AudioTrack.cpp` - Implement recording logic
- [ ] `zenith-core/include/AudioClip.h` - Support comp lanes
- [ ] `zenith-core/src/Engine.cpp` - Manage recording sessions

**Features:**
- [ ] Arm track for recording
- [ ] Record to new clip
- [ ] Support multiple takes (comp lanes)
- [ ] Auto-create new take on each recording pass
- [ ] Punch in/out recording

### 3.2 VST3/AU Plugin Hosting

**Files to Create:**
- [ ] `zenith-core/include/PluginManager.h`
- [ ] `zenith-core/src/PluginManager.cpp`

**Features:**
- [ ] Scan for plugins using `PluginDirectoryScanner`
- [ ] Cache known plugins in `KnownPluginList`
- [ ] Load plugin instances via `AudioPluginFormatManager`
- [ ] Add plugins to `AudioProcessorGraph` as nodes
- [ ] Route audio between nodes
- [ ] Host plugin UI in separate windows
- [ ] Expose plugin list to React via WebSocket

**Reference:**
- Study `JUCE/extras/AudioPluginHost/` example

### 3.3 MIDI Editing

**Files to Modify:**
- [ ] `zenith-core/include/ProjectState.h` - Store MIDI clips in ValueTree
- [ ] `zenith-core/src/Engine.cpp` - Handle MIDI commands from React

**Features:**
- [ ] Store MIDI sequences in `MidiClip`
- [ ] Send MIDI data to React piano roll
- [ ] Receive MIDI edits from React
- [ ] Quantize MIDI notes
- [ ] Apply velocity curves
- [ ] Support MIDI CC automation

### 3.4 Automation

**Files to Create:**
- [ ] `zenith-core/include/AutomationLane.h`
- [ ] `zenith-core/src/AutomationLane.cpp`

**Features:**
- [ ] Store automation curves in `ProjectState`
- [ ] Read automation in audio callback (lock-free)
- [ ] Send automation curves to React
- [ ] Receive automation edits from React
- [ ] Support multiple automation modes (Read, Touch, Latch, Write)
- [ ] Automation for volume, pan, plugin parameters

### 3.5 File I/O

**Files to Modify:**
- [ ] `zenith-core/src/ProjectState.cpp` - Implement save/load

**Features:**
- [ ] Save project to XML (via `ValueTree::toXml()`)
- [ ] Load project from XML
- [ ] Import audio files (WAV, AIFF, MP3)
- [ ] Export mixdown (WAV, AIFF)
- [ ] Export stems
- [ ] Handle missing audio files

**WebSocket Commands:**
- [ ] `project.save { path: "..." }`
- [ ] `project.load { path: "..." }`
- [ ] `audio.import { path: "...", trackId: "..." }`
- [ ] `audio.export { path: "...", format: "wav" }`

### 3.6 Testing

- [ ] Record audio to track, verify clip created
- [ ] Load VST3 plugin, verify audio passes through
- [ ] Edit MIDI in React piano roll, verify playback
- [ ] Draw automation, verify parameter changes
- [ ] Save and load project, verify state preserved

---

## Phase 4: Wingman AI Integration (Weeks 9-10)

### 4.1 AI Command Interface

**WebSocket Commands:**
- [ ] `ai.generate_midi { prompt: "create trap beat", trackId: "..." }`
- [ ] `ai.auto_mix { targetLoudness: -14 }`
- [ ] `ai.harmonize { clipId: "...", style: "thirds" }`
- [ ] `ai.detect_key { clipId: "..." }`

### 4.2 Preview Mode

**Features:**
- [ ] Implement undo stack for AI actions
- [ ] Send preview state to React (show diff)
- [ ] Allow user to confirm/reject changes
- [ ] Rollback changes if rejected

### 4.3 Testing

- [ ] Generate MIDI via Wingman, verify clip created
- [ ] Auto-mix, verify faders adjusted
- [ ] Preview and reject changes, verify rollback

---

## Phase 5: Polish & Optimization (Weeks 11-12)

### 5.1 Performance Optimization

- [ ] Profile WebSocket message frequency
- [ ] Batch meter updates (30 Hz, not 60 Hz)
- [ ] Use lock-free FIFOs for all audio thread data
- [ ] Optimize ValueTree updates (batch changes)
- [ ] Profile CPU usage in audio callback

### 5.2 Error Handling

- [ ] Add error codes to WebSocket protocol
- [ ] Send error events to React
- [ ] Log errors to file
- [ ] Crash reporter integration

### 5.3 Testing

- [ ] Stress test: 100 tracks, 1000 clips
- [ ] Plugin crash isolation
- [ ] WebSocket reconnection handling
- [ ] Memory leak detection (Valgrind/Instruments)

---

## Summary Checklist

### Must-Have for Phase 1 (Weeks 1-2)
- [ ] WebSocket bridge implemented
- [ ] Basic command protocol working
- [ ] Transport controls functional
- [ ] Playhead updates sent to React

### Must-Have for Phase 2 (Weeks 3-4)
- [ ] CEF integrated into build
- [ ] React UI renders inside JUCE window
- [ ] Full-screen layout working
- [ ] Window resize handled correctly

### Must-Have for Phase 3 (Weeks 5-8)
- [ ] Multi-track recording
- [ ] VST3/AU plugin hosting
- [ ] MIDI editing
- [ ] Automation
- [ ] Project save/load

### Must-Have for Phase 4 (Weeks 9-10)
- [ ] Wingman AI commands working
- [ ] Audio generation integrated
- [ ] Preview/confirm workflow

### Must-Have for Phase 5 (Weeks 11-12)
- [ ] Performance optimized
- [ ] Error handling complete
- [ ] Beta testing complete

---

## Additional Resources

- **JUCE Forum:** https://forum.juce.com/
- **CEF Forum:** http://www.magpcss.org/ceforum/
- **Boost.Beast Tutorial:** https://www.boost.org/doc/libs/release/libs/beast/
- **AudioPluginHost Example:** `JUCE/extras/AudioPluginHost/`

---

**Document Version:** 1.0
**Last Updated:** 2025-11-11
**Next Review:** After Phase 1 completion
