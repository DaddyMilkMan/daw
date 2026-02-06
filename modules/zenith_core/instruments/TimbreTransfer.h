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

    TimbreTransfer.h
    Created: 2025-02-05
    Author:  Zenith DAW

    Real-time timbre extraction and transfer between audio sources.
    Features neural style transfer, spectral morphing, and timbre interpolation.

    ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_core/juce_core.h>
#include <memory>
#include <vector>

#ifdef ZENITH_USE_ONNX_RUNTIME
#include <onnxruntime_cxx_api.h>
#endif

namespace zenith {

//==============================================================================
// Timbre Analysis Result
//==============================================================================

struct TimbreAnalysis {
    // Spectral features
    std::vector<float> spectralCentroid;
    std::vector<float> spectralRolloff;
    std::vector<float> spectralFlux;
    std::vector<float> spectralContrast;
    std::vector<float> spectralBandwidth;

    // MFCCs (Mel-Frequency Cepstral Coefficients)
    std::vector<float> mfcc;
    static constexpr int numMFCC = 13;

    // Chroma features
    std::vector<float> chroma;
    static constexpr int numChroma = 12;

    // Temporal features
    float zeroCrossingRate{0.0f};
    float energy{0.0f};
    float rms{0.0f};

    // Harmonic features
    float harmonicity{0.0f};
    float inharmonicity{0.0f};
    std::vector<float> harmonicPeaks;

    // Neural embedding
    std::vector<float> neuralEmbedding;
    static constexpr int embeddingDim = 128;

    // Metadata
    double analysisTime{0.0};
    bool valid{false};
};

//==============================================================================
// Timbre Transfer Parameters
//==============================================================================

struct TimbreTransferParams {
    // Transfer method
    enum class Method {
        Neural,         // Neural network-based transfer
        Spectral,       // Spectral envelope replacement
        Hybrid,         // Combination of neural and spectral
        Morphing        // Timbre morphing
    };
    Method method{Method::Hybrid};

    // Transfer amount
    float transferAmount{0.5f};         // 0=source, 1=target

    // Spectral parameters
    float spectralSmoothing{0.3f};      // Envelope smoothing
    float harmonicMatch{0.7f};          // Harmonic structure matching
    float noiseMatching{0.5f};          // Noise component matching

    // Morphing parameters
    float morphPosition{0.5f};          // Position between source and target
    float morphSmoothness{0.5f};        // Interpolation smoothness

    // Neural parameters
    float temperature{0.7f};            // Sampling temperature
    float styleStrength{0.8f};          // Style transfer strength

    // Real-time parameters
    bool realTimeMode{true};
    int fftSize{2048};
    int hopSize{512};
};

//==============================================================================
// Timbre Database Entry
//==============================================================================

struct TimbrePreset {
    juce::String name;
    juce::String category;
    TimbreAnalysis timbre;
    juce::String filepath;

    // Metadata
    juce::String description;
    juce::Array<juce::String> tags;
};

//==============================================================================
/**
    Real-time timbre extraction and transfer engine

    This class provides:
    - Timbre feature extraction from audio
    - Neural network-based style transfer
    - Spectral envelope manipulation
    - Timbre morphing and interpolation
    - Preset management
*/
class TimbreTransfer {
public:
    //==========================================================================
    // Construction/Destruction
    //==========================================================================

    TimbreTransfer();
    ~TimbreTransfer();

    //==========================================================================
    // Initialization
    //==========================================================================

    /**
        Initialize the timbre transfer engine

        @param sampleRate Audio sample rate
        @param maxBlockSize Maximum block size
        @return true if initialization succeeded
    */
    bool initialize(double sampleRate, int maxBlockSize);

    /**
        Release resources
    */
    void release();

    /**
        Check if ready
    */
    bool isReady() const { return initialized_; }

    //==========================================================================
    // Timbre Analysis
    //==========================================================================

    /**
        Analyze timbre from audio buffer

        @param audio Input audio buffer
        @return Timbre analysis results
    */
    TimbreAnalysis analyzeTimbre(const juce::AudioBuffer<float>& audio);

    /**
        Analyze timbre from file

        @param audioFile Path to audio file
        @return Timbre analysis results
    */
    TimbreAnalysis analyzeTimbreFromFile(const juce::File& audioFile);

    /**
        Real-time timbre analysis (streaming)

        @param audio Input audio block
        @return Current timbre analysis
    */
    TimbreAnalysis analyzeRealTime(const juce::dsp::AudioBlock<float>& audio);

    //==========================================================================
    // Timbre Transfer
    //==========================================================================

    /**
        Apply timbre transfer to audio

        @param sourceAudio Source audio to modify
        @param targetTimbre Target timbre characteristics
        @param params Transfer parameters
        @return Processed audio with transferred timbre
    */
    juce::AudioBuffer<float> applyTimbreTransfer(
        const juce::AudioBuffer<float>& sourceAudio,
        const TimbreAnalysis& targetTimbre,
        const TimbreTransferParams& params);

    /**
        Real-time timbre transfer (streaming)

        @param audioBlock Audio block to process
        @param targetTimbre Target timbre
        @param params Transfer parameters
    */
    void processRealTime(juce::dsp::AudioBlock<float>& audioBlock,
                        const TimbreAnalysis& targetTimbre,
                        const TimbreTransferParams& params);

    /**
        Transfer timbre from one audio to another

        @param source Source audio
        @param target Target audio to extract timbre from
        @return Audio with source content + target timbre
    */
    juce::AudioBuffer<float> transferBetweenAudio(
        const juce::AudioBuffer<float>& source,
        const juce::AudioBuffer<float>& target);

    //==========================================================================
    // Timbre Morphing
    //==========================================================================

    /**
        Morph between two timbres

        @param timbreA First timbre
        @param timbreB Second timbre
        @param morphPosition Morph position (0=A, 1=B)
        @return Morphed timbre analysis
    */
    TimbreAnalysis morphTimbres(const TimbreAnalysis& timbreA,
                               const TimbreAnalysis& timbreB,
                               float morphPosition);

    /**
        Apply morphing to audio

        @param audio Input audio
        @param timbreA First timbre
        @param timbreB Second timbre
        @param morphPosition Morph position
        @return Morphed audio
    */
    juce::AudioBuffer<float> applyMorphing(
        const juce::AudioBuffer<float>& audio,
        const TimbreAnalysis& timbreA,
        const TimbreAnalysis& timbreB,
        float morphPosition);

    //==========================================================================
    // Preset Management
    //==========================================================================

    /**
        Save current timbre as preset

        @param name Preset name
        @param timbre Timbre to save
        @param category Optional category
    */
    void savePreset(const juce::String& name,
                   const TimbreAnalysis& timbre,
                   const juce::String& category = "");

    /**
        Load timbre preset

        @param name Preset name
        @return Loaded timbre or invalid if not found
    */
    TimbreAnalysis loadPreset(const juce::String& name);

    /**
        Get all available presets
    */
    juce::Array<TimbrePreset> getPresets() const;

    /**
        Delete a preset
    */
    void deletePreset(const juce::String& name);

    /**
        Export presets to file
    */
    bool exportPresets(const juce::File& outputFile);

    /**
        Import presets from file
    */
    bool importPresets(const juce::File& inputFile);

    //==========================================================================
    // Neural Network Integration
    //==========================================================================

    /**
        Load neural style transfer model

        @param modelPath Path to ONNX model
        @return true if loaded successfully
    */
    bool loadNeuralModel(const juce::File& modelPath);

    /**
        Use neural network for timbre transfer

        @param source Source audio
        @param targetTimbre Target timbre embedding
        @return Processed audio
    */
    juce::AudioBuffer<float> neuralTransfer(
        const juce::AudioBuffer<float>& source,
        const std::vector<float>& targetEmbedding);

    /**
        Extract neural embedding from audio

        @param audio Input audio
        @return Neural embedding vector
    */
    std::vector<float> extractEmbedding(const juce::AudioBuffer<float>& audio);

    //==========================================================================
    // Utilities
    //==========================================================================

    /**
        Calculate similarity between two timbres

        @param timbreA First timbre
        @param timbreB Second timbre
        @return Similarity score (0-1, higher is more similar)
    */
    float calculateSimilarity(const TimbreAnalysis& timbreA,
                             const TimbreAnalysis& timbreB);

    /**
        Find most similar timbre in database

        @param query Query timbre
        @return Name of most similar preset
    */
    juce::String findMostSimilar(const TimbreAnalysis& query);

    /**
        Get default timbre transfer parameters
    */
    static TimbreTransferParams getDefaultParams();

    /**
        Check if neural model is available
    */
    bool isNeuralAvailable() const { return neuralModelLoaded_; }

private:
    //==========================================================================
    // Internal Analysis
    //==========================================================================

    /**
        Extract spectral features
    */
    void extractSpectralFeatures(const juce::AudioBuffer<float>& audio,
                                TimbreAnalysis& analysis);

    /**
        Extract MFCCs
    */
    void extractMFCCs(const juce::AudioBuffer<float>& audio,
                     TimbreAnalysis& analysis);

    /**
        Extract chroma features
    */
    void extractChroma(const juce::AudioBuffer<float>& audio,
                      TimbreAnalysis& analysis);

    /**
        Extract harmonic features
    */
    void extractHarmonicFeatures(const juce::AudioBuffer<float>& audio,
                                TimbreAnalysis& analysis);

    //==========================================================================
    // Internal Transfer
    //==========================================================================

    /**
        Spectral envelope-based transfer
    */
    juce::AudioBuffer<float> spectralTransfer(
        const juce::AudioBuffer<float>& source,
        const TimbreAnalysis& targetTimbre,
        const TimbreTransferParams& params);

    /**
        Extract spectral envelope
    */
    std::vector<float> extractSpectralEnvelope(
        const juce::AudioBuffer<float>& audio);

    /**
        Apply spectral envelope
    */
    void applySpectralEnvelope(juce::AudioBuffer<float>& audio,
                              const std::vector<float>& envelope,
                              float amount);

    //==========================================================================
    // Neural Network Processing
    //==========================================================================

    /**
        Run neural network inference
    */
    std::vector<float> runNeuralInference(
        const std::vector<float>& inputFeatures);

    //==========================================================================
    // Morphing
    //==========================================================================

    /**
        Interpolate between timbre feature vectors
    */
    std::vector<float> interpolateFeatures(
        const std::vector<float>& a,
        const std::vector<float>& b,
        float t);

    /**
        Spherical interpolation for timbre vectors
    */
    std::vector<float> slerpTimbres(
        const std::vector<float>& a,
        const std::vector<float>& b,
        float t);

    //==========================================================================
    // FFT Processing
    //==========================================================================

    /**
        Perform FFT analysis
    */
    void performFFT(const juce::AudioBuffer<float>& audio,
                   std::vector<std::complex<float>>& fftResult);

    /**
        Perform IFFT synthesis
    */
    void performIFFT(const std::vector<std::complex<float>>& fftData,
                    juce::AudioBuffer<float>& audio);

    //==========================================================================
    // Member Variables
    //==========================================================================

    // Initialization state
    bool initialized_{false};
    double sampleRate_{44100.0};
    int maxBlockSize_{512};

    // FFT processing
    juce::dsp::FFT fft_;
    juce::dsp::WindowingFunction<float> window_;
    std::vector<std::complex<float>> fftBuffer_;
    std::vector<float> magnitudeBuffer_;
    std::vector<float> phaseBuffer_;

    // MFCC computation
    std::vector<float> melFilterbank_;
    std::vector<float> dctMatrix_;

    // Neural network
#ifdef ZENITH_USE_ONNX_RUNTIME
    std::unique_ptr<Ort::Env> env_;
    std::unique_ptr<Ort::Session> session_;
    std::unique_ptr<Ort::SessionOptions> sessionOptions_;
    std::unique_ptr<Ort::MemoryInfo> memoryInfo_;
#endif
    bool neuralModelLoaded_{false};

    // Preset database
    juce::Array<TimbrePreset> presets_;
    juce::File presetDatabasePath_;

    // Real-time state
    TimbreAnalysis currentAnalysis_;
    std::vector<TimbreAnalysis> analysisHistory_;
    static constexpr int historySize = 10;

    // Processing buffers
    juce::AudioBuffer<float> processBuffer_;
    std::vector<float> featureBuffer_;

    // Thread safety
    juce::CriticalSection transferLock_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimbreTransfer)
};

//==============================================================================
/**
    Factory for creating timbre transfer presets and tools
*/
class TimbreTransferFactory {
public:
    /**
        Create preset from audio file

        @param audioFile Path to audio file
        @param name Preset name
        @return Timbre preset
    */
    static TimbrePreset createPresetFromFile(const juce::File& audioFile,
                                            const juce::String& name);

    /**
        Create built-in presets

        @return Array of default presets
    */
    static juce::Array<TimbrePreset> createBuiltInPresets();

    /**
        Create procedural timbre (synthetic)

        @param type Type of timbre to create
        @return Timbre analysis for synthetic timbre
    */
    static TimbreAnalysis createProceduralTimbre(const juce::String& type);

private:
    /**
        Generate synthetic spectral envelope
    */
    static std::vector<float> generateSpectralEnvelope(
        const juce::String& type,
        int fftSize);
};

} // namespace zenith
