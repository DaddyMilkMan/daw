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

    ZenithUltraSynth.cpp
    Created: 2025-02-05
    Author:  Zenith DAW

    Phase 1.1: Basic implementation for ZenithUltraSynth - Advanced AI-Powered
    Synthesizer Engine foundation.

    ==============================================================================
*/

#include "ZenithUltraSynth.h"
#include "ZenithUltraSynthVoice.h"
#include "UltraSynthParameterManager.h"
#include "ContentPaths.h"
// Placeholder includes for engines - assuming they exist or will exist
#include "physical_modeling/PhysicalModelEngine.h"
#include "neural_synthesis/NeuralSynthesisEngine.h"
#include "wavetable/HybridWavetableEngine.h"
#include "workflow/WorkflowManager.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace zenith {

//==============================================================================
// ZenithUltraSynth Implementation
//==============================================================================

ZenithUltraSynth::ZenithUltraSynth()
    : InstrumentBase(std::make_unique<ZenithUltraSynthProcessor>(), createMetadata())
{
    // Initialize synthesis engines
    initializeEngines();
    registerPresets();
}

void ZenithUltraSynth::initialize() {
    if (!initialized_) {
        // Initialize all synthesis engines
        if (physicalEngine_) physicalEngine_->initialize();
        if (neuralEngine_) neuralEngine_->initialize();
        if (wavetableEngine_) wavetableEngine_->initialize();
        if (workflowManager_) workflowManager_->initialize();
        
        // parameterManager_ in ZenithUltraSynth seems redundant or incorrect
        // as it requires apvts which is in processor.
        // Leaving it null here to avoid compilation error with missing arguments.
        
        initialized_ = true;
    }
}

void ZenithUltraSynth::configure(double sampleRate, int blockSize) {
    if (initialized_) {
        // Configure all engines with sample rate and block size
        if (physicalEngine_) physicalEngine_->configure(sampleRate, blockSize);
        if (neuralEngine_) neuralEngine_->configure(sampleRate, blockSize);
        if (wavetableEngine_) wavetableEngine_->configure(sampleRate, blockSize);
        if (workflowManager_) workflowManager_->configure(sampleRate, blockSize);
    }
}

juce::AudioProcessor* ZenithUltraSynth::createAudioProcessor() {
    return new ZenithUltraSynthProcessor();
}

InstrumentMetadata ZenithUltraSynth::createMetadata() {
    InstrumentMetadata metadata;
    metadata.name = "Zenith Ultra Synth";
    metadata.category = "Synthesizer";
    metadata.description = "Advanced AI-Powered Synthesizer with Physical Modeling, Neural Synthesis, and Hybrid Wavetable Engines";
    metadata.version = "1.0.0";
    metadata.author = "Zenith DAW Team";
    metadata.supportsMidi = true;
    metadata.supportsMPE = true;
    metadata.maxVoices = 16;
    metadata.parameterCount = 256; // Estimated
    metadata.requiresNetwork = false; // For neural models
    metadata.memoryUsage = "High"; // Due to neural models
    metadata.cpuUsage = "Medium-High";
    return metadata;
}

void ZenithUltraSynth::initializeEngines() {
    // Create synthesis engines
    physicalEngine_ = std::make_unique<PhysicalModelEngine>();
    neuralEngine_ = std::make_unique<NeuralSynthesisEngine>();
    wavetableEngine_ = std::make_unique<HybridWavetableEngine>();
    workflowManager_ = std::make_unique<WorkflowManager>();
}

void ZenithUltraSynth::registerPresets() {
    auto contentRoot = ContentPaths::getInstance().getContentRoot();
    // Assuming structure: Content/Presets/ZenithUltraSynth/*.json
    auto presetDir = contentRoot.getChildFile("Presets").getChildFile("ZenithUltraSynth");

    if (!presetDir.exists()) {
        DBG("ZenithUltraSynth presets directory not found: " << presetDir.getFullPathName());
        return;
    }

    // Find all bank files
    auto bankFiles = presetDir.findChildFiles(juce::File::findFiles, false, "*_bank.json");

    for (const auto& bankFile : bankFiles) {
        juce::String jsonString = bankFile.loadFileAsString();
        auto result = juce::JSON::parse(jsonString);

        if (!result.isObject()) {
            DBG("ZenithUltraSynth: Warning - failed to parse preset bank: " + bankFile.getFileName());
            continue;
        }

        auto* bankObj = result.getDynamicObject();
        if (!bankObj) continue;

        auto presetsVar = bankObj->getProperty("presets");
        if (!presetsVar.isArray()) continue;

        auto* presetsArray = presetsVar.getArray();
        for (const auto& presetVar : *presetsArray) {
            if (!presetVar.isObject()) continue;
            auto* presetObj = presetVar.getDynamicObject();

            juce::String id = presetObj->getProperty("id").toString();
            juce::String name = presetObj->getProperty("name").toString();
            auto parametersVar = presetObj->getProperty("parameters");

            std::map<juce::String, float> values;

            if (parametersVar.isObject()) {
                auto* paramsObj = parametersVar.getDynamicObject();
                for (auto& prop : paramsObj->getProperties()) {
                    juce::String key = prop.name.toString();

                    // Handle array parameters (e.g. timbre_params)
                    if (prop.value.isArray()) {
                         auto* arr = prop.value.getArray();
                         if (key == "timbre_params") {
                             for (int i = 0; i < arr->size(); ++i) {
                                 if (i >= 16) break; // Limit to 16
                                 float val = static_cast<float>(static_cast<double>((*arr)[i]));
                                 values[UltraSynthParameterManager::getTimbreParamID(i)] = val;
                             }
                         }
                    } else if (prop.value.isDouble() || prop.value.isInt()) {
                        float val = static_cast<float>(static_cast<double>(prop.value));

                        // Map JSON keys to Parameter IDs
                        if (key == "latent_x") values[UltraSynthParameterManager::LatentX] = val;
                        else if (key == "latent_y") values[UltraSynthParameterManager::LatentY] = val;
                        else if (key == "latent_z") values[UltraSynthParameterManager::LatentZ] = val;
                        else if (key == "temperature") values[UltraSynthParameterManager::Temperature] = val;
                        else if (key == "top_k") values[UltraSynthParameterManager::TopK] = val;
                        else if (key == "top_p") values[UltraSynthParameterManager::TopP] = val;
                        else if (key == "overlapping") values[UltraSynthParameterManager::Overlapping] = val;
                    }
                }
            }
            registerPreset(id, name, values);
        }
    }
}

//==============================================================================
// ZenithUltraSynthProcessor Implementation
//==============================================================================

ZenithUltraSynthProcessor::ZenithUltraSynthProcessor()
    : parameters_(*this, nullptr, "PARAMETERS", createParameterLayout())
    , parameterManager_(std::make_unique<UltraSynthParameterManager>(parameters_))
{
    // Initialize MPE synthesiser
    synthesiser_.setVoiceStealingEnabled(true);
    synthesiser_.setMaxPolyphony(16);
    
    // Initialize voices
    initializeVoices();
}

ZenithUltraSynthProcessor::~ZenithUltraSynthProcessor() {
    // Cleanup voices
    for (auto& voice : voices_) {
        voice.reset();
    }
}

void ZenithUltraSynthProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    currentSampleRate_ = sampleRate;
    maxBlockSize_.store(samplesPerBlock);
    
    // Prepare synthesiser
    synthesiser_.setCurrentPlaybackSampleRate(sampleRate);
    
    // Initialize all voices
    for (auto& voice : voices_) {
        if (voice) {
            voice->setSampleRate(sampleRate);
        }
    }
    
    // Initialize parameter manager
    parameterManager_->prepareToPlay(sampleRate, samplesPerBlock);
}

void ZenithUltraSynthProcessor::releaseResources() {
    // Release synthesiser resources
    synthesiser_.releaseAllNotes();
    
    // Clear processing times
    processingTimes_.clear();
    
    // Reset performance monitoring
    maxProcessingTime_.store(0.0);
}

void ZenithUltraSynthProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    jassert(buffer.getNumSamples() > 0);
    jassert(buffer.getNumChannels() > 0);
    
    auto startTime = juce::Time::getMillisecondCounterHiRes();
    
    // Clear output buffer
    buffer.clear();
    
    // Process MIDI messages
    synthesiser_.processNextMidiBuffer(midiMessages, 0, buffer.getNumSamples(), true);
    
    // Process audio
    processPerformanceMonitoring(buffer.getNumSamples());
    
    // Update performance monitoring
    auto endTime = juce::Time::getMillisecondCounterHiRes();
    double processingTime = (endTime - startTime) / 1000.0;
    
    processingTimes_.add(processingTime);
    if (processingTimes_.size() > 100) {
        processingTimes_.remove(0);
    }
    
    maxProcessingTime_.store(jmax(maxProcessingTime_.load(), processingTime));
}

juce::AudioProcessorEditor* ZenithUltraSynthProcessor::createEditor() {
    // TODO: Create UltraSynth editor
    // Will be implemented in Phase 2
    return nullptr;
}

void ZenithUltraSynthProcessor::getStateInformation(juce::MemoryBlock& destData) {
    // TODO: Implement state save
    // Will be implemented in Phase 3
}

void ZenithUltraSynthProcessor::setStateInformation(const void* data, int sizeInBytes) {
    // TODO: Implement state load
    // Will be implemented in Phase 3
}

void ZenithUltraSynthProcessor::initializeVoices() {
    for (int i = 0; i < 16; ++i) {
        voices_[i] = std::make_unique<ZenithUltraSynthVoice>();
        voices_[i]->setSampleRate(currentSampleRate_);
    }
}

ZenithUltraSynthVoice* ZenithUltraSynthProcessor::getVoice(int index) const {
    if (index >= 0 && index < 16 && voices_[index]) {
        return voices_[index].get();
    }
    return nullptr;
}

void ZenithUltraSynthProcessor::updateVoiceParameters() {
    // Update all voices with current parameters
    for (auto& voice : voices_) {
        if (voice) {
            // TODO: Apply parameter updates to voice
            // Will be implemented in Phase 2
        }
    }
}

void ZenithUltraSynthProcessor::processPerformanceMonitoring(int numSamples) {
    // Monitor performance metrics
    // TODO: Implement detailed performance monitoring
    // Will be enhanced in Phase 3
}

juce::AudioProcessorValueTreeState::ParameterLayout ZenithUltraSynthProcessor::createParameterLayout() {
    return UltraSynthParameterManager::createParameterLayout();
}

} // namespace zenith
