/**
 * @file EngineAdapter.cpp
 * @brief Implementation of EngineAdapter.
 */

#include "../../include/EngineAdapter.h"
#include <JuceHeader.h>

namespace zenith {

//==============================================================================
// Static member definition
//==============================================================================

Engine* EngineAdapter::sharedEngine_ = nullptr;

//==============================================================================
// Helper functions
//==============================================================================

std::string EngineAdapter::toString(const juce::String& s)
{
    return s.toStdString();
}

juce::String EngineAdapter::toJuceString(const std::string& s)
{
    return juce::String(s);
}

//==============================================================================
// Construction
//==============================================================================

EngineAdapter::EngineAdapter(Engine& engine)
    : engine_(engine)
    , errorHandler_(nullptr)
    , statusHandler_(nullptr)
    , recordingCompleteHandler_(nullptr)
{
}

//==============================================================================
// Error & Event Callbacks (UI‑agnostic)
//==============================================================================

void EngineAdapter::setErrorHandler(ErrorHandler handler)
{
    errorHandler_ = std::move(handler);
}

void EngineAdapter::setStatusHandler(StatusHandler handler)
{
    statusHandler_ = std::move(handler);
}

void EngineAdapter::setRecordingCompleteHandler(RecordingCompleteHandler handler)
{
    recordingCompleteHandler_ = std::move(handler);
}

//==============================================================================
// Initialization & Shutdown (Message Thread)
//==============================================================================

bool EngineAdapter::initialize()
{
    bool success = engine_.initialize();
    if (!success && errorHandler_)
    {
        errorHandler_(toString(engine_.getLastInitError()));
    }
    return success;
}

void EngineAdapter::shutdown()
{
    engine_.shutdown();
}

//==============================================================================
// Project State Integration (Message Thread)
//==============================================================================

void EngineAdapter::setProjectState(ProjectState* state)
{
    engine_.setProjectState(state);
}

ProjectState* EngineAdapter::getProjectState() const
{
    return engine_.getProjectState();
}

void EngineAdapter::syncWithProjectState()
{
    engine_.syncWithProjectState();
}

//==============================================================================
// Transport Control (Message Thread)
//==============================================================================

void EngineAdapter::play()
{
    engine_.play();
}

void EngineAdapter::stop()
{
    engine_.stop();
}

bool EngineAdapter::isPlaying() const
{
    return engine_.isPlaying();
}

void EngineAdapter::record()
{
    engine_.record();
}

void EngineAdapter::stopRecording()
{
    engine_.stopRecording();
}

bool EngineAdapter::isRecording() const
{
    return engine_.isRecording();
}

void EngineAdapter::toggleRecording()
{
    engine_.toggleRecording();
}

//==============================================================================
// Transport Position & Looping (Message Thread for setters)
//==============================================================================

std::int64_t EngineAdapter::getPlayheadSamples() const
{
    return static_cast<std::int64_t>(engine_.getPlayheadSamples());
}

double EngineAdapter::getPlaybackPositionBeats() const
{
    return engine_.getPlaybackPositionBeats();
}

void EngineAdapter::setPlayheadSamples(std::int64_t position)
{
    engine_.setPlayheadSamples(static_cast<juce::int64>(position));
}

void EngineAdapter::setLooping(bool shouldLoop)
{
    engine_.setLooping(shouldLoop);
}

bool EngineAdapter::isLooping() const
{
    return engine_.isLooping();
}

void EngineAdapter::setLoopRegion(std::int64_t start, std::int64_t end)
{
    engine_.setLoopRegion(static_cast<juce::int64>(start), static_cast<juce::int64>(end));
}

std::int64_t EngineAdapter::getLoopStart() const
{
    return static_cast<std::int64_t>(engine_.getLoopStart());
}

std::int64_t EngineAdapter::getLoopEnd() const
{
    return static_cast<std::int64_t>(engine_.getLoopEnd());
}

//==============================================================================
// Audio Device Info (Thread Safe)
//==============================================================================

std::string EngineAdapter::getAudioDeviceInfo() const
{
    return toString(engine_.getAudioDeviceInfo());
}

double EngineAdapter::getSampleRate() const
{
    return engine_.getSampleRate();
}

int EngineAdapter::getBufferSize() const
{
    return engine_.getBufferSize();
}

double EngineAdapter::getCpuUsage() const
{
    return engine_.getCpuUsage();
}

//==============================================================================
// Track Management (Message Thread)
//==============================================================================

int EngineAdapter::getNumTracks() const
{
    return engine_.getNumTracks();
}

std::string EngineAdapter::createTrack(const std::string& name, const std::string& type)
{
    return toString(engine_.createTrack(toJuceString(name), toJuceString(type)));
}

void EngineAdapter::removeTrack(int index)
{
    engine_.removeTrack(index);
}

//==============================================================================
// Mixer Control (Message Thread)
//==============================================================================

void EngineAdapter::setTrackVolume(int trackIndex, float volume)
{
    engine_.setTrackVolume(trackIndex, volume);
}

void EngineAdapter::setTrackPan(int trackIndex, float pan)
{
    engine_.setTrackPan(trackIndex, pan);
}

void EngineAdapter::setTrackMute(int trackIndex, bool muted)
{
    engine_.setTrackMute(trackIndex, muted);
}

void EngineAdapter::setTrackSolo(int trackIndex, bool solo)
{
    engine_.setTrackSolo(trackIndex, solo);
}

void EngineAdapter::setTrackArmed(int trackIndex, bool armed)
{
    engine_.setTrackArmed(trackIndex, armed);
}

void EngineAdapter::setTrackInputChannel(int trackIndex, int channelIndex)
{
    engine_.setTrackInputChannel(trackIndex, channelIndex);
}

//==============================================================================
// Metering (Audio Thread Safe)
//==============================================================================

float EngineAdapter::getTrackLevel(int trackIndex) const
{
    return engine_.getTrackLevel(trackIndex);
}

float EngineAdapter::getTrackPeakLevel(int trackIndex) const
{
    return engine_.getTrackPeakLevel(trackIndex);
}

float EngineAdapter::getMasterLevel() const
{
    return engine_.getMasterLevel();
}

float EngineAdapter::getMasterPeakLevel() const
{
    return engine_.getMasterPeakLevel();
}

void EngineAdapter::resetPeakMeters()
{
    engine_.resetPeakMeters();
}

//==============================================================================
// Plugin Delay Compensation (Message Thread)
//==============================================================================

int EngineAdapter::getTrackLatency(int trackIndex) const
{
    return engine_.getTrackLatency(trackIndex);
}

int EngineAdapter::getMasterLatency() const
{
    return engine_.getMasterLatency();
}

void EngineAdapter::recalculatePDC()
{
    engine_.recalculatePDC();
}

bool EngineAdapter::isPDCEnabled() const
{
    return engine_.isPDCEnabled();
}

void EngineAdapter::setPDCEnabled(bool enabled)
{
    engine_.setPDCEnabled(enabled);
}

int EngineAdapter::getMaxTrackLatency() const
{
    return engine_.getMaxTrackLatency();
}

//==============================================================================
// Aux Bus Management (Message Thread)
//==============================================================================

int EngineAdapter::createAuxBus(const std::string& name)
{
    return engine_.createAuxBus(toJuceString(name));
}

void EngineAdapter::removeAuxBus(int auxIndex)
{
    engine_.removeAuxBus(auxIndex);
}

int EngineAdapter::getNumAuxBuses() const
{
    return engine_.getNumAuxBuses();
}

float EngineAdapter::getAuxBusLevel(int auxIndex) const
{
    return engine_.getAuxBusLevel(auxIndex);
}

float EngineAdapter::getAuxBusPeakLevel(int auxIndex) const
{
    return engine_.getAuxBusPeakLevel(auxIndex);
}

//==============================================================================
// Plugin Hosting (Message Thread)
//==============================================================================

PluginHost& EngineAdapter::getPluginHost()
{
    return engine_.getPluginHost();
}

int EngineAdapter::scanForPlugins()
{
    return engine_.scanForPlugins();
}

PluginEditorWindowManager& EngineAdapter::getPluginEditorWindowManager()
{
    return engine_.getPluginEditorWindowManager();
}

//==============================================================================
// Core Services Access (Thread Safe where noted)
//==============================================================================

AudioFilePool& EngineAdapter::getAudioFilePool()
{
    return engine_.getAudioFilePool();
}

const TempoMap& EngineAdapter::getTempoMap() const
{
    return engine_.getTempoMap();
}

InstrumentRegistry& EngineAdapter::getInstrumentRegistry()
{
    return engine_.getInstrumentRegistry();
}

const InstrumentRegistry& EngineAdapter::getInstrumentRegistry() const
{
    return engine_.getInstrumentRegistry();
}

//==============================================================================
// Real‑time Event Queue (Lock‑Free)
//==============================================================================

bool EngineAdapter::queueEvent(const EngineEvent& e)
{
    return engine_.queueEvent(e);
}

//==============================================================================
// Project Export (Message Thread)
//==============================================================================

bool EngineAdapter::exportProjectToWav(const juce::File& outputFile,
                                       double sampleRate, int bitDepth,
                                       double durationInSeconds)
{
    return engine_.exportProjectToWav(outputFile, sampleRate, bitDepth, durationInSeconds);
}

bool EngineAdapter::exportProject(const ExportOptions& options)
{
    return engine_.exportProject(options);
}

//==============================================================================
// Factory Function Implementation (from AudioEngineCore)
//==============================================================================

std::unique_ptr<AudioEngineCore> EngineAdapter::create()
{
    if (sharedEngine_ != nullptr)
    {
        // Create an adapter wrapping the shared engine.
        // The caller takes ownership of the adapter.
        return std::make_unique<EngineAdapter>(*sharedEngine_);
    }

    // Fallback: If no shared engine, we cannot function.
    // In a real scenario, we might try to find one or throw.
    // For now, we return nullptr to indicate failure, but the caller should have setSharedEngine first.
    
    juce::Logger::writeToLog("EngineAdapter::create() called but no shared Engine instance is set. Call EngineAdapter::setSharedEngine() first.");
    
    // Returning nullptr is the only safe "no stub" behavior if pre-conditions aren't met.
    return nullptr; 
}

//==============================================================================
// Shared Engine setter
//==============================================================================

void EngineAdapter::setSharedEngine(Engine* engine)
{
    sharedEngine_ = engine;
}

} // namespace zenith
