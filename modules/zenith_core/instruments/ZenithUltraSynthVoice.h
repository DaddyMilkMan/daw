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

    ZenithUltraSynthVoice.h
    Created: 2025-02-05
    Author:  Zenith DAW

    Phase 1.2: Header for ZenithUltraSynthVoice - Advanced Polyphonic Voice
    supporting multiple synthesis engines with AI-powered modulation and
    cross-engine capabilities.

    ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <memory>

namespace zenith {

// Forward declarations
class PhysicalModelEngine;
class NeuralSynthesisEngine;
class HybridWavetableEngine;
class UltraSynthParameterManager;

// Synthesis Engine Types
enum class SynthesisEngineType {
    PhysicalModel,
    NeuralSynthesis,
    HybridWavetable,
    CrossEngine
};

// Voice State
enum class VoiceState {
    Idle,
    Attack,
    Decay,
    Sustain,
    Release,
    Finished
};

// Cross-Engine Modulation Types
enum class CrossEngineModulation {
    PhysicalToNeural,
    NeuralToPhysical,
    WavetableToNeural,
    PhysicalToWavetable,
    NeuralToWavetable,
    Bidirectional
};

//==============================================================================
/**
    ZenithUltraSynthVoice - Advanced polyphonic voice with multi-engine support
    
    This voice class can operate in different synthesis modes and supports
    cross-engine modulation, AI-powered parameter smoothing, and advanced
    envelope following for realistic sound design.
*/
class ZenithUltraSynthVoice : public juce::MPESynthesiserVoice {
public:
    ZenithUltraSynthVoice();
    ~ZenithUltraSynthVoice() override = default;

    // MPE Overrides - RT-SAFE operations
    void noteStarted() override;
    void noteStopped(bool allowTailOff) override;
    void notePressureChanged() override;
    void notePitchbendChanged() override;
    void noteTimbreChanged() override;
    void noteKeyStateChanged() override;

    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

    //==========================================================================
    // Engine Configuration and Control
    //==========================================================================
    
    // Primary synthesis engine
    void setPrimaryEngine(SynthesisEngineType engine) { primaryEngine_ = engine; }
    SynthesisEngineType getPrimaryEngine() const { return primaryEngine_; }
    
    // Cross-engine modulation setup
    void enableCrossEngineModulation(CrossEngineModulation type, float amount);
    void disableCrossEngineModulation(CrossEngineModulation type);
    float getCrossEngineModulation(CrossEngineModulation type) const;
    
    // Engine-specific parameters
    void configurePhysicalModelEngine(const juce::String& modelType, double parameters);
    void configureNeuralSynthesisEngine(const juce::String& modelPath, float complexity);
    void configureWavetableEngine(const juce::String& wavetablePath, float morphingRate);
    
    //==========================================================================
    // Voice Control and Parameters
    //==========================================================================
    
    // Voice state management
    VoiceState getVoiceState() const { return voiceState_; }
    void setVoiceState(VoiceState state) { voiceState_ = state; }
    
    // AI-powered parameter smoothing
    void setAISmoothing(bool enabled) { aiSmoothingEnabled_ = enabled; }
    void setAILearningRate(float rate) { aiLearningRate_ = rate; }
    void updateAIParameters(const float* inputBuffer, int numSamples);
    
    // Multi-engine mixing
    void setEngineMix(SynthesisEngineType engine, float level);
    float getEngineMix(SynthesisEngineType engine) const;
    
    // Advanced envelope following
    void enableEnvelopeFollowing(bool enabled) { envelopeFollowingEnabled_ = enabled; }
    float getEnvelopeFollowingLevel() const { return envelopeFollowingLevel_; }
    
    // Real-time analysis
    void enableRealTimeAnalysis(bool enabled) { realTimeAnalysisEnabled_ = enabled; }
    void getRealTimeAnalysis(float* analysisData, int numBands);
    
    //==========================================================================
    // Performance Optimization
    //==========================================================================
    
    // Adaptive quality based on CPU load
    void setAdaptiveQuality(bool enabled) { adaptiveQualityEnabled_ = enabled; }
    void updateQualityBasedOnPerformance(double cpuLoad);
    
    // Memory optimization
    void optimizeMemoryUsage();
    void prefetchModelData();
    
    // State serialization for voice stealing
    void serializeState(juce::MemoryBlock& block) const;
    void deserializeState(const juce::MemoryBlock& block);

private:
    //==========================================================================
    // Core Components
    //==========================================================================
    
    // Synthesis engines (initialized on-demand)
    std::unique_ptr<PhysicalModelEngine> physicalEngine_;
    std::unique_ptr<NeuralSynthesisEngine> neuralEngine_;
    std::unique_ptr<HybridWavetableEngine> wavetableEngine_;
    
    // Voice state management
    VoiceState voiceState_{VoiceState::Idle};
    bool isActive_{false};
    
    // Primary synthesis engine
    SynthesisEngineType primaryEngine_{SynthesisEngineType::PhysicalModel};
    
    //==========================================================================
    // Cross-Engine Modulation
    //==========================================================================
    
    struct CrossEngineModulator {
        CrossEngineModulation type;
        float amount;
        float smoothing;
        std::atomic<float> currentValue{0.0f};
        juce::SmoothedValue<float> smoothedValue;
    };
    
    std::array<CrossEngineModulator, 6> crossEngineModulators_;
    bool crossEngineModulationEnabled_{false};
    
    //==========================================================================
    // AI-Powered Features
    //==========================================================================
    
    // AI parameter smoothing and learning
    bool aiSmoothingEnabled_{false};
    float aiLearningRate_{0.01f};
    juce::Array<float> aiParameterHistory_;
    juce::Array<float> aiPredictionBuffer_;
    
    // Neural network inference state
    struct NeuralState {
        juce::Array<float> hiddenLayer;
        juce::Array<float> outputLayer;
        float confidence{0.0f};
        juce::uint64 lastUpdate{0};
    };
    
    NeuralState neuralState_;
    
    //==========================================================================
    // Multi-Engine Mixing
    //==========================================================================
    
    // Engine mix levels
    struct EngineMix {
        float level{0.0f};
        juce::SmoothedValue<float> smoothedLevel;
        float envelopeFollow{0.0f};
    };
    
    std::array<EngineMix, 4> engineMixes_; // Physical, Neural, Wavetable, Cross
    
    //==========================================================================
    // Advanced Envelope Following
    //==========================================================================
    
    bool envelopeFollowingEnabled_{false};
    float envelopeFollowingLevel_{0.0f};
    float envelopeAttack_{0.01f};
    float envelopeRelease_{0.1f};
    float envelopeThreshold_{0.001f};
    
    //==========================================================================
    // Real-Time Analysis
    //==========================================================================
    
    bool realTimeAnalysisEnabled_{false};
    juce::Array<float> analysisBands_;
    juce::Array<float> analysisHistory_;
    
    //==========================================================================
    // Performance Optimization
    //==========================================================================
    
    bool adaptiveQualityEnabled_{false};
    QualityPreset currentQuality_{QualityPreset::Medium};
    double lastCpuLoad_{0.0};
    juce::Array<double> cpuHistory_;
    
    // Memory management
    bool memoryOptimized_{false};
    juce::Array<std::unique_ptr<void>> modelDataCache_;
    std::atomic<bool> modelLoaded_{false};
    
    //==========================================================================
    // Internal State
    //==========================================================================
    
    // MPE state
    float currentNote_{60.0f};
    float currentVelocity_{0.0f};
    float currentModWheel_{0.0f};
    float currentAftertouch_{0.0f};
    float currentTimbre_{0.0f};
    float currentPitchBend_{0.0f};
    
    // Audio processing state
    double sampleRate_{44100.0};
    int blockSize_{512};
    juce::AudioBuffer<float> internalBuffer_;
    
    // Performance monitoring
    double renderingTime_{0.0};
    double modulationTime_{0.0};
    double analysisTime_{0.0};
    
    //==========================================================================
    // Private Methods
    //==========================================================================
    
    // Engine initialization
    void initializeEngine(SynthesisEngineType engine);
    void initializePhysicalEngine();
    void initializeNeuralEngine();
    void initializeWavetableEngine();
    
    // Cross-engine modulation
    void processCrossEngineModulation(int numSamples);
    float computeCrossEngineModulation(CrossEngineModulation type);
    
    // AI processing
    void processAIParameters(int numSamples);
    float predictParameter(int parameterIndex, const float* inputBuffer);
    void updateNeuralNetwork(const float* inputBuffer, const float* targetBuffer);
    
    // Multi-engine mixing
    void processMultiEngineMixing(juce::AudioBuffer<float>& buffer, int numSamples);
    float getEngineOutput(SynthesisEngineType engine, int sample);
    
    // Envelope following
    void processEnvelopeFollowing(const float* inputBuffer, int numSamples);
    
    // Real-time analysis
    void performRealTimeAnalysis(const float* inputBuffer, int numSamples);
    void computeSpectrum(float* spectrum, int numBands);
    
    // Performance optimization
    void updateAdaptiveQuality();
    void manageMemoryUsage();
    
    // Voice lifecycle
    void startVoice();
    void stopVoice();
    void updateVoiceParameters();
    void renderEngineOutput(juce::AudioBuffer<float>& buffer, int startSample, int numSamples);
    
    // Utility methods
    void clearBuffers();
    void checkThreadSafety();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithUltraSynthVoice)
};

} // namespace zenith
