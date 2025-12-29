/*
  ==============================================================================
    ReferenceMatcher.h
    Reference track matching with efficient audio fingerprinting
    Uses compressed spectral analysis for minimal storage
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>
#include "GrokJourney.h"
#include <memory>

namespace zenith {
namespace ai {

class ReferenceMatcher {
public:
    ReferenceMatcher();
    ~ReferenceMatcher();

    // Analyze reference track and create fingerprint
    ReferenceFingerprint analyzeReference(const juce::AudioBuffer<float>& audio,
                                        double sampleRate,
                                        const juce::String& trackName = "",
                                        const juce::String& genre = "",
                                        const juce::String& mood = "");

    // Find similar reference tracks
    std::vector<ReferenceFingerprint> findSimilarTracks(const juce::AudioBuffer<float>& queryAudio,
                                                        double sampleRate,
                                                        GrokJourney& journey,
                                                        int maxResults = 5);

    // Extract target settings from reference
    juce::var extractTargetSettings(const ReferenceFingerprint& fingerprint);

    // Compare two fingerprints (0.0 to 1.0 similarity)
    double compareFingerprints(const ReferenceFingerprint& a, const ReferenceFingerprint& b);

private:
    // Analysis parameters
    static constexpr int fftOrder = 11;  // 2048 samples
    static constexpr int numBands = 32;   // Reduced for efficiency
    static constexpr float analysisDuration = 30.0f;  // Analyze first 30 seconds

    // DSP components
    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;

    // Analysis methods
    std::vector<float> extractSpectralCentroid(const juce::AudioBuffer<float>& audio, double sampleRate);
    std::vector<float> extractRMSProfile(const juce::AudioBuffer<float>& audio);
    std::vector<float> extractStereoWidth(const juce::AudioBuffer<float>& audio);

    // Helper methods
    std::vector<float> downsampleForAnalysis(const std::vector<float>& data, int targetSize);
    float calculateSpectralCentroid(const std::vector<float>& magnitudes);
    double calculateSimilarity(const std::vector<float>& a, const std::vector<float>& b);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReferenceMatcher)
};

} // namespace ai
} // namespace zenith
