/**
 * Audio Engine - Core JUCE audio processing engine
 *
 * This is a placeholder header. When JUCE is integrated, this will
 * contain the full audio engine implementation.
 */

#pragma once

// TODO: Include JUCE headers when integrated
// #include <JuceHeader.h>

#include <memory>
#include <vector>
#include <string>

/**
 * Main audio engine class
 * Handles real-time audio processing, plugin hosting, and MIDI
 */
class AudioEngine {
public:
    AudioEngine();
    ~AudioEngine();

    // Initialization
    bool initialize();
    void shutdown();

    // Transport controls
    void play();
    void stop();
    void pause();
    void record();
    void setTempo(double bpm);
    void setTimeSignature(int numerator, int denominator);

    // Audio device management
    bool setAudioDevice(const std::string& deviceName);
    bool setAudioDeviceType(const std::string& typeName); // ASIO, WASAPI, DirectSound, MME
    void setBufferSize(int samples);
    void setSampleRate(int rate);
    std::vector<std::string> getAvailableDeviceTypes() const;
    std::vector<std::string> getAvailableDevices(const std::string& typeName) const;
    std::string getCurrentDeviceType() const;

    // Track management
    int addTrack(const std::string& name, bool isAudio);
    void removeTrack(int trackId);
    void setTrackVolume(int trackId, float volume);
    void setTrackPan(int trackId, float pan);
    void setTrackMute(int trackId, bool muted);
    void setTrackSolo(int trackId, bool solo);

    // State queries
    bool isPlaying() const { return m_isPlaying; }
    double getTempo() const { return m_tempo; }
    double getCurrentBar() const { return m_currentBar; }

private:
    // TODO: JUCE audio device manager, processor graph, etc.
    // std::unique_ptr<juce::AudioDeviceManager> m_deviceManager;
    // std::unique_ptr<juce::AudioProcessorGraph> m_processorGraph;

    bool m_isPlaying = false;
    double m_tempo = 120.0;
    double m_currentBar = 0.0;
    int m_sampleRate = 44100;
    int m_bufferSize = 512;
};
