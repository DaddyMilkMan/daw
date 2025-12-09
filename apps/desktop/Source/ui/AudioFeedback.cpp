/*
  ==============================================================================
    AudioFeedback.cpp
    Audio feedback system for UI interactions
    
    Plays synthesized audio cues for button clicks, errors, and notifications.
  ==============================================================================
*/

#include "AudioFeedback.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include <cmath>

namespace zenith {

//==============================================================================
// Internal Synthesizer for UI Sounds
//==============================================================================
class FeedbackSynth : public juce::AudioSource {
public:
    FeedbackSynth() = default;

    void prepareToPlay(int /*samplesPerBlockExpected*/, double sampleRate) override {
        sampleRate_ = sampleRate;
    }

    void releaseResources() override {}

    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override {
        bufferToFill.clearActiveBufferRegion();

        if (!isPlaying_) return;

        float* channelData = bufferToFill.buffer->getWritePointer(0, bufferToFill.startSample);
        int numSamples = bufferToFill.numSamples;

        for (int i = 0; i < numSamples; ++i) {
            if (samplesPlayed_ >= totalDurationSamples_) {
                isPlaying_ = false;
                break;
            }

            // Simple Sine Wave
            float currentSample = (float)std::sin(currentPhase_) * amplitude_;
            
            // Apply Envelope (AR)
            float env = 1.0f;
            if (samplesPlayed_ < attackSamples_) {
                env = (float)samplesPlayed_ / attackSamples_;
            } else if (samplesPlayed_ > totalDurationSamples_ - releaseSamples_) {
                env = (float)(totalDurationSamples_ - samplesPlayed_) / releaseSamples_;
            }

            channelData[i] += currentSample * env;

            // Duplicate to other channels
            for (int ch = 1; ch < bufferToFill.buffer->getNumChannels(); ++ch) {
                bufferToFill.buffer->setSample(ch, bufferToFill.startSample + i, channelData[i]);
            }

            currentPhase_ += phaseIncrement_;
            samplesPlayed_++;
        }
    }

    void trigger(float frequency, float durationSec, float amplitude) {
        if (sampleRate_ <= 0) return;

        frequency_ = frequency;
        amplitude_ = amplitude;
        totalDurationSamples_ = (int)(durationSec * sampleRate_);
        attackSamples_ = (int)(0.005 * sampleRate_); // 5ms attack
        releaseSamples_ = (int)(0.05 * sampleRate_); // 50ms release
        
        phaseIncrement_ = (frequency_ * 2.0f * juce::MathConstants<float>::pi) / (float)sampleRate_;
        currentPhase_ = 0.0f;
        samplesPlayed_ = 0;
        isPlaying_ = true;
    }

private:
    double sampleRate_ = 0.0;
    std::atomic<bool> isPlaying_{false};
    
    float frequency_ = 440.0f;
    float amplitude_ = 0.5f;
    float currentPhase_ = 0.0f;
    float phaseIncrement_ = 0.0f;
    
    int samplesPlayed_ = 0;
    int totalDurationSamples_ = 0;
    int attackSamples_ = 0;
    int releaseSamples_ = 0;
};

//==============================================================================
// Static State
//==============================================================================
static std::unique_ptr<juce::AudioDeviceManager> deviceManager;
static std::unique_ptr<juce::AudioSourcePlayer> sourcePlayer;
static std::unique_ptr<FeedbackSynth> synthSource;

//==============================================================================
// Public API
//==============================================================================

void AudioFeedback::initialize() {
    if (deviceManager) return; // Already initialized

    deviceManager = std::make_unique<juce::AudioDeviceManager>();
    // Initialize with no inputs, default outputs
    deviceManager->initialise(0, 2, nullptr, true, "", nullptr);

    synthSource = std::make_unique<FeedbackSynth>();
    sourcePlayer = std::make_unique<juce::AudioSourcePlayer>();
    sourcePlayer->setSource(synthSource.get());
    
    deviceManager->addAudioCallback(sourcePlayer.get());
    
    DBG("AudioFeedback: Initialized real-time synthesis engine");
}

void AudioFeedback::shutdown() {
    if (deviceManager) {
        deviceManager->removeAudioCallback(sourcePlayer.get());
        sourcePlayer->setSource(nullptr);
        sourcePlayer.reset();
        synthSource.reset();
        deviceManager.reset();
    }
    DBG("AudioFeedback: Shutdown");
}

void AudioFeedback::play(const juce::String& id) {
    // Ensure initialized (lazy init if needed, though main should call it)
    if (!synthSource) initialize();

    if (id.equalsIgnoreCase("click")) {
        // High tick
        synthSource->trigger(1200.0f, 0.05f, 0.1f); 
    } else if (id.equalsIgnoreCase("error")) {
        // Low buzz
        synthSource->trigger(150.0f, 0.2f, 0.3f);
    } else if (id.equalsIgnoreCase("success")) {
        // Pleasant chime
        synthSource->trigger(880.0f, 0.15f, 0.2f);
    } else if (id.equalsIgnoreCase("notify")) {
        // Mid tone
        synthSource->trigger(440.0f, 0.1f, 0.2f);
    } else {
        // Default click
        synthSource->trigger(1000.0f, 0.02f, 0.05f);
    }
    
    DBG("AudioFeedback: Synthesizing cue for '" + id + "'");
}

} // namespace zenith