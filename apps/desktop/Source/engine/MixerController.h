/*
  ==============================================================================

    MixerController.h
    Created: 2025-12-19
    Author:  Zenith DAW AI Team

    Handles multi-track audio processing, including gain, panning, and metering.
    Decoupled from Engine for better modularity.

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <memory>
#include "../engine/Track.h"

namespace zenith {

class MixerController {
public:
    MixerController();
    ~MixerController();

    /**
     * @brief Process audio for a collection of tracks
     * @param tracks The tracks to process
     * @param buffer Output buffer to mix into
     * @param context Audio processing context
     */
    void processTracks(const std::vector<std::shared_ptr<Track>>& tracks,
                       juce::AudioBuffer<float>& buffer,
                       const juce::AudioSourceChannelInfo& bufferToFill);

    /**
     * @brief Update levels/metering for all tracks
     */
    void updateMetering(const std::vector<std::shared_ptr<Track>>& tracks,
                        const juce::AudioBuffer<float>& buffer);

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerController)
};

} // namespace zenith
