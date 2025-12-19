#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <memory>
#include "../dsp/StereoAudioFifo.h"

namespace zenith {

/**
 * @class MeteringSystem
 * @brief Manages audio metering and visualizer data.
 */
class MeteringSystem {
public:
    MeteringSystem();
    ~MeteringSystem() = default;

    // Audio thread
    void process(const juce::AudioBuffer<float>& buffer);
    
    // Message thread
    float getMasterLevel() const { return masterLevel.load(); }
    float getMasterPeak() const { return masterPeak.load(); }
    void resetMasterPeak() { masterPeak.store(0.0f); }
    
    StereoAudioFifo& getAnalysisFifo() { return *analysisFifo; }

private:
    std::atomic<float> masterLevel{ 0.0f };
    std::atomic<float> masterPeak{ 0.0f };
    std::unique_ptr<StereoAudioFifo> analysisFifo;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MeteringSystem)
};

} // namespace zenith
