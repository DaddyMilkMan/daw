# Zenith DAW Skia UI Developer Guide

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Integration Guide](#integration-guide)
3. [Creating New Skia Components](#creating-new-skia-components)
4. [Common Patterns and Examples](#common-patterns-and-examples)
5. [Troubleshooting](#troubleshooting)
6. [Performance Optimization](#performance-optimization)

---

## Architecture Overview

### System Design

The Zenith DAW Skia UI system provides GPU-accelerated rendering with sophisticated spring physics animations. The architecture is organized into several layers:

```
┌─────────────────────────────────────────────────┐
│   Application Layer (MainWindow, Components)    │
├─────────────────────────────────────────────────┤
│   SkiaMainWindowIntegration                     │
│   ├─ Animation Controller (Spring Physics)     │
│   ├─ Theme System (Colors, Effects)            │
│   └─ Settings Manager (Persistence)            │
├─────────────────────────────────────────────────┤
│   UI Components (Skia-rendered)                │
│   ├─ Mixer Channels                            │
│   ├─ Transport Controls                        │
│   ├─ Meters                                    │
│   ├─ Effects Chain                             │
│   └─ Browser Panels                            │
├─────────────────────────────────────────────────┤
│   Skia Rendering System                        │
│   ├─ SkiaTheme (Colors, Effects, Physics)     │
│   ├─ SkiaTextRenderer (Text Effects)          │
│   ├─ SkiaWaveformRenderer (Audio Vis.)        │
│   ├─ SkiaPianoRollRenderer (MIDI Vis.)        │
│   └─ SkiaClipRenderer (Timeline Clips)        │
├─────────────────────────────────────────────────┤
│   Skia Graphics Library                        │
│   └─ Direct3D 12 Backend (Windows)             │
└─────────────────────────────────────────────────┘
```

### Key Components

#### SkiaTheme
Central theme system providing:
- Dark/Light color palettes (Apple-inspired)
- Spring physics presets per component type
- Depth style configurations (subtle/moderate/dramatic)
- Typography settings with text effects
- GPU rendering optimizations

#### SkiaAnimationController
Manages spring-physics-based animations:
- RK4 integration for smooth motion
- Per-component animation state
- Automatic FPS adaptation
- Automatic settling detection

#### SkiaMainWindowIntegration
High-level integration point:
- Theme switching with animation
- Animation loop coordination
- FPS monitoring and adaptation
- GPU resource management

#### SkiaSettingsManager
Persistent user preferences:
- Theme mode persistence
- Physics customization
- GPU settings
- Automatic save/load from configuration files

---

## Integration Guide

### Step 1: Enable Skia in CMake

```cmake
cmake -DZENITH_ENABLE_SKIA=ON ..
```

This enables all Skia components for compilation with conditional includes.

### Step 2: Initialize Skia System in MainWindow

In `MainWindow::MainWindow()`:

```cpp
// Create Skia integration
skiaIntegration_ = std::make_unique<zenith::SkiaMainWindowIntegration>();
skiaIntegration_->initializeSkiaRendering();

// Load saved user preferences
auto& settingsManager = zenith::SkiaSettingsManager::getInstance();
settingsManager.loadSettings();

// Apply loaded theme
auto& theme = zenith::SkiaTheme::getInstance();
theme.setThemeMode(settingsManager.getThemeMode());
```

### Step 3: Add Skia Components to UI

```cpp
// Create a mixer channel component
auto mixerChannel = std::make_unique<zenith::SkiaMixerChannelComponent>();
mixerChannel->setChannelName("Channel 1");
addAndMakeVisible(mixerChannel.get());

// Animate the fader with spring physics
skiaIntegration_->animateComponent(
    "channel1_fader",
    0.7f,
    zenith::SpringPhysicsSettings::smooth()
);
```

### Step 4: Implement Paint Callback

```cpp
void MyComponent::paint(juce::Graphics& g)
{
    // Use theme colors
    const auto& colors = zenith::SkiaTheme::getInstance().getColors();

    g.fillAll(juce::Colour(
        colors.background >> 16 & 0xFF,
        colors.background >> 8 & 0xFF,
        colors.background & 0xFF
    ));

    // Use flashy text effects
    zenith::SkiaTextRenderer textRenderer;
    auto options = textRenderer.flashy();
    options.color = colors.textPrimary;
    // Text rendering logic...
}
```

---

## Creating New Skia Components

### Template for New Component

Create `SkiaNewComponentComponent.h`:

```cpp
#pragma once

#include <JuceHeader.h>

#ifdef ZENITH_USE_SKIA
    #include <include/core/SkCanvas.h>
#endif

#include "SkiaTheme.h"

namespace zenith {

#ifdef ZENITH_USE_SKIA

class SkiaNewComponentComponent : public juce::Component, private juce::Timer
{
public:
    SkiaNewComponentComponent();
    ~SkiaNewComponentComponent() override;

    // Value accessors
    void setValue(float value);
    float getValue() const { return value_; }

    // Component interface
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;

private:
    void timerCallback() override;

    float value_ = 0.5f;
    float targetValue_ = 0.5f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaNewComponentComponent)
};

#endif

} // namespace zenith
```

Create `SkiaNewComponentComponent.cpp`:

```cpp
#include "SkiaNewComponentComponent.h"

#ifdef ZENITH_USE_SKIA

namespace zenith {

SkiaNewComponentComponent::SkiaNewComponentComponent()
{
    setSize(100, 100);
    startTimer(16); // ~60 FPS
}

SkiaNewComponentComponent::~SkiaNewComponentComponent()
{
    stopTimer();
}

void SkiaNewComponentComponent::setValue(float value)
{
    targetValue_ = juce::jlimit(0.0f, 1.0f, value);
}

void SkiaNewComponentComponent::paint(juce::Graphics& g)
{
    const auto& colors = zenith::SkiaTheme::getInstance().getColors();

    // Draw background
    g.fillAll(juce::Colour(
        colors.backgroundSecondary >> 16 & 0xFF,
        colors.backgroundSecondary >> 8 & 0xFF,
        colors.backgroundSecondary & 0xFF
    ));

    // Draw component content using current value_
}

void SkiaNewComponentComponent::resized()
{
    // Layout child components
}

void SkiaNewComponentComponent::mouseDown(const juce::MouseEvent& event)
{
    // Handle mouse input
}

void SkiaNewComponentComponent::timerCallback()
{
    // Smooth animation toward target
    const float smoothingFactor = 0.2f;
    value_ += (targetValue_ - value_) * smoothingFactor;
    repaint();
}

} // namespace zenith

#endif
```

### Add to CMakeLists.txt

```cmake
$<$<BOOL:${ZENITH_ENABLE_SKIA}>:Source/ui/skia/SkiaNewComponentComponent.h>
$<$<BOOL:${ZENITH_ENABLE_SKIA}>:Source/ui/skia/SkiaNewComponentComponent.cpp>
```

### Use in MainWindow

```cpp
auto newComponent = std::make_unique<zenith::SkiaNewComponentComponent>();
addAndMakeVisible(newComponent.get());
```

---

## Common Patterns and Examples

### Example 1: Spring Physics Animation

```cpp
// Animate with snappy spring physics
skiaIntegration_->animateComponent(
    "button_click",
    1.0f,
    zenith::SpringPhysicsSettings::snappy()
);

// Query animation progress
float progress = skiaIntegration_->getAnimationController()
    .getValue("button_click");
```

### Example 2: Theme Switching

```cpp
// Toggle between dark and light theme
skiaIntegration_->toggleTheme();

// Or explicitly set theme
skiaIntegration_->setThemeMode(zenith::ThemeMode::Light, true);

// Save preference
auto& settings = zenith::SkiaSettingsManager::getInstance();
settings.setThemeMode(zenith::ThemeMode::Light);
```

### Example 3: Customizing Physics

```cpp
// Customize button physics for more snappy feel
zenith::SpringPhysicsSettings customPhysics = {
    600.0f,   // stiffness (higher = snappier)
    40.0f,    // damping (higher = less overshoot)
    144.0f    // fps (higher = more frequent updates)
};

auto& settings = zenith::SkiaSettingsManager::getInstance();
settings.setButtonPhysics(customPhysics);
```

### Example 4: Using Text Effects

```cpp
zenith::SkiaTextRenderer textRenderer;

// Standard text
auto standardOptions = textRenderer.standard();
// textRenderer.drawText(canvas, "Hello", bounds, standardOptions);

// Flashy text with glow, shadow, gradient
auto flashyOptions = textRenderer.flashy();
flashyOptions.color = colors.primary;
// textRenderer.drawText(canvas, "Important", bounds, flashyOptions);

// Custom effects
auto customOptions = textRenderer.outlined();
customOptions.effectColor = colors.success;
```

### Example 5: Using Waveform Renderer

```cpp
zenith::SkiaWaveformRenderer waveformRenderer;

// Create waveform data from audio buffer
zenith::SkiaWaveformRenderer::WaveformData waveData;
waveData.sampleRate = 44100;
waveData.numSamples = audioBuffer.getNumSamples();
waveData.audioBuffer = &audioBuffer;

// Render with producer-quality settings
auto options = waveformRenderer.producer();
// waveformRenderer.render(canvas, waveData, bounds, options);
```

---

## Troubleshooting

### Issue: "Skia not found" CMake Error

**Solution:**
```bash
C:/vcpkg/vcpkg.exe install skia:x64-windows
cmake -DZENITH_ENABLE_SKIA=ON ..
```

### Issue: Slow Rendering / Low FPS

**Solution:**
```cpp
// Reduce waveform detail level
auto& settings = zenith::SkiaSettingsManager::getInstance();
auto gpuSettings = settings.getGPUSettings();
gpuSettings.waveformDetailLevel = 2; // Lower = faster
settings.setGPUSettings(gpuSettings);

// Or enable adaptive FPS
settings.setAdaptiveFPS(true);
```

### Issue: Text Artifacts or Blurry Rendering

**Solution:**
```cpp
// Ensure antialiasing is enabled
auto& settings = zenith::SkiaSettingsManager::getInstance();
auto gpuSettings = settings.getGPUSettings();
gpuSettings.enableAntialiasing = true;
gpuSettings.msaaSamples = 4; // Or 8 for better quality
settings.setGPUSettings(gpuSettings);
```

### Issue: Theme Colors Don't Apply

**Solution:**
```cpp
// Force theme update
auto& theme = zenith::SkiaTheme::getInstance();
theme.setThemeMode(theme.getThemeMode()); // Trigger update
repaint(); // Force redraw
```

### Issue: Animations Stutter or Don't Smooth

**Solution:**
```cpp
// Check FPS monitoring
float currentFPS = skiaIntegration_->getCurrentFPS();
DBG("Current FPS: " << currentFPS);

// Increase target FPS if system can handle it
auto& settings = zenith::SkiaSettingsManager::getInstance();
settings.setTargetFPS(120);
```

---

## Performance Optimization

### Frame Rate Optimization

```cpp
// Adaptive FPS automatically adjusts between 30-144 Hz
auto& settings = zenith::SkiaSettingsManager::getInstance();
settings.setAdaptiveFPS(true);
settings.setTargetFPS(60); // Base FPS when idle
```

### Memory Optimization

```cpp
// Reduce waveform detail for large files
auto gpuSettings = settings.getGPUSettings();
gpuSettings.waveformDetailLevel = 2; // 1-5, lower = less memory
settings.setGPUSettings(gpuSettings);
```

### Rendering Quality vs Performance Trade-off

```cpp
// High quality mode
auto highQuality = settings.getGPUSettings();
highQuality.prioritizeQuality = true;
highQuality.msaaSamples = 8;
highQuality.enableMipmaps = true;

// Performance mode
auto performance = settings.getGPUSettings();
performance.prioritizeQuality = false;
performance.msaaSamples = 2;
performance.enableMipmaps = false;
```

### Caching Strategies

```cpp
// Waveform rendering is cached automatically
// For custom rendering, implement your own cache:

class CachedComponent : public juce::Component
{
    juce::Image cachedImage_;
    bool cacheValid_ = false;

    void paint(juce::Graphics& g) override
    {
        if (!cacheValid_)
        {
            // Regenerate cache
            cacheValid_ = true;
        }
        g.drawImageAt(cachedImage_, 0, 0);
    }

    void resized() override
    {
        cachedImage_ = juce::Image(juce::Image::RGB,
            getWidth(), getHeight(), true);
        cacheValid_ = false;
        repaint();
    }
};
```

---

## API Reference

### SkiaTheme

```cpp
// Get singleton instance
auto& theme = SkiaTheme::getInstance();

// Theme mode
theme.setThemeMode(ThemeMode::Dark);
theme.getColors(); // Get current color palette

// Spring physics per component
theme.getButtonPhysics();
theme.setSliderPhysics(SpringPhysicsSettings::smooth());

// Effects
theme.getDepthStyle();
theme.getTypography();
theme.getGPUSettings();
```

### SkiaMainWindowIntegration

```cpp
// Initialize
skiaIntegration_->initializeSkiaRendering();

// Theme
skiaIntegration_->setThemeMode(ThemeMode::Light);
skiaIntegration_->toggleTheme();

// Animation
skiaIntegration_->animateComponent("id", 1.0f, physics);
skiaIntegration_->getAnimationController().getValue("id");

// FPS
skiaIntegration_->getCurrentFPS();
skiaIntegration_->setAdaptiveFPS(true);
```

### SkiaSettingsManager

```cpp
auto& settings = SkiaSettingsManager::getInstance();

// Persistence
settings.loadSettings();
settings.saveSettings();
settings.resetToDefaults();

// Theme
settings.setThemeMode(ThemeMode::Dark);
settings.isDarkModePreferred();

// Physics customization
settings.setButtonPhysics(customPhysics);
settings.setSliderPhysics(customPhysics);

// GPU settings
settings.setTargetFPS(120);
settings.setAdaptiveFPS(true);
```

---

## Next Steps

- Implement performance profiling dashboard
- Add FFT visualization for frequency analysis
- Integrate real-time audio waveform analysis
- Optimize clip rendering with caching
- Add more theme presets (cyberpunk, retro, etc.)

---

**Last Updated:** November 21, 2025
**Skia Version:** Latest (installed via vcpkg)
**Build Status:** Ready for production
