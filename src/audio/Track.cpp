/**
 * Track.cpp
 * Track implementations
 */

#include "Track.h"

namespace vexel {

void AudioTrack::process(const float* const* input, float* const* output,
                        int numInputs, int numOutputs, int numSamples,
                        double currentBeat)
{
    juce::ignoreUnused(input, currentBeat);

    // Clear output if muted
    if (m_muted) {
        for (int ch = 0; ch < numOutputs; ++ch) {
            juce::FloatVectorOperations::clear(output[ch], numSamples);
        }
        return;
    }

    // TODO: Process clips at current beat position
    // TODO: Apply plugin chain
    // TODO: Apply volume and pan

    // Placeholder: silence for now
    for (int ch = 0; ch < numOutputs; ++ch) {
        juce::FloatVectorOperations::clear(output[ch], numSamples);
    }
}

void MidiTrack::process(const float* const* input, float* const* output,
                       int numInputs, int numOutputs, int numSamples,
                       double currentBeat)
{
    juce::ignoreUnused(input, currentBeat);

    // TODO: Process MIDI clips
    // TODO: Route to instrument plugin
    // TODO: Apply volume and pan

    // Placeholder: silence for now
    for (int ch = 0; ch < numOutputs; ++ch) {
        juce::FloatVectorOperations::clear(output[ch], numSamples);
    }
}

} // namespace vexel
