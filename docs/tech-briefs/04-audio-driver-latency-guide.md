# Low-Latency Audio Driver Guide

**ASIO, CoreAudio, and WASAPI Configuration for Zenith DAW**
**Version:** 1.0
**Date:** 2025-11-10

---

## Executive Summary

This document provides comprehensive guidance for implementing low-latency audio I/O using:

- **ASIO** (Windows) — Industry standard for professional audio
- **CoreAudio** (macOS) — Native macOS audio API
- **WASAPI** (Windows) — Native Windows audio with low-latency mode

**Target Latencies:**
- **Recording:** 32-64 samples (0.7-1.5ms @ 48kHz) for direct monitoring
- **Mixing:** 128-256 samples (2.7-5.3ms @ 48kHz) for CPU headroom
- **Mastering:** 512-1024 samples (10.7-21.3ms @ 48kHz) for complex processing

---

## Platform-Specific Driver Technologies

### ASIO (Windows) — Steinberg

**Official Resources:**
- ASIO Forum: https://forums.steinberg.net/c/developer/asio/104
- Built-in ASIO Driver (latest): https://helpcenter.steinberg.de/hc/en-us/articles/17863730844946
- ASIO SDK Download: https://steinberg.net/developers/ (requires registration)

**What is ASIO?**
> "Audio Stream Input/Output (ASIO) is a computer device driver protocol for digital audio specified by Steinberg that provides a low-latency and high fidelity interface between a software application and the sound card of a computer."
> — [Steinberg Forums: ASIO Overview](https://forums.steinberg.net/t/asio-what-is-it/201552)

> "ASIO is supported by all well-known manufacturers of audio interfaces and is required by practically all professional audio programs under Windows, partly because of its low latency."
> — [Steinberg Help Center](https://helpcenter.steinberg.de/hc/en-us/articles/17863730844946)

**Recent Release (2024):**
> "Steinberg built-in ASIO Driver 1.0.9 was released on December 18, 2024 for Windows 10 64-Bit, Windows 11, and Windows 11 on Arm."
> — [Steinberg Help Center](https://helpcenter.steinberg.de/hc/en-us/articles/17863730844946)

**Pros:**
- ✅ **Lowest latency** on Windows (sub-10ms round-trip)
- ✅ **Exclusive device access** (no OS audio mixing)
- ✅ **Sample-accurate timing** (critical for MIDI)
- ✅ **Industry standard** (all pro audio hardware provides ASIO drivers)

**Cons:**
- ❌ **Exclusive access** (only one application can use device)
- ❌ **One driver at a time** (can't mix multiple USB interfaces)
- ❌ **Driver quality varies** (hardware manufacturer dependent)

### CoreAudio (macOS) — Apple

**Official Documentation:**
- Core Audio Overview: https://developer.apple.com/library/archive/documentation/MusicAudio/Conceptual/CoreAudioOverview/
- Core Audio Essentials: https://developer.apple.com/library/archive/documentation/MusicAudio/Conceptual/CoreAudioOverview/CoreAudioEssentials/CoreAudioEssentials.html
- Technical Q&A QA1467 (Overload Warnings): https://developer.apple.com/library/archive/qa/qa1467/
- Reducing Latency (MainStage): https://support.apple.com/en-us/101938

**Buffer Size Range:**
> "The buffer size ranges from 16 to 1024 samples. Lower I/O Buffer Sizes result in less latency, but might induce audio artifacts, especially if you use a lot of plug-ins and channel strips simultaneously."
> — [Apple Support: How to Reduce Latency in MainStage](https://support.apple.com/en-us/101938)

**System Integration:**
> "The Mac offers reduced variables, with computer hardware, Mac OS, CoreAudio, and AudioUnits all made by one company, which generally provides better driver consistency."
> — [Gearspace: Audio Driver Comparison](https://gearspace.com/board/music-computers/1376608-asio-still-necessary-low-latency.html)

**Pros:**
- ✅ **Unified API** (same API for all hardware)
- ✅ **Automatic sample rate conversion** (OS handles resampling)
- ✅ **Multi-app support** (shared access by default)
- ✅ **Stable drivers** (OS-level integration)

**Cons:**
- ❌ **Slightly higher latency** than ASIO on Windows (but still <10ms)
- ❌ **Less control** over driver behavior (OS-managed)

### WASAPI (Windows) — Microsoft

**Official Documentation:**
- WASAPI Overview: https://learn.microsoft.com/en-us/windows/win32/coreaudio/
- Exclusive-Mode Streams: https://learn.microsoft.com/en-us/windows/win32/coreaudio/exclusive-mode-streams
- Low Latency Audio: https://learn.microsoft.com/en-us/windows-hardware/drivers/audio/low-latency-audio

**Low Latency Improvements (Windows 10+):**
> "One advantage of Windows 10 is that WASAPI Shared mode supports low latency playback and recording at buffer sizes down to 2 milliseconds, which is a significant improvement over Windows 7."
> — [Microsoft Q&A: WASAPI Latency](https://learn.microsoft.com/en-us/answers/questions/280467/is-it-possible-that-shared-audio-latency-in-wasapi)

> "In Windows 10 and later, the audio engine latency has been reduced to 1.3 ms for all applications."
> — [Microsoft Learn: Low Latency Audio](https://learn.microsoft.com/en-us/windows-hardware/drivers/audio/low-latency-audio)

**Exclusive Mode:**
> "When used in exclusive mode WASAPI bypasses the Win audio engine. It transports the output of the media player directly (and unaltered) to the driver of the audio device."
> — [The Well Tempered Computer: WASAPI Guide](https://www.thewelltemperedcomputer.com/KB/WASAPI.htm)

> "Applications that use exclusive-mode streams often do so because they require low latencies in the data paths between the audio endpoint devices and the application threads that access the endpoint buffers."
> — [Microsoft Learn: Exclusive-Mode Streams](https://learn.microsoft.com/en-us/windows/win32/coreaudio/exclusive-mode-streams)

**Shared Mode:**
> "When used in shared mode, it uses the Win audio engine. All audio sent to this engine is converted to the settings in the Win audio panel, which adds latency but allows multiple applications to use the audio device simultaneously."
> — [The Well Tempered Computer: WASAPI Guide](https://www.thewelltemperedcomputer.com/KB/WASAPI.htm)

**Pros:**
- ✅ **Native Windows API** (no third-party drivers needed)
- ✅ **Shared mode** (multiple apps can use audio simultaneously)
- ✅ **Low latency in exclusive mode** (comparable to ASIO)
- ✅ **Future-proof** (Microsoft's recommended API)

**Cons:**
- ❌ **Not as low as ASIO** (typically ~3-10ms vs ASIO's 2-5ms)
- ❌ **Few DAWs support WASAPI Low Latency Mode** (limited adoption)
- ❌ **Compatibility issues** with some hardware

---

## Latency Tuning Guide

### Understanding Latency Components

**Total Round-Trip Latency = Input Latency + Processing Latency + Output Latency**

```
Audio Input → [Driver Buffer] → [DAW Processing] → [Driver Buffer] → Audio Output
               ↑ Input Latency  ↑ Plugin/DSP     ↑ Output Latency
```

**Example Calculation (44.1kHz, 128 samples):**
- Input buffer: 128 samples = 2.9ms
- Processing: ~1ms (depends on CPU/plugins)
- Output buffer: 128 samples = 2.9ms
- **Total:** ~6.8ms round-trip

**Example Calculation (48kHz, 64 samples):**
- Input buffer: 64 samples = 1.33ms
- Processing: ~0.5ms (minimal DSP)
- Output buffer: 64 samples = 1.33ms
- **Total:** ~3.2ms round-trip

### Recommended Buffer Sizes by Workflow

> **For MIDI Recording:**
> "For MIDI recording, buffer sizes of 32 or 64 samples are recommended to minimize latency and enable playing notes without delay. This is especially important when recording notes with fast attack, like drum hits, stabs, or plucks."
> — [Sweetwater: Which Buffer Size Should I Use?](https://www.sweetwater.com/sweetcare/articles/which-buffer-size-setting-should-i-use-in-my-daw/)

> **For Audio Recording:**
> "64 samples is a good target for recording; 32 samples is excellent if your computer can handle it. However, for recording audio, the 128-256 sample range is recommended to avoid crackling and other audio interruptions."
> — [Gemtracks: Buffer Size Guide](https://www.gemtracks.com/resources/guides/view.php?title=what-buffer-size-should-i-use&id=5922)

> **For Mixing/Mastering:**
> "Higher buffer sizes (256 to 1024 samples) are preferable during mixing and mastering, allowing for extensive processing without latency concerns."
> — [Home Studio Magic: Buffer Size Guide](https://homestudiomagic.com/what-is-daw-buffer-size-and-does-it-affect-sound-quality/)

**Summary Table:**

| **Workflow** | **Buffer Size (samples)** | **Latency @ 48kHz** | **Use Case** |
|--------------|---------------------------|---------------------|--------------|
| **MIDI Input** | 32-64 | 0.7-1.3ms | Virtual instruments, drum programming |
| **Audio Recording** | 64-128 | 1.3-2.7ms | Vocals, guitars, live monitoring |
| **Mixing (Light)** | 128-256 | 2.7-5.3ms | Light plugin use, automation |
| **Mixing (Heavy)** | 256-512 | 5.3-10.7ms | Many plugins, parallel processing |
| **Mastering** | 512-1024 | 10.7-21.3ms | Complex chains, offline bounces |

**Key Insight:**
> "A lower buffer size means quicker data handling but increases demand for processing power and is more strenuous on your computer, while a higher buffer size is easier on the CPU but may cause latency."
> — [Orpheus Audio Academy: Buffer Size Explained](https://www.orpheusaudioacademy.com/buffer-size/)

> "Buffer size does not harm sound quality and is only known to affect CPU speed and cause latency."
> — [Orpheus Audio Academy: Buffer Size Explained](https://www.orpheusaudioacademy.com/buffer-size/)

---

## Platform-Specific Tuning

### Windows: ASIO vs WASAPI

**When to Use ASIO:**
- Professional recording with audio interfaces
- Lowest possible latency (<5ms)
- MIDI recording with virtual instruments
- Exclusive device control needed

**When to Use WASAPI:**
- Built-in sound cards (no ASIO driver available)
- Multiple applications need audio simultaneously
- Lower CPU usage (shared mode)
- No dedicated audio interface

**Comparison:**
> "ASIO on Windows can achieve real world round trip latencies below 10ms with modern audio interfaces, with 2-3ms in each direction being ideal."
> — [Sound on Sound: Optimising Latency](https://www.soundonsound.com/techniques/optimising-latency-pc-audio-interface)

> "WASAPI Exclusive: Works with all audio devices and has reasonably low latency, not quite as low as ASIO, but probably low enough for most uses."
> — [FlexASIO: Real-World Latency Testing](https://github.com/dechamps/FlexASIO/issues/153)

**Recommendation:**
> "For gaming, streaming, or video calls, Windows audio drivers (MME, WDM, WASAPI) work fine, but for pro audio recording, ASIO is preferred for its lowest latency."
> — [Gearspace: ASIO vs WASAPI](https://gearspace.com/board/music-computers/1376608-asio-still-necessary-low-latency.html)

### macOS: CoreAudio Optimization

**System Settings:**
1. **Disable Bluetooth Audio:** Adds 150-250ms latency
2. **Close Background Apps:** Safari, Chrome can cause glitches
3. **Disable Energy Saving:** Use "High Performance" mode during recording

**Sample Rate Considerations:**
- **44.1kHz:** CD quality, standard for music production
- **48kHz:** Video standard, slightly lower latency (fewer samples = less time)
- **96kHz/192kHz:** High-res, but 2-4x CPU usage (rarely needed)

**Best Practice:**
> "When using CoreAudio on macOS, latency measurements are consistent if no dropouts occur."
> — [Sound Design Stack Exchange: Focusrite Latency](https://sound.stackexchange.com/questions/38466/focusrite-latency-drivers-win-vs-mac)

---

## Device Selection Flow (UI Pseudo-Logic)

### Startup Device Detection

```cpp
// On application startup
void ZenithDAW::initializeAudioDevice()
{
    auto& deviceManager = engine.getDeviceManager();

    // 1. Check for saved device preferences
    auto savedSetup = projectState.getAudioDeviceSetup();

    if (savedSetup.isValid()) {
        // Try to use saved device
        auto error = deviceManager.setAudioDeviceSetup(savedSetup, true);

        if (error.isEmpty()) {
            showNotification("Audio device ready: " + savedSetup.outputDeviceName);
            return;
        }
        else {
            showWarning("Saved audio device not found. Please select a device.");
        }
    }

    // 2. Auto-detect best available device
    String error = autoSelectAudioDevice();

    if (error.isEmpty()) {
        showNotification("Audio device auto-configured");
    }
    else {
        showAudioSettingsDialog("Please configure your audio device");
    }
}

// Auto-select best available device
String ZenithDAW::autoSelectAudioDevice()
{
    auto& deviceManager = engine.getDeviceManager();

    // Platform-specific device selection
    #if JUCE_WINDOWS
        // Priority: ASIO > WASAPI Exclusive > WASAPI Shared
        if (auto* asioType = deviceManager.getAvailableDeviceTypes().getFirst()) {
            if (asioType->getTypeName() == "ASIO") {
                asioType->scanForDevices();
                auto devices = asioType->getDeviceNames(false); // outputs

                if (devices.size() > 0) {
                    // Use first ASIO device
                    AudioDeviceManager::AudioDeviceSetup setup;
                    setup.outputDeviceName = devices[0];
                    setup.inputDeviceName = asioType->getDeviceNames(true)[0];
                    setup.sampleRate = 48000;
                    setup.bufferSize = 128; // Default to 128 samples

                    return deviceManager.setAudioDeviceSetup(setup, true);
                }
            }
        }

        // Fallback: WASAPI
        // (Similar logic for WASAPI)

    #elif JUCE_MAC
        // CoreAudio is the only option on macOS
        auto* coreAudioType = deviceManager.getAvailableDeviceTypes().getFirst();

        if (coreAudioType) {
            coreAudioType->scanForDevices();
            auto devices = coreAudioType->getDeviceNames(false); // outputs

            // Prefer dedicated audio interfaces over built-in
            String selectedDevice;
            for (auto& device : devices) {
                if (!device.containsIgnoreCase("built-in")) {
                    selectedDevice = device;
                    break;
                }
            }

            if (selectedDevice.isEmpty() && devices.size() > 0)
                selectedDevice = devices[0];

            AudioDeviceManager::AudioDeviceSetup setup;
            setup.outputDeviceName = selectedDevice;
            setup.inputDeviceName = selectedDevice; // Same device for I/O
            setup.sampleRate = 48000;
            setup.bufferSize = 128;

            return deviceManager.setAudioDeviceSetup(setup, true);
        }
    #endif

    return "No audio devices found";
}
```

### User-Facing Audio Settings UI

```
┌─────────────────────────────────────────────────────────────┐
│ Audio Settings                                              │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│ Driver Type:     [ASIO ▼]                                  │
│                  • ASIO (Lowest Latency)                    │
│                  • WASAPI Exclusive                         │
│                  • WASAPI Shared                            │
│                                                             │
│ Output Device:   [Focusrite USB ASIO ▼]                    │
│ Input Device:    [Focusrite USB ASIO ▼]                    │
│                                                             │
│ Sample Rate:     [48000 Hz ▼]                              │
│                                                             │
│ Buffer Size:     [128 samples]  ← [Slider: 32-1024] →      │
│                  Latency: 2.7ms (Input) + 2.7ms (Output)   │
│                          = 5.4ms Round-Trip                 │
│                                                             │
│ [Test Audio]  [Apply]  [Cancel]                            │
│                                                             │
│ Tip: Lower buffer sizes reduce latency but increase CPU    │
│      usage. Start with 128 samples for recording.          │
└─────────────────────────────────────────────────────────────┘
```

### Device Hot-Plug Handling

**Challenge:**
> "Users commonly experience issues when switching from ASIO to WASAPI, with sound vanishing and receiving 'audio device closed' messages. This typically occurs because applications take exclusive control of audio devices on a 'first come, first served' basis."
> — [Sound Design Stack Exchange: REAPER Device Switching](https://sound.stackexchange.com/questions/52846/reaper-switching-between-asio-and-wasapi-devices)

**Limitation:**
> "ASIO's primary weakness is that you can only use one ASIO driver at a time, which poses challenges as more studio equipment comes with USB interfaces."
> — [KVR Audio: ASIO Limitations](https://www.kvraudio.com/forum/viewtopic.php?t=600803)

**Best Practice:**
1. **Detect device removal:** Listen to `AudioDeviceManager::audioDeviceAboutToStop()`
2. **Pause playback immediately:** Stop audio engine gracefully
3. **Show dialog:** "Audio device disconnected. Please reconnect or select a new device."
4. **Auto-reconnect:** If same device comes back, restore settings

```cpp
// In ZenithEngine class
void ZenithEngine::audioDeviceStopped()
{
    // CALLED ON AUDIO THREAD!
    // Device was removed or stopped

    // Post message to GUI thread
    MessageManager::callAsync([this]() {
        showDeviceDisconnectedDialog();
    });
}

void ZenithEngine::showDeviceDisconnectedDialog()
{
    // On GUI thread
    auto result = AlertWindow::showYesNoCancelBox(
        AlertWindow::WarningIcon,
        "Audio Device Disconnected",
        "Your audio device has been disconnected. Would you like to:\n\n"
        "• Reconnect (if you plugged it back in)\n"
        "• Select a different device\n"
        "• Continue without audio",
        "Reconnect", "Select Device", "Continue"
    );

    if (result == 1) {  // Reconnect
        // Try to initialize same device
        initializeAudioDevice();
    }
    else if (result == 2) {  // Select Device
        showAudioSettingsDialog();
    }
}
```

---

## QA Testing Checklist: Glitch Testing at Low Buffer Sizes

### Test Matrix

| **Buffer Size** | **Sample Rate** | **Expected Round-Trip Latency** | **CPU Load** | **Stability** |
|-----------------|-----------------|----------------------------------|--------------|---------------|
| 32 samples      | 48kHz           | ~1.3ms                           | Very High    | May crackle on lower-end CPUs |
| 64 samples      | 48kHz           | ~2.7ms                           | High         | Stable on modern CPUs |
| 128 samples     | 48kHz           | ~5.3ms                           | Medium       | Stable (recommended default) |
| 256 samples     | 48kHz           | ~10.7ms                          | Low          | Very stable |

### Test Scenarios

#### 1. Clean Passthrough (Baseline)

**Setup:**
- No plugins loaded
- One audio track (stereo)
- Input: Sine wave generator (1kHz)
- Output: Loopback to input

**Test:**
- [ ] **32 samples:** Audio passes cleanly without glitches for 5 minutes
- [ ] **64 samples:** Audio passes cleanly without glitches for 5 minutes
- [ ] **128 samples:** Audio passes cleanly without glitches for 5 minutes

**Success Criteria:** Zero dropouts, zero clicks/pops

#### 2. Plugin Stress Test

**Setup:**
- 10 audio tracks, each with:
  - 1x EQ plugin
  - 1x Compressor plugin
  - 1x Reverb plugin (DSP-heavy)
- Play all tracks simultaneously

**Test:**
- [ ] **64 samples:** System handles load without glitches
- [ ] **128 samples:** System handles load without glitches
- [ ] **CPU usage < 80%** at peak load

**Success Criteria:** No audio glitches, CPU meter shows headroom

#### 3. MIDI Latency Test

**Setup:**
- MIDI controller → Virtual instrument (piano, synth)
- Buffer size: 64 samples

**Test:**
- [ ] Play rapid notes (16th notes at 120 BPM)
- [ ] No perceivable delay between key press and sound
- [ ] Timing is tight (no "sloppy" feel)

**Success Criteria:** <5ms perceived latency (feels "instant")

#### 4. Device Switching Test

**Test:**
- [ ] Start playback with Device A
- [ ] Unplug Device A (simulate disconnect)
- [ ] Verify DAW shows error message (doesn't crash)
- [ ] Select Device B
- [ ] Verify playback resumes cleanly

**Success Criteria:** No crash, graceful recovery

#### 5. Sample Rate Change Test

**Test:**
- [ ] Start project at 44.1kHz
- [ ] Change to 48kHz mid-session
- [ ] Verify audio continues without artifacts
- [ ] Verify buffer size scales correctly (time stays constant)

**Success Criteria:** No clicks, no glitches, no crash

#### 6. Background App Stress Test

**Setup:**
- Open DAW with 64-sample buffer
- Open Chrome with 20 tabs
- Open OBS (streaming software)
- Play a Spotify track in background

**Test:**
- [ ] DAW still plays audio cleanly
- [ ] No xruns/buffer underruns
- [ ] CPU usage doesn't spike excessively

**Success Criteria:** DAW remains stable despite background load

#### 7. Exclusive Mode Test (ASIO/WASAPI)

**Windows ASIO:**
- [ ] Open DAW, start playback
- [ ] Try to play YouTube video in Chrome
- [ ] Verify: Chrome cannot play audio (ASIO is exclusive)
- [ ] Stop DAW
- [ ] Verify: Chrome can now play audio

**WASAPI Exclusive:**
- [ ] Same test as ASIO
- [ ] Verify exclusive mode is enforced

**Success Criteria:** Exclusive access is respected

---

## Common Pitfalls & Solutions

### Problem 1: Crackling at Low Buffer Sizes

**Symptoms:** Audio crackles, pops, or stutters at 32-64 samples

**Causes:**
- CPU not fast enough
- Too many plugins loaded
- Background apps consuming CPU
- USB controller sharing bandwidth (USB audio interfaces)

**Solutions:**
1. **Increase buffer size** to 128 or 256 samples
2. **Freeze tracks** (bounce to audio) to reduce plugin load
3. **Close background apps** (browsers, Discord, Spotify)
4. **Use dedicated USB controller** for audio interface (not shared with mouse/keyboard)

### Problem 2: Device Not Found After Disconnect

**Symptoms:** Device was working, unplugged, re-plugged, but DAW can't find it

**Causes:**
- Driver cache not refreshed
- Device enumeration bug

**Solutions:**
1. **Close and reopen audio settings dialog**
2. **Restart DAW**
3. **Re-scan devices** (refresh button in audio settings)

### Problem 3: ASIO "Device Already in Use"

**Symptoms:** Can't open ASIO device, error: "Device is already in use"

**Causes:**
- Another application has exclusive access (e.g., Spotify, Chrome with ASIO plugin)
- Previous DAW instance didn't release device cleanly

**Solutions:**
1. **Close all audio applications**
2. **Kill background processes** (check Task Manager / Activity Monitor)
3. **Restart audio driver** (Windows: Device Manager → Disable/Enable)

### Problem 4: High Latency Despite Low Buffer Size

**Symptoms:** Buffer size is 64 samples, but latency feels >50ms

**Causes:**
- Plugin with high internal latency (e.g., Linear Phase EQ)
- Incorrect latency compensation in DAW
- Sample rate mismatch (OS resampling audio)

**Solutions:**
1. **Check plugin latency:** Use `getLatencySamples()` API
2. **Enable automatic delay compensation** (PDC)
3. **Match sample rates:** Set OS sample rate = DAW sample rate

---

## Implementation Checklist

### Phase 1: Basic Audio I/O

- [ ] **Initialize JUCE AudioDeviceManager**
  ```cpp
  auto error = deviceManager.initialiseWithDefaultDevices(2, 2);
  ```

- [ ] **Enumerate Available Devices**
  - Get device types (ASIO, CoreAudio, WASAPI)
  - List input/output devices per type

- [ ] **Expose Buffer Size Control**
  - UI slider: 32, 64, 128, 256, 512, 1024 samples
  - Display calculated latency in ms

- [ ] **Audio Settings Dialog**
  - Driver type dropdown
  - Device selection
  - Sample rate selection
  - Buffer size slider
  - Test audio button

### Phase 2: Advanced Features

- [ ] **Automatic Device Selection**
  - Prefer ASIO (Windows) or CoreAudio (macOS)
  - Save last-used device to project

- [ ] **Device Hot-Plug Handling**
  - Detect device disconnect
  - Show error dialog
  - Auto-reconnect if available

- [ ] **Latency Compensation (PDC)**
  - Query plugin latency via `getLatencySamples()`
  - Delay tracks to align timing
  - Display total reported latency

- [ ] **CPU Meter**
  - Show current CPU usage %
  - Warn if approaching 80-90%
  - Red indicator if glitches occur

- [ ] **Buffer Underrun Detection**
  - Log xruns/dropouts
  - Display warning in UI
  - Suggest increasing buffer size

---

## Sources

### ASIO (Steinberg)
1. Steinberg ASIO Help Center: https://helpcenter.steinberg.de/hc/en-us/articles/17863730844946
2. Steinberg ASIO Forum: https://forums.steinberg.net/c/developer/asio/104
3. Steinberg Developers Portal: https://steinberg.net/developers/

### CoreAudio (Apple)
4. Core Audio Overview: https://developer.apple.com/library/archive/documentation/MusicAudio/Conceptual/CoreAudioOverview/
5. Core Audio Essentials: https://developer.apple.com/library/archive/documentation/MusicAudio/Conceptual/CoreAudioOverview/CoreAudioEssentials/CoreAudioEssentials.html
6. Apple Technical Q&A QA1467: https://developer.apple.com/library/archive/qa/qa1467/
7. Apple Support - Reducing Latency: https://support.apple.com/en-us/101938

### WASAPI (Microsoft)
8. WASAPI Overview: https://learn.microsoft.com/en-us/windows/win32/coreaudio/
9. Exclusive-Mode Streams: https://learn.microsoft.com/en-us/windows/win32/coreaudio/exclusive-mode-streams
10. Low Latency Audio: https://learn.microsoft.com/en-us/windows-hardware/drivers/audio/low-latency-audio
11. WASAPI Q&A (Microsoft): https://learn.microsoft.com/en-us/answers/questions/280467/

### Buffer Size Guidance
12. Sweetwater: Which Buffer Size Should I Use?: https://www.sweetwater.com/sweetcare/articles/which-buffer-size-setting-should-i-use-in-my-daw/
13. Orpheus Audio Academy: Buffer Size Explained: https://www.orpheusaudioacademy.com/buffer-size/
14. Sound on Sound: Optimising Latency: https://www.soundonsound.com/techniques/optimising-latency-pc-audio-interface

### Comparison & Best Practices
15. Gearspace: ASIO vs CoreAudio: https://gearspace.com/board/music-computers/1378825-what-delivers-lower-latency-core-audio-asio.html
16. FlexASIO: Real-World Testing: https://github.com/dechamps/FlexASIO/issues/153

---

**Document Version:** 1.0
**Last Updated:** 2025-11-10
**Next Steps:** Implement audio device management UI, test at 32/64-sample buffers
