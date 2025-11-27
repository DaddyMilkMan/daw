#pragma once

#include "../skia/SkiaCanvasComponent.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>

namespace zenith {

class PianoKeyboardViewSkia : public SkiaCanvasComponent,
                              public juce::MidiKeyboardState::Listener {
public:
  PianoKeyboardViewSkia(juce::MidiKeyboardState &state);
  ~PianoKeyboardViewSkia() override;

  void paintSkia(SkCanvas &canvas, const juce::Rectangle<int> &bounds) override;

  void mouseDown(const juce::MouseEvent &e) override;
  void mouseUp(const juce::MouseEvent &e) override;
  void mouseDrag(const juce::MouseEvent &e) override;

  void handleNoteOn(juce::MidiKeyboardState *, int midiChannel,
                    int midiNoteNumber, float velocity) override;
  void handleNoteOff(juce::MidiKeyboardState *, int midiChannel,
                     int midiNoteNumber, float velocity) override;

private:
  juce::MidiKeyboardState &midiState;
  int rangeStart = 24; // C0
  int rangeEnd = 96;   // C6
  float whiteKeyWidth = 40.0f;
  float blackKeyWidth = 24.0f;
  float blackKeyHeightRatio = 0.6f;

  int getNoteAtPosition(juce::Point<float> pos);
  juce::Rectangle<float> getKeyBounds(int noteNumber);
  bool isBlackKey(int noteNumber) const;

  int currentNote = -1;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoKeyboardViewSkia)
};

} // namespace zenith
