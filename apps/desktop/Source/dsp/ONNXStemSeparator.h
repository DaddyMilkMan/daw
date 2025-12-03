#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

namespace zenith {

/**
 * @brief Handles AI-based stem separation using ONNX Runtime
 * 
 * This class provides an interface for running Demucs or similar models
 * to separate audio into stems (Vocals, Drums, Bass, Other).
 */
class ONNXStemSeparator {
public:
    ONNXStemSeparator();
    ~ONNXStemSeparator();

    /**
     * @brief Check if the ONNX runtime is available and supported
     */
    bool isAvailable() const;

    /**
     * @brief Initialize the engine with a specific model file
     */
    bool initialize(const juce::File& modelPath);
    
    struct SeparationResult {
        juce::AudioBuffer<float> vocals;
        juce::AudioBuffer<float> drums;
        juce::AudioBuffer<float> bass;
        juce::AudioBuffer<float> other;
        bool success = false;
        juce::String error;
    };

    /**
     * @brief Perform separation on an audio buffer
     * @param input Stereo input buffer
     * @param sampleRate Sample rate of the audio
     * @return SeparationResult containing the stems
     */
    SeparationResult separate(const juce::AudioBuffer<float>& input, double sampleRate);

private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;
};

}
