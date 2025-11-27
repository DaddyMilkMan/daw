/**
 * @file SkiaPianoRollComponent.h
 * @brief Logic Pro style piano roll editor
 *
 * Features:
 * - Piano keys (white #444444, black #111111)
 * - MIDI notes with velocity colors
 * - Fine grid lines
 * - Playhead
 * - Note selection and editing
 */

#pragma once

#include <JuceHeader.h>

#ifdef ZENITH_USE_SKIA
    #include <include/core/SkCanvas.h>
#endif

namespace zenith {

#ifdef ZENITH_USE_SKIA

struct MidiNote
{
    int pitch;          // 0-127
    float startBeat;
    float duration;
    int velocity;       // 0-127
    bool isSelected;
};

class SkiaPianoRollComponent : public juce::Component
{
public:
    SkiaPianoRollComponent();
    ~SkiaPianoRollComponent() override = default;

    // Notes
    void addNote(const MidiNote& note);
    void clearNotes();
    
    // Playback
    void setPlayheadPosition(float beats);
    
    // View
    void setZoom(float pixelsPerBeat);
    void setScrollPosition(float beats);
    void setKeyRange(int lowestKey, int highestKey);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    std::vector<MidiNote> notes_;
    float playheadPosition_ = 0.0f;
    float pixelsPerBeat_ = 40.0f;
    float scrollPosition_ = 0.0f;
    
    int lowestKey_ = 36;   // C2
    int highestKey_ = 96;  // C7
    
    static constexpr int keyboardWidth_ = 60;
    static constexpr int keyHeight_ = 12;
    
    void drawPianoKeys(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void drawGrid(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void drawNotes(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void drawPlayhead(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    
    bool isBlackKey(int pitch) const;
    juce::String getNoteName(int pitch) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaPianoRollComponent)
};

#endif // ZENITH_USE_SKIA

} // namespace zenith
