/**
 * @file SkiaMixerChannelComponent.h
 * @brief Logic Pro style mixer channel strip
 *
 * Features:
 * - Chrome/silver fader cap with concave middle
 * - Color-accurate audio meter (green/yellow/orange/red)
 * - M/S/R buttons
 * - Pan knob with green ring
 * - Track name and icon
 * - Insert/send slots
 */

#pragma once

#include <JuceHeader.h>

#ifdef ZENITH_USE_SKIA
    #include <include/core/SkCanvas.h>
#endif

namespace zenith {

#ifdef ZENITH_USE_SKIA

class SkiaMixerChannelComponent : public juce::Component,
                                   public juce::Timer
{
public:
    SkiaMixerChannelComponent();
    ~SkiaMixerChannelComponent() override = default;

    // Channel properties
    void setChannelName(const juce::String& name);
    void setChannelColor(juce::Colour color);
    void setChannelType(bool isBus);  // false = audio/MIDI, true = bus/aux
    
    // Fader
    void setFaderValue(float db);  // -inf to +6dB
    float getFaderValue() const { return faderDb_; }
    
    // Meter
    void setMeterLevel(float level);  // 0.0 to 1.0
    void setPeakLevel(float peak);
    
    // Pan
    void setPan(float pan);  // -1.0 to +1.0
    float getPan() const { return pan_; }
    
    // Buttons
    void setMuted(bool muted);
    void setSoloed(bool soloed);
    void setRecordEnabled(bool enabled);
    
    bool isMuted() const { return isMuted_; }
    bool isSoloed() const { return isSoloed_; }
    bool isRecordEnabled() const { return isRecordEnabled_; }
    
    // Selection
    void setSelected(bool selected);
    bool isSelected() const { return isSelected_; }

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void timerCallback() override;

private:
    // Channel properties
    juce::String channelName_ = "Audio 1";
    juce::Colour channelColor_ = juce::Colour(0xff006FFF);
    bool isBus_ = false;
    bool isSelected_ = false;
    
    // Audio properties
    float faderDb_ = 0.0f;      // Current fader position
    float meterLevel_ = 0.0f;   // Current meter level
    float peakLevel_ = 0.0f;    // Peak hold
    float peakHoldTime_ = 0.0f; // Peak hold timer (2 seconds)
    float pan_ = 0.0f;          // Pan position
    
    // Button states
    bool isMuted_ = false;
    bool isSoloed_ = false;
    bool isRecordEnabled_ = false;
    
    // UI bounds
    juce::Rectangle<int> faderTrackBounds_;
    juce::Rectangle<int> faderCapBounds_;
    juce::Rectangle<int> meterBounds_;
    juce::Rectangle<int> panKnobBounds_;
    juce::Rectangle<int> muteButtonBounds_;
    juce::Rectangle<int> soloButtonBounds_;
    juce::Rectangle<int> recordButtonBounds_;
    
    // Interaction
    bool isDraggingFader_ = false;
    bool isDraggingPan_ = false;
    int dragStartY_ = 0;
    float dragStartValue_ = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaMixerChannelComponent)
};

#endif // ZENITH_USE_SKIA

} // namespace zenith
