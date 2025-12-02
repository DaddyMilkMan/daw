/*
  ==============================================================================

    BottomBar.h
    Created: 2025-11-28
    Author:  David Chen + Leo Rossi

    Bottom bar container with Piano Keyboard and Mixer Strip.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SkiaComponent.h"
#include "../views/PianoKeyboardViewSkia.h"

namespace zenith {

#ifdef ZENITH_USE_SKIA

class BottomBar : public SkiaComponent {
public:
    explicit BottomBar(juce::MidiKeyboardState& state);
    ~BottomBar() override;
    
    void drawSkia(SkCanvas* canvas) override;
    void resized() override;
    
    void setKeyboardVisible(bool visible);
    bool isKeyboardVisible() const { return keyboardVisible_; }

private:
    juce::MidiKeyboardState& midiState_;
    std::unique_ptr<PianoKeyboardViewSkia> pianoKeyboard_;
    
    bool keyboardVisible_ = false;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BottomBar)

private:
    // Cached resources for 60FPS rendering
    SkPaint bgPaint_;
    SkPaint borderPaint_;
    SkPaint channelBgPaint_;
    SkPaint meterTrackPaint_;
    SkPaint meterFillPaint_;
    SkPaint textPaint_;
    SkFont font_;
    SkRect cachedBounds_;
    
    void updateCachedPaints(const SkRect& bounds);
};

#endif // ZENITH_USE_SKIA

} // namespace zenith
