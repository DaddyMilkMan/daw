#include "MeteringSystem.h"

namespace zenith {

MeteringSystem::MeteringSystem() {
    analysisFifo = std::make_unique<StereoAudioFifo>(16384);
}

void MeteringSystem::process(const juce::AudioBuffer<float>& buffer) {
    // Update levels
    float magnitude = buffer.getMagnitude(0, buffer.getNumSamples());
    masterLevel.store(magnitude);
    
    if (magnitude > masterPeak.load()) {
        masterPeak.store(magnitude);
    }
    
    // Update FIFO
    analysisFifo->push(buffer);
}

} // namespace zenith
