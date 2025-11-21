/**
 * @file AudioFeedback.cpp
 * @brief Implementation of UI audio feedback system
 */

#include "AudioFeedback.h"

namespace zenith {

AudioFeedback::AudioFeedback()
{
    // Initialize audio device manager
    juce::String error = audioDeviceManager_.initialise(
        0,  // num input channels
        2,  // num output channels (stereo)
        nullptr,  // no XML settings
        true  // select default device
    );

    if (error.isEmpty())
    {
        // Generate all sound effects
        generateClickSound();
        generateToggleSound();
        generateSuccessSound();
        generateErrorSound();
        generateWhooshSound();
    }
}

void AudioFeedback::playSound(SoundType type, float volume)
{
    // Don't play if disabled OR if muted during playback/recording
    if (!enabled_ || mutedDuringPlayback_) return;

    // Clamp volume
    volume = juce::jlimit(0.0f, 1.0f, volume);

    // Select buffer based on type
    const juce::AudioBuffer<float>* buffer = nullptr;

    switch (type)
    {
        case Click:   buffer = &clickBuffer_; break;
        case Toggle:  buffer = &toggleBuffer_; break;
        case Success: buffer = &successBuffer_; break;
        case Error:   buffer = &errorBuffer_; break;
        case Whoosh:  buffer = &whooshBuffer_; break;
    \n    default: break;\n\n    default: break;\n}

    if (buffer && buffer->getNumSamples() > 0)
    {
        // Play sound using JUCE's audio player
        // Note: In production, you'd use a proper audio player component
        // For now, this is a placeholder for the audio playback system
        DBG("AudioFeedback: Playing sound type " + juce::String(type) + " at volume " + juce::String(volume));
    }
}

void AudioFeedback::generateClickSound()
{
    // Generate a soft, professional click sound (44.1kHz, 50ms)
    const int sampleRate = 44100;
    const int durationMs = 50;
    const int numSamples = (sampleRate * durationMs) / 1000;

    clickBuffer_.setSize(2, numSamples);
    clickBuffer_.clear();

    auto* leftChannel = clickBuffer_.getWritePointer(0);
    auto* rightChannel = clickBuffer_.getWritePointer(1);

    // Generate soft sine wave with envelope
    const float frequency = 800.0f;  // Gentle frequency
    const float twoPi = juce::MathConstants<float>::twoPi;

    for (int i = 0; i < numSamples; ++i)
    {
        float phase = (float)i / (float)sampleRate;
        float sine = std::sin(twoPi * frequency * phase);

        // Exponential decay envelope for natural sound
        float envelope = std::exp(-5.0f * phase / (durationMs / 1000.0f));

        float sample = sine * envelope * 0.15f;  // Keep it subtle
        leftChannel[i] = sample;
        rightChannel[i] = sample;
    }
}

void AudioFeedback::generateToggleSound()
{
    // Two-tone click for toggle (on/off feel)
    const int sampleRate = 44100;
    const int durationMs = 80;
    const int numSamples = (sampleRate * durationMs) / 1000;

    toggleBuffer_.setSize(2, numSamples);
    toggleBuffer_.clear();

    auto* leftChannel = toggleBuffer_.getWritePointer(0);
    auto* rightChannel = toggleBuffer_.getWritePointer(1);

    const float freq1 = 600.0f;  // First tone
    const float freq2 = 900.0f;  // Second tone (higher)
    const float twoPi = juce::MathConstants<float>::twoPi;

    for (int i = 0; i < numSamples; ++i)
    {
        float phase = (float)i / (float)sampleRate;
        float progress = (float)i / (float)numSamples;

        // Blend from freq1 to freq2
        float freq = std::lerp(freq1, freq2, progress);
        float sine = std::sin(twoPi * freq * phase);

        // Sharp attack, quick decay
        float envelope = std::exp(-8.0f * progress);

        float sample = sine * envelope * 0.12f;
        leftChannel[i] = sample;
        rightChannel[i] = sample;
    }
}

void AudioFeedback::generateSuccessSound()
{
    // Pleasant ascending chime for success
    const int sampleRate = 44100;
    const int durationMs = 150;
    const int numSamples = (sampleRate * durationMs) / 1000;

    successBuffer_.setSize(2, numSamples);
    successBuffer_.clear();

    auto* leftChannel = successBuffer_.getWritePointer(0);
    auto* rightChannel = successBuffer_.getWritePointer(1);

    const float twoPi = juce::MathConstants<float>::twoPi;

    for (int i = 0; i < numSamples; ++i)
    {
        float phase = (float)i / (float)sampleRate;
        float progress = (float)i / (float)numSamples;

        // Ascending frequency (major third interval - happy sound)
        float freq = 523.0f + (659.0f - 523.0f) * progress;  // C to E
        float sine = std::sin(twoPi * freq * phase);

        // Gentle envelope
        float envelope = std::exp(-3.0f * progress);

        float sample = sine * envelope * 0.1f;
        leftChannel[i] = sample;
        rightChannel[i] = sample;
    }
}

void AudioFeedback::generateErrorSound()
{
    // Subtle low beep for errors
    const int sampleRate = 44100;
    const int durationMs = 100;
    const int numSamples = (sampleRate * durationMs) / 1000;

    errorBuffer_.setSize(2, numSamples);
    errorBuffer_.clear();

    auto* leftChannel = errorBuffer_.getWritePointer(0);
    auto* rightChannel = errorBuffer_.getWritePointer(1);

    const float frequency = 220.0f;  // Low frequency for "no"
    const float twoPi = juce::MathConstants<float>::twoPi;

    for (int i = 0; i < numSamples; ++i)
    {
        float phase = (float)i / (float)sampleRate;
        float progress = (float)i / (float)numSamples;

        float sine = std::sin(twoPi * frequency * phase);

        // Quick decay
        float envelope = std::exp(-6.0f * progress);

        float sample = sine * envelope * 0.08f;  // Very subtle
        leftChannel[i] = sample;
        rightChannel[i] = sample;
    }
}

void AudioFeedback::generateWhooshSound()
{
    // Swoosh sound for transitions
    const int sampleRate = 44100;
    const int durationMs = 200;
    const int numSamples = (sampleRate * durationMs) / 1000;

    whooshBuffer_.setSize(2, numSamples);
    whooshBuffer_.clear();

    auto* leftChannel = whooshBuffer_.getWritePointer(0);
    auto* rightChannel = whooshBuffer_.getWritePointer(1);

    juce::Random random;

    for (int i = 0; i < numSamples; ++i)
    {
        float progress = (float)i / (float)numSamples;

        // White noise filtered for swoosh
        float noise = random.nextFloat() * 2.0f - 1.0f;

        // Envelope with peak in the middle
        float envelope = 4.0f * progress * (1.0f - progress);

        // Low-pass effect by averaging
        float sample = noise * envelope * 0.05f;
        leftChannel[i] = sample;
        rightChannel[i] = sample;
    }
}

} // namespace zenith


