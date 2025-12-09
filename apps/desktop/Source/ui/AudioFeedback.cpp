/*
  ==============================================================================
    AudioFeedback.cpp
    Audio feedback system for UI interactions
<<<<<<< HEAD
    
    Plays synthesized audio cues for button clicks, errors, and notifications.
=======

    Plays short audio cues for button clicks, errors, and notifications
    using synthesized tones for minimal resource usage.
>>>>>>> origin/master
  ==============================================================================
*/

#include "AudioFeedback.h"
#include <cmath>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include <cmath>


namespace zenith {

//==============================================================================
<<<<<<< HEAD
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
=======
// Simple tone synthesizer for feedback sounds
//==============================================================================
class FeedbackToneSource : public juce::AudioSource {
public:
  FeedbackToneSource() = default;

  void setTone(float frequency, float durationMs, float amplitude = 0.3f) {
    frequency_ = frequency;
    durationSamples_ = (int)(sampleRate_ * durationMs / 1000.0f);
    amplitude_ = amplitude;
    currentSample_ = 0;
    isPlaying_ = true;
  }

  void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override {
    juce::ignoreUnused(samplesPerBlockExpected);
    sampleRate_ = (float)sampleRate;
  }

  void releaseResources() override {}

  void
  getNextAudioBlock(const juce::AudioSourceChannelInfo &bufferToFill) override {
    if (!isPlaying_) {
      bufferToFill.clearActiveBufferRegion();
      return;
    }

    float *leftChannel =
        bufferToFill.buffer->getWritePointer(0, bufferToFill.startSample);
    float *rightChannel =
        bufferToFill.buffer->getNumChannels() > 1
            ? bufferToFill.buffer->getWritePointer(1, bufferToFill.startSample)
            : nullptr;

    for (int i = 0; i < bufferToFill.numSamples; ++i) {
      if (currentSample_ >= durationSamples_) {
        isPlaying_ = false;
        leftChannel[i] = 0.0f;
        if (rightChannel)
          rightChannel[i] = 0.0f;
        continue;
      }

      // Generate sine wave with envelope
      float t = (float)currentSample_ / sampleRate_;
      float envelope = 1.0f;

      // Attack (first 5ms)
      float attackSamples = sampleRate_ * 0.005f;
      if (currentSample_ < attackSamples) {
        envelope = currentSample_ / attackSamples;
      }

      // Release (last 20%)
      float releaseSamples = durationSamples_ * 0.2f;
      if (currentSample_ > durationSamples_ - releaseSamples) {
        envelope = (durationSamples_ - currentSample_) / releaseSamples;
      }

      float sample =
          amplitude_ * envelope * std::sin(2.0f * 3.14159265f * frequency_ * t);
      leftChannel[i] = sample;
      if (rightChannel)
        rightChannel[i] = sample;

      currentSample_++;
    }
  }

  bool isPlaying() const { return isPlaying_; }

private:
  float frequency_ = 440.0f;
  float amplitude_ = 0.3f;
  float sampleRate_ = 44100.0f;
  int durationSamples_ = 0;
  int currentSample_ = 0;
  bool isPlaying_ = false;
};

//==============================================================================
// Static members
//==============================================================================
static std::unique_ptr<juce::AudioDeviceManager> feedbackDeviceManager;
static std::unique_ptr<juce::AudioSourcePlayer> feedbackPlayer;
static std::unique_ptr<FeedbackToneSource> toneSource;
static bool isInitialized = false;

//==============================================================================
// AudioFeedback Implementation
//==============================================================================

void AudioFeedback::play(const juce::String &id) {
  // If not initialized, just log
  if (!isInitialized || !toneSource) {
    DBG("AudioFeedback: " + id + " (not initialized)");
    return;
  }

  // Map feedback IDs to tones
  if (id == "click") {
    // Short, high-pitched click (1000Hz, 30ms)
    toneSource->setTone(1000.0f, 30.0f, 0.2f);
  } else if (id == "error") {
    // Low warning tone (300Hz, 150ms)
    toneSource->setTone(300.0f, 150.0f, 0.4f);
  } else if (id == "success") {
    // Pleasant ascending chime (800Hz, 100ms)
    toneSource->setTone(800.0f, 100.0f, 0.25f);
  } else if (id == "notify") {
    // Soft notification (600Hz, 80ms)
    toneSource->setTone(600.0f, 80.0f, 0.2f);
  } else if (id == "hover") {
    // Very subtle hover feedback (1200Hz, 15ms, quiet)
    toneSource->setTone(1200.0f, 15.0f, 0.1f);
  } else {
    // Generic feedback (500Hz, 50ms)
    toneSource->setTone(500.0f, 50.0f, 0.15f);
  }

  DBG("AudioFeedback: " + id);
}

void AudioFeedback::initialize() {
  if (isInitialized)
    return;

  try {
    feedbackDeviceManager = std::make_unique<juce::AudioDeviceManager>();
    toneSource = std::make_unique<FeedbackToneSource>();
    feedbackPlayer = std::make_unique<juce::AudioSourcePlayer>();

    // Initialize with default output device
    auto result = feedbackDeviceManager->initialiseWithDefaultDevices(0, 2);

    if (result.isEmpty()) {
      feedbackPlayer->setSource(toneSource.get());
      feedbackDeviceManager->addAudioCallback(feedbackPlayer.get());
      isInitialized = true;
      DBG("AudioFeedback system initialized successfully");
    } else {
      DBG("AudioFeedback: Failed to initialize audio device: " + result);
      // Clean up on failure
      feedbackPlayer.reset();
      toneSource.reset();
      feedbackDeviceManager.reset();
    }
  } catch (...) {
    DBG("AudioFeedback: Exception during initialization");
    feedbackPlayer.reset();
    toneSource.reset();
    feedbackDeviceManager.reset();
  }
}

void AudioFeedback::shutdown() {
  if (feedbackDeviceManager && feedbackPlayer) {
    feedbackDeviceManager->removeAudioCallback(feedbackPlayer.get());
  }

  if (feedbackPlayer) {
    feedbackPlayer->setSource(nullptr);
  }

  feedbackPlayer.reset();
  toneSource.reset();
  feedbackDeviceManager.reset();
  isInitialized = false;

  DBG("AudioFeedback system shut down");
>>>>>>> origin/master
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