/**
 * @file AudioFeedback.h
 * @brief Subtle audio feedback for UI interactions
 *
 * Provides minimal, high-quality click sounds for premium feel.
 * Sounds are generated programmatically (no external files needed).
 */

#pragma once

#include <JuceHeader.h>

namespace zenith {

/**
 * @class AudioFeedback
 * @brief Manages UI sound effects
 *
 * Features:
 * - Soft click sound for buttons
 * - Success chime for positive actions
 * - Error beep for negative actions
 * - Toggle switch sound
 * - All sounds are subtle and professional
 */
class AudioFeedback
{
public:
    enum SoundType
    {
        Click,      // Soft click for normal buttons
        Toggle,     // Two-tone for toggle switches
        Success,    // Pleasant chime for success
        Error,      // Subtle beep for errors
        Whoosh      // Swipe/transition sound
    };

    /**
     * @brief Get the singleton instance
     */
    static AudioFeedback& getInstance()
    {
        static AudioFeedback instance;
        return instance;
    }

    /**
     * @brief Play a UI sound effect
     * @param type The type of sound to play
     * @param volume Volume level (0.0 to 1.0), default 0.3 for subtlety
     */
    void playSound(SoundType type, float volume = 0.3f);

    /**
     * @brief Enable or disable all UI sounds
     */
    void setEnabled(bool shouldBeEnabled) [[maybe_unused]] { enabled_ = shouldBeEnabled; }

    /**
     * @brief Check if UI sounds are enabled
     */
    bool isEnabled() const { return enabled_; }

    /**
     * @brief Mute UI sounds during playback/recording
     * @param shouldMute True to mute during playback/recording, false to allow sounds
     *
     * Call this when transport state changes:
     * - setMutedDuringPlayback(true) when playback or recording starts
     * - setMutedDuringPlayback(false) when playback or recording stops
     */
    void setMutedDuringPlayback(bool shouldMute) [[maybe_unused]] { mutedDuringPlayback_ = shouldMute; }

    /**
     * @brief Check if sounds are muted due to playback/recording
     */
    bool isMutedDuringPlayback() const { return mutedDuringPlayback_; }

private:
    AudioFeedback();
    ~AudioFeedback() = default;

    // Generate synth sounds programmatically
    void generateClickSound();
    void generateToggleSound();
    void generateSuccessSound();
    void generateErrorSound();
    void generateWhooshSound();

    bool enabled_ = true;
    bool mutedDuringPlayback_ = false;  // Mute during playback/recording

    // Simple audio player (using JUCE's audio system)
    juce::AudioDeviceManager audioDeviceManager_;

    // Pre-generated sound buffers
    juce::AudioBuffer<float> clickBuffer_;
    juce::AudioBuffer<float> toggleBuffer_;
    juce::AudioBuffer<float> successBuffer_;
    juce::AudioBuffer<float> errorBuffer_;
    juce::AudioBuffer<float> whooshBuffer_;

    JUCE_DECLARE_NON_COPYABLE(AudioFeedback)
};

} // namespace zenith

