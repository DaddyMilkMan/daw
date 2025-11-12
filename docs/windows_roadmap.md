# Windows Performance & Production Roadmap

This document outlines the complete plan for hardening Zenith DAW on Windows, from audio engine optimization to production-ready packaging. Each work item (W0-W12+) builds on previous steps to create a professional, performant DAW.

---

## Table of Contents

1. [Progress Overview](#progress-overview)
2. [Audio Engine](#audio-engine)
3. [UI Performance](#ui-performance)
4. [Diagnostics & Profiling](#diagnostics--profiling)
5. [Driver Guidance & Audio Interfaces](#driver-guidance--audio-interfaces)
6. [Packaging & Distribution](#packaging--distribution)
7. [Quality Assurance](#quality-assurance)
8. [Future Enhancements](#future-enhancements)

---

## Progress Overview

### ✅ Completed Work Items

| Step | Description | Status | Details |
|------|-------------|--------|---------|
| **W0** | Visual Studio 2022 Toolchain | ✅ Complete | MSVC 19.x, C++20, CMake 3.22+ |
| **W1** | DPI Awareness | ✅ Complete | Per-monitor-v2 DPI, app manifest |
| **W2** | MMCSS Audio Priority | ✅ Complete | Windows Multimedia Class Scheduler, Pro Audio priority |
| **W3** | WASAPI UI Stub | ✅ Complete | Device enumeration, buffer size config, sample rate |
| **W4** | Font/Text Rendering Audit | ✅ Complete | Zero allocations in paint paths, GlyphCache analysis |
| **W5** | TrackView Virtualization | ✅ Complete | Viewport culling, 100 tracks × 50 clips (5000 total), <5ms paint |
| **W6** | Frame-Time Telemetry HUD | ✅ Complete | StatsOverlay with paint timing, track/clip counts |
| **W6.1** | HUD Polish | ✅ Complete | View menu toggle, ApplicationProperties persistence |
| **W7** | ETW Tracing (docs/scripts) | ✅ Complete | WPR/WPA profiles, 4 presets, capture scripts |
| **W8** | Crashpad Crash Reporting | ✅ Complete | Optional crash capture, local-only, user opt-in |
| **W9** | Windows Roadmap Doc | ✅ Complete | This document |

### 🚧 In-Progress Work Items

| Step | Description | Priority | ETA |
|------|-------------|----------|-----|
| **W10** | Audio Engine Core | 🔥 Critical | Next |
| **W11** | Plugin Graph | High | TBD |
| **W12** | ASIO Integration | High | TBD |

### 📋 Planned Work Items

See sections below for detailed roadmaps in each area.

---

## Audio Engine

The audio engine is the heart of any DAW. Windows-specific optimizations focus on low-latency, lock-free design, and efficient resource usage.

### Current State (W0-W8)
- ✅ JUCE `AudioDeviceManager` integrated
- ✅ WASAPI device enumeration working
- ✅ Buffer size/sample rate configuration UI (W3)
- ✅ MMCSS Pro Audio thread priority (W2)
- ⚠️ **Missing**: Actual audio callback implementation (stubbed)
- ⚠️ **Missing**: Track mixing, plugin hosting, routing

### W10: Audio Engine Core (Priority: Critical)

**Goal**: Implement lock-free, real-time audio callback with track mixing.

#### Key Requirements:
- **Callback budget**: <50% of buffer period (e.g., <2.5ms @ 512 samples, 48kHz)
- **Lock-free design**: No mutexes, heap allocations, or syscalls in audio thread
- **Zero allocations**: Pre-allocate all buffers during initialization
- **Thread affinity**: Pin audio thread to dedicated CPU core (MMCSS handles this)
- **SIMD optimization**: Use SSE2/AVX for mixing (JUCE provides helpers)

#### Implementation Plan:

**Phase 1: Basic Track Mixing**
1. Add `AudioTrack::processBlock(AudioBuffer&, MidiBuffer&)`
   - Read from `AudioFormatReaderSource` (for audio clips)
   - Apply gain, pan, solo, mute
   - Mix into master bus
2. Implement track routing (track → bus → master)
3. Add simple gain/pan automation (read from envelope)
4. **Perf target**: 32 tracks, 64 samples buffer, <30% CPU

**Phase 2: Lock-Free State Updates**
1. Use `juce::AbstractFifo` for UI → audio thread messages
   - Transport state changes (play/stop/seek)
   - Clip add/remove (defer to safe point)
   - Parameter changes (gain, pan, etc.)
2. Implement double-buffered session state
   - UI thread writes to staging buffer
   - Audio thread swaps at safe point (block boundary)
3. **Perf target**: No audio dropouts during UI interaction

**Phase 3: Clip Scheduling**
1. Add time-sorted clip queue per track
2. Implement sample-accurate clip start/stop
3. Handle clip loops, fades, crossfades
4. **Perf target**: 1000+ clips in session, no overhead for offscreen clips

**Phase 4: Metering & Latency Compensation**
1. Add peak/RMS metering per track (lock-free ring buffer)
2. Implement plugin delay compensation (PDC)
3. Add input monitoring with latency-aware recording
4. **Perf target**: <0.1ms metering overhead

#### Validation:
- **Smoke test**: Play 32 tracks simultaneously, no dropouts
- **Stress test**: 128 tracks, 5000 clips, <50% CPU @ 512 samples
- **Latency test**: Roundtrip latency <10ms @ 128 samples (ASIO)

#### References:
- JUCE Audio Tutorial: [Building an Audio Player](https://docs.juce.com/master/tutorial_playing_sound_files.html)
- Lock-Free Programming: [Timur Doumler - C++ in Audio](https://www.youtube.com/watch?v=boPEO2auJj4)
- Real-Time Safety: [P1202R0 - Asymmetric Fences](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1202r0.html)

---

### W11: Plugin Graph (Priority: High)

**Goal**: VST3 plugin hosting with efficient parameter automation.

#### Key Requirements:
- **VST3 support**: Use JUCE `AudioPluginHost` infrastructure
- **Sandboxing**: Load plugins out-of-process (separate process per plugin)
- **Parameter changes**: Lock-free automation updates
- **Preset management**: Fast preset loading (off audio thread)
- **Crash isolation**: Plugin crashes don't kill DAW

#### Implementation Plan:

**Phase 1: Basic VST3 Hosting**
1. Enable JUCE VST3 hosting:
   ```cmake
   target_compile_definitions(ZenithDAW PRIVATE
       JUCE_PLUGINHOST_VST3=1
       JUCE_PLUGINHOST_VST3_PLUGIN_EDITOR_SHOWS_BY_DEFAULT=0
   )
   ```
2. Implement plugin scanning (background thread, cache results)
3. Add plugin insert slots per track (6 slots: 5 inserts + 1 instrument)
4. Create plugin editor window wrapper
5. **Perf target**: 16 VST3 plugins, <40% CPU overhead

**Phase 2: Out-of-Process Hosting**
1. Use JUCE `ChildProcessMaster`/`ChildProcessSlave` for sandboxing
2. Implement plugin → host communication protocol
   - Audio: Shared memory ring buffers
   - Parameters: Lock-free message queue
   - MIDI: Timestamped event queue
3. Handle plugin crashes gracefully (restart, bypass, or remove)
4. **Perf target**: <1ms latency overhead for out-of-process plugins

**Phase 3: Parameter Automation**
1. Add envelope curves per parameter (Bézier splines)
2. Implement sample-accurate automation playback
3. Add automation recording from plugin UI
4. Support automation editing in TrackView
5. **Perf target**: 100+ automated parameters, no audio dropouts

**Phase 4: Plugin Delay Compensation**
1. Measure plugin latency (VST3 `getLatencySamples()`)
2. Add per-track delay buffers for alignment
3. Implement automatic PDC routing
4. **Perf target**: <5ms worst-case PDC latency

#### Validation:
- **Compatibility test**: Load top 50 VST3 plugins (survey results)
- **Stability test**: 24-hour stress test with randomized plugin load/unload
- **Latency test**: Verify PDC accuracy with null test (phase-inverted signal)

#### References:
- JUCE Plugin Hosting: [AudioPluginHost example](https://github.com/juce-framework/JUCE/tree/master/examples/Plugins/AudioPluginHost)
- VST3 Spec: [Steinberg VST3 Documentation](https://steinbergmedia.github.io/vst3_doc/)

---

### W12: Audio Buffer Strategy (Priority: Medium)

**Goal**: Optimize memory usage and cache coherency for audio buffers.

#### Optimizations:
1. **Pre-allocate buffer pools**: Avoid allocations in audio callback
2. **SIMD alignment**: Align buffers to 16/32-byte boundaries (AVX requires 32)
3. **Cache-friendly layout**: Interleaved stereo → planar (better SIMD)
4. **Reuse scratch buffers**: Track-local scratch space for processing

#### Implementation:
```cpp
class AudioBufferPool
{
public:
    AudioBuffer<float>* acquire(int numChannels, int numSamples);
    void release(AudioBuffer<float>* buffer);

private:
    std::vector<std::unique_ptr<AudioBuffer<float>>> pool;
    std::atomic<int> nextFree{0};
};
```

#### Validation:
- **Benchmark**: Measure alloc/free overhead in audio callback (should be 0)
- **Memory audit**: Profile heap allocations during playback (Valgrind, HeapProfiler)

---

## UI Performance

UI responsiveness is critical for DAW usability. Windows-specific optimizations focus on rendering efficiency and GPU utilization.

### Current State (W0-W8)
- ✅ TrackView virtualization (W5): Viewport culling, <5ms paint
- ✅ Font rendering audit (W4): Zero allocations in paint()
- ✅ Frame-time telemetry (W6): StatsOverlay for real-time monitoring
- ✅ ETW tracing (W7): GPU/DWM analysis with WPA
- ⚠️ **Missing**: Waveform rendering, plugin UI embedding, mixer view

### W13: Waveform Rendering (Priority: High)

**Goal**: Efficient audio waveform display with GPU acceleration.

#### Key Requirements:
- **Render budget**: <2ms per paint() call (60 FPS target)
- **LOD system**: Multi-resolution waveform data (min/max pairs)
- **GPU offload**: Use OpenGL shaders for waveform rendering
- **Caching**: Cache rendered waveforms as textures

#### Implementation Plan:

**Phase 1: Waveform Data Generation**
1. Pre-compute min/max pairs at multiple resolutions:
   - Level 0: 1 sample per pixel (zoom: 1:1)
   - Level 1: 10 samples per pixel
   - Level 2: 100 samples per pixel
   - Level 3: 1000 samples per pixel
   - ... (up to full file length)
2. Store in binary format (fast loading):
   ```cpp
   struct WaveformLOD
   {
       int samplesPerPixel;
       std::vector<float> minValues;
       std::vector<float> maxValues;
   };
   ```
3. Generate LODs on import (background thread, show progress)

**Phase 2: OpenGL Rendering**
1. Use JUCE `OpenGLContext` for GPU rendering:
   ```cpp
   class WaveformRenderer : public juce::Component,
                            public juce::OpenGLRenderer
   {
       void newOpenGLContextCreated() override;
       void renderOpenGL() override;
       // ...
   };
   ```
2. Implement vertex shader for waveform lines
3. Use instanced rendering for multiple clips (batch draw calls)
4. **Perf target**: 5000 visible clips, <2ms render time

**Phase 3: Texture Caching**
1. Render waveforms to off-screen textures (1x per zoom level)
2. Cache textures in GPU memory (LRU eviction)
3. Invalidate cache on zoom, scroll, or clip edit
4. **Perf target**: Cached waveforms render in <0.5ms

**Phase 4: Progressive Loading**
1. Show low-res waveform immediately (Level 3 LOD)
2. Load higher-res LODs in background (Level 2 → 1 → 0)
3. Fade in higher-res data when ready (no flicker)
4. **UX target**: Waveforms appear instantly, sharpen within 100ms

#### Validation:
- **Render benchmark**: 5000 clips on screen, 60 FPS sustained
- **Zoom test**: Smooth zooming from full session to sample level
- **Memory audit**: GPU memory usage <500 MB for 100 audio files

#### References:
- JUCE OpenGL: [OpenGLAppComponent Tutorial](https://docs.juce.com/master/tutorial_open_gl_application.html)
- GPU Waveforms: [Surge Synthesizer - Oscilloscope Rendering](https://github.com/surge-synthesizer/surge/blob/main/src/gui/widgets/Oscilloscope.cpp)

---

### W14: Plugin UI Embedding (Priority: High)

**Goal**: Embed VST3 plugin UIs with proper DPI scaling and window management.

#### Key Requirements:
- **Native embedding**: Use `HWND` parenting for VST3 editors
- **DPI awareness**: Scale plugin UI to match system DPI
- **Resizing**: Support resizable plugin windows
- **Multiple windows**: Allow multiple plugin editors open simultaneously

#### Implementation Plan:

**Phase 1: Basic Embedding**
1. Use JUCE `AudioProcessorEditor` wrapper:
   ```cpp
   class PluginWindow : public juce::DocumentWindow
   {
   public:
       PluginWindow(juce::AudioProcessor& proc);
       void setPlugin(juce::AudioProcessor* newProc);
   };
   ```
2. Get plugin editor HWND and parent to JUCE window
3. Handle window close, minimize, maximize
4. **Validation**: Load 10 different VST3 plugins, verify UI works

**Phase 2: DPI Scaling**
1. Query plugin DPI awareness (VST3 attribute)
2. Apply scaling if plugin is not DPI-aware:
   ```cpp
   SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
   ```
3. Handle DPI changes dynamically (user moves window between monitors)
4. **Validation**: Verify plugin UI on 100%, 125%, 150%, 200% DPI

**Phase 3: Resizable Windows**
1. Implement resize handle for plugin windows
2. Call `AudioProcessorEditor::setSize()` on resize
3. Persist window size per plugin (ApplicationProperties)
4. **UX target**: Smooth resize, no flicker

#### References:
- JUCE Plugin Hosting: [PluginWindow.h](https://github.com/juce-framework/JUCE/blob/master/examples/Plugins/AudioPluginHost/Source/UI/PluginWindow.h)
- VST3 GUI: [IPlugView Documentation](https://steinbergmedia.github.io/vst3_doc/vstinterfaces/classSteinberg_1_1IPlugView.html)

---

### W15: Mixer View (Priority: Medium)

**Goal**: Implement mixer panel with meters, faders, pan knobs.

#### Key Requirements:
- **Real-time meters**: Peak/RMS with 60 FPS updates
- **Smooth faders**: Sub-pixel rendering, no jumps
- **Zero allocations**: No heap allocations during paint()
- **Keyboard shortcuts**: Fader nudge, solo, mute shortcuts

#### Implementation Plan:

**Phase 1: Basic Layout**
1. Create `MixerChannel` component:
   - Meter (vertical bar with peak hold)
   - Fader (logarithmic scale, -∞ to +12 dB)
   - Pan knob (center-detent)
   - Solo/Mute buttons
   - Track name label
2. Use `juce::Viewport` for scrolling (many tracks)
3. **Perf target**: 128 mixer channels, <3ms paint time

**Phase 2: Real-Time Metering**
1. Audio thread writes peak/RMS to lock-free ring buffer:
   ```cpp
   struct MeterData
   {
       float peakL, peakR;
       float rmsL, rmsR;
       uint64_t timestamp;
   };
   ```
2. UI thread reads latest values at 60 FPS
3. Implement ballistics (peak hold, decay rate)
4. **Perf target**: <0.1ms metering overhead in audio callback

**Phase 3: Fader Automation**
1. Display automation curves on fader
2. Support touch/latch/write automation modes
3. Record fader movements to automation lane
4. **UX target**: Sample-accurate automation recording

#### Validation:
- **Render benchmark**: 128 mixer channels, 60 FPS sustained
- **Metering accuracy**: Verify dB readings with test tones
- **Automation test**: Record fader movement, verify playback matches

---

### W16: GPU/DWM Optimization (Priority: Low)

**Goal**: Minimize DWM composition overhead and GPU sync stalls.

#### Current State:
- ✅ ETW tracing (W7) provides DWM/Present visibility
- ⚠️ No explicit GPU optimizations yet

#### Optimizations:
1. **Flip model presentation**: Use `DXGI_SWAP_EFFECT_FLIP_DISCARD` for lower latency
2. **Reduce Present() calls**: Only present when UI changes (dirty flag)
3. **VSync control**: Allow disabling VSync for lower latency (trade: tearing)
4. **Texture atlases**: Batch small UI elements into single texture

#### Implementation:
- JUCE handles most DWM interaction automatically
- Focus on reducing paint() calls (virtualization, dirty regions)
- Use ETW tracing to identify Present() hotspots

#### Validation:
- **ETW analysis**: Verify Present() latency <8ms @ 60 Hz
- **Frame pacing**: Check for frame drops using PresentMon

---

## Diagnostics & Profiling

Comprehensive diagnostics enable developers and power users to identify performance bottlenecks and crashes.

### Current State (W0-W8)
- ✅ **W6**: Frame-time telemetry HUD (real-time paint timing)
- ✅ **W7**: ETW tracing (CPU, GPU, audio, DPC/ISR)
- ✅ **W8**: Crashpad crash reporting (minidumps)
- ⚠️ **Missing**: Audio buffer underrun logging, plugin crash detection

### W17: Audio Diagnostics (Priority: High)

**Goal**: Detect and log audio dropouts, glitches, and underruns.

#### Key Metrics:
1. **Buffer underruns**: Audio callback missed deadline
2. **Glitch count**: Discontinuities in audio stream
3. **CPU budget**: % of buffer period used by audio callback
4. **Longest callback**: Worst-case callback duration

#### Implementation Plan:

**Phase 1: Underrun Detection**
1. Measure audio callback duration using `std::chrono::high_resolution_clock`
2. Compare to buffer period (e.g., 512 samples / 48kHz = 10.67ms)
3. Log underruns with timestamp and stack trace:
   ```cpp
   if (callbackDuration > bufferPeriod * 0.9)  // 90% threshold
   {
       DBG("Audio underrun: " + String(callbackDuration) + "ms");
       // Optionally capture stack trace
   }
   ```
4. **Alert threshold**: Log warning if >5% CPU budget used

**Phase 2: Underrun HUD Overlay**
1. Add underrun counter to StatsOverlay (W6 HUD)
2. Display:
   - Total underruns this session
   - Time since last underrun
   - Worst-case callback duration
   - Current CPU budget %
3. **UX**: Flash red border on underrun (visual alert)

**Phase 3: ETW Audio Tracing**
1. Emit custom ETW events from audio callback:
   ```cpp
   // W7: Use existing ETW infrastructure
   TraceLoggingWrite(g_traceProvider,
       "AudioCallback",
       TraceLoggingFloat64(callbackDuration, "DurationMs"),
       TraceLoggingInt32(numSamples, "BufferSize"));
   ```
2. Analyze with WPA (W7 profile includes audio providers)
3. **Analysis**: Correlate underruns with DPC/ISR spikes

#### Validation:
- **Stress test**: Overload system (Prime95, disk I/O), verify underruns logged
- **ETW test**: Capture underrun with ETW, verify event appears in WPA

#### References:
- JUCE Audio Callbacks: [AudioIODeviceCallback](https://docs.juce.com/master/classAudioIODeviceCallback.html)
- ETW Custom Events: [TraceLogging API](https://learn.microsoft.com/en-us/windows/win32/tracelogging/trace-logging-portal)

---

### W18: Plugin Crash Detection (Priority: Medium)

**Goal**: Detect and recover from plugin crashes without killing DAW.

#### Implementation Plan:

**Phase 1: Out-of-Process Protection**
1. Load plugins in separate process (see W11)
2. Monitor plugin process health (heartbeat messages)
3. Detect crashes via process exit code
4. **Recovery**: Bypass crashed plugin, show error UI

**Phase 2: Crash Reporting**
1. Capture plugin crash dumps (Crashpad in plugin process)
2. Log plugin info to main DAW minidump:
   ```cpp
   annotations.set("plugin_crashed", "MyPlugin VST3 v1.2.3");
   ```
3. **Analysis**: Identify problematic plugins from crash reports

**Phase 3: Auto-Disable**
1. Track plugin crash count per plugin
2. After 3 crashes, auto-disable plugin (blacklist)
3. Prompt user: "MyPlugin has crashed 3 times. Disable permanently?"
4. **UX**: Prevent repeated crashes from same plugin

#### Validation:
- **Test**: Intentionally crash test plugin, verify DAW survives
- **Recovery**: Verify audio engine continues running after plugin crash

---

## Driver Guidance & Audio Interfaces

Windows audio drivers vary widely in quality and latency. Provide users with clear guidance on optimal configurations.

### W19: ASIO Integration (Priority: High)

**Goal**: Support ASIO drivers for low-latency professional audio.

#### Current State:
- ✅ WASAPI support (W3)
- ⚠️ ASIO not yet implemented

#### Implementation Plan:

**Phase 1: ASIO Device Support**
1. Enable JUCE ASIO support:
   ```cmake
   target_compile_definitions(ZenithDAW PRIVATE
       JUCE_ASIO=1
   )
   ```
2. Add ASIO device enumeration to AudioSettingsWindows (W3 UI)
3. Test with common ASIO drivers:
   - Focusrite Scarlett (Focusrite USB ASIO)
   - Universal Audio Apollo (UA Thunderbolt)
   - RME Babyface (RME USB ASIO)
   - ASIO4ALL (generic ASIO wrapper)

**Phase 2: Buffer Size Recommendations**
1. Show recommended buffer sizes per driver:
   - ASIO: 64-256 samples (1.3-5.3ms @ 48kHz)
   - WASAPI Exclusive: 128-512 samples (2.7-10.7ms)
   - WASAPI Shared: 480-960 samples (10-20ms)
2. Display latency estimates in UI (input + output)
3. **UX**: Warn if buffer size too low (risk of dropouts)

**Phase 3: Driver-Specific Optimizations**
1. Detect problematic drivers (e.g., ASIO4ALL with high-latency devices)
2. Suggest driver updates if known issues exist
3. Add "Audio Device Diagnostics" panel:
   - Current driver version
   - Supported sample rates
   - Buffer size range
   - Latency measurements (loopback test)

#### Validation:
- **Compatibility test**: Test with top 20 ASIO interfaces (survey results)
- **Latency test**: Measure roundtrip latency with loopback cable
- **Stress test**: Record 32 tracks @ 64 samples, verify no dropouts

#### References:
- JUCE ASIO: [ASIOAudioIODevice](https://docs.juce.com/master/classASIOAudioIODevice.html)
- ASIO SDK: [Steinberg ASIO SDK](https://www.steinberg.net/asiosdk)

---

### W20: MMCSS Fine-Tuning (Priority: Low)

**Goal**: Optimize MMCSS settings for lowest latency.

#### Current State:
- ✅ MMCSS Pro Audio priority enabled (W2)
- ⚠️ Default "Pro Audio" task only (no custom tuning)

#### Enhancements:
1. **Custom MMCSS tasks**: Register DAW-specific task profile
2. **CPU affinity**: Pin audio thread to performance cores (P-cores on hybrid CPUs)
3. **Priority boost**: Request higher priority during playback/recording
4. **Telemetry**: Log MMCSS scheduling metrics (context switches, priority inversions)

#### Implementation:
```cpp
// W2: Current implementation
HANDLE taskHandle = AvSetMmThreadCharacteristics(L"Pro Audio", &taskIndex);

// W20: Enhanced implementation
HANDLE taskHandle = AvSetMmThreadCharacteristics(L"ZenithDAW_Audio", &taskIndex);
AvSetMmThreadPriority(taskHandle, AVRT_PRIORITY_CRITICAL);  // Max priority

// Pin to P-core (CPU 0-7 on 12th Gen Intel)
SetThreadAffinityMask(GetCurrentThread(), 0xFF);  // Cores 0-7
```

#### Validation:
- **ETW analysis**: Verify reduced context switches during playback
- **Latency test**: Measure improvement in roundtrip latency (if any)

---

### W21: Buffer Size Guidance (Priority: Low)

**Goal**: Educate users on buffer size trade-offs.

#### UI Enhancements:
1. Add latency calculator to AudioSettingsWindows:
   ```
   Buffer Size: [256 samples]
   Sample Rate: [48000 Hz]

   → Latency: 5.3 ms (input + output)
   → Trade-off: Lower = more responsive, higher CPU load

   Recommended for your system: 128-256 samples
   ```

2. Show CPU load per buffer size (live graph):
   - X-axis: Buffer size (64, 128, 256, 512, 1024)
   - Y-axis: CPU % (estimated from current session)
   - Highlight optimal range

3. Add presets:
   - **Ultra-Low Latency**: 64 samples (1.3ms) - tracking, live performance
   - **Low Latency**: 128 samples (2.7ms) - mixing, editing
   - **Balanced**: 256 samples (5.3ms) - general use
   - **High Stability**: 512 samples (10.7ms) - large projects, plugin-heavy

#### References:
- Buffer Size Guide: [Sweetwater - Audio Buffer Size Explained](https://www.sweetwater.com/sweetcare/articles/setting-the-correct-buffer-size-for-your-audio-interface/)

---

## Packaging & Distribution

Production-ready packaging requires installers, code signing, and update infrastructure.

### W22: Symbol Server (Priority: Medium)

**Goal**: Host PDB files for crash dump analysis.

#### Current State:
- ✅ PDBs generated for all builds (CMake `/Zi /DEBUG:FULL`)
- ⚠️ PDBs not archived or versioned

#### Implementation Plan:

**Phase 1: Local Symbol Archive**
1. Create `symbols/` directory in repo:
   ```
   symbols/
   ├── 0.1.0/
   │   ├── ZenithDAW.pdb
   │   ├── ZenithDAW.exe (copy for version tracking)
   │   └── build_info.json
   ├── 0.1.1/
   └── ...
   ```
2. Add post-build script to copy PDBs:
   ```cmake
   add_custom_command(TARGET ZenithDAW POST_BUILD
       COMMAND ${CMAKE_COMMAND} -E copy
           $<TARGET_FILE:ZenithDAW>
           ${CMAKE_SOURCE_DIR}/symbols/${PROJECT_VERSION}/
       COMMAND ${CMAKE_COMMAND} -E copy
           $<TARGET_PDB_FILE:ZenithDAW>
           ${CMAKE_SOURCE_DIR}/symbols/${PROJECT_VERSION}/
   )
   ```

**Phase 2: Microsoft Symbol Server**
1. Set up Azure Blob Storage for symbols (optional)
2. Upload PDBs via `symstore.exe`:
   ```cmd
   symstore add /f ZenithDAW.pdb /s \\symbols\share /t "Zenith DAW" /v "0.1.0"
   ```
3. Configure WinDbg symbol path:
   ```
   SRV*C:\Symbols*https://zenith.blob.core.windows.net/symbols
   ```

**Phase 3: Automated Uploads**
1. Add CI/CD step to upload PDBs on release
2. Verify uploads with test crash dump analysis
3. **SLA**: Symbols available within 5 minutes of release

#### References:
- Symbol Server: [Microsoft Docs - Using SymStore](https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/symstore)
- Azure Symbols: [Azure DevOps Symbol Server](https://learn.microsoft.com/en-us/azure/devops/artifacts/symbols/)

---

### W23: Code Signing (Priority: Critical for Release)

**Goal**: Sign executables to avoid SmartScreen warnings.

#### Implementation Plan:

**Phase 1: Certificate Acquisition**
1. Purchase code signing certificate:
   - **EV Certificate** (Extended Validation) - instant SmartScreen reputation
   - Cost: ~$300-500/year
   - Providers: DigiCert, Sectigo, SSL.com
2. Store certificate securely (HSM or Azure Key Vault)

**Phase 2: Signing Infrastructure**
1. Use `signtool.exe` for signing:
   ```cmd
   signtool sign /f cert.pfx /p password /t http://timestamp.digicert.com ZenithDAW.exe
   ```
2. Add to build pipeline:
   ```cmake
   if(WIN32 AND DEFINED ENV{SIGN_CERTIFICATE})
       add_custom_command(TARGET ZenithDAW POST_BUILD
           COMMAND signtool sign /f $ENV{SIGN_CERTIFICATE} /p $ENV{SIGN_PASSWORD}
                   /t http://timestamp.digicert.com $<TARGET_FILE:ZenithDAW>
       )
   endif()
   ```

**Phase 3: Installer Signing**
1. Sign installer executable (NSIS, WiX, or Inno Setup)
2. Sign all DLLs and executables inside installer
3. **Validation**: Verify signature in Windows properties, check SmartScreen

#### References:
- Code Signing: [Microsoft Docs - SignTool](https://learn.microsoft.com/en-us/windows/win32/seccrypto/signtool)
- SmartScreen: [Building Reputation](https://learn.microsoft.com/en-us/windows/security/threat-protection/intelligence/prevent-malware-infection)

---

### W24: Installer (Priority: High)

**Goal**: Create professional installer with uninstaller.

#### Implementation Plan:

**Phase 1: Installer Choice**
- **Option 1: NSIS** (Nullsoft Scriptable Install System) - Free, mature
- **Option 2: WiX Toolset** - Windows Installer (MSI), enterprise-friendly
- **Option 3: Inno Setup** - Free, simple scripting
- **Recommendation**: WiX for MSI support (Group Policy, silent installs)

**Phase 2: Installer Features**
1. Install locations:
   - Executable: `C:\Program Files\ZenithDAW\`
   - User data: `%APPDATA%\ZenithDAW\` (settings, presets)
   - Shared data: `%PROGRAMDATA%\ZenithDAW\` (plugin cache)
2. Registry keys:
   - Uninstall info: `HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall\ZenithDAW`
   - File associations: `.zdw` (Zenith DAW project)
3. Start Menu shortcuts
4. Desktop shortcut (optional)
5. Uninstaller with clean removal

**Phase 3: Silent Install**
1. Support `/S` flag for silent install (IT admins)
2. Support `/D=C:\CustomPath` for custom install location
3. Example:
   ```cmd
   ZenithDAW-Setup.exe /S /D=C:\DAW\Zenith
   ```

**Phase 4: Upgrades**
1. Detect previous version (registry check)
2. Preserve user settings during upgrade
3. Offer "Repair" and "Modify" options

#### References:
- WiX Toolset: [wixtoolset.org](https://wixtoolset.org/)
- NSIS: [nsis.sourceforge.io](https://nsis.sourceforge.io/)

---

### W25: Winget / Microsoft Store (Priority: Low)

**Goal**: Distribute via Windows Package Manager and Microsoft Store.

#### Winget Manifest:
1. Create `zenith-daw.yaml` manifest:
   ```yaml
   PackageIdentifier: ZenithAudio.ZenithDAW
   PackageVersion: 0.1.0
   PackageName: Zenith DAW
   Publisher: ZenithAudio
   License: Proprietary
   ShortDescription: Professional Digital Audio Workstation
   Installers:
     - Architecture: x64
       InstallerType: msi
       InstallerUrl: https://zenith.audio/downloads/ZenithDAW-0.1.0.msi
       InstallerSha256: <hash>
   ```
2. Submit to [winget-pkgs repo](https://github.com/microsoft/winget-pkgs)
3. **Benefit**: Users can install via `winget install ZenithAudio.ZenithDAW`

#### Microsoft Store:
1. Requires MSIX packaging (different from MSI)
2. Submit via Partner Center
3. **Benefits**: Auto-updates, sandboxing, wider distribution
4. **Trade-offs**: 30% revenue share (if paid app)

---

## Quality Assurance

Rigorous QA ensures stability and performance in production.

### W26: Performance Gates (Priority: High)

**Goal**: Automated performance regression testing.

#### Implementation Plan:

**Phase 1: Benchmark Suite**
1. Create benchmark tests:
   ```cpp
   TEST(AudioEngine, MixingPerformance)
   {
       // Load 32 tracks, 5000 clips
       // Measure audio callback duration
       EXPECT_LT(avgCallbackTime, 2.5);  // <2.5ms @ 512 samples
   }

   TEST(TrackView, RenderPerformance)
   {
       // Render 100 tracks visible
       auto start = HighResolutionClock::now();
       trackView.paint(g);
       auto duration = HighResolutionClock::now() - start;
       EXPECT_LT(duration.count(), 5.0);  // <5ms
   }
   ```

2. Run benchmarks in CI/CD:
   - Every commit on main branch
   - Compare to baseline (previous release)
   - Fail build if >10% regression

**Phase 2: Perf Gates**
| Component | Metric | Threshold | Action on Fail |
|-----------|--------|-----------|----------------|
| Audio callback | Avg duration | <50% buffer period | Block merge |
| TrackView paint | 99th percentile | <8ms (60 FPS) | Block merge |
| Plugin load | Time to open UI | <500ms | Warning |
| Project load | Time to playback | <3s (1000 clips) | Warning |
| Memory usage | Heap size | <500 MB (empty project) | Warning |

**Phase 3: Telemetry**
1. Collect perf metrics from users (opt-in)
2. Aggregate stats (median, p95, p99)
3. Identify regressions in production
4. **Privacy**: Anonymize, no personal data

#### References:
- Google Benchmark: [github.com/google/benchmark](https://github.com/google/benchmark)
- Tracy Profiler: [github.com/wolfpld/tracy](https://github.com/wolfpld/tracy) (frame-level profiling)

---

### W27: Smoke Tests (Priority: High)

**Goal**: Automated UI/integration tests for critical workflows.

#### Test Scenarios:
1. **Project Lifecycle**:
   - Create new project → Add track → Import audio → Play → Save → Load
   - **Expected**: No crashes, project loads correctly
2. **Audio Playback**:
   - Load 100-track project → Play → Stop → Seek → Play
   - **Expected**: No dropouts, playback position correct
3. **Plugin Hosting**:
   - Insert VST3 plugin → Open UI → Change parameters → Close → Reopen
   - **Expected**: Plugin UI loads, parameters persist
4. **Settings**:
   - Change audio device → Change buffer size → Restart app
   - **Expected**: Settings persist, audio device switches correctly

#### Implementation:
- Use JUCE `UnitTest` framework for UI tests
- Run tests on CI (Windows Server VM with audio device)
- **Frequency**: Every release candidate build

---

### W28: Crash Budget (Priority: Medium)

**Goal**: Define acceptable crash rates and monitor in production.

#### Crash Metrics:
1. **Crash rate**: Crashes per user-hour
   - **Target**: <0.01% (1 crash per 10,000 user-hours)
   - **Threshold**: >0.1% triggers investigation
2. **Top crashes**: Most common crash stacks
   - **Target**: Top crash <10% of total crashes (diverse failure modes)
   - **Action**: Fix top 3 crashes each release
3. **Plugin crashes**: Crashes attributed to plugins
   - **Target**: <50% of crashes (isolate plugin issues)

#### Monitoring:
- Collect crash dumps via W8 Crashpad
- Analyze with WinDbg, stack trace grouping
- Prioritize fixes based on frequency and severity

---

## Future Enhancements

Beyond the core roadmap, these enhancements improve user experience and performance.

### W29: Multi-Threaded Rendering (Priority: Low)

- **Goal**: Parallelize TrackView rendering across multiple threads
- **Benefit**: Faster paint() on multi-core CPUs
- **Complexity**: High (thread safety, synchronization)

### W30: Vulkan Rendering (Priority: Low)

- **Goal**: Replace OpenGL with Vulkan for lower-latency GPU access
- **Benefit**: <1ms reduced Present() latency
- **Complexity**: Very High (Vulkan API is verbose)

### W31: Remote Plugin Hosting (Priority: Medium)

- **Goal**: Load plugins on remote machine (network audio)
- **Benefit**: Offload CPU to separate computer
- **Use case**: Large orchestral templates with 100+ plugins

### W32: Hardware Monitoring (Priority: Low)

- **Goal**: Display CPU temperature, GPU usage, disk I/O in HUD
- **Benefit**: Identify thermal throttling, disk bottlenecks
- **Implementation**: Use Windows Performance Counters (PDH API)

### W33: Automatic Performance Tuning (Priority: Low)

- **Goal**: ML-based buffer size recommendation
- **Input**: System specs, project complexity, plugin list
- **Output**: Optimal buffer size for best latency without dropouts

---

## Appendix: Tool Reference

### Profiling Tools

| Tool | Purpose | When to Use |
|------|---------|-------------|
| **W6 HUD** | Real-time frame timing | Continuous monitoring during dev |
| **W7 ETW + WPA** | Kernel-level CPU/GPU analysis | Investigating dropouts, DPC storms |
| **W8 Crashpad** | Crash dump capture | Post-mortem crash analysis |
| **PresentMon** | Frame latency measurement | Validating render optimizations |
| **Visual Studio Profiler** | CPU sampling, instrumentation | Hotspot identification |
| **Intel VTune** | Advanced CPU/memory profiling | Deep performance analysis |
| **Tracy Profiler** | Frame-level timeline | UI frame drops, async tasks |
| **Windows Performance Monitor** | System-level metrics | Disk I/O, memory, network |

### Build Tools

| Tool | Purpose |
|------|---------|
| **CMake 3.22+** | Build system generator |
| **Visual Studio 2022** | C++ compiler (MSVC 19.x) |
| **Ninja** | Fast build executor (optional) |
| **JUCE 8.0.9** | Audio framework |
| **Git LFS** | Binary asset storage (audio files) |

### Distribution Tools

| Tool | Purpose |
|------|---------|
| **WiX Toolset** | MSI installer creation |
| **SignTool** | Code signing |
| **SymStore** | Symbol server uploads |
| **Winget** | Package manager distribution |

---

## References

- **JUCE Documentation**: [docs.juce.com](https://docs.juce.com/)
- **Windows Audio**: [Microsoft Docs - Core Audio APIs](https://learn.microsoft.com/en-us/windows/win32/coreaudio/core-audio-apis-in-windows-vista)
- **ETW**: [Microsoft Docs - Event Tracing](https://learn.microsoft.com/en-us/windows/win32/etw/event-tracing-portal)
- **Crashpad**: [chromium.googlesource.com/crashpad](https://chromium.googlesource.com/crashpad/crashpad/)
- **Real-Time Audio**: [Ross Bencina - Real-Time Audio Programming 101](http://www.rossbencina.com/code/real-time-audio-programming-101-time-waits-for-nothing)

---

**Document Version**: 1.0
**Last Updated**: 2025-01-12
**Status**: Living Document (update as roadmap evolves)
