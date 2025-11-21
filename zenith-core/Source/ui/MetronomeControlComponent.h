/**
 * @file MetronomeControlComponent.h
 * @brief Metronome/Click track control with visual feedback
 *
 * Features:
 * - Enable/disable toggle with animated indicator
 * - Volume slider with real-time dB display
 * - Click sound type selector (acoustic, electronic, bell, wood block)
 * - Accent on beat 1 toggle
 * - Visual beat indicator (animated circle pulse on each beat)
 * - Time signature display synchronized with project
 * - Pre-roll count-in selector (0, 1, 2, 4 bars)
 * - MIDI click enable toggle
 * - Only Click vs Count-in mode selector
 * - Beautiful animated beat indicator with color changes
 * - Apple-inspired design with smooth transitions
 *
 * @phase Phase 2: Recording Setup
 */

#pragma once

#include <JuceHeader.h>

class ProjectState;

namespace zenith {

/**
 * @class MetronomeControlComponent
 * @brief Beautiful metronome control with animated beat feedback
 */
class MetronomeControlComponent : public juce::Component,
                                 public juce::Slider::Listener,
                                 public juce::Button::Listener,
                                 public juce::ComboBox::Listener,
                                 public juce::Timer
{
public:
    //==========================================================================
    explicit MetronomeControlComponent(ProjectState& state);
    ~MetronomeControlComponent() override;

    //==========================================================================
    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    //==========================================================================
    // Listeners
    //==========================================================================
    void sliderValueChanged(juce::Slider* slider) override;
    void buttonClicked(juce::Button* button) override;
    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override;

    //==========================================================================
    // Methods
    //==========================================================================

    /**
     * @brief Trigger beat pulse (called from audio thread, queued on UI)
     */
    void triggerBeatPulse();

    /**
     * @brief Enable/disable metronome
     */
    void setMetronomeEnabled(bool enabled) [[maybe_unused]];

    /**
     * @brief Get metronome enabled state
     */
    bool isMetronomeEnabled() const { return metronomeEnabled_; }

private:
    //==========================================================================
    // Helpers
    //==========================================================================

    /**
     * @brief Paint beat indicator
     */
    void paintBeatIndicator(juce::Graphics& g, const juce::Rectangle<int>& bounds);

    /**
     * @brief Paint animated pulse ring
     */
    void paintPulseRing(juce::Graphics& g, const juce::Rectangle<int>& bounds,
                       float pulse);

    /**
     * @brief Update beat animation
     */
    void updateBeatAnimation();

    //==========================================================================
    // Members
    //==========================================================================

    ProjectState& projectState_;

    // Enable/disable button
    juce::ToggleButton metronomeButton_;
    bool metronomeEnabled_ = true;

    // Volume slider (-60 to 0 dB)
    juce::Slider volumeSlider_;

    // Click sound type selector
    juce::ComboBox clickTypeSelector_;

    // Accent on beat 1
    juce::ToggleButton accentButton_;

    // Pre-roll count-in selector
    juce::ComboBox preRollSelector_;

    // MIDI click toggle
    juce::ToggleButton midiClickButton_;

    // Mode selector (Click, Count-in, Both)
    juce::ComboBox modeSelector_;

    // Beat animation state
    float beatPulse_ = 0.0f;  // 0.0 to 1.0, animates on each beat
    int beatPulseFrames_ = 0;
    static constexpr int BEAT_PULSE_DURATION = 20;  // frames

    // Current beat (0-3 for 4/4 time)
    int currentBeat_ = 0;
    int totalBeats_ = 4;

    // Layout constants
    static constexpr int BUTTON_SIZE = 50;
    static constexpr int INDICATOR_SIZE = 60;
    static constexpr int SPACING = 12;
    static constexpr int PADDING = 16;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MetronomeControlComponent)
};

}  // namespace zenith

