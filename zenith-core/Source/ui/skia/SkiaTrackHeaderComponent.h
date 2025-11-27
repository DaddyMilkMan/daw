/**
 * @file SkiaTrackHeaderComponent.h
 * @brief Logic Pro style track header with M/S/R/I buttons
 *
 * Features:
 * - Track number and icon
 * - Mute/Solo/Record/Input buttons with Logic Pro colors
 * - Volume fader and pan knob
 * - Track name field
 * - 4px color bar indicator
 */

#pragma once

#include <JuceHeader.h>

#ifdef ZENITH_USE_SKIA
    #include <include/core/SkCanvas.h>
#endif

namespace zenith {

#ifdef ZENITH_USE_SKIA

class SkiaTrackHeaderComponent : public juce::Component
{
public:
    SkiaTrackHeaderComponent();
    ~SkiaTrackHeaderComponent() override = default;

    // Track state
    void setTrackNumber(int number);
    void setTrackName(const juce::String& name);
    void setTrackColor(juce::Colour color);
    
    // Button states
    void setMuted(bool muted);
    void setSoloed(bool soloed);
    void setRecordEnabled(bool enabled);
    void setInputMonitoring(bool enabled);
    
    bool isMuted() const { return isMuted_; }
    bool isSoloed() const { return isSoloed_; }
    bool isRecordEnabled() const { return isRecordEnabled_; }
    bool isInputMonitoring() const { return isInputMonitoring_; }
    
    // Volume and pan
    void setVolume(float db);  // -inf to +6dB
    void setPan(float pan);     // -1.0 (left) to +1.0 (right)
    
    float getVolume() const { return volumeDb_; }
    float getPan() const { return pan_; }
    
    // Selection
    void setSelected(bool selected);
    bool isSelected() const { return isSelected_; }

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;

private:
    // Track properties
    int trackNumber_ = 1;
    juce::String trackName_ = "Audio 1";
    juce::Colour trackColor_ = juce::Colour(0xff006FFF);  // Logic Blue
    
    // Button states
    bool isMuted_ = false;
    bool isSoloed_ = false;
    bool isRecordEnabled_ = false;
    bool isInputMonitoring_ = false;
    bool isSelected_ = false;
    
    // Audio properties
    float volumeDb_ = 0.0f;   // 0dB unity
    float pan_ = 0.0f;        // Center
    
    // UI bounds (calculated in resized())
    juce::Rectangle<int> muteButtonBounds_;
    juce::Rectangle<int> soloButtonBounds_;
    juce::Rectangle<int> recordButtonBounds_;
    juce::Rectangle<int> inputButtonBounds_;
    juce::Rectangle<int> volumeSliderBounds_;
    juce::Rectangle<int> panKnobBounds_;
    
    // Interaction state
    bool isDraggingVolume_ = false;
    bool isDraggingPan_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaTrackHeaderComponent)
};

#endif // ZENITH_USE_SKIA

} // namespace zenith
