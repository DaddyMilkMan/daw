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
#include <juce_audio_utils/juce_audio_utils.h>

#ifdef ZENITH_USE_SKIA
#include <core/SkCanvas.h>
#include <core/SkPath.h>
#endif

namespace zenith {

class BrowserPreviewPanel : public SkiaComponent, 
                            public juce::ChangeListener {
public:
  explicit BrowserPreviewPanel(BrowserPreviewEngine &engine);
  ~BrowserPreviewPanel() override;

  void drawSkia(SkCanvas *canvas) override;
  void mouseDown(const juce::MouseEvent &e) override;
  void resized() override;

  void loadWaveform(const juce::File &file);
  void clearWaveform();
  
  // ChangeListener implementation
  void changeListenerCallback(juce::ChangeBroadcaster* source) override;

private:
  void drawWaveform(SkCanvas *canvas, const SkRect &bounds);
  void drawIconButton(SkCanvas *canvas, const juce::Rectangle<int> &bounds, const SkPath &iconPath, bool active);
  void drawButton(SkCanvas *canvas, const juce::Rectangle<int> &bounds, const juce::String &text, bool active);
  void updateWaveformPath(const SkRect& bounds);

  BrowserPreviewEngine &engine_;
  
  juce::AudioFormatManager formatManager_;
  juce::AudioThumbnailCache thumbnailCache_;
  juce::AudioThumbnail thumbnail_;
  
  juce::File waveformFile_;
  
  SkPath cachedWaveformPath_;
  bool waveformPathDirty_ = true;
  SkRect lastWaveformBounds_ = SkRect::MakeEmpty();

  juce::Rectangle<int> playButtonBounds_;
  juce::Rectangle<int> stopButtonBounds_;
  juce::Rectangle<int> loopButtonBounds_;
  juce::Rectangle<int> autoPlayButtonBounds_;
  juce::Rectangle<int> waveformBounds_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BrowserPreviewPanel)
};

} // namespace zenith
