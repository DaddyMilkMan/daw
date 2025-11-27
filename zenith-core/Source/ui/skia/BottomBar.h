/**
 * @file BottomBar.h
 * @brief Bottom bar hosting piano keyboard and mixer strip
 *
 * Full-width bar (96-120px height) containing:
 * - Piano keyboard (full width or majority of width)
 * - Optional slim mixer strip
 * - Toggle buttons for visibility
 */

#pragma once

#include "../views/PianoKeyboardViewSkia.h"
#include "SkiaCanvasComponent.h"
#include "SkiaTheme.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_gui_basics/juce_gui_basics.h>


namespace zenith {

/**
 * @class BottomBar
 * @brief Bottom panel with piano keyboard and mixer controls
 *
 * Provides:
 * - Virtual MIDI keyboard for note input
 * - Optional slim mixer strip
 * - Toggleable visibility
 */
class BottomBar : public SkiaComponent {
public:
  //==========================================================================
  // Construction
  //==========================================================================

  BottomBar(juce::MidiKeyboardState &keyboardState);
  ~BottomBar() override = default;

  //==========================================================================
  // SkiaComponent Implementation
  //==========================================================================

#ifdef ZENITH_USE_SKIA
  void drawSkia(SkCanvas *canvas) override;
#endif

  //==========================================================================
  // Visibility Control
  //==========================================================================

  void setKeyboardVisible(bool visible);
  bool isKeyboardVisible() const { return keyboardVisible_; }

  void setMixerStripVisible(bool visible);
  bool isMixerStripVisible() const { return mixerStripVisible_; }

  //==========================================================================
  // Keyboard Access
  //==========================================================================

  PianoKeyboardViewSkia *getKeyboard() const { return keyboard_.get(); }

  //==========================================================================
  // Component Overrides
  //==========================================================================

  void resized() override;
#ifndef ZENITH_USE_SKIA
  void paint(juce::Graphics &g) override;
#endif

private:
  //==========================================================================
  // Mixer Strip Component (Placeholder)
  //==========================================================================

  class MixerStrip : public SkiaCanvasComponent {
  public:
    MixerStrip();
    ~MixerStrip() override = default;

    void setChannelCount(int count);

  protected:
    void paintSkia(SkCanvas &canvas,
                   const juce::Rectangle<int> &bounds) override;

  private:
    int channelCount_ = 8;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerStrip)
  };

  //==========================================================================
  // State
  //==========================================================================

  std::unique_ptr<PianoKeyboardViewSkia> keyboard_;
  std::unique_ptr<MixerStrip> mixerStrip_;

  bool keyboardVisible_ = true;
  bool mixerStripVisible_ = false;

  static constexpr int KEYBOARD_HEIGHT = 96;
  static constexpr int MIXER_STRIP_WIDTH = 600;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BottomBar)
};

} // namespace zenith
