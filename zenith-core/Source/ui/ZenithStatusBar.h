/**
 * @file ZenithStatusBar.h
 * @brief Unified status bar component for Zenith DAW
 */

#pragma once

#include <JuceHeader.h>
#include "ZenithLookAndFeel.h"

namespace zenith {

// Forward declarations
class Engine;

/**
 * @class ZenithStatusBar
 * @brief Bottom status bar displaying system info and messages
 */
class ZenithStatusBar : public juce::Component,
                        public juce::Timer
{
public:
    //==========================================================================
    /**
     * @brief Construct status bar
     * @param engine Engine reference for CPU/Audio stats
     */
    explicit ZenithStatusBar(Engine& engine);
    ~ZenithStatusBar() override;

    //==========================================================================
    // Component interface
    //==========================================================================

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    //==========================================================================
    // Status API
    //==========================================================================

    /**
     * @brief Show a temporary status message
     * @param message Text to display
     * @param isError If true, display in error color
     * @param durationMs Duration in milliseconds (default 3000)
     */
    void showMessage(const juce::String& message, bool isError = false, int durationMs = 3000);

private:
    //==========================================================================
    // Internal state
    //==========================================================================

    Engine& engine_;
    
    // Status message
    juce::String currentMessage_;
    bool isErrorMessage_ = false;
    int messageTimeout_ = 0;
    float messageAlpha_ = 0.0f;

    // Cached stats
    double cpuUsage_ = 0.0;
    juce::String audioDeviceName_;
    juce::String audioSettings_;

    //==========================================================================
    // Helper methods
    //==========================================================================

    void updateStats();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithStatusBar)
};

} // namespace zenith

