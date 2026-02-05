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

/*
    ==============================================================================
    Original file header:
*/

  ==============================================================================
    AudioFeedback.cpp
    Audio feedback system for UI interactions

    Plays short audio cues for button clicks, errors, and notifications
    using synthesized tones for minimal resource usage.
  ==============================================================================
*/


#include "AudioFeedback.h"
#include <cmath>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>


namespace zenith {

//==============================================================================
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
  } catch (const std::exception& e) {
    DBG("AudioFeedback: Exception during initialization: " + juce::String(e.what()));
    feedbackPlayer.reset();
    toneSource.reset();
    feedbackDeviceManager.reset();
  } catch (...) {
    DBG("AudioFeedback: Unknown exception during initialization");
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
}

} // namespace zenith
