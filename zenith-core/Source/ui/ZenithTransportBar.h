/**
 * @file ZenithTransportBar.h
 * @brief Unified transport bar for Zenith DAW
 *
 * Features:
 * - Single horizontal bar spanning window width
 * - Left: Project name/logo
 * - Center: Play/Stop/Record controls
 * - Right: Time display, tempo, CPU, track count
 *
 * Replaces the floating transport window and bottom bar buttons
 */

#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_events/juce_events.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_data_structures/juce_data_structures.h>

class Engine;

namespace zenith {

/**
 * @class ZenithTransportBar
 * @brief Unified transport control bar
 */
class ZenithTransportBar : public juce::Component,
                          private juce::Timer
{
public:
    //==========================================================================
    explicit ZenithTransportBar(Engine& engine);
    ~ZenithTransportBar() override;

    //==========================================================================
    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;

private:
    //==========================================================================
    // Helper methods
    //==========================================================================
    
    void drawTransportButton(juce::Graphics& g,
                            const juce::Rectangle<int>& bounds,
                            const juce::String& symbol,
                            const juce::Colour& color,
                            bool isActive,
                            bool isHovered,
                            bool isPressed);
    
    juce::String formatTime(double seconds);
    juce::String formatTempo(double bpm);
    
    //==========================================================================
    // Hit testing
    //==========================================================================
    
    juce::Rectangle<int> getPlayButtonBounds() const;
    juce::Rectangle<int> getStopButtonBounds() const;
    juce::Rectangle<int> getRecordButtonBounds() const;
    juce::Rectangle<int> getLoopButtonBounds() const;
    
    //==========================================================================
    // Members
    //==========================================================================
    
    Engine& engine_;
    
    // State
    bool playHovered_ = false;
    bool stopHovered_ = false;
    bool recordHovered_ = false;
    bool loopHovered_ = false;
    
    bool playPressed_ = false;
    bool stopPressed_ = false;
    bool recordPressed_ = false;
    bool loopPressed_ = false;
    
    // Display values (updated from timer)
    juce::String timeDisplay_;
    juce::String tempoDisplay_;
    juce::String cpuDisplay_;
    int trackCount_ = 0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithTransportBar)
};

} // namespace zenith

