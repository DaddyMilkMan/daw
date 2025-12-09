/**
 * @file EngineAdapter.cpp
 * @brief Implementation of EngineAdapter.
 */

#include "../../include/EngineAdapter.h"
#include <JuceHeader.h>

namespace zenith {

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
    return engine_.initialize();
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
    // Create a new Engine instance and wrap it in an EngineAdapter.
    // Note: The Engine must be owned by something. In the real application,
    // the Engine is owned by MainWindow or similar. For now, we'll create a
    // static Engine? That's not good. Instead, we'll require the application
    // to pass an Engine to the adapter. However, the factory method in
    // AudioEngineCore is static and expects to create an instance.
    // We need to change the design: The factory method should be implemented
    // elsewhere (like in a factory class) that knows about the application's
    // lifetime. For now, we'll implement it to create a new Engine and manage
    // it with a shared_ptr, but that's not ideal because the Engine might
    // need to be integrated with other parts (like ProjectState).
    // Since this is a migration step, we'll leave it as a placeholder and
    // return nullptr. The actual creation will be done by the application
    // (e.g., MainWindow) by constructing an EngineAdapter with its Engine.
    // We'll change the factory method in AudioEngineCore to be a pure
    // virtual? Actually, we can keep it static and return a default
    // implementation that uses a global Engine? That's not good.

    // For now, we'll return nullptr and rely on the application to create
    // the adapter manually. This is a temporary solution.
    // In the next phase, we'll update the factory to work with the existing
    // Engine instance.

    // Returning nullptr will cause runtime errors, so we must not call this.
    // Instead, let's create a new Engine and wrap it. This is acceptable for
    // testing but not for production because the Engine must be integrated
    // with the rest of the application (device manager, etc.).
    // We'll create a new Engine and hope that the application doesn't have
    // two Engines. This is a temporary solution until we refactor the
    // ownership.

    // Actually, we can't create a new Engine because the application already
    // has one. The factory method should be removed or implemented differently.
    // Since we are in the migration phase, we'll keep the factory method
    // but make it return an adapter that wraps the existing global Engine.
    // However, there is no global Engine. We need to get a reference to the
    // existing Engine. We can use a static variable or a function that sets
    // the global Engine. That's a hack.

    // For the purpose of this migration, we'll change the design:
    // The factory method will be removed from AudioEngineCore and we'll
    // rely on dependency injection. However, changing the interface would
    // break the plan. So we'll keep it and implement it with a static
    // pointer that must be set by the application.

    // We'll add a static function to set the global Engine instance.
    // This is a temporary solution.

    // Since we haven't implemented that, we'll return nullptr and log an error.
    jassertfalse; // This should not be called yet.
    return nullptr;
}

} // namespace zenith