#include "MIDITrack.h"
#include "Clip.h"
#include "Instrument.h"
#include "TempoMap.h"

namespace zenith {

void MIDITrack::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill, 
                                 int64_t playheadSamples,
                                 const juce::MidiBuffer* incomingMidi,
                                 const std::vector<juce::AudioBuffer<float>*>& auxBuffers,
                                 const TempoMap* tempoMap) {
    juce::ignoreUnused(incomingMidi);
    bufferToFill.clearActiveBufferRegion();

    if (!enabled.load()) return;

    juce::MidiBuffer midiOut;
    const double tempo = tempoMap ? tempoMap->getTempoAt(playheadSamples) : 120.0;
    generateMidiForBlock(tempo, currentSampleRate, playheadSamples, bufferToFill.numSamples, midiOut);

    juce::AudioBuffer<float> localBuffer(bufferToFill.buffer->getArrayOfWritePointers(), 
                                         bufferToFill.buffer->getNumChannels(), 
                                         bufferToFill.startSample, 
                                         bufferToFill.numSamples);

    processPluginChain(localBuffer, midiOut, bufferToFill.numSamples);

    juce::AudioSourceChannelInfo mixerInfo(&localBuffer, 0, bufferToFill.numSamples);
    mixerChannel.getNextAudioBlock(mixerInfo, auxBuffers);
}

void MIDITrack::generateMidiForBlock(double tempo, double sampleRate, juce::int64 blockStartSample, int blockSize, juce::MidiBuffer& midiOut) {
    const ClipSnapshot* snapshot = activeClipSnapshot_.load(std::memory_order_acquire);
    if (!snapshot) return;

    for (auto* clip : snapshot->clips) {
        if (clip != nullptr && clip->getType() == Clip::Type::MIDI && clip->isPlaying() && clip->isActiveAt(blockStartSample)) {
            clip->getMidiEvents(midiOut, blockSize);
        }
    }
}

// INSTRUMENT TRACK IMPLEMENTATION

void InstrumentTrack::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill, 
                                       int64_t playheadSamples,
                                       const juce::MidiBuffer* incomingMidi,
                                       const std::vector<juce::AudioBuffer<float>*>& auxBuffers,
                                       const TempoMap* tempoMap) {
    bufferToFill.clearActiveBufferRegion();

    if (!enabled.load()) return;

    juce::MidiBuffer midiOut;
    
    // Route live incoming MIDI if armed
    if (incomingMidi != nullptr && armed.load()) {
        midiOut.addEvents(*incomingMidi, 0, bufferToFill.numSamples, 0);
    }

    // Generate clip MIDI
    const double tempo = tempoMap ? tempoMap->getTempoAt(playheadSamples) : 120.0;
    generateMidiForBlock(tempo, currentSampleRate, playheadSamples, bufferToFill.numSamples, midiOut);

    juce::AudioBuffer<float> localBuffer(bufferToFill.buffer->getArrayOfWritePointers(), 
                                         bufferToFill.buffer->getNumChannels(), 
                                         bufferToFill.startSample, 
                                         bufferToFill.numSamples);

    // Process instrument
    if (instrument_ != nullptr) {
        instrumentBuffer_.setSize(2, bufferToFill.numSamples, false, false, true);
        instrumentBuffer_.clear();
        
        auto* processor = instrument_->getAudioProcessor();
        if (processor != nullptr) {
            processor->processBlock(instrumentBuffer_, midiOut);
        }
        
        for (int ch = 0; ch < juce::jmin(localBuffer.getNumChannels(), instrumentBuffer_.getNumChannels()); ++ch) {
            localBuffer.addFrom(ch, 0, instrumentBuffer_, ch, 0, bufferToFill.numSamples);
        }
    }

    // Process MIDI through plugins (if any respond to MIDI)
    processPluginChain(localBuffer, midiOut, bufferToFill.numSamples);

    // Mixer
    juce::AudioSourceChannelInfo mixerInfo(&localBuffer, 0, bufferToFill.numSamples);
    mixerChannel.getNextAudioBlock(mixerInfo, auxBuffers);
}

void InstrumentTrack::setInstrument(std::unique_ptr<Instrument> instrument) {
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    instrument_ = std::move(instrument);
    if (instrument_ != nullptr && currentSampleRate > 0) {
        auto* processor = instrument_->getAudioProcessor();
        if (processor != nullptr) {
            processor->prepareToPlay(currentSampleRate, currentBlockSize);
        }
    }
}

} // namespace zenith
