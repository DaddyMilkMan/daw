/*
  ==============================================================================

    ONNXStemSeparator.h
    Created: 2025-11-29
    Author:  Zenith DAW - AI Integration Team

    Implements the specific STFT/iSTFT pre-processing required for Demucs v4 ONNX.
    Based on sevagh/demucs.onnx implementation.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>

#ifdef ZENITH_ENABLE_ONNX
#include <onnxruntime_cxx_api.h>
#else
// Forward declare ONNX Runtime classes to avoid header dependency if not linked
namespace Ort { class Session; class Env; class MemoryInfo; }
#endif

namespace zenith {

class ONNXStemSeparator {
public:
    explicit ONNXStemSeparator(const juce::File& modelPath);
    ~ONNXStemSeparator();

    struct Stems {
        juce::AudioBuffer<float> vocals;
        juce::AudioBuffer<float> bass;
        juce::AudioBuffer<float> drums;
        juce::AudioBuffer<float> other;
    };

    /**
     * @brief Process audio using the Demucs ONNX model.
     * @param input Audio buffer (stereo)
     * @return Stems separated audio
     */
    Stems process(const juce::AudioBuffer<float>& input) const;

    bool isLoaded() const { return modelLoaded_; }

private:
    // Demucs Constants
    static constexpr int FFT_SIZE = 4096;
    static constexpr int HOP_SIZE = 1024;
    
    bool modelLoaded_ = false;
    juce::File modelPath_;

    // JUCE DSP
    // Mutable to allow const methods if perform is non-const (though it should be const)
    mutable juce::dsp::FFT fft_{ 12 }; // 2^12 = 4096
    mutable juce::dsp::WindowingFunction<float> window_;

    // Internal Helpers
    void padSignal(const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& padded) const;
    
    // STFT: Converts Time Domain -> Complex Spectrogram (Real/Imag as channels)
    // Output: [Channels * 2, FreqBins, TimeFrames]
    void computeSTFT(const juce::AudioBuffer<float>& input, std::vector<float>& outputTensor, int& numFrames) const;
    
    // iSTFT: Converts Complex Spectrogram -> Time Domain
    void computeISTFT(const std::vector<float>& inputTensor, juce::AudioBuffer<float>& output, int numFrames) const;

#ifdef ZENITH_ENABLE_ONNX
    // ONNX Runtime Members (Pointers to keep header clean)
    std::unique_ptr<Ort::Session> session_;
    std::unique_ptr<Ort::Env> env_;
#endif
};

} // namespace zenith
