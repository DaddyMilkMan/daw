/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "zenith_core/engine/Settings.h"
#include "zenith_core/instruments/ZenithPolySynth.h"
#include "zenith_core/instruments/ZenithPresetManager.h"
#include "ai_client/WingmanSynthBridge.h"
#include "../../rendering/SkiaRenderer.h"
#include "../controls/ZenithUIComponents.h"
#include "../visualizations/FilterResponseDisplay.h"
#include "../visualizations/ModulationVisualizer.h"
#include "../visualizations/SynthOscilloscope.h"
#include "RenderTree.h"
#include "SkiaMainWindowIntegration.h"
#include "../legacy/ZenithLookAndFeel.h"
#include <JuceHeader.h>

namespace zenith {

// Forward declaration
class WingmanParameterChange;

//==============================================================================
/**
    Main Editor for ZenithPolySynth
    Uses direct OpenGL/Skia rendering via SkiaRenderer.
    Manages a list of lightweight SkiaWidgets.
*/
class ZenithPolySynthUI : public juce::AudioProcessorEditor,
                          public juce::ChangeListener,
                          public juce::Timer,
                          public WingmanSynthListener { // Listen for Wingman parameter changes
public:
  ZenithPolySynthUI(ZenithPolySynthProcessor &p);
  ~ZenithPolySynthUI() override;

  //==============================================================================
  // Component overrides
  void paint(juce::Graphics &g) override;
  void resized() override;

  // Mouse handling for SkiaWidgets
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseDrag(const juce::MouseEvent &e) override;
  void mouseUp(const juce::MouseEvent &e) override;
  void mouseMove(const juce::MouseEvent &e) override;

  // Settings Listener
  void changeListenerCallback(juce::ChangeBroadcaster *) override;
  
  // WingmanSynthListener interface
  void wingmanParameterChanged(const WingmanParameterChange& change) override;
  void wingmanBatchStart() override;
  void wingmanBatchEnd() override;
  void wingmanSoundGenerated(const juce::String& description) override;

protected:
  // Skia draw callback
  void drawSkiaContent(SkCanvas *canvas);

  // Timer callback for frame capture (Message Thread)
  void timerCallback() override;

  void buildUI();
  void syncProcessorToUI();

private:
  ZenithPolySynthProcessor &processor;
  ZenithLookAndFeel zenithLookAndFeel_;

  // Skia Renderer (Composition)
  std::unique_ptr<SkiaRenderer> renderer_;
  void recreateRenderer();

  // UI State
  bool isAdvancedMode_ = false;

  // Layout Constants
  static constexpr int kSimpleWidth = 600;
  static constexpr int kSimpleHeight = 400;
  static constexpr int kAdvancedWidth = 800;
  static constexpr int kAdvancedHeight = 600;

  // Lightweight Widget Container
  std::vector<std::unique_ptr<ZenithControl>> widgets_;
  ZenithControl *activeWidget_ = nullptr; // Widget currently being dragged
  ZenithControl *hoveredWidget_ = nullptr;

  // Complex Components (kept as JUCE components for now)
  std::unique_ptr<ZenithVisualizer> visualizer_;
  std::unique_ptr<FilterResponseDisplay> filterResponseDisplay_;
  std::unique_ptr<ModulationVisualizer> modulationVisualizer_;
  std::unique_ptr<SynthOscilloscope> oscilloscope_;
  // std::unique_ptr<ZenithModMatrix> modMatrix_;
  // std::unique_ptr<ZenithPresetBar> presetBar_;

  // Internal helpers
  void renderComponentRecursively(juce::Component *comp, SkCanvas *canvas);
  void drawBackground(SkCanvas *canvas);

  // Widget Helpers
  template <typename T>
  T *addWidget(const juce::String &name, const juce::String &paramId);

  void layoutWidgets();
  void toggleAdvancedMode();
  void toggleLearningMode();

  // Preset Management
  std::vector<PresetMetadata> presetList_;
  int currentPresetIndex_ = -1;
  void loadPreset(int index);
  void loadNextPreset();
  void loadPrevPreset();
  void refreshPresetList();

  // Initialization
  
  // ========================================================================
  // WINGMAN ANIMATION SYSTEM
  // ========================================================================
  
  /**
   * Active widget animation state for smooth parameter transitions
   */
  struct WidgetAnimation {
    ZenithControl* widget;
    float startValue;
    float targetValue;
    float progress;
    float speed;
    double startTime;
    
    WidgetAnimation() : widget(nullptr), startValue(0.0f), targetValue(0.0f),
                      progress(0.0f), speed(0.3f), startTime(0.0) {}
  };
  
  juce::Array<WidgetAnimation> activeAnimations_;
  
  /**
   * Animate a widget to a new value with smooth easing
   */
  void animateWidgetToValue(ZenithControl* widget, float targetValue, float speed);
  
  // ========================================================================
  // RENDER TREE (PHASE 1: Thread Safety)
  // ========================================================================

  /**
   * Triple buffer for lock-free frame swapping between threads.
   * Message Thread: Snapshots UI state -> writes to buffer -> swaps to ready
   * Render Thread: Reads from render buffer -> draws frame
   */
  render::FrameBufferSwap frameBuffer_;

  /**
   * Capture current UI state into a frame snapshot.
   * Called from timerCallback() on the Message Thread.
   */
  void captureFrameSnapshot();

  /**
   * Draw knob from render state (no Component access).
   * Called from drawSkiaContent() on the Render Thread.
   */
  void drawKnobFromState(SkCanvas *canvas,
                         const render::KnobRenderState &state);

  /**
   * Draw slider from render state (no Component access).
   * Called from drawSkiaContent() on the Render Thread.
   */
  void drawSliderFromState(SkCanvas *canvas,
                           const render::SliderRenderState &state);

  /**
   * Draw button from render state (no Component access).
   * Called from drawSkiaContent() on the Render Thread.
   */
  void drawButtonFromState(SkCanvas *canvas,
                           const render::ButtonRenderState &state);

  /**
   * Draw visualizer from render state (no Component access).
   * Called from drawSkiaContent() on the Render Thread.
   */
  void drawVisualizerFromState(SkCanvas *canvas,
                               const std::vector<float> &samples,
                               const SkRect &bounds);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithPolySynthUI)
};

} // namespace zenith
