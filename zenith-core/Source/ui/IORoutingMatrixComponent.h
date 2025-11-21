/**
 * @file IORoutingMatrixComponent.h
 * @brief Input/Output routing matrix for recording and mixing
 *
 * Features:
 * - Audio input selector per track
 * - MIDI input selector per track
 * - Audio output routing (track → master or aux bus)
 * - Send levels for each send (up to 4)
 * - Send pre/post fader indicators
 * - Channel width selector (mono, stereo)
 * - Input monitoring controls
 * - Input gain display
 * - Output pan display
 * - Visual routing diagram with animated lines
 * - Color-coded routing (audio=blue, MIDI=green)
 * - Click to edit routing
 * - Beautiful matrix visualization
 * - Smooth connection animations
 *
 * @phase Phase 11: Mixing & Routing
 */

#pragma once

#include <JuceHeader.h>

class Engine;
class ProjectState;

namespace zenith {

/**
 * @class IORoutingMatrixComponent
 * @brief Visual I/O routing matrix with animated connections
 */
class IORoutingMatrixComponent : public juce::Component,
                                public juce::Button::Listener,
                                public juce::ComboBox::Listener,
                                public juce::Slider::Listener
{
public:
    //==========================================================================
    IORoutingMatrixComponent(Engine& eng, ProjectState& state);
    ~IORoutingMatrixComponent() override;

    //==========================================================================
    void paint(juce::Graphics& g) override;
    void resized() override;

    //==========================================================================
    // Listeners
    //==========================================================================
    void buttonClicked(juce::Button* button) override;
    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override;
    void sliderValueChanged(juce::Slider* slider) override;

    //==========================================================================
    // Methods
    //==========================================================================

    /**
     * @brief Refresh routing matrix from engine
     */
    void refreshRoutingMatrix();

    /**
     * @brief Set selected track for editing
     */
    void setSelectedTrack(const juce::String& trackId);

private:
    //==========================================================================
    // Helpers
    //==========================================================================

    /**
     * @brief Paint routing diagram
     */
    void paintRoutingDiagram(juce::Graphics& g, const juce::Rectangle<int>& bounds);

    /**
     * @brief Paint routing line with animation
     */
    void paintRoutingLine(juce::Graphics& g,
                         const juce::Point<float>& from,
                         const juce::Point<float>& to,
                         const juce::Colour& color, bool animated);

    /**
     * @brief Paint routing matrix grid
     */
    void paintRoutingGrid(juce::Graphics& g, const juce::Rectangle<int>& bounds);

    /**
     * @brief Paint input column
     */
    void paintInputColumn(juce::Graphics& g, const juce::Rectangle<int>& bounds);

    /**
     * @brief Paint output column
     */
    void paintOutputColumn(juce::Graphics& g, const juce::Rectangle<int>& bounds);

    /**
     * @brief Get input device list
     */
    juce::StringArray getInputDevices() const;

    /**
     * @brief Get output buses list
     */
    juce::StringArray getOutputBuses() const;

    //==========================================================================
    // Members
    //==========================================================================

    Engine& engine_;
    ProjectState& projectState_;

    // Selected track
    juce::String selectedTrackId_;

    // Routing data
    struct TrackRouting {
        juce::String trackId;
        juce::String trackName;
        juce::String audioInput;      // Input device
        juce::String midiInput;       // MIDI device
        juce::String audioOutput;     // Output bus (Master, Aux1, etc.)
        juce::Array<float> sendLevels;  // Send levels (0.0-1.0)
        juce::Array<bool> sendPrePost;  // Pre/Post fader flags
        bool monitoring;  // Input monitoring enabled
        float inputGain;
    };
    juce::Array<TrackRouting> trackRoutings_;

    // UI Controls
    juce::ComboBox audioInputSelector_;
    juce::ComboBox midiInputSelector_;
    juce::ComboBox audioOutputSelector_;

    // Send level sliders (up to 4)
    juce::Slider sendLevelSliders_[4];
    juce::ToggleButton sendPrePostButtons_[4];

    // Monitoring
    juce::ToggleButton monitoringButton_;
    juce::Slider inputGainSlider_;

    // Layout
    static constexpr int COLUMN_WIDTH = 120;
    static constexpr int ROW_HEIGHT = 32;
    static constexpr int SPACING = 8;
    static constexpr int PADDING = 12;
    static constexpr int LABEL_WIDTH = 80;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IORoutingMatrixComponent)
};

}  // namespace zenith

