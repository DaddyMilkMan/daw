/**
 * AudioEngine.cpp
 * Implementation of core audio engine
 */

#include "AudioEngine.h"

namespace zenith {

AudioEngine::AudioEngine()
{
    m_deviceManager = std::make_unique<juce::AudioDeviceManager>();
    m_dspGraph = std::make_unique<DSPGraph>();
    m_midiRouter = std::make_unique<MidiRouter>();
    m_pluginHost = std::make_unique<PluginHost>();
}

AudioEngine::~AudioEngine()
{
    shutdown();
}

// ============================================================================
// Initialization
// ============================================================================

bool AudioEngine::initialize()
{
    juce::Logger::writeToLog("AudioEngine: Initializing...");

    // Initialize audio device with default settings
    juce::String error = m_deviceManager->initialise(
        2,   // num input channels
        2,   // num output channels
        nullptr,  // saved state
        true  // select default device
    );

    if (error.isNotEmpty()) {
        juce::Logger::writeToLog("AudioEngine: Failed to initialize - " + error);
        return false;
    }

    // Enable all available MIDI inputs
    for (auto& midiInput : juce::MidiInput::getAvailableDevices()) {
        m_deviceManager->setMidiInputDeviceEnabled(midiInput.identifier, true);
        m_deviceManager->addMidiInputDeviceCallback(midiInput.identifier, this);
    }

    // Register audio callback
    m_deviceManager->addAudioCallback(this);

    // Initialize DSP graph
    if (auto* device = m_deviceManager->getCurrentAudioDevice()) {
        m_sampleRate = device->getCurrentSampleRate();
        m_blockSize = device->getCurrentBufferSizeSamples();
        m_dspGraph->prepare(m_sampleRate, m_blockSize);
    }

    juce::Logger::writeToLog("AudioEngine: Initialized successfully");
    return true;
}

void AudioEngine::shutdown()
{
    juce::Logger::writeToLog("AudioEngine: Shutting down...");

    stop();

    m_deviceManager->removeAudioCallback(this);
    m_deviceManager->closeAudioDevice();

    m_tracks.clear();

    juce::Logger::writeToLog("AudioEngine: Shutdown complete");
}

// ============================================================================
// Audio Device Management
// ============================================================================

juce::StringArray AudioEngine::getAvailableDeviceTypes() const
{
    juce::StringArray types;
    for (auto* type : m_deviceManager->getAvailableDeviceTypes())
        types.add(type->getTypeName());
    return types;
}

juce::StringArray AudioEngine::getAvailableDevices(const juce::String& deviceType) const
{
    juce::StringArray devices;

    for (auto* type : m_deviceManager->getAvailableDeviceTypes()) {
        if (type->getTypeName() == deviceType) {
            devices = type->getDeviceNames();
            break;
        }
    }

    return devices;
}

bool AudioEngine::setAudioDevice(const juce::String& deviceType, const juce::String& deviceName)
{
    m_deviceManager->setCurrentAudioDeviceType(deviceType, true);

    juce::AudioDeviceManager::AudioDeviceSetup setup;
    m_deviceManager->getAudioDeviceSetup(setup);
    setup.outputDeviceName = deviceName;

    juce::String error = m_deviceManager->setAudioDeviceSetup(setup, true);
    return error.isEmpty();
}

bool AudioEngine::setAudioSettings(int sampleRate, int bufferSize)
{
    juce::AudioDeviceManager::AudioDeviceSetup setup;
    m_deviceManager->getAudioDeviceSetup(setup);

    setup.sampleRate = sampleRate;
    setup.bufferSize = bufferSize;

    juce::String error = m_deviceManager->setAudioDeviceSetup(setup, true);
    return error.isEmpty();
}

juce::String AudioEngine::getCurrentDeviceName() const
{
    if (auto* device = m_deviceManager->getCurrentAudioDevice())
        return device->getName();
    return "No device";
}

int AudioEngine::getCurrentSampleRate() const
{
    if (auto* device = m_deviceManager->getCurrentAudioDevice())
        return static_cast<int>(device->getCurrentSampleRate());
    return 44100;
}

int AudioEngine::getCurrentBufferSize() const
{
    if (auto* device = m_deviceManager->getCurrentAudioDevice())
        return device->getCurrentBufferSizeSamples();
    return 512;
}

// ============================================================================
// Transport Control
// ============================================================================

void AudioEngine::play()
{
    m_transportState.store(TransportState::Playing);
    juce::Logger::writeToLog("Transport: Play");
}

void AudioEngine::stop()
{
    m_transportState.store(TransportState::Stopped);
    m_currentPositionBeats.store(0.0);
    juce::Logger::writeToLog("Transport: Stop");
}

void AudioEngine::pause()
{
    if (m_transportState.load() == TransportState::Playing) {
        m_transportState.store(TransportState::Paused);
        juce::Logger::writeToLog("Transport: Pause");
    }
}

void AudioEngine::record()
{
    m_transportState.store(TransportState::Recording);
    juce::Logger::writeToLog("Transport: Record");
}

void AudioEngine::togglePlayPause()
{
    auto state = m_transportState.load();
    if (state == TransportState::Playing || state == TransportState::Recording)
        pause();
    else
        play();
}

// ============================================================================
// Tempo & Timeline
// ============================================================================

void AudioEngine::setTempo(double bpm)
{
    m_tempo.store(juce::jlimit(20.0, 999.0, bpm));
}

void AudioEngine::setTimeSignature(int numerator, int denominator)
{
    m_timeSigNumerator = numerator;
    m_timeSigDenominator = denominator;
}

void AudioEngine::getTimeSignature(int& numerator, int& denominator) const
{
    numerator = m_timeSigNumerator;
    denominator = m_timeSigDenominator;
}

void AudioEngine::setLoopEnabled(bool enabled)
{
    m_loopEnabled = enabled;
}

void AudioEngine::setLoopRange(double startBeat, double endBeat)
{
    m_loopStartBeats = startBeat;
    m_loopEndBeats = endBeat;
}

double AudioEngine::getCurrentPositionSeconds() const
{
    return beatsToSeconds(m_currentPositionBeats.load());
}

void AudioEngine::setCurrentPosition(double positionBeats)
{
    m_currentPositionBeats.store(positionBeats);
}

// ============================================================================
// Track Management
// ============================================================================

Track* AudioEngine::addAudioTrack(const juce::String& name)
{
    juce::ScopedLock lock(m_trackLock);

    auto track = std::make_unique<AudioTrack>(m_nextTrackId++, name);
    track->prepare(m_sampleRate, m_blockSize);

    auto* trackPtr = track.get();
    m_tracks.push_back(std::move(track));

    return trackPtr;
}

Track* AudioEngine::addMidiTrack(const juce::String& name)
{
    juce::ScopedLock lock(m_trackLock);

    auto track = std::make_unique<MidiTrack>(m_nextTrackId++, name);
    track->prepare(m_sampleRate, m_blockSize);

    auto* trackPtr = track.get();
    m_tracks.push_back(std::move(track));

    return trackPtr;
}

void AudioEngine::removeTrack(int trackId)
{
    juce::ScopedLock lock(m_trackLock);

    m_tracks.erase(
        std::remove_if(m_tracks.begin(), m_tracks.end(),
            [trackId](const auto& track) { return track->getId() == trackId; }),
        m_tracks.end()
    );
}

Track* AudioEngine::getTrack(int trackId)
{
    juce::ScopedLock lock(m_trackLock);

    for (auto& track : m_tracks) {
        if (track->getId() == trackId)
            return track.get();
    }
    return nullptr;
}

// ============================================================================
// Master Output
// ============================================================================

void AudioEngine::setMasterVolume(float gainDb)
{
    m_masterGainDb.store(gainDb);
}

void AudioEngine::setMasterMute(bool muted)
{
    m_masterMuted.store(muted);
}

float AudioEngine::getCpuLoad() const
{
    return static_cast<float>(m_cpuMeter.getLoadAsProportion());
}

// ============================================================================
// Audio Callback (REAL-TIME THREAD)
// ============================================================================

void AudioEngine::audioDeviceIOCallbackWithContext(
    const float* const* inputChannelData,
    int numInputChannels,
    float* const* outputChannelData,
    int numOutputChannels,
    int numSamples,
    const juce::AudioIODeviceCallbackContext& context)
{
    juce::ignoreUnused(context);

    m_cpuMeter.processBlockStarting();

    // Clear output buffers
    for (int ch = 0; ch < numOutputChannels; ++ch) {
        juce::FloatVectorOperations::clear(outputChannelData[ch], numSamples);
    }

    // Only process if playing or recording
    auto state = m_transportState.load();
    if (state == TransportState::Playing || state == TransportState::Recording) {
        processBlock(inputChannelData, outputChannelData,
                    numInputChannels, numOutputChannels, numSamples);

        updateTransportPosition(numSamples);
    }

    // Apply master gain
    if (!m_masterMuted.load()) {
        float targetGain = juce::Decibels::decibelsToGain(m_masterGainDb.load());
        m_masterGainSmoothed.setTargetValue(targetGain);

        for (int ch = 0; ch < numOutputChannels; ++ch) {
            m_masterGainSmoothed.applyGain(outputChannelData[ch], numSamples);
        }
    } else {
        for (int ch = 0; ch < numOutputChannels; ++ch) {
            juce::FloatVectorOperations::clear(outputChannelData[ch], numSamples);
        }
    }

    m_cpuMeter.processBlockEnding();
}

void AudioEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    m_sampleRate = device->getCurrentSampleRate();
    m_blockSize = device->getCurrentBufferSizeSamples();

    m_masterGainSmoothed.reset(m_sampleRate, 0.05); // 50ms smoothing
    m_cpuMeter.reset(m_sampleRate, m_blockSize);

    m_dspGraph->prepare(m_sampleRate, m_blockSize);

    juce::Logger::writeToLog(juce::String("Audio device started: ") +
                            juce::String(m_sampleRate) + " Hz, " +
                            juce::String(m_blockSize) + " samples");
}

void AudioEngine::audioDeviceStopped()
{
    juce::Logger::writeToLog("Audio device stopped");
}

// ============================================================================
// MIDI Input
// ============================================================================

void AudioEngine::handleIncomingMidiMessage(juce::MidiInput* source,
                                           const juce::MidiMessage& message)
{
    // Route MIDI to the MIDI router
    m_midiRouter->addMidiEvent(message, source->getName());
}

// ============================================================================
// Internal Processing
// ============================================================================

void AudioEngine::processBlock(const float* const* input, float* const* output,
                              int numInputs, int numOutputs, int numSamples)
{
    // Process all tracks
    juce::ScopedTryLock lock(m_trackLock);
    if (lock.isLocked()) {
        for (auto& track : m_tracks) {
            track->process(input, output, numInputs, numOutputs, numSamples,
                          m_currentPositionBeats.load());
        }
    } else {
        // Lock failed - audio thread contention
        m_xrunCount.fetch_add(1);
    }

    // Process DSP graph (master effects chain)
    m_dspGraph->process(output, numOutputs, numSamples);
}

void AudioEngine::updateTransportPosition(int numSamples)
{
    double tempo = m_tempo.load();
    double increment = (tempo / 60.0) * numSamples / m_sampleRate;

    double newPosition = m_currentPositionBeats.load() + increment;

    // Handle looping
    if (m_loopEnabled && newPosition >= m_loopEndBeats) {
        newPosition = m_loopStartBeats + (newPosition - m_loopEndBeats);
    }

    m_currentPositionBeats.store(newPosition);
}

double AudioEngine::beatsToSeconds(double beats) const
{
    double tempo = m_tempo.load();
    return (beats * 60.0) / tempo;
}

double AudioEngine::secondsToBeats(double seconds) const
{
    double tempo = m_tempo.load();
    return (seconds * tempo) / 60.0;
}

} // namespace zenith
