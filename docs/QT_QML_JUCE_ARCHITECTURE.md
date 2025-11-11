# Qt/QML + JUCE Hybrid Architecture

## Executive Summary

This DAW project uses a **hybrid architecture** combining the strengths of two industry-standard frameworks:
- **JUCE** - Professional audio engine with VST/AU/AAX plugin support
- **Qt/QML** - Modern, fluid, hardware-accelerated UI framework

This approach is used by professional audio software companies and provides the optimal balance between audio performance and UI flexibility.

---

## Architecture Overview

```
┌───────────────────────────────────────────────────────────┐
│                   Qt/QML UI Layer                          │
│  ┌─────────────────────────────────────────────────────┐  │
│  │  • Animated waveform visualizations                 │  │
│  │  • Fluid track controls with smooth transitions     │  │
│  │  • Dynamic mixer with real-time level meters        │  │
│  │  • Hardware-accelerated timeline (OpenGL/Metal)     │  │
│  │  • Touch and gesture support                        │  │
│  └─────────────────────────────────────────────────────┘  │
└──────────────────────┬────────────────────────────────────┘
                       │
                       │ Qt Signals/Slots + IPC Bridge
                       │
┌──────────────────────▼────────────────────────────────────┐
│                 JUCE Audio Engine                          │
│  ┌─────────────────────────────────────────────────────┐  │
│  │  • Real-time audio processing (C++)                 │  │
│  │  • VST/VST3/AU/AAX plugin hosting                   │  │
│  │  • Low-latency MIDI I/O                             │  │
│  │  • Multi-threaded audio processing                  │  │
│  │  • Professional audio driver support (ASIO/CoreAudio)│ │
│  └─────────────────────────────────────────────────────┘  │
└───────────────────────────────────────────────────────────┘
```

---

## Why This Architecture?

### Commercial DAW Analysis

Research shows that **ZERO** major commercial DAWs use web-based frameworks (Electron/CEF):

| DAW | Technology Stack |
|-----|-----------------|
| Ableton Live | Custom C++ + Metal/OpenGL |
| FL Studio | Delphi (2.5M+ lines of code) |
| Cubase | C/C++ + custom UI |
| Pro Tools | Native C/C++ |
| Tracktion | JUCE framework |

### Why Not Electron/CEF?

1. ❌ **Real-time audio processing** - JavaScript garbage collection causes unpredictable latency
2. ❌ **CPU efficiency** - Chromium overhead is unacceptable for DSP
3. ❌ **Memory management** - Audio buffers need precise, deterministic control
4. ❌ **Hardware access** - Direct ASIO/CoreAudio drivers not accessible
5. ❌ **Plugin hosting** - VST/AU plugins require native C++ interfaces

### Why Qt/QML + JUCE?

#### Qt/QML Strengths:
- ✅ **Hardware-accelerated animations** (60+ FPS)
- ✅ **Declarative UI syntax** - Easy to create fluid interfaces
- ✅ **GPU-based rendering** - OpenGL/Vulkan/Metal
- ✅ **Modern design capabilities** - Material Design, custom themes
- ✅ **Cross-platform** - Windows, macOS, Linux, iOS, Android
- ✅ **Dynamic property bindings** - Automatic UI updates
- ✅ **Professional UI framework** - Used by automotive, industrial, enterprise

#### JUCE Strengths:
- ✅ **Audio-first design** - Built specifically for DAWs and plugins
- ✅ **Plugin formats** - VST, VST3, AU, AUv3, AAX, LV2, CLAP (JUCE 9)
- ✅ **Low-latency audio** - Professional audio driver support
- ✅ **Industry standard** - Used by Arturia, Focusrite, Korg, Tracktion
- ✅ **DSP library** - Comprehensive audio processing tools
- ✅ **Cross-platform** - Same codebase for all platforms

---

## Technical Implementation

### Component Communication

**Method 1: Qt Signals/Slots (Same Process)**
```cpp
// JUCE Audio Engine
class AudioEngine : public QObject {
    Q_OBJECT
    Q_PROPERTY(float masterVolume READ getMasterVolume WRITE setMasterVolume NOTIFY masterVolumeChanged)

signals:
    void masterVolumeChanged(float volume);
    void waveformDataReady(QVector<float> samples);
    void levelMeterUpdate(int trackId, float level);

public slots:
    void play() { jucePlayer.start(); }
    void stop() { jucePlayer.stop(); }
    void setTempo(double bpm);
};
```

**Method 2: IPC (Separate Processes)**
- Use Qt's `QLocalSocket` / `QLocalServer` for inter-process communication
- Serialize audio state as JSON or binary protocol
- Maintains process isolation for stability

### QML UI Examples

**Animated Volume Fader:**
```qml
import QtQuick 2.15
import QtQuick.Controls 2.15

Rectangle {
    id: volumeFader
    width: 60
    height: 400
    color: "#2a2a2a"

    // Smooth animated slider
    Slider {
        id: volumeSlider
        anchors.fill: parent
        orientation: Qt.Vertical
        from: 0.0
        to: 1.0
        value: audioEngine.trackVolume

        onValueChanged: {
            audioEngine.setTrackVolume(value)
        }

        // Custom handle with glow effect
        handle: Rectangle {
            width: 70
            height: 20
            radius: 10
            color: volumeSlider.pressed ? "#4CAF50" : "#8BC34A"

            // Smooth animation on interaction
            Behavior on color {
                ColorAnimation { duration: 150 }
            }
        }
    }

    // Real-time level meter
    Rectangle {
        id: levelMeter
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: 5
        width: 10
        height: audioEngine.levelMeter * parent.height
        color: Qt.rgba(0.3, 0.8, 0.3, 0.8)

        // Smooth peak animation
        Behavior on height {
            SmoothedAnimation { velocity: 500 }
        }
    }
}
```

**Waveform Visualization:**
```qml
import QtQuick 2.15

Canvas {
    id: waveformCanvas
    width: 800
    height: 200

    // GPU-accelerated rendering
    renderStrategy: Canvas.Threaded
    renderTarget: Canvas.FramebufferObject

    property var waveformData: audioEngine.waveformData

    onWaveformDataChanged: requestPaint()

    onPaint: {
        var ctx = getContext("2d")
        ctx.clearRect(0, 0, width, height)

        // Draw waveform with gradient
        var gradient = ctx.createLinearGradient(0, 0, 0, height)
        gradient.addColorStop(0, "#00BCD4")
        gradient.addColorStop(1, "#0288D1")

        ctx.strokeStyle = gradient
        ctx.lineWidth = 2
        ctx.beginPath()

        for (var i = 0; i < waveformData.length; i++) {
            var x = (i / waveformData.length) * width
            var y = (1 - waveformData[i]) * height / 2 + height / 2

            if (i === 0) ctx.moveTo(x, y)
            else ctx.lineTo(x, y)
        }

        ctx.stroke()
    }

    // Smooth zoom animation
    transform: Scale {
        id: zoomTransform
        origin.x: waveformCanvas.width / 2
        origin.y: waveformCanvas.height / 2

        Behavior on xScale {
            NumberAnimation { duration: 300; easing.type: Easing.OutQuad }
        }
    }
}
```

---

## Project Structure

```
/daw
├── src/
│   ├── qt-qml/                    # Qt/QML UI application
│   │   ├── main.cpp               # Qt application entry point
│   │   ├── qml/                   # QML UI files
│   │   │   ├── main.qml           # Main application window
│   │   │   ├── components/
│   │   │   │   ├── TrackView.qml
│   │   │   │   ├── MixerPanel.qml
│   │   │   │   ├── WaveformView.qml
│   │   │   │   ├── Timeline.qml
│   │   │   │   └── PluginRack.qml
│   │   │   └── styles/
│   │   │       └── AppTheme.qml
│   │   └── bridge/                # Qt/JUCE integration
│   │       ├── AudioEngineInterface.h
│   │       └── AudioEngineInterface.cpp
│   │
│   ├── juce-engine/               # JUCE audio engine
│   │   ├── Source/
│   │   │   ├── AudioEngine.h
│   │   │   ├── AudioEngine.cpp
│   │   │   ├── PluginHost.h
│   │   │   ├── PluginHost.cpp
│   │   │   ├── MidiProcessor.h
│   │   │   └── AudioProcessor.cpp
│   │   └── JuceLibraryCode/       # JUCE framework files
│   │
│   └── shared/                    # Shared data structures
│       ├── AudioState.h
│       └── ProjectData.h
│
├── CMakeLists.txt                 # Main CMake configuration
├── qt-app.pro                     # Qt project file
└── juce-engine.jucer              # JUCE Projucer file
```

---

## Build System

### Requirements

- **CMake** 3.21+
- **Qt 6.5+** with Qt Quick module
- **JUCE 8.0+**
- **Compiler**: MSVC 2022 (Windows), Clang 14+ (macOS), GCC 11+ (Linux)

### CMakeLists.txt (Root)

```cmake
cmake_minimum_required(VERSION 3.21)
project(VexelDAW VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt
find_package(Qt6 REQUIRED COMPONENTS Core Quick Widgets)
qt_standard_project_setup()

# Find JUCE
add_subdirectory(JUCE)

# Add JUCE audio engine library
add_subdirectory(src/juce-engine)

# Add Qt/QML application
add_subdirectory(src/qt-qml)
```

---

## Performance Characteristics

### UI Performance
- **60+ FPS** animations with GPU acceleration
- **Hardware-accelerated** waveform rendering (OpenGL/Metal/Vulkan)
- **Smooth transitions** with declarative animations
- **Touch-optimized** for tablets and touch screens

### Audio Performance
- **<5ms** round-trip latency with ASIO/CoreAudio
- **Multi-threaded** audio processing
- **Zero-allocation** audio callback (JUCE best practices)
- **Professional plugin hosting** with sandboxing

### Memory Footprint
- **~150MB** base application (vs ~300MB+ for Electron)
- **Efficient audio buffer management** (no JavaScript overhead)
- **Native code optimization** (SIMD, vectorization)

---

## Windows Audio API Support

### Comprehensive Driver Support

The Vexel DAW supports **ALL major Windows audio APIs** through JUCE, providing maximum compatibility and flexibility:

| API | Latency | Status | Use Case |
|-----|---------|--------|----------|
| **ASIO** | 1-10ms | ⭐ Best | Professional audio interfaces |
| **WASAPI** | 10-30ms | ✅ Recommended | Modern Windows (Vista+) |
| **DirectSound** | 50-80ms | ⚠️ Legacy | Compatibility mode |
| **MME** | 100-200ms+ | ❌ Last Resort | Maximum compatibility |

### Auto-Detection Strategy

```cpp
// Automatic API selection in order of preference:
1. ASIO       // If professional driver installed
2. WASAPI     // Default for Windows Vista+
3. DirectSound // Fallback for older systems
4. MME        // Last resort (maximum compatibility)
```

### Why All APIs?

**ASIO:**
- Used by professional audio interfaces (Focusrite, RME, UA, MOTU)
- Lowest latency (1-10ms) for real-time recording
- Industry standard for pro audio

**WASAPI:**
- Built into Windows Vista+ (no driver needed)
- Excellent latency in Exclusive Mode (10-15ms)
- **Recommended default** when ASIO unavailable

**DirectSound:**
- Legacy Windows API (Windows 95+)
- Higher latency (50-80ms) but wide compatibility
- Fallback for systems without WASAPI

**MME:**
- Original Windows audio API (Windows 3.1+)
- Very high latency (100-200ms+)
- **Only for maximum compatibility**

### Implementation

```cpp
// JUCE automatically handles all Windows audio APIs
audioEngine.setAudioDeviceType("ASIO");     // Professional
audioEngine.setAudioDeviceType("WASAPI");   // Modern Windows
audioEngine.setAudioDeviceType("DirectSound"); // Legacy
audioEngine.setAudioDeviceType("MME");      // Compatibility
```

### Documentation

See **[WINDOWS_AUDIO_APIS_GUIDE.md](./WINDOWS_AUDIO_APIS_GUIDE.md)** for complete details including:
- Detailed latency comparisons
- Buffer size recommendations
- Driver installation guides
- Troubleshooting tips
- JUCE implementation examples

---

## Development Workflow

### 1. UI Development (QML)
```bash
# Hot-reload QML during development
qml -I ./src/qt-qml/qml main.qml
```

### 2. Audio Engine Development (JUCE)
```bash
# Build with Projucer or CMake
cmake --build build --target juce-engine
```

### 3. Integration Testing
```bash
# Build complete application
cmake --build build --target VexelDAW
./build/VexelDAW
```

---

## Migration Strategy

See [MIGRATION_FROM_ELECTRON.md](./MIGRATION_FROM_ELECTRON.md) for detailed migration steps.

---

## Resources

### Qt/QML Documentation
- [Qt Quick Best Practices](https://doc.qt.io/qt-6/qtquick-bestpractices.html)
- [QML Animation Tutorial](https://doc.qt.io/qt-6/qtquick-statesanimations-animations.html)
- [Qt for Audio Applications](https://www.qt.io/blog/juce-x-qt)

### JUCE Documentation
- [JUCE Framework](https://juce.com/)
- [JUCE Tutorials](https://docs.juce.com/master/tutorial_getting_started_juce.html)
- [Audio Plugin Development](https://docs.juce.com/master/tutorial_create_projucer_basic_plugin.html)

### Example Projects
- [Tracktion Engine](https://github.com/Tracktion/tracktion_engine) - Open source DAW engine
- [Qt/JUCE Integration Example](https://www.qt.io/blog/juce-x-qt)

---

## License Considerations

### JUCE Licensing
- **GPL v3** - Free for open-source projects
- **Commercial License** - Required for commercial closed-source products ($799-$1499)

### Qt Licensing
- **LGPL v3** - Free for open-source and commercial projects (with dynamic linking)
- **Commercial License** - Required for static linking or proprietary modifications

---

## Conclusion

This hybrid architecture provides:
- ✅ **Professional audio performance** matching commercial DAWs
- ✅ **Modern, fluid UI** with hardware acceleration
- ✅ **Industry-standard approach** used by professional audio companies
- ✅ **Best developer experience** - QML for UI, C++ for audio
- ✅ **Future-proof** - Active development of both frameworks

This is the **optimal technical foundation** for a professional-grade DAW application.
