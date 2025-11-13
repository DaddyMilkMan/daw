/**
 * AudioEngine.h
 * Core audio processing engine for Vexel DAW
 *
 * Handles real-time audio I/O, plugin hosting, routing, and DSP graph management.
 * Thread-safe design with lock-free queues for audio/UI communication.
 */

#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>
#include <atomic>
#include "DSPGraph.h"
#include "MidiRouter.h"
#include "AudioDevice.h"
#include "Track.h"
#include "PluginHost.h"

namespace vexel {

/**
 * Transport state for playback control
 */
enum class TransportState {
    Stopped,
    Playing,
    Recording,
    Paused
};

/**
 * Main audio engine coordinating all audio processing
 */
class AudioEngine : public juce::AudioIODeviceCallback,
                   public juce::MidiInputCallback
{
public:
    AudioEngine();
    ~AudioEngine() override;

    // ========================================================================
    // Initialization & Lifecycle
    // ========================================================================

    /**
     * Initialize audio engine with default device
     * @return true if initialization successful
     */
    bool initialize();

    /**
     * Shutdown audio engine and release resources
     */
    void shutdown();

    // ========================================================================
    // Audio Device Management
    // ========================================================================

    /**
     * Get available audio device types (ASIO, CoreAudio, ALSA, etc.)
     */
    juce::StringArray getAvailableDeviceTypes() const;

    /**
     * Get available devices for a given type
     */
    juce::StringArray getAvailableDevices(const juce::String& deviceType) const;

    /**
     * Set active audio device
     */
    bool setAudioDevice(const juce::String& deviceType, const juce::String& deviceName);

    /**
     * Configure audio settings
     */
    bool setAudioSettings(int sampleRate, int bufferSize);

    /**
     * Get current audio device info
     */
    juce::String getCurrentDeviceName() const;
    int getCurrentSampleRate() const;
    int getCurrentBufferSize() const;

    // ========================================================================
    // Transport Control
    // ========================================================================

    void play();
    void stop();
    void pause();
    void record();
    void togglePlayPause();

    bool isPlaying() const { return m_transportState.load() == TransportState::Playing; }
    bool isRecording() const { return m_transportState.load() == TransportState::Recording; }
    TransportState getTransportState() const { return m_transportState.load(); }

    // ========================================================================
    // Timeline & Tempo
    // ========================================================================

    void setTempo(double bpm);
    double getTempo() const { return m_tempo.load(); }

    void setTimeSignature(int numerator, int denominator);
    void getTimeSignature(int& numerator, int& denominator) const;

    void setLoopEnabled(bool enabled);
    void setLoopRange(double startBeat, double endBeat);

    // Current playback position
    double getCurrentPositionBeats() const { return m_currentPositionBeats.load(); }
    double getCurrentPositionSeconds() const;
    void setCurrentPosition(double positionBeats);

    // ========================================================================
    // Track Management
    // ========================================================================

    Track* addAudioTrack(const juce::String& name);
    Track* addMidiTrack(const juce::String& name);
    void removeTrack(int trackId);

    Track* getTrack(int trackId);
    int getNumTracks() const { return static_cast<int>(m_tracks.size()); }

    // ========================================================================
    // DSP Graph Access
    // ========================================================================

    DSPGraph* getDSPGraph() { return m_dspGraph.get(); }
    MidiRouter* getMidiRouter() { return m_midiRouter.get(); }
    PluginHost* getPluginHost() { return m_pluginHost.get(); }

    // ========================================================================
    // Master Output
    // ========================================================================

    void setMasterVolume(float gainDb);
    float getMasterVolume() const { return m_masterGainDb.load(); }

    void setMasterMute(bool muted);
    bool isMasterMuted() const { return m_masterMuted.load(); }

    // CPU & Performance Monitoring
    float getCpuLoad() const;
    int getXrunCount() const { return m_xrunCount.load(); }
    void resetXrunCount() { m_xrunCount.store(0); }

protected:
    // ========================================================================
    // JUCE Audio Callbacks (real-time thread)
    // ========================================================================

    void audioDeviceIOCallbackWithContext(
        const float* const* inputChannelData,
        int numInputChannels,
        float* const* outputChannelData,
        int numOutputChannels,
        int numSamples,
        const juce::AudioIODeviceCallbackContext& context) override;

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

    // MIDI input callback
    void handleIncomingMidiMessage(juce::MidiInput* source,
                                   const juce::MidiMessage& message) override;

private:
    // Core components
    std::unique_ptr<juce::AudioDeviceManager> m_deviceManager;
    std::unique_ptr<DSPGraph> m_dspGraph;
    std::unique_ptr<MidiRouter> m_midiRouter;
    std::unique_ptr<PluginHost> m_pluginHost;

    // Track management
    std::vector<std::unique_ptr<Track>> m_tracks;
    juce::CriticalSection m_trackLock;
    int m_nextTrackId = 1;

    // Transport state (atomic for lock-free access)
    std::atomic<TransportState> m_transportState { TransportState::Stopped };
    std::atomic<double> m_currentPositionBeats { 0.0 };
    std::atomic<double> m_tempo { 120.0 };

    int m_timeSigNumerator = 4;
    int m_timeSigDenominator = 4;

    bool m_loopEnabled = false;
    double m_loopStartBeats = 0.0;
    double m_loopEndBeats = 16.0;

    // Master output
    std::atomic<float> m_masterGainDb { 0.0f };
    std::atomic<bool> m_masterMuted { false };
    juce::LinearSmoothedValue<float> m_masterGainSmoothed;

    // Performance monitoring
    std::atomic<int> m_xrunCount { 0 };
    juce::AudioProcessLoadMeasurer m_cpuMeter;

    // Sample rate info (set in audioDeviceAboutToStart)
    double m_sampleRate = 44100.0;
    int m_blockSize = 512;

    // Internal helpers
    void processBlock(const float* const* input, float* const* output,
                     int numInputs, int numOutputs, int numSamples);
    void updateTransportPosition(int numSamples);
    double beatsToSeconds(double beats) const;
    double secondsToBeats(double seconds) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEngine)
};

} // namespace vexel
