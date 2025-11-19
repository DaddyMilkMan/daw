/**
 * @file MixerView.h
 * @brief Mixer view UI component
 *
 * Displays all mixer channels in a horizontal layout.
 * Automatically updates when tracks are added/removed from the engine.
 *
 * Thread Safety:
 * - All UI operations on MESSAGE THREAD
 * - Reads from Engine via const methods (thread-safe)
 */

#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>

// Forward declarations
class Engine;
class MixerChannelComponent;

//==============================================================================
/**
 * @class MixerView
 * @brief Container for all mixer channels
 *
 * Displays one MixerChannelComponent per track in the engine.
 * Updates automatically when tracks are added or removed.
 */
class MixerView : public juce::Component,
                  private juce::Timer
{
public:
    //==========================================================================
    /**
     * @brief Constructor
     * @param engine Reference to the audio engine
     */
    explicit MixerView(Engine& engine);
    ~MixerView() override;

    //==========================================================================
    // Component interface
    //==========================================================================

    void paint(juce::Graphics& g) override;
    void resized() override;

    //==========================================================================
    // Mixer operations
    //==========================================================================

    /**
     * @brief Rebuild channel strips from engine tracks
     *
     * Called when tracks are added/removed from the engine.
     * Clears existing channels and creates new ones.
     */
    void rebuildChannels();

private:
    //==========================================================================
    // Timer interface (for periodic updates)
    //==========================================================================

    void timerCallback() override;

    //==========================================================================
    // Member variables
    //==========================================================================

    Engine& engine_;

    // Channel strips (one per track)
    std::vector<std::unique_ptr<MixerChannelComponent>> channels_;

    // Viewport for scrolling when there are many tracks
    juce::Viewport viewport_;
    juce::Component channelContainer_;

    // State tracking
    int lastTrackCount_ = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerView)
};
