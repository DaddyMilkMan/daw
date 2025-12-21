/*
  ==============================================================================

    TransportController.cpp
    Created: 2025-12-09
    Author:  Zenith DAW

    Transport controller implementation.

  ==============================================================================
*/

#include "TransportController.h"
#include "TempoMap.h"

namespace zenith {

//==============================================================================
TransportController::LoopInfo TransportController::getLoopInfo(int numSamples) const {
    LoopInfo info;
    info.currentPos = playheadSamples_.load();
    info.loopStart = loopStartSamples_.load();
    juce::int64 loopEnd = loopEndSamples_.load();

    if (!isLooping_.load() || loopEnd <= info.loopStart) {
        info.samplesBeforeLoop = numSamples;
        info.samplesAfterLoop = 0;
        info.wrapped = false;
        return info;
    }

    if (info.currentPos + numSamples > loopEnd) {
        info.samplesBeforeLoop = static_cast<int>(loopEnd - info.currentPos);
        info.samplesAfterLoop = numSamples - info.samplesBeforeLoop;
        info.wrapped = true;
    } else {
        info.samplesBeforeLoop = numSamples;
        info.samplesAfterLoop = 0;
        info.wrapped = false;
    }

    return info;
}

//==============================================================================
double TransportController::getPlayheadBeats() const {
    double sr = sampleRate_.load();
    if (sr <= 0.0) {
        return 0.0;
    }

    juce::int64 samples = playheadSamples_.load();

    // Use tempo map if available for accurate beat position
    if (tempoMap_ != nullptr) {
        return tempoMap_->samplesToBeats(samples, sr);
    }

    // Fallback: simple calculation using current tempo
    double seconds = static_cast<double>(samples) / sr;
    double bpm = tempo_.load();
    return seconds * bpm / 60.0;
}

} // namespace zenith
