/**
 * @file Engine.cpp
 * @brief Audio engine core implementation (Constructor, Destructor, Processing Loop)
 */

#include "../include/Engine.h"
#include <JuceHeader.h>
#include "../include/ProjectState.h"
#include "Track.h"
#include "Clip.h"
#include "MixerChannel.h"
#include "AudioFilePool.h"
#include "PluginHost.h"
#include "AuxBus.h"
#include "../ui/PluginEditorWindow.h"
#include "../instruments/InstrumentRegistry.h"
#include "../instruments/RegisterBuiltInInstruments.h"
#include "../include/TrackAutomationSynchronizer.h"

namespace zenith {

Engine::Engine()
{
    DBG("Engine: Constructor");

    audioFilePool_ = std::make_unique<zenith::AudioFilePool>();
    pluginHost_ = std::make_unique<zenith::PluginHost>();
    pluginEditorWindowManager_ = std::make_unique<zenith::PluginEditorWindowManager>();
    
    instrumentRegistry_ = std::make_unique<zenith::InstrumentRegistry>();
    zenith::registerBuiltInInstruments(*instrumentRegistry_);

    audioWriterThread_ = std::make_unique<juce::TimeSliceThread>("Audio Writer Thread");
    audioWriterThread_->startThread(juce::Thread::Priority::normal);

    tempoMap_ = std::make_unique<zenith::TempoMap>();

    updateTrackSnapshot();
}

Engine::~Engine()
{
    DBG("Engine: Destructor");
    isShuttingDown_.store(true);
    disableMidiInput();
    shutdown();

    if (audioWriterThread_ != nullptr)
    {
        audioWriterThread_->stopThread(1000);
        audioWriterThread_.reset();
    }
    audioFilePool_.reset();
}

//==============================================================================
// Audio IO
//==============================================================================

void Engine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    DBG("Engine: Audio device starting...");

    currentSampleRate.store(device->getCurrentSampleRate());
    currentBufferSize.store(device->getCurrentBufferSizeSamples());

    phase = 0.0;
    playheadSamples_.store(0);

    prepareTracks(device->getCurrentBufferSizeSamples(), device->getCurrentSampleRate());

    const int bufferSize = currentBufferSize.load();
    const int numTracks = static_cast<int>(tracks_.size());

    trackBuffers_.clear();
    trackBuffers_.resize(numTracks);

    for (int i = 0; i < numTracks; ++i)
    {
        trackBuffers_[i].setSize(2, bufferSize);
        trackBuffers_[i].clear();
        if (tracks_[i] != nullptr) tracks_[i]->prepareToPlay(bufferSize, currentSampleRate.load());
    }

    masterBuffer_.setSize(2, bufferSize);
    masterBuffer_.clear();

    auxBusBuffers_.clear();
    auxBusBuffers_.resize(auxBuses_.size());
    for (size_t i = 0; i < auxBusBuffers_.size(); ++i) {
        auxBusBuffers_[i].setSize(2, bufferSize);
        auxBusBuffers_[i].clear();
        if (auxBuses_[i]) auxBuses_[i]->prepareToPlay(bufferSize, currentSampleRate.load());
    }

    auxBufferPtrs_.clear();
    auxBufferPtrs_.reserve(auxBusBuffers_.size());
    for (auto& buf : auxBusBuffers_) {
        auxBufferPtrs_.push_back(&buf);
    }
}

void Engine::audioDeviceStopped()
{
    DBG("Engine: Audio device stopped");
    for (auto& track : tracks_)
    {
        if (track != nullptr) track->releaseResources();
    }
}

void Engine::audioDeviceIOCallbackWithContext(
    const float* const* inputChannelData,
    int numInputChannels,
    float* const* outputChannelData,
    int numOutputChannels,
    int numSamples,
    const juce::AudioIODeviceCallbackContext& context) noexcept
{
    juce::ignoreUnused(inputChannelData, numInputChannels, context);

    bool playing = isPlaying_.load();
    bool recording = isRecording_.load();

    if (playing)
    {
        const juce::int64 currentPos = playheadSamples_.load();
        const bool looping = isLooping_.load();
        const juce::int64 loopEnd = loopEndSamples_.load();
        const juce::int64 loopStart = loopStartSamples_.load();
        
        if (looping && loopEnd > 0 && loopEnd > loopStart)
        {
            const juce::int64 bufferEndPos = currentPos + numSamples;
            
            if (bufferEndPos > loopEnd && currentPos < loopEnd)
            {
                const int samplesBeforeLoop = static_cast<int>(loopEnd - currentPos);
                const int samplesAfterLoop = numSamples - samplesBeforeLoop;
                loopWrapSampleOffset_.store(samplesBeforeLoop);
                
                if (samplesBeforeLoop > 0)
                {
                    juce::AudioBuffer<float> outputBuffer1(outputChannelData, numOutputChannels, samplesBeforeLoop);
                    outputBuffer1.clear();
                    juce::MidiBuffer localMidi1;
                    midiFifo_.drainTo(localMidi1, samplesBeforeLoop);
                    renderAudioGraph(outputBuffer1, samplesBeforeLoop, currentPos, &localMidi1);
                }
                
                if (samplesAfterLoop > 0)
                {
                    std::array<float*, 32> offsetOutputStack;
                    for (int ch = 0; ch < numOutputChannels; ++ch) offsetOutputStack[ch] = outputChannelData[ch] + samplesBeforeLoop;
                    
                    juce::AudioBuffer<float> outputBuffer2(offsetOutputStack.data(), numOutputChannels, samplesAfterLoop);
                    outputBuffer2.clear();
                    juce::MidiBuffer localMidi2;
                    midiFifo_.drainTo(localMidi2, samplesAfterLoop);
                    renderAudioGraph(outputBuffer2, samplesAfterLoop, loopStart, &localMidi2);
                }
                playheadSamples_.store(loopStart + samplesAfterLoop);
            }
            else
            {
                loopWrapSampleOffset_.store(-1);
                processAudioBlock(inputChannelData, numInputChannels, outputChannelData, numOutputChannels, numSamples);
                
                juce::int64 newPosition = currentPos + numSamples;
                if (newPosition >= loopEnd)
                {
                    const juce::int64 loopLength = loopEnd - loopStart;
                    if (loopLength > 0)
                    {
                        while (newPosition >= loopEnd) newPosition -= loopLength;
                        if (newPosition < loopStart) newPosition = loopStart;
                    }
                }
                playheadSamples_.store(newPosition);
            }
        }
        else
        {
            loopWrapSampleOffset_.store(-1);
            processAudioBlock(inputChannelData, numInputChannels, outputChannelData, numOutputChannels, numSamples);
            playheadSamples_.store(currentPos + numSamples);
        }
    }
    else
    {
        for (int channel = 0; channel < numOutputChannels; ++channel)
        {
            if (outputChannelData[channel] != nullptr)
                juce::FloatVectorOperations::clear(outputChannelData[channel], numSamples);
        }
    }

    if (recording)
    {
        captureAudioInput(inputChannelData, numInputChannels, numSamples);
    }
}

void Engine::processAudioBlock(
    const float* const* inputChannelData,
    int numInputChannels,
    float* const* outputChannelData,
    int numOutputChannels,
    int numSamples) noexcept
{
    juce::ignoreUnused(inputChannelData, numInputChannels);

    juce::AudioBuffer<float> outputBuffer(outputChannelData, numOutputChannels, numSamples);
    juce::int64 position = playheadSamples_.load();
    auto* snapshot = activeSnapshot_.load();
    
    processEvents();

    if (snapshot)
    {
        for (auto* track : snapshot->tracks)
        {
            if (track == nullptr) continue;
            for (int i = 0; i < track->getNumClips(); ++i)
            {
                auto* clip = track->getClip(i);
                if (clip != nullptr) clip->setTransportPosition(position);
            }
        }
    }

    juce::MidiBuffer localMidi;
    midiFifo_.drainTo(localMidi, numSamples);
    renderAudioGraph(outputBuffer, numSamples, position, &localMidi);

    bool testToneEnabled = enableTestTone_.load();
    if (testToneEnabled && tracks_.empty())
    {
        const double sampleRate = currentSampleRate.load();
        const double frequency = 440.0;
        const double amplitude = 0.25;
        const double phaseIncrement = frequency * 2.0 * juce::MathConstants<double>::pi / sampleRate;

        for (int sample = 0; sample < numSamples; ++sample)
        {
            float value = static_cast<float>(std::sin(phase) * amplitude);
            for (int channel = 0; channel < numOutputChannels; ++channel)
            {
                if (outputChannelData[channel] != nullptr) outputChannelData[channel][sample] += value;
            }
            phase += phaseIncrement;
            if (phase >= 2.0 * juce::MathConstants<double>::pi) phase -= 2.0 * juce::MathConstants<double>::pi;
        }
    }
}

//==============================================================================
// Rendering
//==============================================================================

void Engine::renderAudioGraph(juce::AudioBuffer<float>& outputBuffer,
                        int numSamples,
                        juce::int64 playheadPosition,
                        const juce::MidiBuffer* incomingMidi)
{
    outputBuffer.clear();
    for (auto& buf : auxBusBuffers_) buf.clear();
    
    auto* snapshot = activeSnapshot_.load();
    if (!snapshot) return;

    for (size_t trackIdx = 0; trackIdx < snapshot->tracks.size(); ++trackIdx)
    {
        if (trackIdx >= trackBuffers_.size()) continue;

        auto& trackBuffer = trackBuffers_[trackIdx];
        if (trackBuffer.getNumSamples() < numSamples) continue;

        trackBuffer.clear();
        juce::AudioSourceChannelInfo trackInfo(&trackBuffer, 0, numSamples);
        auto* track = snapshot->tracks[trackIdx];

        const juce::MidiBuffer* trackMidiInput = nullptr;
        if (incomingMidi != nullptr && !incomingMidi->isEmpty() &&
            track->getType() == zenith::Track::Type::Instrument &&
            track->isArmed())
        {
            trackMidiInput = incomingMidi;
        }

        track->getNextAudioBlock(trackInfo, playheadPosition, const_cast<juce::MidiBuffer*>(trackMidiInput), auxBufferPtrs_, tempoMap_.get());

        for (int channel = 0; channel < juce::jmin(outputBuffer.getNumChannels(), trackBuffer.getNumChannels()); ++channel)
        {
            outputBuffer.addFrom(channel, 0, trackBuffer.getReadPointer(channel), numSamples);
        }
    }
    
    for (size_t i = 0; i < auxBuses_.size() && i < auxBusBuffers_.size(); ++i) {
        if (auxBuses_[i]) {
            juce::AudioSourceChannelInfo auxInfo(&auxBusBuffers_[i], 0, numSamples);
            auxBuses_[i]->getNextAudioBlock(auxInfo);
             for (int channel = 0; channel < juce::jmin(outputBuffer.getNumChannels(), auxBusBuffers_[i].getNumChannels()); ++channel)
            {
                outputBuffer.addFrom(channel, 0, auxBusBuffers_[i], channel, 0, numSamples);
            }
        }
    }

    if (!masterPlugins_.empty())
    {
        juce::MidiBuffer midi;
        for (auto& plugin : masterPlugins_)
        {
            if (plugin != nullptr && !plugin->isSuspended()) plugin->processBlock(outputBuffer, midi);
        }
    }
}

//==============================================================================
// Event Queue
//==============================================================================

bool Engine::queueEvent(const zenith::EngineEvent& e)
{
    int start1, size1, start2, size2;
    commandFifo_.prepareToWrite(1, start1, size1, start2, size2);
    if (size1 > 0) {
        commandBuffer_[start1] = e;
        commandFifo_.finishedWrite(1);
        return true;
    }
    return false;
}

void Engine::processEvents() noexcept
{
    int start1, size1, start2, size2;
    commandFifo_.prepareToRead(commandFifo_.getNumReady(), start1, size1, start2, size2);
    auto* snapshot = activeSnapshot_.load();
    
    auto process = [&](const zenith::EngineEvent& e) {
        if (e.type == zenith::EngineEvent::Type::SetPluginParam) {
             if (snapshot && e.trackIndex >= 0 && e.trackIndex < (int)snapshot->tracks.size()) {
                 auto* track = snapshot->tracks[e.trackIndex];
                 if (track) {
                     auto* plugin = track->getPlugin(e.pluginIndex);
                     if (plugin) {
                         auto params = plugin->getParameters();
                         if (e.paramIndex >= 0 && e.paramIndex < (int)params.size()) {
                             params[e.paramIndex]->setValueNotifyingHost(e.value);
                         }
                     }
                 }
             }
        }
        else if (e.type == zenith::EngineEvent::Type::SetTrackVolume) {
            if (snapshot && e.trackIndex >= 0 && e.trackIndex < (int)snapshot->tracks.size())
                if (auto* track = snapshot->tracks[e.trackIndex]) track->setVolume(e.value);
        }
        else if (e.type == zenith::EngineEvent::Type::SetTrackPan) {
            if (snapshot && e.trackIndex >= 0 && e.trackIndex < (int)snapshot->tracks.size())
                if (auto* track = snapshot->tracks[e.trackIndex]) track->setPan(e.value);
        }
        else if (e.type == zenith::EngineEvent::Type::SetTrackMute) {
            if (snapshot && e.trackIndex >= 0 && e.trackIndex < (int)snapshot->tracks.size())
                if (auto* track = snapshot->tracks[e.trackIndex]) track->setMuted(e.boolValue);
        }
        else if (e.type == zenith::EngineEvent::Type::SetTrackSolo) {
            if (snapshot && e.trackIndex >= 0 && e.trackIndex < (int)snapshot->tracks.size())
                if (auto* track = snapshot->tracks[e.trackIndex]) track->setSolo(e.boolValue);
        }
    };

    if (size1 > 0) for (int i = 0; i < size1; ++i) process(commandBuffer_[start1 + i]);
    if (size2 > 0) for (int i = 0; i < size2; ++i) process(commandBuffer_[start2 + i]);
    
    commandFifo_.finishedRead(size1 + size2);
}

//==============================================================================
// Track Management
//==============================================================================

juce::String Engine::createTrack(const juce::String& name, const juce::String& type)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    if (projectState_) return projectState_->addTrack(name, type);
    
    zenith::Track::Type trackType = (type == "midi") ? zenith::Track::Type::MIDI : zenith::Track::Type::Audio;
    auto track = std::make_shared<zenith::Track>(name, trackType);
    track->setTrackId("track_" + juce::String(tracks_.size()));
    addTrack(track);
    return track->getTrackId();
}

void Engine::addTrack(std::shared_ptr<zenith::Track> track)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    if (currentSampleRate.load() > 0) track->prepareToPlay(currentBufferSize.load(), currentSampleRate.load());
    tracks_.push_back(track);
    updateSoloState();
    updateTrackSnapshot();
}

void Engine::removeTrack(int index)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    if (index >= 0 && index < static_cast<int>(tracks_.size()))
    {
        tracks_[index]->releaseResources();
        routingGraph_.removeNode(tracks_[index]->getTrackId());
        tracks_.erase(tracks_.begin() + index);
        updateSoloState();
        updateTrackSnapshot();
    }
}

void Engine::updateTrackSnapshot()
{
    auto newSnapshot = std::make_shared<TrackSnapshot>(tracks_);
    activeSnapshot_.store(newSnapshot.get());
    snapshotTrash_.push_back(currentSnapshotHolder_);
    currentSnapshotHolder_ = newSnapshot;
    if (snapshotTrash_.size() > 5) snapshotTrash_.erase(snapshotTrash_.begin());
}

void Engine::addTestTracks(int count)
{
    if (count <= 0) return;
    tracks_.reserve(tracks_.size() + static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        auto track = std::make_shared<zenith::Track>("Track " + juce::String(tracks_.size() + 1), zenith::Track::Type::Audio);
        if (currentSampleRate.load() > 0) track->prepareToPlay(currentBufferSize.load(), currentSampleRate.load());
        tracks_.push_back(track);
    }
    updateTrackSnapshot();
}

//==============================================================================
// Getters / Setters (Mixer & System)
//==============================================================================

int Engine::getNumTracks() const noexcept { return static_cast<int>(tracks_.size()); }
const std::vector<std::shared_ptr<zenith::Track>>& Engine::tracks() const noexcept { return tracks_; }
zenith::AudioFilePool& Engine::getAudioFilePool() { return *audioFilePool_; }
zenith::PluginHost& Engine::getPluginHost() noexcept { return *pluginHost_; }
int Engine::scanForPlugins() { return pluginHost_ ? pluginHost_->scanDefaultLocations() : 0; }
zenith::PluginEditorWindowManager& Engine::getPluginEditorWindowManager() noexcept { return *pluginEditorWindowManager_; }
const zenith::TempoMap& Engine::getTempoMap() const noexcept { return *tempoMap_; }

void Engine::setTrackVolume(int trackIndex, float volume) { if (trackIndex >= 0 && trackIndex < tracks_.size()) tracks_[trackIndex]->setVolume(volume); }
void Engine::setTrackPan(int trackIndex, float pan) { if (trackIndex >= 0 && trackIndex < tracks_.size()) tracks_[trackIndex]->setPan(pan); }
void Engine::setTrackInputChannel(int trackIndex, int channelIndex) { if (trackIndex >= 0 && trackIndex < tracks_.size()) tracks_[trackIndex]->setInputChannel(channelIndex); }
void Engine::setTrackMute(int trackIndex, bool muted) { if (trackIndex >= 0 && trackIndex < tracks_.size()) tracks_[trackIndex]->setMuted(muted); }
void Engine::setTrackSolo(int trackIndex, bool solo) { if (trackIndex >= 0 && trackIndex < tracks_.size()) { tracks_[trackIndex]->setSolo(solo); updateSoloState(); } }
void Engine::setTrackArmed(int trackIndex, bool armed) { if (trackIndex >= 0 && trackIndex < tracks_.size()) { tracks_[trackIndex]->setArmed(armed); if (armed) prepareRecordingForTrack(trackIndex); } }

float Engine::getTrackLevel(int trackIndex) const { return (trackIndex >= 0 && trackIndex < tracks_.size()) ? tracks_[trackIndex]->getCurrentLevel() : 0.0f; }
float Engine::getTrackPeakLevel(int trackIndex) const { return (trackIndex >= 0 && trackIndex < tracks_.size()) ? tracks_[trackIndex]->getPeakLevel() : 0.0f; }
float Engine::getMasterLevel() const { return masterLevel_.load(); }
float Engine::getMasterPeakLevel() const { return masterPeakLevel_.load(); }

void Engine::resetPeakMeters()
{
    masterPeakLevel_.store(0.0f);
    for (auto& track : tracks_) if (track) track->resetPeakLevel();
}

} // namespace zenith
