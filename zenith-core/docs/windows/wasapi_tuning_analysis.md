# WASAPI Tuning Surface - Technical Analysis

**Target:** Windows-only audio device configuration for Zenith DAW
**JUCE Version:** 8.0.9
**Date:** 2025-11-12

---

## 1. Device Enumeration via JUCE AudioDeviceManager

### API Summary

```cpp
// Get available device types (WASAPI, ASIO, DirectSound)
AudioDeviceManager::getAvailableDeviceTypes() -> OwnedArray<AudioIODeviceType>*

// For each device type:
deviceType->scanForDevices()                    // Must call first
deviceType->getDeviceNames(true)                // Output devices
deviceType->getDeviceNames(false)               // Input devices
deviceType->getTypeName()                       // "Windows Audio", "ASIO", etc.

// Open a device to query its capabilities:
AudioIODevice* device = deviceType->createDevice(outputName, inputName)
device->getAvailableSampleRates()               // Array<double>
device->getAvailableBufferSizes()               // Array<int>
device->getDefaultBufferSize()                  // int
```

### Device Types on Windows

1. **"Windows Audio"** (WASAPI)
   - Default on Windows Vista+
   - Supports both Shared and Exclusive modes
   - Lower latency than legacy DirectSound

2. **"ASIO"** (if installed)
   - Third-party driver (RME, Focusrite, Universal ASIO, etc.)
   - Lowest latency (sub-5ms possible)
   - Requires separate driver installation

3. **"DirectSound"** (legacy)
   - High latency (>50ms typical)
   - Not recommended for DAW use

---

## 2. WASAPI Modes: Shared vs Exclusive

### Shared Mode (Default)

**Characteristics:**
- Audio mixer shared with other applications
- Windows resamples to system default (typically 48 kHz)
- Higher CPU overhead due to mixing layer
- Typical latency: **10-20ms** (480-960 samples @ 48 kHz)
- **Minimum latency:** ~3ms on Windows 10+ with low-latency mode

**Buffer Sizes (JUCE-reported):**
- Multiples of 32: 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 480, 512
- Multiples of 64: 576, 640, 704, 768, 832, 896, 960, 1024
- Multiples of 128: 1152, 1280, 1408, 1536, ... (up to ~4096)

**When to use:**
- Consumer audio devices (laptop speakers, USB headphones)
- Testing/debugging (other apps can play audio simultaneously)
- Users unfamiliar with audio configuration

### Exclusive Mode

**Characteristics:**
- Direct hardware access (no Windows mixer)
- Application controls sample rate (no forced resampling)
- Lower CPU overhead (no mixing layer)
- Typical latency: **5-10ms** (256-512 samples @ 48 kHz)
- **Minimum latency:** ~1.5ms (64 samples @ 48 kHz) on pro interfaces
- Requires device restart when switching applications

**Buffer Sizes (hardware-dependent):**
- Professional interfaces: 32, 64, 128, 256, 512, 1024, 2048
- Consumer devices: Often only support 480+ samples

**When to use:**
- Professional audio interfaces (RME, Focusrite Scarlett, Universal Audio)
- Sub-10ms latency required (live monitoring, virtual instruments)
- Fixed sample rate workflows (film: 48 kHz, music: 44.1/48 kHz)

**Caveats:**
- Other applications cannot play audio while DAW is using the device
- Some consumer devices don't support exclusive mode
- JUCE doesn't expose shared/exclusive selection directly (uses system policy)

### Detection in JUCE

**JUCE limitation:** The `AudioDeviceManager` does not expose a `setExclusiveMode()` API. WASAPI mode selection is handled internally based on:
- System policy (Windows audio service decides)
- Device capabilities (some devices only support shared)
- Buffer size requested (very low buffer sizes may force exclusive)

**Workaround for detection:**
```cpp
// Indirect detection (not foolproof):
// 1. Check if very low buffer sizes (32, 64) are available
//    → Likely exclusive mode capable
// 2. Check if device supports sample rates other than system default
//    → Likely exclusive mode capable
// 3. Query device type: ASIO is always exclusive
```

**Recommendation:**
For now, display "Mode: Shared/Exclusive (automatic)" and note that Windows selects the mode based on buffer size and device capabilities. Expose explicit control in a future step if needed (via JUCE fork or direct WASAPI wrapper).

---

## 3. Sample Rates

### Common Sample Rates (Windows/WASAPI)

| Rate (Hz) | Use Case                     | Availability       |
|-----------|------------------------------|--------------------|
| 44100     | CD audio, music production   | Near-universal     |
| 48000     | Film/video, game audio       | **Universal**      |
| 88200     | High-res music (2x CD)       | Pro interfaces     |
| 96000     | Film post-production         | Pro interfaces     |
| 176400    | Archival/mastering (4x CD)   | Pro interfaces     |
| 192000    | Film, classical mastering    | Pro interfaces     |
| 22050     | Legacy games, telephony      | Rare               |
| 32000     | Broadcast                    | Rare               |

**System default:** Most Windows systems default to **48 kHz** in shared mode.

### Recommendations

**Sane defaults:**
- **48 kHz:** Best compatibility, lowest CPU (1:1 with video frame rates)
- **44.1 kHz:** Music-focused workflows (CD release)
- **96 kHz:** Film post-production

**Fallback rules:**
1. Try 48 kHz first (most compatible)
2. If unavailable, try 44.1 kHz
3. If unavailable, use device's default (query via `device->getCurrentSampleRate()`)

**UI behavior:**
- Populate dropdown with `device->getAvailableSampleRates()`
- Pre-select 48 kHz if available
- Display warning if system default doesn't match selected rate (shared mode limitation)

---

## 4. Buffer Sizes

### WASAPI Buffer Size Patterns (JUCE)

**Shared mode:**
- Multiples of 32 up to 512
- Multiples of 64 from 512 to 1024
- Multiples of 128 from 1024 onwards
- Default: **480 samples** (~10ms @ 48 kHz)

**Exclusive mode (pro interfaces):**
- Powers of 2: 32, 64, 128, 256, 512, 1024, 2048
- Some devices: 48, 96, 192 (multiples of 48 @ 48 kHz)

### Latency Calculation

```
Latency (ms) = (Buffer Size / Sample Rate) × 1000
Round-trip = Input Latency + Processing + Output Latency
           ≈ 2 × Buffer Latency (assuming symmetric I/O)
```

**Examples @ 48 kHz:**
| Buffer Size | One-Way Latency | Round-Trip Latency |
|-------------|-----------------|---------------------|
| 32          | 0.67 ms         | 1.3 ms              |
| 64          | 1.33 ms         | 2.7 ms              |
| 128         | 2.67 ms         | 5.3 ms              |
| 256         | 5.33 ms         | 10.7 ms             |
| 480         | 10.0 ms         | 20.0 ms             |
| 512         | 10.67 ms        | 21.3 ms             |
| 1024        | 21.33 ms        | 42.7 ms             |

### Recommendations

**Low-latency targets (Exclusive mode):**
- **64-128 samples:** Sub-5ms latency for virtual instruments, live monitoring
- **128-256 samples:** Good balance for most music production
- **Requires:** Modern CPU (Intel i7/Ryzen 7+), professional interface

**Shared mode targets:**
- **128-256 samples:** Best effort low latency (Windows 10+ low-latency mode)
- **480-512 samples:** Safe default for consumer devices
- **1024+ samples:** Mixing, non-real-time tasks

**Fallback rules:**
1. Try 128 samples first (good balance)
2. If unavailable or causing dropouts, try 256
3. If still unstable, fall back to device default (query via `device->getDefaultBufferSize()`)
4. Display CPU usage indicator; if > 80%, suggest increasing buffer size

**Min/Max/Default:**
- **Min:** JUCE exposes via `getAvailableBufferSizes()[0]`
- **Max:** JUCE exposes via `getAvailableBufferSizes()[last]`
- **Default:** Query via `device->getDefaultBufferSize()` (typically 480 for WASAPI)

**Note:** JUCE does not expose min/max directly - must iterate `getAvailableBufferSizes()` array.

---

## 5. Recommended Defaults for Zenith DAW

### Initial Device Setup

```cpp
// Priority order:
1. ASIO (if available)          → Lowest latency
2. Windows Audio (WASAPI)       → Modern, low-latency capable
3. DirectSound (avoid)          → Legacy, high latency

// Sample Rate: 48000 Hz (most compatible)
// Buffer Size:
//   - ASIO: 128 samples
//   - WASAPI: 256 samples (shared mode safe default)
```

### Adaptive Selection

```cpp
if (device->getAvailableBufferSizes().contains(128)) {
    // Assume exclusive-capable or ASIO
    defaultBufferSize = 128;
} else if (device->getAvailableBufferSizes().contains(256)) {
    // Shared mode low-latency
    defaultBufferSize = 256;
} else {
    // Fallback to device default
    defaultBufferSize = device->getDefaultBufferSize();
}
```

---

## 6. ASIO Support

### Detection

```cpp
// Check if ASIO device type exists:
for (auto* deviceType : deviceManager.getAvailableDeviceTypes()) {
    if (deviceType->getTypeName() == "ASIO") {
        // ASIO available
        asioDevices = deviceType->getDeviceNames(true);
    }
}
```

### ASIO Control Panel

**JUCE API:**
```cpp
AudioIODevice* device = deviceManager.getCurrentAudioDevice();
if (device->getTypeName() == "ASIO") {
    device->showControlPanel();  // Opens native ASIO control panel
}
```

**UI Behavior:**
- Show "Open ASIO Panel…" button only when ASIO device is selected
- Button opens native driver settings (sample rate, buffer size, routing)
- Gray out Zenith's sample rate/buffer size dropdowns (ASIO driver controls these)

---

## 7. Known Issues & Limitations

### JUCE Limitations

1. **No explicit exclusive mode control**
   → Windows/WASAPI decides based on system policy

2. **No mismatched sample rate support**
   → Input and output must use same rate (JUCE limitation)

3. **Buffer size changes require device restart**
   → Must call `deviceManager.closeAudioDevice()` then `setAudioDeviceSetup()`

4. **Some devices report incorrect buffer sizes**
   → Always validate actual latency with test signals

### Windows WASAPI Issues

1. **Shared mode forced resampling**
   → If system default is 48 kHz and app requests 44.1 kHz, Windows resamples in shared mode

2. **Consumer devices often don't support exclusive mode**
   → Laptop speakers, USB headphones may only support shared mode with high latencies

3. **Windows Audio service can override settings**
   → "Allow applications to take exclusive control" must be enabled in Windows Sound settings

---

## 8. Testing Recommendations

### Basic Functionality

- [ ] Device dropdown populates with all available outputs
- [ ] Sample rate dropdown shows only supported rates for selected device
- [ ] Buffer size dropdown shows only supported sizes for selected device
- [ ] Changing device updates sample rate/buffer size options
- [ ] Status line displays current mode (Shared/Exclusive detection heuristic)

### Audio Validation

- [ ] Changing buffer size stops/restarts audio device without crashes
- [ ] Changing sample rate stops/restarts audio device without crashes
- [ ] Test tone plays after device change (validate audio path)
- [ ] CPU usage updates in real-time
- [ ] No dropouts when buffer size is adequate for CPU load

### Edge Cases

- [ ] No devices available (display "No audio devices found")
- [ ] Device removed while in use (handle gracefully, fall back to default)
- [ ] ASIO and WASAPI coexist (prioritize ASIO if available)
- [ ] Very low buffer sizes (32, 64) → may cause dropouts, display warning

### ASIO-Specific

- [ ] "Open ASIO Panel…" button appears only for ASIO devices
- [ ] ASIO panel opens native control window
- [ ] Zenith respects ASIO driver settings (don't override sample rate/buffer size)

---

## 9. Future Enhancements (Post-W3)

1. **Explicit exclusive mode toggle**
   → Requires JUCE fork or direct WASAPI API integration

2. **Input/output device mismatching**
   → Allow separate input/output devices (currently JUCE limits this)

3. **Multi-client ASIO**
   → Support ASIO devices with multiple concurrent clients (rare)

4. **Device hotplug handling**
   → Auto-switch to new device when USB interface plugged in

5. **Latency compensation**
   → Measure round-trip latency with loopback test

6. **Per-device presets**
   → Save preferred sample rate/buffer size per device

---

**END OF ANALYSIS**
