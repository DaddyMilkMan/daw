/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

    ==============================================================================

    NeuralSynthEngine.h
    Created: 2025-02-05
    Author:  Zenith DAW

    Real-time neural audio synthesis engine with ONNX Runtime integration.
    Features low-latency neural synthesis, MPE support, polyphony, timbre
    transfer, and AI-powered parameter suggestions.

    ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_core/juce_core.h>
#include <atomic>
#include <memory>
#include <vector>

#ifdef ZENITH_USE_ONNX_RUNTIME
#include <onnxruntime_cxx_api.h>
#endif

namespace zenith {

//==============================================================================
// Neural Synthesis Model Types
//==============================================================================

enum class NeuralModelType {
    WaveNet,            // WaveNet-based autoregressive synthesis
    DDSP,               // Differentiable Digital Signal Processing
    NSynth,             // Google NSynth style encoder-decoder
    Rave,              // Real-time Audio Variational autoEncoder
    TimbreTransfer,     // Neural timbre transfer
    Custom              // User-provided ONNX model
};

enum class NeuralSynthesisMode {
    Generative,         // Pure neural generation
    Hybrid,            // Neural + traditional synthesis
    Transfer,          // Timbre transfer from source
    Interactive        // Interactive parameter control
};

//==============================================================================
// Neural Audio Parameters
//==============================================================================

struct NeuralSynthParameters {
    // Model configuration
    NeuralModelType modelType{NeuralModelType::DDSP};
    juce::String modelPath;
    float complexity{0.5f};          // Model complexity (0-1)
    float temperature{0.7f};         // Sampling temperature
    float noiseScale{0.1f};          // Added noise for variation

    // Synthesis parameters
    float fundamentalFreq{440.0f};
    float brightness{0.5f};          // Spectral brightness
    float spectralContrast{0.5f};    // Spectral contrast
    float modulationDepth{0.3f};     // Modulation amount
    float harmonicity{0.5f};         // Harmonic vs inharmonic

    // Timbre parameters
    float timbreVector[16]{0.0f};    // Neural timbre embedding
    float morphPosition{0.0f};       // Morphing between timbres

    // MPE control
    float mpePressure{0.0f};
    float mpeTimbre{0.0f};
    float mpePitchBend{0.0f};

    // Performance
    bool lowLatencyMode{true};
    int maxVoices{16};
    float cpuLimit{0.8f};            // CPU usage limit (0-1)
};

//==============================================================================
// Latent Space Representation
//==============================================================================

struct LatentSpace {
    static constexpr int latentDim = 512;
    std::vector<float> z;           // Latent vector
    std::vector<float> zMean;        // Mean for sampling
    std::vector<float> zLogVar;      // Log variance for sampling

    void resize(int size) {
        z.resize(size);
        zMean.resize(size);
        zLogVar.resize(size);
    }

    void clear() {
        std::fill(z.begin(), z.end(), 0.0f);
        std::fill(zMean.begin(), zMean.end(), 0.0f);
        std::fill(zLogVar.begin(), zLogVar.end(), 0.0f);
    }
};

//==============================================================================
// Neural Voice State
//==============================================================================

struct NeuralVoice {
    bool active{false};
    int midiNote{0};
    float frequency{440.0f};
    float velocity{0.0f};
    float amplitude{0.0f};

    // Voice-specific latent space
    LatentSpace latent;
    juce::ADSR envelope;

    // Phase and modulation
    double phase{0.0};
    float modPhase{0.0f};

    // Output buffer
    juce::AudioBuffer<float> voiceBuffer{2, 512};

    void clear() {
        active = false;
        amplitude = 0.0f;
        phase = 0.0;
        modPhase = 0.0f;
        voiceBuffer.clear();
    }
};

//==============================================================================
// Model Inference Result
//==============================================================================

struct NeuralInferenceResult {
    juce::AudioBuffer<float> audio;
    bool success{false};
    double inferenceTimeMs{0.0};
    float confidence{0.0f};
    juce::String errorMessage;

    NeuralInferenceResult() : audio(2, 512) {}
};

//==============================================================================
/**
    Real-time neural audio synthesis engine

    This class provides a complete neural synthesis engine with:
    - ONNX Runtime integration for model inference
    - Low-latency real-time synthesis
    - MPE support with polyphony
    - Timbre transfer capabilities
    - SIMD-optimized processing
    - Adaptive quality based on CPU load
*/
class NeuralSynthEngine {
public:
    //==========================================================================
    // Construction/Destruction
    //==========================================================================

    NeuralSynthEngine();
    ~NeuralSynthEngine();

    //==========================================================================
    // Initialization
    //==========================================================================

    /**
        Initialize the neural engine with a specific model

        @param modelPath Path to ONNX model file
        @param sampleRate Audio sample rate
        @param maxBlockSize Maximum expected block size
        @return true if initialization succeeded
    */
    bool initialize(const juce::File& modelPath, double sampleRate, int maxBlockSize);

    /**
        Initialize with default model search paths

        @param modelType Type of neural model to use
        @param sampleRate Audio sample rate
        @param maxBlockSize Maximum expected block size
        @return true if initialization succeeded
    */
    bool initializeWithType(NeuralModelType modelType, double sampleRate, int maxBlockSize);

    /**
        Release resources and unload model
    */
    void release();

    /**
        Check if engine is ready for synthesis
    */
    bool isReady() const { return initialized_ && modelLoaded_; }

    //==========================================================================
    // Real-time Synthesis
    //==========================================================================

    /**
        Render audio output for all active voices

        @param outputBuffer Output buffer to fill
        @param startSample Starting sample index
        @param numSamples Number of samples to render
        @param midiMessages MIDI messages for this block
    */
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                        int startSample,
                        int numSamples,
                        const juce::MidiBuffer& midiMessages);

    /**
        Note-on event (MPE-aware)

        @param midiNote MIDI note number
        @param velocity Note velocity (0-1)
        @param pitchBend Initial pitch bend
    */
    void noteOn(int midiNote, float velocity, float pitchBend = 0.0f);

    /**
        Note-off event

        @param midiNote MIDI note number
        @param allowTailOff Whether to allow release envelope
    */
    void noteOff(int midiNote, bool allowTailOff);

    /**
        MPE pressure change

        @param midiNote MIDI note number
        @param pressure Pressure value (0-1)
    */
    void setPressure(int midiNote, float pressure);

    /**
        MPE timbre change (slide)

        @param midiNote MIDI note number
        @param timbre Timbre value (0-1)
    */
    void setTimbre(int midiNote, float timbre);

    /**
        MPE pitch bend

        @param midiNote MIDI note number
        @param pitchBend Pitch bend amount (-1 to 1)
    */
    void setPitchBend(int midiNote, float pitchBend);

    //==========================================================================
    // Parameters
    //==========================================================================

    /**
        Update synthesis parameters

        @param params New parameter set
    */
    void setParameters(const NeuralSynthParameters& params);

    /**
        Get current parameters
    */
    const NeuralSynthParameters& getParameters() const { return parameters_; }

    /**
        Set individual parameter value

        @param parameterIndex Parameter index
        @param value New value
    */
    void setParameterValue(int parameterIndex, float value);

    /**
        Get timbre embedding vector
    */
    const float* getTimbreVector() const { return parameters_.timbreVector; }

    /**
        Set timbre embedding vector
    */
    void setTimbreVector(const float* vector, int size);

    //==========================================================================
    // Timbre Transfer
    //==========================================================================

    /**
        Extract timbre from audio buffer

        @param audio Input audio buffer
        @return Timbre embedding vector
    */
    std::vector<float> extractTimbre(const juce::AudioBuffer<float>& audio);

    /**
        Apply timbre transfer to synthesis

        @param sourceTimbre Source timbre embedding
        @param transferAmount How much to transfer (0-1)
    */
    void applyTimbreTransfer(const std::vector<float>& sourceTimbre, float transferAmount);

    /**
        Morph between two timbres

        @param timbreA First timbre embedding
        @param timbreB Second timbre embedding
        @param morphPosition Morph position (0=A, 1=B)
    */
    void morphTimbres(const std::vector<float>& timbreA,
                     const std::vector<float>& timbreB,
                     float morphPosition);

    //==========================================================================
    // AI-Powered Suggestions
    //==========================================================================

    /**
        Get parameter suggestions based on target sound

        @param targetDescription Description of target sound (e.g., "warm pad")
        @return Suggested parameter values
    */
    NeuralSynthParameters getSuggestedParameters(const juce::String& targetDescription);

    /**
        Learn from user feedback (reinforcement learning)

        @param parameters Parameters that produced good result
        @param reward Reward value (0-1, higher is better)
    */
    void provideFeedback(const NeuralSynthParameters& parameters, float reward);

    /**
        Get random parameter variation (exploration)

        @param baseParameters Base parameters to vary
        @param variationAmount Amount of variation (0-1)
        @return Varied parameters
    */
    NeuralSynthParameters exploreVariations(const NeuralSynthParameters& baseParameters,
                                           float variationAmount);

    //==========================================================================
    // Performance Monitoring
    //==========================================================================

    /**
        Get current CPU usage (0-1)
    */
    float getCpuUsage() const { return cpuUsage_; }

    /**
        Get average inference time in milliseconds
    */
    float getAverageInferenceTime() const { return avgInferenceTime_; }

    /**
        Get number of active voices
    */
    int getNumActiveVoices() const { return activeVoiceCount_; }

    /**
        Enable/disable adaptive quality
    */
    void setAdaptiveQuality(bool enabled) { adaptiveQualityEnabled_ = enabled; }

    //==========================================================================
    // Model Management
    //==========================================================================

    /**
        Load a new ONNX model

        @param modelPath Path to ONNX model file
        @return true if loaded successfully
    */
    bool loadModel(const juce::File& modelPath);

    /**
        Get information about loaded model
    */
    juce::String getModelInfo() const;

    /**
        Check if ONNX Runtime is available
    */
    static bool isONNXRuntimeAvailable();

    //==========================================================================
    // State Management
    //==========================================================================

    /**
        Reset all voices and clear state
    */
    void reset();

    /**
        Save current state to memory block
    */
    void getState(juce::MemoryBlock& block);

    /**
        Restore state from memory block
    */
    void setState(const void* data, int sizeInBytes);

private:
    //==========================================================================
    // Internal Processing
    //==========================================================================

    /**
        Process MIDI messages for this block
    */
    void processMidi(const juce::MidiBuffer& midiMessages, int numSamples);

    /**
        Render single voice output
    */
    void renderVoice(NeuralVoice& voice, juce::AudioBuffer<float>& buffer, int numSamples);

    /**
        Perform neural network inference
    */
    NeuralInferenceResult runInference(const NeuralVoice& voice, int numSamples);

    /**
        Generate audio using traditional DSP synthesis (fallback)
    */
    void generateDSPAudio(NeuralVoice& voice, juce::AudioBuffer<float>& buffer, int numSamples);

    /**
        Process latent space interpolation
    */
    void processLatentSpace(NeuralVoice& voice, int numSamples);

    /**
        Apply SIMD-optimized post-processing
    */
    void postProcess(juce::AudioBuffer<float>& buffer, int numSamples);

    //==========================================================================
    // Voice Management
    //==========================================================================

    /**
        Find free voice slot
    */
    int findFreeVoice() const;

    /**
        Find voice by MIDI note
    */
    NeuralVoice* findVoice(int midiNote);

    /**
        Steal oldest voice if needed
    */
    void stealVoice();

    //==========================================================================
    // Neural Network Operations
    //==========================================================================

    /**
        Encode audio to latent space
    */
    std::vector<float> encodeToLatent(const juce::AudioBuffer<float>& audio);

    /**
        Decode latent space to audio
    */
    juce::AudioBuffer<float> decodeFromLatent(const std::vector<float>& latent);

    /**
        Sample from latent distribution (VAE-style)
    */
    void sampleLatent(LatentSpace& latent);

    //==========================================================================
    // AI/ML Helper Methods
    //==========================================================================

    /**
        Interpolate between latent vectors
    */
    std::vector<float> slerp(const std::vector<float>& a,
                            const std::vector<float>& b,
                            float t);

    /**
        Normalize latent vector
    */
    void normalizeLatent(std::vector<float>& latent);

    /**
        Add controlled noise to latent vector
    */
    void addLatentNoise(std::vector<float>& latent, float amount);

    //==========================================================================
    // Performance Optimization
    //==========================================================================

    /**
        Update adaptive quality based on CPU load
    */
    void updateAdaptiveQuality();

    /**
        Pre-compute frequently used values
    */
    void preComputeValues();

    //==========================================================================
    // Member Variables
    //==========================================================================

    // Initialization state
    bool initialized_{false};
    bool modelLoaded_{false};
    double sampleRate_{44100.0};
    int maxBlockSize_{512};

    // Neural synthesis parameters
    NeuralSynthParameters parameters_;

    // Voice management
    static constexpr int maxVoices_ = 16;
    std::array<NeuralVoice, maxVoices_> voices_;
    std::atomic<int> activeVoiceCount_{0};

    // ONNX Runtime integration
#ifdef ZENITH_USE_ONNX_RUNTIME
    std::unique_ptr<Ort::Env> env_;
    std::unique_ptr<Ort::Session> session_;
    std::unique_ptr<Ort::SessionOptions> sessionOptions_;
    std::unique_ptr<Ort::MemoryInfo> memoryInfo_;

    // Model metadata
    std::vector<const char*> inputNames_;
    std::vector<const char*> outputNames_;
    std::vector<int64_t> inputShape_;
    std::vector<int64_t> outputShape_;
#endif

    // Inference buffers
    juce::AudioBuffer<float> inferenceBuffer_;
    std::vector<float> inputTensorData_;
    std::vector<float> outputTensorData_;

    // Latent space cache
    LatentSpace cachedLatent_;

    // Performance monitoring
    std::atomic<float> cpuUsage_{0.0f};
    float avgInferenceTime_{0.0f};
    juce::Array<double> inferenceTimes_;
    double maxInferenceTime_{0.0};

    // Adaptive quality
    bool adaptiveQualityEnabled_{true};
    QualityPreset currentQuality_{QualityPreset::Medium};

    // AI learning
    struct ParameterHistory {
        NeuralSynthParameters params;
        float reward;
        juce::uint64 timestamp;
    };
    juce::Array<ParameterHistory> learningHistory_;
    juce::Array<NeuralSynthParameters> suggestedPresets_;

    // Thread safety
    juce::CriticalSection engineLock_;

    // Friends for testing
    friend class NeuralSynthEngineTest;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NeuralSynthEngine)
};

//==============================================================================
/**
    Factory for creating and managing neural synthesis models
*/
class NeuralSynthFactory {
public:
    /**
        Create pre-trained model configuration

        @param type Type of model
        @return File path to default model for this type
    */
    static juce::File findDefaultModel(NeuralModelType type);

    /**
        Validate ONNX model file

        @param modelFile Path to model file
        @return true if valid ONNX model
    */
    static bool validateModel(const juce::File& modelFile);

    /**
        Get recommended buffer size for model

        @param type Model type
        @return Recommended buffer size in samples
    */
    static int getRecommendedBufferSize(NeuralModelType type);

    /**
        Get expected latency for model type

        @param type Model type
        @return Expected latency in milliseconds
    */
    static float getExpectedLatency(NeuralModelType type);
};

} // namespace zenith
