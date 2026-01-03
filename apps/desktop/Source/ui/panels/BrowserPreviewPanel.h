/*
  ==============================================================================

    BrowserPreviewPanel.h
    Created: 2025-12-26
    Author:  Zenith DAW

  ==============================================================================
*/

#pragma once

#include "SkiaComponent.h"
#include "../../browser/BrowserPreviewEngine.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>

#include <include/core/SkCanvas.h>


namespace zenith {

class BrowserPreviewPanel : public SkiaComponent {
public:
  explicit BrowserPreviewPanel(BrowserPreviewEngine &engine);
  ~BrowserPreviewPanel() override;

  void drawSkia(SkCanvas *canvas) override;
  void mouseDown(const juce::MouseEvent &e) override;
  void resized() override;

  void loadWaveform(const juce::File &file);
  void clearWaveform();

private:
  void drawWaveform(SkCanvas *canvas, const SkRect &bounds);
  void drawIconButton(SkCanvas *canvas, const juce::Rectangle<int> &bounds, const SkPath &iconPath, bool active);
  void drawButton(SkCanvas *canvas, const juce::Rectangle<int> &bounds, const juce::String &text, bool active);

  BrowserPreviewEngine &engine_;
  
  std::vector<float> waveformData_;
  juce::File waveformFile_;

  juce::Rectangle<int> playButtonBounds_;
  juce::Rectangle<int> stopButtonBounds_;
  juce::Rectangle<int> loopButtonBounds_;
  juce::Rectangle<int> autoPlayButtonBounds_;
  juce::Rectangle<int> waveformBounds_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BrowserPreviewPanel)
};

} // namespace zenith
