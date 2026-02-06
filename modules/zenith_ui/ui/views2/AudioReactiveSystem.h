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
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <array>
#include <functional>

namespace zenith::ui {

//==============================================================================
// Audio Analysis Data Structure
//==============================================================================

/**
 * @brief Real-time audio analysis data for reactive visuals
 */
struct AudioAnalysisData {
    // Frequency analysis (32 bands)
    std::array<float, 32> frequencies{};  // Initialized to 0
    float bassLevel = 0.0f;        // 0-100, average of first 4 bands
    float midLevel = 0.0f;         // 0-100, average of bands 8-16
    float trebleLevel = 0.0f;      // 0-100, average of bands 24-32
    float overallLevel = 0.0f;     // 0-1, overall RMS level
    float peakLevel = 0.0f;        // 0-1, peak level over last 100ms

    // Dynamic analysis
    float spectralCentroid = 0.0f;  // "brightness" of sound (0-20000 Hz)
    float spectralSpread = 0.0f;    // Width of frequency distribution
    float zeroCrossingRate = 0.0f; // 0-1, indicates noise vs tonal content

    // Tempo and rhythm detection
    float tempo = 120.0f;          // BPM detected (default 120)
    float beatIntensity = 0.0f;    // 0-1, strength of current beat
    bool isOnBeat = false;         // True if on detected beat
    float phase = 0.0f;            // 0-1, phase within beat cycle

    // Transient detection
    bool hasTransient = false;     // True if transient detected
    float transientStrength = 0.0f; // 0-1, strength of transient
    juce::uint64 lastTransientTime = 0; // Time of last transient
};

//==============================================================================
// Audio Reactivity Configuration
//==============================================================================

/**
 * @brief Configuration for audio-reactive visual effects
 */
struct AudioReactiveConfig {
    // Sensitivity settings
    float bassSensitivity = 1.0f;        // Multiplier for bass response
    float midSensitivity = 1.0f;         // Multiplier for mid response
    float trebleSensitivity = 1.0f;      // Multiplier for treble response
    float overallSensitivity = 1.0f;     // Multiplier for overall level
    float transientSensitivity = 1.0f;    // Multiplier for transient response

    // Smoothing settings
    float frequencySmoothing = 0.8f;    // Low-pass filter for frequencies
    float levelSmoothing = 0.9f;         // Low-pass filter for levels
    float transientSmoothing = 0.7f;     // Low-pass filter for transients

    // Visual mapping
    bool enableBassReactivity = true;     // Enable bass-driven effects
    bool enableMidReactivity = true;      // Enable mid-driven effects
    bool enableTrebleReactivity = true;   // Enable treble-driven effects
    bool enableBeatReactivity = true;     // Enable beat-synced animations
    bool enableTransientReactivity = true; // Enable transient-triggered effects

    // Visual scaling
    float glowIntensityScale = 1.0f;     // Scale glow intensity based on audio
    float animationSpeedScale = 1.0f;    // Scale animation speed based on tempo
    float colorShiftScale = 1.0f;        // Scale color shifts based on spectral content
};

//==============================================================================
// Audio Reactive Visual Effects
//==============================================================================

/**
 * @brief Audio-reactive visual effects for UI elements
 */
struct AudioReactiveEffects {
    // Glow effects
    struct GlowEffect {
        float intensity = 0.0f;      // 0-1, current intensity
        float targetIntensity = 0.0f; // 0-1, target intensity based on audio
        float size = 10.0f;          // Glow size in pixels
        juce::Colour color = juce::Colours::white;   // Glow color
        float frequency = 0.0f;      // Frequency band this glow responds to
    };

    // Pulse effects
    struct PulseEffect {
        float amplitude = 0.0f;     // Pulse amplitude
        float frequency = 2.0f;      // Pulse frequency (Hz)
        float phase = 0.0f;         // Current phase
        bool beatSynced = true;     // True if synced to beat
    };

    // Color modulation
    struct ColorModulation {
        float hueShift = 0.0f;      // Hue shift (0-360 degrees)
        float saturation = 1.0f;     // Saturation modulation (0-1)
        float brightness = 1.0f;    // Brightness modulation (0-1)
        bool respondToBass = false;   // True if bass affects hue
        bool respondToMids = false;   // True if mids affects saturation
        bool respondToTreble = false; // True if trebles affects brightness
    };

    // Distortion effects
    struct DistortionEffect {
        float amount = 0.0f;        // 0-1, amount of distortion
        float frequency = 5000.0f;      // Frequency threshold for distortion
        juce::Colour color = juce::Colours::red;   // Distortion color
    };

    // Current effect values
    std::vector<GlowEffect> glowEffects;
    PulseEffect pulseEffect{};
    ColorModulation colorMod{};
    DistortionEffect distortionEffect{};
};

//==============================================================================
// Audio Reactive System Class
//==============================================================================

/**
 * @brief Real-time audio analysis and reactive visual system
 */
class AudioReactiveSystem : public juce::Timer,
                           public juce::AudioIODeviceCallback {
public:
    //==========================================================================
    // Construction
    //==========================================================================

    AudioReactiveSystem();
    ~AudioReactiveSystem() override;

    //==========================================================================
    // Audio Device Setup
    //==========================================================================

    /**
     * @brief Set up audio device for real-time analysis
     * @param deviceManager Audio device manager
     * @param sampleRate Audio sample rate
     * @param bufferSize Buffer size in samples
     * @return True if successful
     */
    bool setupAudioAnalysis(juce::AudioDeviceManager& deviceManager,
                           double sampleRate = 48000.0,
                           int bufferSize = 512);

    /**
     * @brief Tear down audio analysis
     * @param deviceManager Audio device manager to remove callback from
     */
    void shutdownAudioAnalysis(juce::AudioDeviceManager& deviceManager);

    //==========================================================================
    // Configuration
    //==========================================================================

    /**
     * @brief Set audio reactivity configuration
     * @param config Configuration parameters
     */
    void setReactiveConfig(const AudioReactiveConfig& config);

    /**
     * @brief Get current configuration
     */
    const AudioReactiveConfig& getConfig() const { return config_; }

    /**
     * @brief Enable/disable audio reactivity
     * @param enabled True to enable
     */
    void setEnabled(bool enabled);

    //==========================================================================
    // Visual Effects
    //==========================================================================

    /**
     * @brief Add a glow effect
     * @param frequency Frequency band to respond to (0-31)
     * @param size Glow size in pixels
     * @param color Glow color
     * @return Glow effect index
     */
    int addGlowEffect(int frequency, float size, const juce::Colour& color);

    /**
     * @brief Remove a glow effect
     * @param index Effect index
     */
    void removeGlowEffect(int index);

    /**
     * @brief Configure pulse effect
     * @param frequency Pulse frequency in Hz
     * @param beatSynced True if synced to beat
     */
    void configurePulseEffect(float frequency, bool beatSynced = true);

    /**
     * @brief Configure color modulation
     * @param respondToBass True if bass affects hue
     * @param respondToMids True if mids affects saturation
     * @param respondToTreble True if treble affects brightness
     */
    void configureColorModulation(bool respondToBass,
                                bool respondToMids = false,
                                bool respondToTreble = false);

    /**
     * @brief Configure distortion effect
     * @param amount Distortion amount (0-1)
     * @param frequency Frequency threshold
     * @param color Distortion color
     */
    void configureDistortionEffect(float amount, float frequency, const juce::Colour& color);

    //==========================================================================
    // Audio Data Access
    //==========================================================================

    /**
     * @brief Get current audio analysis data
     */
    const AudioAnalysisData& getAnalysisData() const { return analysisData_; }

    /**
     * @brief Get current reactive effects
     */
    const AudioReactiveEffects& getEffects() const { return effects_; }

    //==========================================================================
    // Integration Methods
    //==========================================================================

    /**
     * @brief Update clip slot colors based on audio analysis
     * @param trackIndex Track index
     * @param sceneIndex Scene index
     * @param baseColor Base color
     * @return Audio-reactive color
     */
    juce::Colour getAudioReactiveColor(int trackIndex, int sceneIndex,
                                     const juce::Colour& baseColor) const;

    /**
     * @brief Get audio-reactive animation phase
     * @param basePhase Base animation phase
     * @return Enhanced phase with audio reactivity
     */
    float getAudioReactivePhase(float basePhase) const;

    /**
     * @brief Get audio-reactive glow intensity
     * @param frequency Frequency band
     * @return Glow intensity (0-1)
     */
    float getGlowIntensity(int frequency) const;

    /**
     * @brief Check if beat occurred
     * @return True if beat detected
     */
    bool isBeatDetected() const { return analysisData_.isOnBeat; }

    //==========================================================================
    // Callbacks
    //==========================================================================

    /**
     * @brief Callback for visual updates
     */
    std::function<void()> onVisualUpdate;

    /**
     * @brief Callback for beat detection
     */
    std::function<void()> onBeatDetected;

    //==========================================================================
    // AudioIODeviceCallback Implementation
    //==========================================================================

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;
    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                        int numInputChannels,
                                        float* const* outputChannelData,
                                        int numOutputChannels,
                                        int numSamples,
                                        const juce::AudioIODeviceCallbackContext& context) override;

private:
    //==========================================================================
    // Audio Analysis Methods
    //==========================================================================

    /**
     * @brief Process audio input data
     * @param inputAudio Input audio data
     * @param numSamples Number of samples
     */
    void processAudio(const float* inputAudio, int numSamples);

    /**
     * @brief Perform FFT analysis
     * @param audioData Audio data
     * @param fftSize FFT size
     */
    void performFFT(const float* audioData, int fftSize);

    /**
     * @brief Analyze frequency spectrum
     */
    void analyzeFrequencies();

    /**
     * @brief Detect tempo and beat
     */
    void detectTempoAndBeat();

    /**
     * @brief Detect transients
     */
    void detectTransients(const float* audioData, int numSamples);

    /**
     * @brief Update analysis data with smoothing
     */
    void updateAnalysisData();

    //==========================================================================
    // Visual Effect Methods
    //==========================================================================

    /**
     * @brief Update glow effects based on audio
     */
    void updateGlowEffects();

    /**
     * @brief Update pulse effect
     */
    void updatePulseEffect();

    /**
     * @brief Update color modulation
     */
    void updateColorModulation();

    /**
     * @brief Update distortion effect
     */
    void updateDistortionEffect();

    //==========================================================================
    // Timer Callback
    //==========================================================================

    void timerCallback() override;

    //==========================================================================
    // Member Variables
    //==========================================================================

    // Audio processing
    juce::AudioBuffer<float> audioBuffer_;
    juce::dsp::FFT fft_;
    std::array<float, 1024> fftData_;
    bool isAudioEnabled_ = false;
    bool isAudioSetup_ = false;

    // Analysis data
    AudioAnalysisData analysisData_;
    AudioAnalysisData previousAnalysisData_;
    AudioReactiveConfig config_;
    AudioReactiveEffects effects_;

    // Timing
    juce::uint64 lastProcessTime_;
    double sampleRate_ = 48000.0;
    int bufferSize_ = 512;

    // Beat detection
    std::array<float, 8> beatHistory_;
    int beatHistoryIndex_ = 0;
    double beatInterval_ = 0.5; // 120 BPM default

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioReactiveSystem)
};

} // namespace zenith::ui