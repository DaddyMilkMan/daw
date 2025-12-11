/*
  ==============================================================================

    PianoKeyboardViewSkia.h
    Created: 2025-11-28
    Author:  Leo Rossi

    Skia-rendered Piano Keyboard with neon glow effects.

  ==============================================================================
*/

#pragma once

#include "../SkiaComponent.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>


namespace zenith {

#ifdef ZENITH_USE_SKIA

class PianoKeyboardViewSkia : public SkiaComponent,
                              public juce::MidiKeyboardState::Listener {
public:
  PianoKeyboardViewSkia(juce::MidiKeyboardState &state,
                        juce::MidiKeyboardComponent::Orientation orientation);
  ~PianoKeyboardViewSkia() override;

  void drawSkia(::SkCanvas *canvas) override;

  // Mouse handling for playing keys
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseDrag(const juce::MouseEvent &e) override;
  void mouseUp(const juce::MouseEvent &e) override;

  // MidiKeyboardState::Listener
  void handleNoteOn(juce::MidiKeyboardState *, int midiChannel,
                    int midiNoteNumber, float velocity) override;
  void handleNoteOff(juce::MidiKeyboardState *, int midiChannel,
                     int midiNoteNumber, float velocity) override;

private:
  juce::MidiKeyboardState &state_;
  juce::MidiKeyboardComponent::Orientation orientation_;

  int rangeStart_ = 24; // C1
  int rangeEnd_ = 96;   // C7
  float keyWidth_ = 40.0f;
  float blackKeyHeightRatio_ = 0.6f;

  // Helper to get note at position
  int getNoteAtPosition(juce::Point<float> pos);

  // Cached resources for 60FPS rendering
  SkPaint whiteKeyPaint_;
  SkPaint blackKeyPaint_;
  SkPaint activeKeyPaint_;
  SkRect cachedBounds_;

  void updateCachedPaints(const SkRect &bounds);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoKeyboardViewSkia)
};

#endif // ZENITH_USE_SKIA

} // namespace zenith
