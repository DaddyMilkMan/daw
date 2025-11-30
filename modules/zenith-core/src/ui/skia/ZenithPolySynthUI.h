/*
  ==============================================================================

    ZenithPolySynthUI.h
    Created: 2025-11-27
    Author:  Zenith DAW

    Skia-based UI for ZenithPolySynth.
    Implements a "Neon Noir" Glassmorphism design.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../../instruments/ZenithPolySynth.h"
#include "../../instruments/ZenithPresetManager.h"
#include "ZenithUIComponents.h"
#include "SkiaComponent.h"

#ifdef ZENITH_USE_SKIA
#include <include/core/SkSurface.h>
#include <include/gpu/ganesh/GrDirectContext.h>
#endif

namespace zenith {

//==============================================================================
/**
    Main Editor for ZenithPolySynth
    Uses direct Skia rendering via SkiaComponent.
    Acts as the OpenGL/Skia root for the plugin editor.
*/
class ZenithPolySynthUI : public juce::AudioProcessorEditor,
                          public SkiaComponent,
                          public juce::OpenGLRenderer {
public:
  ZenithPolySynthUI(ZenithPolySynthProcessor &p);
  ~ZenithPolySynthUI() override;

  //==============================================================================
  // OpenGLRenderer overrides
  void newOpenGLContextCreated() override;
  void renderOpenGL() override;
  void openGLContextClosing() override;

  //==============================================================================
  // SkiaComponent override
  void drawSkia(SkCanvas* canvas) override;

  //==============================================================================
  // Component overrides
  void resized() override;
  void paint(juce::Graphics& g) override; // Needed to paint black background for fallback

private:
  ZenithPolySynthProcessor &processor;
  juce::OpenGLContext openGLContext;

#ifdef ZENITH_USE_SKIA
  GrDirectContext* grContext_ = nullptr;
  SkSurface* surface_ = nullptr;
  int lastWidth_ = 0;
  int lastHeight_ = 0;

  void recreateSurface();
#endif

  // UI State
  bool isAdvancedMode_ = false;
  bool showTooltips_ = false;
  float animationTime_ = 0.0f;

  // Layout Constants
  static constexpr int kSimpleWidth = 600;
  static constexpr int kSimpleHeight = 400;
  static constexpr int kAdvancedWidth = 800;
  static constexpr int kAdvancedHeight = 600;

  // Components
  std::unique_ptr<ZenithKnob> subLevelKnob_;
  std::unique_ptr<ZenithKnob> noiseLevelKnob_;
  
  std::unique_ptr<ZenithKnob> cutoffKnob_;
  std::unique_ptr<ZenithKnob> resKnob_;
  std::unique_ptr<ZenithKnob> envAmtKnob_;

  std::unique_ptr<ZenithSlider> ampAttackSlider_;
  std::unique_ptr<ZenithSlider> ampDecaySlider_;
  std::unique_ptr<ZenithSlider> ampSustainSlider_;
  std::unique_ptr<ZenithSlider> ampReleaseSlider_;

  std::unique_ptr<ZenithButton> expandButton_;

  // Visualizer
  std::unique_ptr<ZenithVisualizer> visualizer_;
  
  // Modulation Matrix
  std::unique_ptr<ZenithModMatrix> modMatrix_;

  // Advanced Controls
  std::unique_ptr<ZenithKnob> lfo1RateKnob_;
  std::unique_ptr<ZenithKnob> lfo1AmountKnob_;
  std::unique_ptr<ZenithKnob> lfo2RateKnob_;
  std::unique_ptr<ZenithKnob> lfo2AmountKnob_;

  // Filter 2 Controls
  std::unique_ptr<ZenithKnob> filter2CutoffKnob_;
  std::unique_ptr<ZenithKnob> filter2ResKnob_;
  std::unique_ptr<ZenithKnob> filter2DriveKnob_;

  // Tooltips
  std::unique_ptr<ZenithTooltipOverlay> tooltipOverlay_;
  std::unique_ptr<ZenithButton> learningModeButton_;
  bool isLearningMode_ = false;

  // Preset Management
  std::unique_ptr<ZenithPresetBar> presetBar_;
  std::vector<PresetMetadata> presetList_;
  int currentPresetIndex_ = -1;

  // Filter Envelope Controls (Advanced)
  std::unique_ptr<ZenithSlider> modAttackSlider_;
  std::unique_ptr<ZenithSlider> modDecaySlider_;
  std::unique_ptr<ZenithSlider> modSustainSlider_;
  std::unique_ptr<ZenithSlider> modReleaseSlider_;

  // Internal helpers
  void drawBackground(SkCanvas *canvas);
  void drawGlassPanel(SkCanvas *canvas, const juce::Rectangle<int> &bounds);
  void toggleAdvancedMode();
  void toggleLearningMode();
  void loadPreset(int index);
  void loadNextPreset();
  void loadPrevPreset();
  void refreshPresetList();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithPolySynthUI)
};

} // namespace zenith
