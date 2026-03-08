#pragma once

/*
  Legacy synth-specific editor skin retained for compatibility.
  It is not the primary Zenith DAW art direction, which is the
  premium matte-black / restrained-blue Skia shell.
*/

#include "../../instruments/ZenithPolySynth.h"
#include "../../rendering/SkiaRenderer.h"
#include "components/CarbonPanel.h"
#include "components/IndustrialButton.h"
#include "components/IndustrialComponent.h"
#include "components/IndustrialDropdown.h"
#include "components/IndustrialKnob.h"
#include "components/IndustrialLED.h"
#include "components/IndustrialSlider.h"
#include "components/IndustrialToggle.h"
#include "components/ModMatrixTable.h"
#include "components/VisualizerDisplay.h"
#include "layout/AdvancedLayout.h"
#include "layout/BeginnerLayout.h"
#include "layout/ExpertLayout.h"
#include "layout/ModeManager.h"
#include "rendering/CarbonFiberTexture.h"
#include "rendering/IndustrialTheme.h"
#include <core/SkBitmap.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <unordered_map>
#include <vector>

namespace zenith::industrial {

class IndustrialUI : public juce::AudioProcessorEditor, private juce::Timer {
public:
  explicit IndustrialUI(ZenithPolySynthProcessor &processor);
  ~IndustrialUI() override;

  void paint(juce::Graphics &g) override;
  void resized() override;

  void mouseDown(const juce::MouseEvent &e) override;
  void mouseDrag(const juce::MouseEvent &e) override;
  void mouseUp(const juce::MouseEvent &e) override;
  void mouseMove(const juce::MouseEvent &e) override;
  bool keyPressed(const juce::KeyPress &key) override;

private:
  struct ComponentEntry {
    std::unique_ptr<IndustrialComponent> component;
    UIMode minMode = UIMode::Beginner;
  };

  void drawSkiaContent(SkCanvas *canvas);
  void drawModeLegend(SkCanvas *canvas);

  void timerCallback() override;

  void setMode(UIMode mode);
  void startModeAnimation(UIMode from, UIMode to);
  void updateLayoutForMode(UIMode mode);
  void syncModeButtons();

  std::vector<ControlSpec> controlsForMode(UIMode mode) const;
  void buildControlsForMode(UIMode mode);

  void addComponent(std::unique_ptr<IndustrialComponent> component,
                    UIMode minMode = UIMode::Beginner);

  ZenithPolySynthProcessor &processor_;
  std::unique_ptr<zenith::SkiaRenderer> renderer_;

  // Parameter attachments - keep them alive so controls stay connected
  std::vector<std::unique_ptr<juce::SliderParameterAttachment>> parameterAttachments_;

  UIMode currentMode_ = UIMode::Beginner;
  ModeManager modeManager_;
  IndustrialTheme theme_;
  SkBitmap carbonTexture_;

  std::vector<ComponentEntry> components_;
  IndustrialComponent *activeComponent_ = nullptr;
  VisualizerDisplay *visualizer_ = nullptr;

  std::unique_ptr<IndustrialButton> beginnerButton_;
  std::unique_ptr<IndustrialButton> advancedButton_;
  std::unique_ptr<IndustrialButton> expertButton_;

  bool animating_ = false;
  double animationStartMs_ = 0.0;
  int animationFromHeight_ = 500;
  int animationToHeight_ = 500;
  UIMode previousMode_ = UIMode::Beginner;
  float modeTransitionProgress_ = 1.0f;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IndustrialUI)
};

} // namespace zenith::industrial
