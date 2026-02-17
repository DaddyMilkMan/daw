/*
  ==============================================================================

    ZenithPolySynthUI.h
    Created: 2025-11-27
    Author:  Zenith DAW

    Skia-based UI for ZenithPolySynth.
    Implements a "Neon Noir" Glassmorphism design.

    REFACTOR: "Pure Skia" - Uses lightweight SkiaWidget structs for controls.
*/

#pragma once

#include "../../Settings.h"
#include "../../instruments/ZenithPolySynth.h"
#include "../../instruments/ZenithPresetManager.h"
#include "../../rendering/SkiaRenderer.h"
#include "../controls/ZenithUIComponents.h"
#include "../controls/ZenithModMatrix.h"
#include "RenderTree.h"
#include "SkiaMainWindowIntegration.h"
#include "ZenithLookAndFeel.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <vector>

namespace zenith {

//==============================================================================
/**
    Main Editor for ZenithPolySynth
    Uses direct OpenGL/Skia rendering via SkiaRenderer.
    Manages a list of lightweight SkiaWidgets.
*/
class ZenithPolySynthUI : public juce::AudioProcessorEditor,
                          public juce::ChangeListener,
                          public juce::Timer { // Listen for settings changes
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

protected:
  // Skia draw callback
  void drawSkiaContent(SkCanvas *canvas);

  // Timer callback for frame capture (Message Thread)
  void timerCallback() override;

  void buildUI();
  void syncProcessorToUI();

private:
  struct MacroTarget {
    enum class Curve {
      Linear,
      Soft,
      Hard
    };

    juce::String paramId;
    float depth = 0.0f;
    bool bipolar = true;
    Curve curve = Curve::Linear;
  };

  struct MacroDefinition {
    juce::String name;
    SkColor color = SK_ColorCYAN;
    std::vector<MacroTarget> targets;
  };

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
  std::vector<std::unique_ptr<SkiaWidget>> widgets_;
  SkiaWidget *activeWidget_ = nullptr; // Widget currently being dragged
  SkiaWidget *hoveredWidget_ = nullptr;

  // Complex Components (kept as JUCE components for now)
  std::unique_ptr<ZenithVisualizer> visualizer_;
  std::unique_ptr<ZenithModMatrix> modMatrix_;

  std::array<std::unique_ptr<ZenithKnob>, 4> macroKnobs_;
  std::array<MacroDefinition, 4> macroDefs_;
  std::array<float, 4> macroValues_{{0.5f, 0.5f, 0.5f, 0.5f}};
  std::array<juce::RangedAudioParameter*, 4> macroParams_{{nullptr, nullptr, nullptr, nullptr}};
  juce::RangedAudioParameter* activeMacroGestureParam_ = nullptr;
  int activeMacroEditorIndex_ = 0;
  int activeMacroTargetIndex_ = 0;
  bool macroAssignArmed_ = false;
  bool macroDepthDragging_ = false;
  float macroDepthDragStartValue_ = 0.0f;
  float macroDepthDragStartY_ = 0.0f;
  juce::String pendingAssignParamId_;
  bool macroAssignDragActive_ = false;
  juce::Rectangle<int> macroEditorBounds_;
  juce::Rectangle<int> macroDepthSliderBounds_;
  juce::Rectangle<int> macroAssignButtonBounds_;
  juce::Rectangle<int> macroBipolarToggleBounds_;
  juce::Rectangle<int> macroAddTargetBounds_;
  juce::Rectangle<int> macroRemoveTargetBounds_;
  juce::Rectangle<int> macroCurveButtonBounds_;
  std::array<juce::Rectangle<int>, 4> macroTabBounds_{};
  std::vector<juce::Rectangle<int>> macroTargetRowBounds_;
  std::array<juce::Rectangle<int>, 5> sectionBounds_{};
  juce::Rectangle<int> macroBounds_;
  juce::Rectangle<int> matrixBounds_;
  juce::Rectangle<int> visualizerBounds_;

  // Internal helpers
  void renderComponentRecursively(juce::Component *comp, SkCanvas *canvas);
  void drawBackground(SkCanvas *canvas);

  // Widget Helpers
  template <typename T>
  T *addWidget(const juce::String &name, const juce::String &paramId);

  void layoutWidgets();
  void initMacroDefinitions();
  void bindMacroTargets();
  const juce::String& macroParamIdForIndex(int macroIndex) const;
  void beginMacroGesture(int macroIndex);
  void endMacroGesture(int macroIndex);
  void applyMacroValue(int macroIndex, float value);
  void updateMacroEditorLayout();
  bool assignParameterToSelectedMacroTarget(juce::Component* sourceComp);
  void assignSelectedTargetToParamId(const juce::String& paramId);
  juce::String getDefaultAssignableParamId() const;
  void setSelectedMacroTargetDepth(float depth, bool notify);
  ZenithKnob* addKnob(const juce::String& name,
                      const juce::String& paramId,
                      const juce::String& helpText,
                      std::vector<ZenithKnob*>& section);
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
