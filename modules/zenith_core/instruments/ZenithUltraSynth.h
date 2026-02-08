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

    ZenithUltraSynth.h
    Created: 2025-02-05
    Author:  Zenith DAW

    Phase 1.1: Header for ZenithUltraSynth - Advanced AI-Powered Synthesizer Engine
    featuring physical modeling, neural synthesis, and hybrid wavetable capabilities.

    ==============================================================================
*/

#pragma once

#include "Instrument.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <memory>

namespace zenith {

// Forward declarations for UltraSynth components
class ZenithUltraSynthProcessor;
class ZenithUltraSynthVoice;
class PhysicalModelEngine;
class NeuralSynthesisEngine;
class HybridWavetableEngine;
class WorkflowManager;
class UltraSynthParameterManager;
struct UltraSynthParameters;

//==============================================================================
/**
    ZenithUltraSynth - Main instrument class for the UltraSynth engine
    
    This class serves as the primary interface for the UltraSynth instrument,
    coordinating between the different synthesis engines (physical modeling,
    neural synthesis, and wavetable) and managing the overall workflow.
*/
class ZenithUltraSynth : public InstrumentBase {
public:
    ZenithUltraSynth();
    ~ZenithUltraSynth() override = default;

    // InstrumentBase overrides
    void initialize() override;
    void configure(double sampleRate, int blockSize) override;
    juce::AudioProcessor* createAudioProcessor() override;
    
    // Accessors for synthesis engines
    PhysicalModelEngine* getPhysicalEngine() { return physicalEngine_.get(); }
    NeuralSynthesisEngine* getNeuralEngine() { return neuralEngine_.get(); }
    HybridWavetableEngine* getWavetableEngine() { return wavetableEngine_.get(); }
    WorkflowManager* getWorkflowManager() { return workflowManager_.get(); }

    // Metadata
    static InstrumentMetadata createMetadata();

private:
    // Core synthesis engines
    std::unique_ptr<PhysicalModelEngine> physicalEngine_;
    std::unique_ptr<NeuralSynthesisEngine> neuralEngine_;
    std::unique_ptr<HybridWavetableEngine> wavetableEngine_;
    std::unique_ptr<WorkflowManager> workflowManager_;
    
    // Parameter management
    // Removed duplicate parameterManager_ from here as it lives in Processor
    
    // Internal state
    bool initialized_ = false;
    
    // Helper methods
    void initializeEngines();
    void registerPresets();
};

//==============================================================================
/**
    ZenithUltraSynthProcessor - AudioProcessor implementation for UltraSynth
    
    This class handles the audio processing, parameter management, and serves
    as the main entry point for the UltraSynth audio engine.
*/
class ZenithUltraSynthProcessor : public juce::AudioProcessor {
public:
    ZenithUltraSynthProcessor();
    ~ZenithUltraSynthProcessor() override;

    // AudioProcessor overrides
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    // Editor
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    // Metadata
    const juce::String getName() const override { return "Zenith Ultra Synth"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    // Program handling
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int /*index*/) override {}
    const juce::String getProgramName(int /*index*/) override { return "Default"; }
    void changeProgramName(int /*index*/, const juce::String& /*newName*/) override {}

    // State save/load
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Parameter management
    juce::AudioProcessorValueTreeState& getParameters() { return parameters_; }
    UltraSynthParameterManager& getParameterManager() { return *parameterManager_; }

    // Engine access
    ZenithUltraSynthVoice* getVoice(int index) const;
    int getNumActiveVoices() const { return activeVoices_; }
    
    // Performance monitoring
    double getMaxProcessingTime() const { return maxProcessingTime_; }
    int getMaxBlockSize() const { return maxBlockSize_; }

private:
    // Core synthesis components
    std::unique_ptr<ZenithUltraSynthVoice> voices_[16]; // Maximum 16 voices
    juce::MPESynthesiser synthesiser_;
    std::atomic<int> activeVoices_{0};
    
    // Parameter management
    juce::AudioProcessorValueTreeState parameters_;
    std::unique_ptr<UltraSynthParameterManager> parameterManager_;
    
    // Helper to create layout
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Performance monitoring
    std::atomic<double> maxProcessingTime_{0.0};
    std::atomic<int> maxBlockSize_{512};
    juce::Array<double> processingTimes_;
    
    // State management
    bool initialized_{false};
    double currentSampleRate_{44100.0};
    
    // Internal helpers
    void initializeVoices();
    void updateVoiceParameters();
    void processPerformanceMonitoring(int numSamples);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithUltraSynthProcessor)
};

} // namespace zenith
