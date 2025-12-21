/*
  ==============================================================================

    InstrumentTrack.cpp
    Created: 2025-12-19
    Author:  Zenith DAW

    Implementation of InstrumentTrack - MIDI track with virtual instrument.

  ==============================================================================
*/

#include "InstrumentTrack.h"
#include "../instruments/Instrument.h"
#include "EngineConstants.h"

namespace zenith {

//==============================================================================
// Construction
//==============================================================================

InstrumentTrack::InstrumentTrack(const juce::String& name)
    : ClipTrack(name, Type::Instrument) {
}

InstrumentTrack::~InstrumentTrack() {
    // Ensure instrument is released properly
    instrument_.reset();
}

//==============================================================================
// AudioSource Interface
//==============================================================================

void InstrumentTrack::prepareToPlay(int samplesPerBlockExpected, double sampleRate) {
    ClipTrack::prepareToPlay(samplesPerBlockExpected, sampleRate);
    
    // Prepare scratch buffers
    midiBuffer_.ensureSize(1024);
    instrumentBuffer_.setSize(2, samplesPerBlockExpected, false, true, true);
    
    // Prepare instrument if available
    if (instrument_ != nullptr) {
        if (auto* processor = instrument_->getAudioProcessor()) {
            processor->setPlayConfigDetails(2, 2, sampleRate, samplesPerBlockExpected);
            processor->prepareToPlay(sampleRate, samplesPerBlockExpected);
        }
    }
}

void InstrumentTrack::releaseResources() {
    ClipTrack::releaseResources();
    
    if (instrument_ != nullptr) {
        if (auto* processor = instrument_->getAudioProcessor()) {
            processor->releaseResources();
        }
    }
}

void InstrumentTrack::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill,
                                        int64_t playheadSamples,
                                        const juce::MidiBuffer* incomingMidi,
                                        const std::vector<juce::AudioBuffer<float>*>& auxBuffers,
                                        const TempoMap* tempoMap) {
    juce::ignoreUnused(auxBuffers, tempoMap);
    const auto numSamples = bufferToFill.numSamples;
    
    // 1. Clear audio output buffer (instrument will fill it)
    bufferToFill.clearActiveBufferRegion();

    // Verify rigorous thread safety
    jassert(juce::MessageManager::getInstance()->currentThreadHasLockedMessageManager() == false);

    // Process any pending cross-thread events/notes safely
    processPendingNotes();
    
    // 2. Prepare MIDI buffer with live input
    midiBuffer_.clear();
    if (incomingMidi != nullptr) {
        midiBuffer_.addEvents(*incomingMidi, 0, numSamples, 0);
    }
    
    // 3. Collect MIDI from clips
    collectMidiFromClips(midiBuffer_, playheadSamples, numSamples);
    
    // 4. Create proxy buffer for correct offset handling
    juce::AudioBuffer<float> proxyBuffer(
        bufferToFill.buffer->getArrayOfWritePointers(),
        bufferToFill.buffer->getNumChannels(),
        bufferToFill.startSample,
        numSamples
    );
    
    // 5. Process through instrument
    processInstrument(proxyBuffer, midiBuffer_, numSamples);
    
    // 6. Process effect plugins
    processPluginChain(proxyBuffer, midiBuffer_, numSamples);
    
    // 7. Apply mixer (volume, pan)
    applyGainAndPan(proxyBuffer, numSamples);
    
    // 8. Update level meters
    updateLevelMeters(proxyBuffer, numSamples);
}

//==============================================================================
// Instrument Management
//==============================================================================

void InstrumentTrack::setInstrument(std::unique_ptr<Instrument> instrument) {
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    
    // Release old instrument
    if (instrument_ != nullptr) {
        if (auto* processor = instrument_->getAudioProcessor()) {
            processor->releaseResources();
        }
    }
    
    // Take ownership of new instrument
    instrument_ = std::move(instrument);
    
    // Prepare new instrument if we have valid audio settings
    if (instrument_ != nullptr && currentSampleRate > 0) {
        if (auto* processor = instrument_->getAudioProcessor()) {
            processor->setPlayConfigDetails(2, 2, currentSampleRate, currentBlockSize);
            processor->prepareToPlay(currentSampleRate, currentBlockSize);
        }
    }
    
    sendChangeMessage();
}

void InstrumentTrack::removeInstrument() {
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    
    if (instrument_ != nullptr) {
        if (auto* processor = instrument_->getAudioProcessor()) {
            processor->releaseResources();
        }
        instrument_.reset();
    }
    
    sendChangeMessage();
}

//==============================================================================
// State Management
//==============================================================================

juce::ValueTree InstrumentTrack::getState() const {
    juce::ValueTree state = ClipTrack::getState();
    state.appendChild(getInstrumentState(), nullptr);
    return state;
}

void InstrumentTrack::loadState(const juce::ValueTree& state) {
    ClipTrack::loadState(state);
    
    juce::ValueTree instrumentTree = state.getChildWithName("Instrument");
    if (instrumentTree.isValid()) {
        loadInstrumentState(instrumentTree);
    }
}

juce::ValueTree InstrumentTrack::getInstrumentState() const {
    juce::ValueTree state("Instrument");
    
    if (instrument_ != nullptr) {
        const auto& metadata = instrument_->getMetadata();
        state.setProperty("id", metadata.instrumentId, nullptr);
        state.setProperty("name", metadata.name, nullptr);
        
        // Save current preset if any
        auto presetId = instrument_->getCurrentPresetId();
        if (presetId.isNotEmpty()) {
            state.setProperty("presetId", presetId, nullptr);
        }
        
        // Save processor state
        if (auto* processor = instrument_->getAudioProcessor()) {
            juce::MemoryBlock stateData;
            processor->getStateInformation(stateData);
            state.setProperty("state", stateData.toBase64Encoding(), nullptr);
        }
    }
    
    return state;
}

void InstrumentTrack::loadInstrumentState(const juce::ValueTree& state) {
    if (!state.hasType("Instrument")) return;
    
    // Note: Actual instrument instantiation requires InstrumentFactory
    // This method loads state into an existing instrument
    
    if (instrument_ != nullptr) {
        // Load preset if specified
        auto presetId = state.getProperty("presetId", "").toString();
        if (presetId.isNotEmpty()) {
            instrument_->loadPreset(presetId);
        }
        
        // Load processor state
        auto stateBase64 = state.getProperty("state", "").toString();
        if (stateBase64.isNotEmpty()) {
            if (auto* processor = instrument_->getAudioProcessor()) {
                juce::MemoryBlock stateData;
                stateData.fromBase64Encoding(stateBase64);
                processor->setStateInformation(stateData.getData(), 
                                               static_cast<int>(stateData.getSize()));
            }
        }
    }
    
    sendChangeMessage();
}

//==============================================================================
// Audio Processing Helpers
//==============================================================================

void InstrumentTrack::collectMidiFromClips(juce::MidiBuffer& midiBuffer,
                                           int64_t playheadSamples,
                                           int numSamples) {
    // Get current clip snapshot (lock-free)
    auto* snapshot = activeClipSnapshot_.load(std::memory_order_acquire);
    if (snapshot == nullptr) return;
    
    for (auto* clip : snapshot->clips) {
        if (clip != nullptr && clip->getType() == Clip::Type::MIDI) {
            clip->setTransportPosition(playheadSamples);
            clip->getMidiEvents(midiBuffer, numSamples);
        }
    }
}

void InstrumentTrack::processInstrument(juce::AudioBuffer<float>& buffer,
                                        juce::MidiBuffer& midiBuffer,
                                        int numSamples) {
    if (instrument_ == nullptr) return;
    
    auto* processor = instrument_->getAudioProcessor();
    if (processor == nullptr) return;
    
    // Process through instrument
    processor->processBlock(buffer, midiBuffer);
}

} // namespace zenith
