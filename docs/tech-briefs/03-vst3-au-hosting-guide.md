# VST3 and Audio Unit (AU) Hosting Guide

**Plugin Hosting Strategy for Zenith DAW**
**Version:** 1.0
**Date:** 2025-11-10

---

## Executive Summary

This document provides a roadmap for implementing VST3 and Audio Unit plugin hosting in Zenith DAW, covering:

1. **Phase 1:** In-process hosting (quick bring-up, single-process)
2. **Phase 2:** Out-of-process sandboxing (crash protection, isolation)
3. Official SDK documentation and JUCE integration patterns
4. Platform-specific considerations (macOS codesigning/sandboxing, Windows security)

---

## Official Documentation & SDKs

### VST3 SDK (Steinberg)

**Official Resources:**
- **VST 3 Developer Portal:** https://steinbergmedia.github.io/vst3_dev_portal/
- **VST 3 SDK Documentation:** https://steinbergmedia.github.io/vst3_doc/vstsdk/index.html
- **GitHub Repository:** https://github.com/steinbergmedia/vst3sdk
- **Documentation Repository:** https://github.com/steinbergmedia/vst3_doc
- **VST 3 Interfaces Reference:** https://steinbergmedia.github.io/vst3_doc/vstinterfaces/
- **VST 3 Examples:** https://steinbergmedia.github.io/vst3_doc/vstexamples/index.html

**Licensing:**
> "VST 3 SDK is under MIT license, and code licensed under MIT license can be used, modified, and redistributed freely — including in commercial products — provided MIT license terms are followed."
> — [VST3 SDK Documentation](https://steinbergmedia.github.io/vst3_doc/vstsdk/index.html)

**What the SDK Provides:**
> "The SDK includes some plug-ins and host implementation examples, which can be helpful for understanding how to implement a VST3 host."
> — [Steinberg Developer Portal](https://steinbergmedia.github.io/vst3_dev_portal/)

### Audio Unit (Apple)

**Official Apple Documentation:**
- **Incorporating Audio Effects and Instruments:** https://developer.apple.com/documentation/AudioToolbox/incorporating-audio-effects-and-instruments
- **Audio Unit Framework Reference:** https://developer.apple.com/documentation/audiounit
- **Migrating to AUv3 API:** https://developer.apple.com/documentation/audiotoolbox/audio_unit_v3_plug-ins/migrating_your_audio_unit_host_to_the_auv3_api
- **Hosting Audio Unit Extensions (AUv2 API):** https://developer.apple.com/documentation/audiotoolbox/audio_unit_v3_plug-ins/hosting_audio_unit_extensions_using_the_auv2_api
- **Audio Unit Programming Guide (Archived):** https://developer.apple.com/library/archive/documentation/MusicAudio/Conceptual/AudioUnitProgrammingGuide/

**Deprecation Notice:**
> ⚠️ "V2 audio units are set to be deprecated in a future operating system release, and all new audio unit development should employ the audio unit V3 API."
> — [Apple Developer Documentation](https://developer.apple.com/library/archive/documentation/MusicAudio/Conceptual/AudioUnitProgrammingGuide/)

**Recommendation:** Support both AUv2 (for legacy plugins) and AUv3 (for modern plugins) during the transition period.

### JUCE AudioPluginHost Example

**Official JUCE Resources:**
- **AudioPluginHost Source:** https://github.com/juce-framework/JUCE/tree/master/extras/AudioPluginHost/Source
- **AudioPluginHost Startup:** https://github.com/juce-framework/JUCE/blob/master/extras/AudioPluginHost/Source/HostStartup.cpp
- **JUCE Plugin Examples:** https://docs.juce.com/master/tutorial_plugin_examples.html

**Build Instructions:**
> "JUCE provides a built-in plug-in host accessible at `extras/AudioPluginHost/`. You can build it with CMake by running commands to build the AudioPluginHost target."
> — [JUCE Documentation](https://docs.juce.com/master/index.html)

**How to Use:**
> "The host allows updating the list of plug-ins by pressing 'Cmd-P' and scanning for plugins."
> — [JUCE Forum: AudioPlugin Host](https://forum.juce.com/t/audioplugin-host-plugin-detection/33952)

**Implementation Note:**
> "One tricky aspect of building a JUCE-based host is creating a 'known plug-ins list' (class `juce::KnownPluginList`) and registering plugins into it."
> — [GitHub: juce-plugin-wrapper](https://github.com/getdunne/juce-plugin-wrapper)

---

## Phase 1: In-Process Hosting (Quick Bring-Up)

### Architecture Overview

In Phase 1, plugins run in the **same process** as the DAW. This is the simplest implementation:

```
┌─────────────────────────────────────────┐
│  Zenith DAW Process                     │
│  ┌───────────────────────────────────┐  │
│  │ Audio Thread (Real-Time)          │  │
│  │  ┌─────────────┐  ┌─────────────┐ │  │
│  │  │ VST3 Plugin │  │ AU Plugin    │ │  │
│  │  │ processBlock│  │ render       │ │  │
│  │  └─────────────┘  └─────────────┘ │  │
│  └───────────────────────────────────┘  │
│  ┌───────────────────────────────────┐  │
│  │ Message Thread (GUI)              │  │
│  │  ┌─────────────┐  ┌─────────────┐ │  │
│  │  │ Plugin GUI  │  │ Plugin GUI  │ │  │
│  │  └─────────────┘  └─────────────┘ │  │
│  └───────────────────────────────────┘  │
└─────────────────────────────────────────┘
```

**Pros:**
- ✅ Simple implementation (all in one process)
- ✅ Low latency (no IPC overhead)
- ✅ Easy debugging (single process)
- ✅ Shared memory (no serialization)

**Cons:**
- ❌ Plugin crash = DAW crash
- ❌ No isolation (plugin can access DAW memory)
- ❌ Security vulnerabilities in plugin = DAW compromised

---

## Phase 1 Implementation Checklist

### 1. Setup Plugin Scanning

- [ ] **Implement `juce::KnownPluginList`**
  - Create persistent storage (XML file in app data folder)
  - Load known plugins on startup

- [ ] **Implement `juce::PluginDirectoryScanner`**
  - Scan standard VST3 locations:
    - Windows: `C:\Program Files\Common Files\VST3\`
    - macOS: `/Library/Audio/Plug-Ins/VST3/`, `~/Library/Audio/Plug-Ins/VST3/`
    - Linux: `~/.vst3/`, `/usr/lib/vst3/`
  - Scan Audio Unit locations (macOS):
    - `/Library/Audio/Plug-Ins/Components/`
    - `~/Library/Audio/Plug-Ins/Components/`

- [ ] **Handle Scan Errors**
  - Log failed plugins (incompatible, corrupted, crashed during scan)
  - Display scan progress to user
  - Allow cancellation of long scans

- [ ] **Implement Incremental Scan**
  - Only re-scan changed/new files (check modification date)
  - Background thread for scanning (not blocking UI)

### 2. Plugin Instantiation

- [ ] **Create `juce::AudioPluginFormatManager`**
  ```cpp
  AudioPluginFormatManager formatManager;
  formatManager.addDefaultFormats(); // Adds VST3, AU, etc.
  ```

- [ ] **Load Plugin from Description**
  ```cpp
  PluginDescription description; // From KnownPluginList
  String errorMessage;

  auto plugin = formatManager.createPluginInstance(
      description,
      sampleRate,
      bufferSize,
      errorMessage
  );
  ```

- [ ] **Handle Loading Errors**
  - Plugin file not found
  - Plugin initialization failed
  - Version incompatibility
  - Missing dependencies (e.g., iLok, licenses)

### 3. Audio Graph Integration

- [ ] **Add Plugin to `juce::AudioProcessorGraph`**
  ```cpp
  auto node = audioGraph->addNode(std::move(plugin));
  ```

- [ ] **Connect Audio/MIDI Routing**
  - Connect track input → plugin input
  - Connect plugin output → track output (or next plugin)
  - Handle multi-channel configurations

- [ ] **Prepare for Playback**
  ```cpp
  plugin->prepareToPlay(sampleRate, bufferSize);
  ```

- [ ] **Real-Time Processing**
  ```cpp
  // On audio thread (REAL-TIME SAFE!)
  plugin->processBlock(audioBuffer, midiMessages);
  ```

### 4. Parameter Management

- [ ] **Query Plugin Parameters**
  ```cpp
  int numParams = plugin->getNumParameters();
  for (int i = 0; i < numParams; ++i) {
      String paramName = plugin->getParameterName(i);
      float defaultValue = plugin->getParameter(i);
  }
  ```

- [ ] **Map Parameters to ValueTree**
  - Store parameter values in project state
  - Enable undo/redo for parameter changes

- [ ] **Automation Recording/Playback**
  - Record parameter changes during playback
  - Play back automation curves (timeline → plugin)

### 5. Editor GUI Handling

- [ ] **Check if Plugin Has Editor**
  ```cpp
  if (plugin->hasEditor()) {
      AudioProcessorEditor* editor = plugin->createEditor();
  }
  ```

- [ ] **Embed Editor in Window**
  - Create native window (HWND on Windows, NSView on macOS)
  - Resize editor based on plugin's preferred size
  - Handle focus and keyboard events

- [ ] **Generic Editor Fallback**
  ```cpp
  if (!plugin->hasEditor()) {
      AudioProcessorEditor* editor = plugin->createEditorIfNeeded();
      // Shows sliders for all parameters
  }
  ```

### 6. State Saving/Loading

- [ ] **Save Plugin State**
  ```cpp
  MemoryBlock stateData;
  plugin->getStateInformation(stateData);
  // Store stateData in project file (ValueTree)
  ```

- [ ] **Load Plugin State**
  ```cpp
  plugin->setStateInformation(stateData.getData(), stateData.getSize());
  ```

- [ ] **Handle Version Compatibility**
  - Plugins may change state format between versions
  - Log warnings if state load fails

### 7. Thread Safety

- [ ] **Audio Thread Operations (Real-Time Safe)**
  - `processBlock()` — process audio
  - `getParameter()` — read parameter values

- [ ] **Message Thread Operations (Not Real-Time Safe)**
  - `setParameter()` — change parameters (from GUI)
  - `createEditor()` — open plugin GUI
  - `getStateInformation()` / `setStateInformation()` — save/load

- [ ] **Use Lock-Free Communication**
  - GUI → Audio: Use `juce::AbstractFifo` or `std::atomic` for parameter changes
  - Audio → GUI: Send meter data via lock-free FIFO

---

## Phase 2: Out-of-Process Sandboxing (Crash Protection)

### Architecture Overview

In Phase 2, plugins run in **separate processes** for isolation and crash protection.

```
┌─────────────────────────────────────────┐
│  Zenith DAW (Main Process)              │
│  ┌───────────────────────────────────┐  │
│  │ Audio Thread                      │  │
│  │  - Reads from shared memory       │  │
│  │  - Writes to shared memory        │  │
│  └───────────────────────────────────┘  │
│           ↕ IPC (Shared Memory)         │
└─────────────────────────────────────────┘
                   ↓
┌─────────────────────────────────────────┐
│  Plugin Subprocess #1 (VST3)            │
│  ┌───────────────────────────────────┐  │
│  │ VST3 Plugin                       │  │
│  │  - processBlock()                 │  │
│  └───────────────────────────────────┘  │
└─────────────────────────────────────────┘
                   ↓
┌─────────────────────────────────────────┐
│  Plugin Subprocess #2 (AU)              │
│  ┌───────────────────────────────────┐  │
│  │ AU Plugin                         │  │
│  │  - render()                       │  │
│  └───────────────────────────────────┘  │
└─────────────────────────────────────────┘
```

**Pros:**
- ✅ Plugin crash = subprocess crash (DAW survives)
- ✅ Security isolation (sandboxed processes)
- ✅ Per-plugin resource limits (CPU, memory)
- ✅ Graceful recovery (restart crashed plugin)

**Cons:**
- ❌ Higher latency (IPC + shared memory overhead)
- ❌ Complex implementation (process management, IPC)
- ❌ More memory usage (multiple processes)
- ❌ Requires OS-specific code (fork/exec, shared memory)

---

## Phase 2 Implementation Plan

### Sandboxing Strategies

Reference implementations from commercial DAWs:

> **Bitwig Studio's Sandboxing Levels:**
> 1. **By Manufacturer:** Individual sandboxes per manufacturer (plugins can communicate)
> 2. **By Plug-in:** Individual sandboxes per plugin type (instances shared)
> 3. **Individually:** One sandbox per plugin instance (maximum safety, most memory)
> — [Bitwig Studio: Plug-in Hosting & Crash Protection](https://www.bitwig.com/learnings/plug-in-hosting-crash-protection-in-bitwig-studio-20/)

**Recommendation for Zenith:** Start with **"By Plug-in"** (level 2) for good safety vs. memory balance.

### Implementation Steps

#### 1. Process Management

- [ ] **Create Plugin Host Subprocess**
  - Windows: Use `CreateProcess()` with job objects
  - macOS/Linux: Use `fork()` + `exec()`

- [ ] **Monitor Subprocess Health**
  - Heartbeat mechanism (ping every 100ms)
  - Detect crashes (process exit code)
  - Restart crashed plugins automatically

- [ ] **Resource Limits**
  - Set CPU usage limits (nice/priority)
  - Set memory limits (job objects on Windows, `setrlimit` on Unix)

#### 2. Inter-Process Communication (IPC)

- [ ] **Shared Memory for Audio**
  - Windows: `CreateFileMapping()` + `MapViewOfFile()`
  - macOS/Linux: `shm_open()` + `mmap()`
  - Pre-allocate ring buffers for audio (lock-free SPSC)

- [ ] **Command Channel**
  - Use named pipes (Windows) or Unix domain sockets (macOS/Linux)
  - Send commands: load plugin, set parameter, save state, etc.
  - Async responses: success/failure, error messages

- [ ] **MIDI Events**
  - Serialize MIDI events in shared memory
  - Use fixed-size buffer (e.g., 1024 events max per block)

#### 3. Audio Synchronization

- [ ] **Lock-Free Ring Buffers**
  - Use `juce::AbstractFifo` or `boost::lockfree::spsc_queue`
  - DAW writes input audio → Plugin reads
  - Plugin writes output audio → DAW reads

- [ ] **Sample-Accurate Timing**
  - Send sample position with each audio block
  - Handle plugin latency (report via `getLatencySamples()`)

- [ ] **Handle Underruns**
  - If plugin is too slow, fill with silence (don't block audio thread)
  - Show warning to user ("Plugin X is overloading")

#### 4. Crash Recovery

- [ ] **Detect Crashes**
  - Monitor subprocess exit code
  - Timeouts (if subprocess doesn't respond in X ms)

- [ ] **Notify User**
  - Show alert: "Plugin X crashed and was removed from the track"
  - Offer to reload plugin or replace with bypass

- [ ] **Auto-Restart**
  - Restart crashed plugin automatically (optional setting)
  - Restore last known state (parameters, preset)

---

## VST3 Parameter Automation (IComponent & IEditController)

### Communication Flow

> "When the host sets a new processor state (`IComponent::setState`), this state is always transmitted to the controller as well (`IEditController::setComponentState`), and the controller then has to synchronize to this state and adjust its parameters."
> — [VST 3 Developer Portal: Parameters and Automation](https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/Parameters+Automation/Index.html)

> "When the controller transmits a parameter change to the host, the host synchronizes the processor by passing the new values as `IParameterChanges` to the process call."
> — [VST 3 Developer Portal: Parameters and Automation](https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/Parameters+Automation/Index.html)

### Automation Recording

> "The plug-in must operate the `IComponentHandler` interface in the UI Thread, signaling the beginning of a manipulation via `IComponentHandler::beginEdit`. Between `beginEdit` and `endEdit`, `performEdit` is called to inform the handler that a given parameter has a new value."
> — [VST 3 Interfaces: IComponentHandler](https://steinbergmedia.github.io/vst3_doc/vstinterfaces/classSteinberg_1_1Vst_1_1IComponentHandler.html)

### Parameter Values

> "Parameter values are always transmitted in a normalized floating point (64bit double) representation [0.0, 1.0]. The host uses `IEditController::normalizedParamToPlain` and `IEditController::plainParamToNormalized` for conversions."
> — [VST 3 Developer Portal: Parameters](https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/Parameters+Automation/Index.html)

### Output Parameter Changes

> "The processor can transmit outgoing parameter changes to the host as well via `ProcessData::outputParameterChanges`, and these are transmitted to the edit controller by the call of `IEditController::setParamNormalized`."
> — [VST 3 Developer Portal: Parameters](https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/Parameters+Automation/Index.html)

---

## macOS: Codesigning & Sandboxing Considerations

### Audio Unit Sandboxing

> ⚠️ "Audio units are hosted by `AUHostingService`, which uses the general-purpose sandboxing facility. This creates unique challenges compared to regular app sandboxing."
> — [Apple TN2312: Audio Unit Host Sandboxing Guide](https://developer.apple.com/library/archive/technotes/tn2312/_index.html)

### Required Entitlements

**For AU Hosts:**
> "The `com.apple.security.temporary-exception.audio-unit-host` entitlement allows loading Audio Units that are not sandbox-safe. However, when a non-sandbox-safe Audio Unit is loaded, the system will show the user a dialog indicating that the process is attempting to open an Audio Component that isn't Sandbox Safe."
> — [Apple Developer Forums: Sandboxing Issues](https://developer.apple.com/forums/thread/770046)

**For Audio Input:**
> "If your app is both sandboxed and hardened, you will want to enable both `com.apple.security.device.microphone` (sandbox entitlement) and `com.apple.security.device.audio-input` (hardened runtime entitlement)."
> — [Stack Overflow: macOS Entitlements](https://stackoverflow.com/questions/49595811/macos-entitlements-audio-input-vs-microphone)

### Making Audio Units Sandbox-Safe

> "To make an audio unit sandbox-safe, you must set the `AudioComponentFlags.sandboxSafe.rawValue` flag in the `AudioComponentDescription`. A Sandbox Safe Audio Component will have the `kAudioComponentFlag_SandboxSafe` flag set."
> — [AudioDog: Sandbox-Safe Audio Units](https://www.audiodog.co.uk/blog/2022/08/12/make-your-audio-unit-native-app-sandbox-safe/)

### macOS Sequoia (2024) Changes

> ⚠️ "Starting with macOS Sequoia 15 (expected in the fall of 2024), Apple has tightened the runtime security protections even more."
> — [Xojo Blog: macOS Sandboxing to Notarization](https://blog.xojo.com/2024/08/22/macos-apps-from-sandboxing-to-notarization-the-basics/)

**Action Items:**
- Test on macOS Sequoia beta
- Request `audio-unit-host` entitlement from Apple
- Ensure all bundled components are properly signed and notarized

---

## Code Stubs

### PluginHost.h

```cpp
/*
  ==============================================================================

    PluginHost.h
    Created: 2025-11-10

    Manages plugin scanning, loading, and instantiation.

    THREAD SAFETY:
    - Scanning runs on background thread (PluginDirectoryScanner)
    - Loading/instantiation runs on message thread
    - Audio processing runs on audio thread (via AudioProcessorGraph)

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
    Manages all plugin hosting functionality for Zenith DAW.

    Responsibilities:
    - Scan for installed VST3 and AU plugins
    - Maintain list of known plugins (KnownPluginList)
    - Create plugin instances on demand
    - Provide plugin browser UI
*/
class PluginHost
{
public:
    //==========================================================================
    PluginHost();
    ~PluginHost();

    //==========================================================================
    // Plugin Scanning (call from message thread, runs async)

    /** Scans for plugins in standard directories. */
    void scanForPlugins(std::function<void(float progress)> progressCallback);

    /** Returns true if a scan is currently in progress. */
    bool isScanning() const;

    /** Cancels an ongoing scan. */
    void cancelScan();

    //==========================================================================
    // Known Plugins

    /** Returns the list of known plugins. */
    juce::KnownPluginList& getKnownPluginList() { return knownPluginList; }

    /** Saves the known plugins list to disk. */
    void saveKnownPluginList();

    /** Loads the known plugins list from disk. */
    void loadKnownPluginList();

    //==========================================================================
    // Plugin Instantiation (call from message thread)

    /**
        Creates a plugin instance from a description.
        Returns nullptr if loading fails.

        @param description   Plugin to load (from KnownPluginList)
        @param sampleRate    Current sample rate
        @param bufferSize    Current buffer size
        @param errorMessage  Output: error message if loading fails
    */
    std::unique_ptr<juce::AudioPluginInstance> createPluginInstance(
        const juce::PluginDescription& description,
        double sampleRate,
        int bufferSize,
        juce::String& errorMessage
    );

    //==========================================================================
    // Format Manager Access

    juce::AudioPluginFormatManager& getFormatManager() { return formatManager; }

private:
    //==========================================================================
    // Plugin formats and known plugins
    juce::AudioPluginFormatManager formatManager;
    juce::KnownPluginList knownPluginList;

    // Scanner (runs on background thread)
    std::unique_ptr<juce::PluginDirectoryScanner> currentScanner;
    juce::CriticalSection scannerLock;

    // Persistent storage
    juce::File knownPluginsFile;

    //==========================================================================
    // TODO: Phase 2 - Out-of-process hosting
    // std::map<String, std::unique_ptr<PluginSubprocess>> subprocesses;
    // void launchPluginSubprocess(const PluginDescription& description);
    // void monitorSubprocessHealth();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginHost)
};
```

### PluginHost.cpp

```cpp
/*
  ==============================================================================

    PluginHost.cpp
    Created: 2025-11-10

    Implementation of plugin hosting.

  ==============================================================================
*/

#include "PluginHost.h"

//==============================================================================
PluginHost::PluginHost()
{
    // Add supported plugin formats
    formatManager.addDefaultFormats();

    // VST3, AU, etc. are now registered

    // Determine location for known plugins list
    auto appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
    knownPluginsFile = appDataDir.getChildFile("ZenithDAW").getChildFile("KnownPlugins.xml");

    // Load previously scanned plugins
    loadKnownPluginList();
}

PluginHost::~PluginHost()
{
    cancelScan();
}

//==============================================================================
// Plugin Scanning

void PluginHost::scanForPlugins(std::function<void(float)> progressCallback)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    // Cancel any existing scan
    cancelScan();

    // Get all search paths for all formats
    juce::FileSearchPath searchPaths;

    for (int i = 0; i < formatManager.getNumFormats(); ++i)
    {
        auto* format = formatManager.getFormat(i);
        searchPaths.addPath(format->getDefaultLocationsToSearch());
    }

    // TODO: Start async scan
    // currentScanner = std::make_unique<PluginDirectoryScanner>(
    //     knownPluginList,
    //     formatManager,
    //     searchPaths,
    //     true,  // recursive
    //     knownPluginsFile
    // );

    // Scan on background thread, call progressCallback periodically
    // When complete, save known plugins list

    juce::ignoreUnused(progressCallback);
}

bool PluginHost::isScanning() const
{
    juce::ScopedLock lock(scannerLock);
    return currentScanner != nullptr;
}

void PluginHost::cancelScan()
{
    juce::ScopedLock lock(scannerLock);
    currentScanner = nullptr;
}

//==============================================================================
// Known Plugins Persistence

void PluginHost::saveKnownPluginList()
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    if (auto xml = knownPluginList.createXml())
    {
        knownPluginsFile.getParentDirectory().createDirectory();
        xml->writeTo(knownPluginsFile);
    }
}

void PluginHost::loadKnownPluginList()
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    if (knownPluginsFile.existsAsFile())
    {
        if (auto xml = juce::parseXML(knownPluginsFile))
        {
            knownPluginList.recreateFromXml(*xml);
        }
    }
}

//==============================================================================
// Plugin Instantiation

std::unique_ptr<juce::AudioPluginInstance> PluginHost::createPluginInstance(
    const juce::PluginDescription& description,
    double sampleRate,
    int bufferSize,
    juce::String& errorMessage)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    // TODO: Phase 2 - Check if this plugin should be sandboxed
    // bool shouldSandbox = shouldRunInSandbox(description);
    // if (shouldSandbox)
    //     return createSandboxedPluginInstance(description, sampleRate, bufferSize, errorMessage);

    // Phase 1: In-process loading
    auto instance = formatManager.createPluginInstance(
        description,
        sampleRate,
        bufferSize,
        errorMessage
    );

    if (instance == nullptr)
    {
        DBG("Failed to load plugin: " + description.name + " - " + errorMessage);
        return nullptr;
    }

    return instance;
}

//==============================================================================
// TODO: Phase 2 Implementation Stubs

/*
void PluginHost::launchPluginSubprocess(const PluginDescription& description)
{
    // 1. Create subprocess (platform-specific)
    // 2. Setup shared memory ring buffers
    // 3. Send "load plugin" command via IPC
    // 4. Monitor health with heartbeat
}

void PluginHost::monitorSubprocessHealth()
{
    // Background thread that checks:
    // - Subprocess still alive? (check process ID)
    // - Heartbeat received? (last ping < 100ms ago)
    // - If crashed: notify user, remove from graph, offer to restart
}
*/
```

---

## QA Testing Checklist

### In-Process Hosting Tests

- [ ] **Plugin Scanning**
  - Scan completes without crashing
  - All installed plugins detected
  - Invalid/corrupted plugins don't crash scanner
  - Scan can be cancelled mid-way

- [ ] **Plugin Loading**
  - VST3 plugins load correctly
  - AU plugins load correctly (macOS)
  - Handle missing plugin files gracefully
  - Handle license check failures (e.g., iLok)

- [ ] **Audio Processing**
  - Audio passes through plugin correctly
  - MIDI events reach plugin
  - Parameter changes affect sound
  - No audio glitches at 64-sample buffer size

- [ ] **Editor GUI**
  - Plugin GUI opens and displays correctly
  - GUI resizes properly
  - Multiple plugin GUIs can be open simultaneously
  - Closing GUI doesn't crash DAW

- [ ] **State Saving/Loading**
  - Save project with plugins loaded
  - Load project and verify plugins restore correctly
  - Verify preset recall works

### Out-of-Process Sandboxing Tests

- [ ] **Subprocess Management**
  - Subprocess launches correctly
  - Audio is routed through shared memory
  - Latency is acceptable (< 10ms added)

- [ ] **Crash Recovery**
  - Crash a plugin (force kill subprocess)
  - Verify DAW continues running
  - Verify user is notified
  - Verify plugin can be reloaded

- [ ] **Resource Limits**
  - CPU-heavy plugin is throttled
  - Memory-heavy plugin doesn't OOM the DAW
  - Multiple subprocesses don't exhaust system resources

---

## Sources

### VST3 (Steinberg)
1. VST 3 Developer Portal: https://steinbergmedia.github.io/vst3_dev_portal/
2. VST 3 SDK Documentation: https://steinbergmedia.github.io/vst3_doc/vstsdk/index.html
3. VST 3 SDK GitHub: https://github.com/steinbergmedia/vst3sdk
4. VST 3 Interfaces Reference: https://steinbergmedia.github.io/vst3_doc/vstinterfaces/
5. Parameters and Automation: https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/Parameters+Automation/Index.html

### Audio Unit (Apple)
6. Incorporating Audio Effects: https://developer.apple.com/documentation/AudioToolbox/incorporating-audio-effects-and-instruments
7. Audio Unit Framework: https://developer.apple.com/documentation/audiounit
8. Migrating to AUv3: https://developer.apple.com/documentation/audiotoolbox/audio_unit_v3_plug-ins/migrating_your_audio_unit_host_to_the_auv3_api
9. TN2312 - AU Host Sandboxing: https://developer.apple.com/library/archive/technotes/tn2312/_index.html

### JUCE
10. AudioPluginHost Source: https://github.com/juce-framework/JUCE/tree/master/extras/AudioPluginHost/Source
11. JUCE Plugin Wrapper Template: https://github.com/getdunne/juce-plugin-wrapper
12. JUCE Forum: Plugin Hosting Discussions: https://forum.juce.com/

### Sandboxing References
13. Bitwig Studio Crash Protection: https://www.bitwig.com/learnings/plug-in-hosting-crash-protection-in-bitwig-studio-20/
14. macOS Sandboxing (2024): https://blog.xojo.com/2024/08/22/macos-apps-from-sandboxing-to-notarization-the-basics/
15. Audio Unit Sandbox-Safe: https://www.audiodog.co.uk/blog/2022/08/12/make-your-audio-unit-native-app-sandbox-safe/

---

**Document Version:** 1.0
**Last Updated:** 2025-11-10
**Next Steps:** Implement Phase 1 checklist, then evaluate Phase 2 necessity
