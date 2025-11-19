# Windows Audio APIs Guide for DAW Development

## Overview

This document provides a comprehensive guide to Windows audio APIs supported by the Zenith DAW, helping developers choose the right audio driver for different scenarios.

---

## Supported Windows Audio APIs

The Zenith DAW supports **ALL major Windows audio APIs** through the JUCE framework:

| API | Introduced | Status | Latency | Use Case |
|-----|-----------|--------|---------|----------|
| **ASIO** | 1997 | Active | 1-10ms | ⭐ **Professional audio** |
| **WASAPI** | 2007 (Vista) | Active | 10-30ms | **Modern Windows default** |
| **DirectSound** | 1995 | Legacy | 50-80ms | **Legacy compatibility** |
| **MME** | 1991 | Deprecated | 100-200ms | **Maximum compatibility** |

---

## 1. ASIO (Audio Stream Input/Output)

### Overview
- **Developer:** Steinberg Media Technologies
- **Year:** 1997
- **Status:** Industry standard for professional audio
- **Platform:** Windows only (third-party drivers)

### Performance
- **Latency:** 1-10ms (excellent)
- **Quality:** Professional-grade
- **CPU Usage:** Low
- **Buffer sizes:** 32-2048 samples

### Advantages
- ✅ **Lowest latency** - Direct hardware communication
- ✅ **Professional standard** - Used by all pro audio software
- ✅ **Multi-client** - Multiple apps can share the device (ASIO4ALL)
- ✅ **High precision** - Sample-accurate timing

### Disadvantages
- ❌ **Requires dedicated driver** - Not built into Windows
- ❌ **Limited availability** - Only professional audio interfaces
- ❌ **Single-client** - Native ASIO is exclusive (one app at a time)

### When to Use
- ✅ Real-time recording and monitoring
- ✅ Professional audio production
- ✅ Live performance
- ✅ Minimum latency required

### JUCE Implementation
```cpp
// Set ASIO as the audio device type
audioEngine.setAudioDeviceType("ASIO");

// Get available ASIO devices
auto devices = audioEngine.getAvailableDevices("ASIO");

// Select specific ASIO device
audioEngine.setAudioDevice("Focusrite USB ASIO");
```

### Driver Availability
- **Professional audio interfaces:** Focusrite, RME, Universal Audio, MOTU, etc.
- **Consumer adapters:** ASIO4ALL (universal ASIO driver for any audio device)
- **Virtual:** FL Studio ASIO, Voicemeeter ASIO

---

## 2. WASAPI (Windows Audio Session API)

### Overview
- **Developer:** Microsoft
- **Year:** 2007 (Windows Vista)
- **Status:** Modern Windows standard
- **Platform:** Windows Vista and later

### Performance
- **Latency:** 10-30ms (good)
- **Quality:** High
- **CPU Usage:** Low-Medium
- **Buffer sizes:** 128-2048 samples

### Modes

#### Shared Mode (Default)
- Multiple applications can use audio simultaneously
- Windows audio mixer processes all audio
- Higher latency (~30ms)

#### Exclusive Mode
- Application has direct control of audio device
- Bypasses Windows audio mixer
- Lower latency (~10-15ms)
- Similar to ASIO, but no dedicated driver needed

### Advantages
- ✅ **Built into Windows** - No driver installation needed
- ✅ **Good latency** - Especially in exclusive mode
- ✅ **Modern API** - Well-designed and maintained
- ✅ **High sample rates** - Supports 192kHz+
- ✅ **Wide compatibility** - Works with all audio devices

### Disadvantages
- ❌ **Not as low-latency as ASIO** - 10-30ms vs 1-10ms
- ❌ **Exclusive mode limitations** - Blocks other apps
- ❌ **Windows Vista+** - Not available on XP

### When to Use
- ✅ **Default choice** when ASIO unavailable
- ✅ Modern Windows systems (Vista+)
- ✅ Consumer audio devices without ASIO drivers
- ✅ Acceptable latency (~10-15ms)
- ✅ High-quality playback and recording

### JUCE Implementation
```cpp
// Set WASAPI as the audio device type
audioEngine.setAudioDeviceType("WASAPI");

// WASAPI is the default on modern Windows if ASIO unavailable
auto types = audioEngine.getAvailableDeviceTypes();
// Returns: ["ASIO", "WASAPI", "DirectSound", "MME"]

// JUCE handles exclusive mode automatically when possible
```

### Recommendation
**WASAPI should be the default fallback** when ASIO drivers are not available. It provides excellent performance on modern Windows systems.

---

## 3. DirectSound

### Overview
- **Developer:** Microsoft
- **Year:** 1995 (DirectX 1.0)
- **Status:** Legacy (still supported for compatibility)
- **Platform:** Windows 95 and later

### Performance
- **Latency:** 50-80ms (acceptable)
- **Quality:** Good
- **CPU Usage:** Medium
- **Buffer sizes:** 512-4096 samples

### Advantages
- ✅ **Wide compatibility** - Windows 95+
- ✅ **Hardware acceleration** - On older systems
- ✅ **3D audio support** - Positional audio
- ✅ **Multi-application** - Shared mode

### Disadvantages
- ❌ **High latency** - 50-80ms minimum
- ❌ **Deprecated** - Microsoft stopped active development
- ❌ **Poor for real-time** - Latency too high for recording
- ❌ **Windows 8+ emulation** - Not native anymore

### When to Use
- ⚠️ **Legacy compatibility** only
- ⚠️ Older Windows systems (XP, Vista, 7)
- ⚠️ Non-real-time playback
- ⚠️ When WASAPI/ASIO unavailable

### JUCE Implementation
```cpp
// Set DirectSound (not recommended for new projects)
audioEngine.setAudioDeviceType("DirectSound");
```

### Recommendation
**Avoid DirectSound** unless you need to support very old systems. WASAPI is superior in every way on modern Windows.

---

## 4. MME (Multimedia Extensions)

### Overview
- **Developer:** Microsoft
- **Year:** 1991 (Windows 3.1)
- **Status:** Legacy/deprecated (still supported for compatibility)
- **Platform:** All Windows versions

### Performance
- **Latency:** 100-200ms+ (poor)
- **Quality:** Basic
- **CPU Usage:** High
- **Buffer sizes:** 2048+ samples

### Advantages
- ✅ **Universal compatibility** - Works everywhere
- ✅ **Simplest API** - Easy to use
- ✅ **Multi-application** - Always shared

### Disadvantages
- ❌ **Extremely high latency** - 100-200ms+
- ❌ **Poor quality** - Limited features
- ❌ **Outdated** - 30+ year old technology
- ❌ **CPU inefficient** - Software mixing

### When to Use
- ⚠️ **Absolute last resort** - Maximum compatibility
- ⚠️ Very old systems
- ⚠️ Simple playback (no real-time requirements)
- ⚠️ Testing/debugging

### JUCE Implementation
```cpp
// Set MME (last resort only)
audioEngine.setAudioDeviceType("MME");
```

### Recommendation
**Avoid MME** except for testing maximum compatibility. Even DirectSound is better.

---

## Automatic Driver Selection Strategy

### Recommended Fallback Order

```cpp
// Priority order for automatic selection
std::vector<std::string> preferredOrder = {
    "ASIO",        // 1st choice: Best latency
    "WASAPI",      // 2nd choice: Good latency, built-in
    "DirectSound", // 3rd choice: Acceptable for playback
    "MME"          // Last resort: Maximum compatibility
};

// Try each in order
for (const auto& type : preferredOrder) {
    if (audioEngine.setAudioDeviceType(type)) {
        std::cout << "Using audio API: " << type << std::endl;
        break;
    }
}
```

### Smart Selection Logic

```cpp
bool AudioEngine::initializeAudio() {
    // 1. Check for ASIO driver first (professional users)
    auto asioDevices = getAvailableDevices("ASIO");
    if (!asioDevices.empty()) {
        std::cout << "ASIO driver detected - using professional audio" << std::endl;
        return setAudioDeviceType("ASIO");
    }

    // 2. Check Windows version for WASAPI
    #ifdef _WIN32
    if (isWindowsVistaOrLater()) {
        std::cout << "Modern Windows detected - using WASAPI" << std::endl;
        return setAudioDeviceType("WASAPI");
    }
    #endif

    // 3. Fall back to DirectSound (XP, Vista, 7 without ASIO)
    std::cout << "Using DirectSound for compatibility" << std::endl;
    return setAudioDeviceType("DirectSound");

    // 4. MME as last resort
    // (automatically used by JUCE if others fail)
}
```

---

## Latency Comparison Chart

```
API           Min Latency    Typical Latency    Max Latency
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
ASIO          1ms            5-10ms             20ms
WASAPI Excl.  8ms            10-15ms            30ms
WASAPI Shared 20ms           30-50ms            100ms
DirectSound   40ms           50-80ms            150ms
MME           80ms           100-200ms          500ms+
```

---

## Buffer Size Recommendations

### ASIO
- **32 samples** - 0.7ms @ 48kHz - Professionals only
- **64 samples** - 1.3ms @ 48kHz - Low latency recording
- **128 samples** - 2.7ms @ 48kHz - **Recommended default**
- **256 samples** - 5.3ms @ 48kHz - Stable on most systems
- **512 samples** - 10.7ms @ 48kHz - Maximum compatibility

### WASAPI
- **128 samples** - Minimum for exclusive mode
- **256 samples** - **Recommended** for exclusive mode
- **512 samples** - Shared mode typical
- **1024 samples** - Safe default

### DirectSound / MME
- **512 samples** - Minimum practical
- **1024 samples** - Typical
- **2048+ samples** - Expected for these APIs

---

## Platform-Specific Defaults

```cpp
std::string AudioEngine::getDefaultAudioType() {
    #ifdef _WIN32
        // Windows: Prefer ASIO, fallback to WASAPI
        auto asioDevices = getAvailableDevices("ASIO");
        if (!asioDevices.empty()) {
            return "ASIO";
        }
        return "WASAPI";  // Default for modern Windows

    #elif __APPLE__
        return "CoreAudio";  // macOS only has CoreAudio

    #elif __linux__
        // Linux: Check for JACK, otherwise ALSA
        if (isJackRunning()) {
            return "JACK";
        }
        return "ALSA";

    #else
        return "Unknown";
    #endif
}
```

---

## User Selection UI

### Settings Panel Example

```cpp
// In your audio settings UI:
QComboBox* deviceTypeCombo = new QComboBox();

// Populate with available types
auto types = audioEngine.getAvailableDeviceTypes();
for (const auto& type : types) {
    QString displayName = formatDeviceTypeName(type);
    deviceTypeCombo->addItem(displayName, QString::fromStdString(type));
}

// Helper function for user-friendly names
QString formatDeviceTypeName(const std::string& type) {
    if (type == "ASIO") return "ASIO (Professional - Lowest Latency)";
    if (type == "WASAPI") return "WASAPI (Recommended for Modern Windows)";
    if (type == "DirectSound") return "DirectSound (Legacy Compatibility)";
    if (type == "MME") return "MME (Maximum Compatibility)";
    return QString::fromStdString(type);
}
```

---

## Troubleshooting

### "No ASIO Drivers Found"
**Solution:**
1. Install ASIO driver for your audio interface
2. Or install ASIO4ALL for generic ASIO support
3. Fall back to WASAPI (still excellent performance)

### "WASAPI Exclusive Mode Fails"
**Solution:**
1. Close other applications using audio
2. Disable Windows audio enhancements
3. Use WASAPI Shared mode
4. Fall back to DirectSound

### "Crackling/Dropouts with Low Latency"
**Solution:**
1. Increase buffer size (128 → 256 → 512)
2. Close background applications
3. Use ASIO instead of WASAPI
4. Check CPU usage

### "High Latency Even with ASIO"
**Solution:**
1. Reduce buffer size in driver settings
2. Check ASIO driver configuration
3. Update audio interface firmware
4. Disable Windows power management

---

## JUCE Integration Examples

### Complete Audio Setup

```cpp
// When JUCE is integrated:
class AudioEngine {
private:
    std::unique_ptr<juce::AudioDeviceManager> deviceManager;

public:
    bool initialize() {
        deviceManager = std::make_unique<juce::AudioDeviceManager>();

        // Set up audio device types
        deviceManager->createAvailableDeviceTypes();

        // Try to initialize with ASIO first
        auto types = deviceManager->getAvailableDeviceTypes();
        juce::AudioIODeviceType* asioType = nullptr;

        for (auto* type : types) {
            if (type->getTypeName() == "ASIO") {
                asioType = type;
                break;
            }
        }

        if (asioType) {
            // Initialize with ASIO
            juce::String error = deviceManager->initialise(
                2,      // numInputChannels
                2,      // numOutputChannels
                nullptr, // savedState
                true,    // selectDefaultDeviceOnFailure
                "ASIO"   // preferredDeviceTypeName
            );

            if (error.isEmpty()) {
                std::cout << "ASIO initialized successfully" << std::endl;
                return true;
            }
        }

        // Fall back to WASAPI
        juce::String error = deviceManager->initialise(2, 2, nullptr, true);
        if (error.isEmpty()) {
            std::cout << "Audio initialized (WASAPI fallback)" << std::endl;
            return true;
        }

        std::cerr << "Audio initialization failed: " << error << std::endl;
        return false;
    }
};
```

---

## Conclusion

### Recommendations Summary

**For Professional DAW:**
1. **Primary:** ASIO (1-10ms latency)
2. **Secondary:** WASAPI Exclusive (10-15ms latency)
3. **Tertiary:** WASAPI Shared (30ms latency)
4. **Avoid:** DirectSound, MME (unless compatibility required)

**Default Configuration:**
- **Windows 10/11:** WASAPI
- **Windows with ASIO interface:** ASIO
- **Windows 7/8:** WASAPI or DirectSound
- **Windows XP:** DirectSound

**User Message:**
"For best performance, install ASIO drivers for your audio interface. WASAPI provides excellent performance on modern Windows systems."

---

## References

- [JUCE AudioDeviceManager Documentation](https://docs.juce.com/master/classAudioDeviceManager.html)
- [JUCE Tutorial: Audio Device Manager](https://docs.juce.com/master/tutorial_audio_device_manager.html)
- [Windows Audio APIs History](http://shanekirk.com/2015/10/a-brief-history-of-windows-audio-apis/)
- [ASIO SDK by Steinberg](https://www.steinberg.net/developers/)
- [Microsoft WASAPI Documentation](https://docs.microsoft.com/en-us/windows/win32/coreaudio/wasapi)

---

**Last Updated:** 2025-11-10
**Version:** 1.0
