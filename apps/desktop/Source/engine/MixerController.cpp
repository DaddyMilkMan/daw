/*
  ==============================================================================

    MixerController.cpp
    Created: 2025-12-19

  ==============================================================================
*/

#include "MixerController.h"

namespace zenith {

MixerController::MixerController() = default;
MixerController::~MixerController() = default;

void MixerController::processTracks(const std::vector<std::shared_ptr<Track>>& tracks,
                                   juce::AudioBuffer<float>& buffer,
                                   const juce::AudioSourceChannelInfo& bufferToFill) {
    juce::ignoreUnused(tracks, buffer, bufferToFill);
    // Delegate to individual track processing in Engine for now
    // This controller is for future modular processing pipelines
}

void MixerController::updateMetering(const std::vector<std::shared_ptr<Track>>& tracks,
                                    const juce::AudioBuffer<float>& buffer) {
    juce::ignoreUnused(tracks, buffer);
    // Metering is handled within each track's mixer channel
}

} // namespace zenith
