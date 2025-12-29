/*
  ==============================================================================
    RenderTree.h
    Created: 2025-12-02
    Author: Zenith DAW

    Thread-safe render state abstraction.
    Decouples JUCE Component state (Message Thread) from Skia rendering (OpenGL
  Thread).
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>

#include "../design-system/ZenithDesignSystem.h"

#include <atomic>
#include <memory>
#include <vector>

namespace zenith::render {

//==============================================================================
// Render State Structs (POD - Safe to copy between threads)
//==============================================================================

/**
 * Render state for a rotary knob control.
 * Contains all data needed to draw the knob without accessing the Component.
 */
struct KnobRenderState {
  SkRect bounds;       // Position and size
  float value;         // Normalized value [0.0, 1.0]
  float defaultValue;  // For reset indicator
  bool isHovered;      // Hover state
  bool isDragging;     // Drag state
  SkColor baseColor;   // Primary color
  SkColor glowColor;   // Glow/accent color
  float glowIntensity; // Animated glow strength [0.0, 1.0]
  float scale;         // Hover scale animation [1.0, 1.05]

  // Display parameters
  float displayMin;
  float displayMax;
  juce::String labelText; // Pre-formatted value label

  // Cached resources (shared pointers, safe across threads)
  sk_sp<SkImage> cachedGlow; // Pre-rendered glow layer
  uint8_t cachedGlowAlpha;   // Pre-computed glow alpha (40% max)

  KnobRenderState()
      : bounds(SkRect::MakeEmpty()), value(0.0f), defaultValue(0.5f),
        isHovered(false), isDragging(false), baseColor(SK_ColorCYAN),
        glowColor(SK_ColorCYAN), glowIntensity(0.0f), scale(1.0f),
        displayMin(0.0f), displayMax(1.0f), cachedGlowAlpha(0) {}
};

/**
 * Render state for a vertical slider control.
 */
struct SliderRenderState {
  SkRect bounds;
  float value; // Normalized value [0.0, 1.0]
  bool isHovered;
  bool isDragging;
  SkColor color;
  juce::String labelText; // e.g., "A", "D", "S", "R"

  SliderRenderState()
      : bounds(SkRect::MakeEmpty()), value(0.0f), isHovered(false),
        isDragging(false), color(SK_ColorWHITE) {}
};

/**
 * Render state for a button control.
 */
struct ButtonRenderState {
  SkRect bounds;
  juce::String text;
  bool isPressed;
  bool isDown; // Currently pressed down
  bool isHovered;
  bool isToggled; // For toggle buttons
  SkColor color;

  ButtonRenderState()
      : bounds(SkRect::MakeEmpty()), isPressed(false), isDown(false),
        isHovered(false), isToggled(false), color(SK_ColorWHITE) {}
};

/**
 * Render state for the waveform visualizer.
 */
struct VisualizerRenderState {
  SkRect bounds;
  std::vector<float> waveformData; // Pre-downsampled waveform points
  float peakLevel;                 // Current peak [0.0, 1.0]
  bool isActive;                   // Is audio playing?

  VisualizerRenderState()
      : bounds(SkRect::MakeEmpty()), peakLevel(0.0f), isActive(false) {}
};

/**
 * Render state for preset bar.
 */
struct PresetBarRenderState {
  SkRect bounds;
  juce::String presetName;
  bool canGoPrev;
  bool canGoNext;

  PresetBarRenderState()
      : bounds(SkRect::MakeEmpty()), canGoPrev(false), canGoNext(false) {}
};

//==============================================================================
// Complete Frame Data
//==============================================================================

/**
 * Complete UI frame snapshot.
 * This is the atomic unit that gets swapped between threads.
 */
struct UiFrameData {
  // Component render states
  std::vector<KnobRenderState> knobs;
  std::vector<SliderRenderState> sliders;
  std::vector<ButtonRenderState> buttons;

  // Complex components
  VisualizerRenderState visualizer;
  PresetBarRenderState presetBar;

  // Visualizer data
  std::vector<float> visualizerSamples;
  SkRect visualizerBounds;

  // Global UI state
  bool isAdvancedMode;
  juce::Rectangle<int> componentBounds; // Overall component size

  // Frame metadata
  uint64_t frameNumber;
  int64_t timestamp; // Microseconds since epoch

  UiFrameData()
      : visualizerBounds(SkRect::MakeEmpty()), isAdvancedMode(false),
        frameNumber(0), timestamp(0) {}

  void clear() {
    knobs.clear();
    sliders.clear();
    buttons.clear();
    visualizerSamples.clear();
    frameNumber = 0;
  }
};

//==============================================================================
// Triple Buffer for Lock-Free Frame Swapping
//==============================================================================

/**
 * Triple buffer implementation for lock-free frame swapping between threads.
 *
 * Pattern:
 * - Message Thread: Writes to "write" buffer, then swaps to "ready"
 * - Render Thread: Reads from "render" buffer, swaps from "ready" when
 * available
 *
 * This ensures:
 * - Zero locks/mutexes
 * - Zero frame drops
 * - Latest data always available to renderer
 */
class FrameBufferSwap {
public:
  FrameBufferSwap() {
    // Initialize all buffers
    buffers_[0] = std::make_unique<UiFrameData>();
    buffers_[1] = std::make_unique<UiFrameData>();
    buffers_[2] = std::make_unique<UiFrameData>();

    // Initial indices
    writeIndex_.store(0, std::memory_order_relaxed);
    readyIndex_.store(1, std::memory_order_relaxed);
    renderIndex_.store(2, std::memory_order_relaxed);
  }

  /**
   * Get writable buffer for Message Thread.
   * Call this to start building a new frame.
   */
  UiFrameData *getWriteBuffer() {
    int idx = writeIndex_.load(std::memory_order_relaxed);
    return buffers_[idx].get();
  }

  /**
   * Swap write buffer to ready.
   * Call this when Message Thread finishes building a frame.
   */
  void swapWriteToReady() {
    int write = writeIndex_.load(std::memory_order_relaxed);
    int ready = readyIndex_.exchange(write, std::memory_order_acq_rel);
    writeIndex_.store(ready, std::memory_order_relaxed);

    hasNewFrame_.store(true, std::memory_order_release);
  }

  /**
   * Get latest frame for Render Thread.
   * Returns the most recent completed frame.
   */
  const UiFrameData *getLatestFrame() {
    // Check if new frame is ready
    if (hasNewFrame_.load(std::memory_order_acquire)) {
      // Swap ready to render
      int render = renderIndex_.load(std::memory_order_relaxed);
      int ready = readyIndex_.exchange(render, std::memory_order_acq_rel);
      renderIndex_.store(ready, std::memory_order_relaxed);

      hasNewFrame_.store(false, std::memory_order_release);
    }

    // Return current render buffer
    int idx = renderIndex_.load(std::memory_order_relaxed);
    return buffers_[idx].get();
  }

  /**
   * Check if new frame is available (non-consuming).
   */
  bool hasNewFrame() const {
    return hasNewFrame_.load(std::memory_order_acquire);
  }

private:
  std::unique_ptr<UiFrameData> buffers_[3];

  std::atomic<int> writeIndex_;  // Message Thread writes here
  std::atomic<int> readyIndex_;  // Swap buffer (completed frames)
  std::atomic<int> renderIndex_; // Render Thread reads here

  std::atomic<bool> hasNewFrame_; // Signal for renderer

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FrameBufferSwap)
};

} // namespace zenith::render
