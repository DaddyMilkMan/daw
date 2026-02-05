/*
  ==============================================================================

    ZenithEffects.h
    Created: 2025-12-06
    Refactored: 2025-12-21 (Pro Audio Upgrade)
    Author:  Zenith DAW

    Global Effects Processor for ZenithPolySynth.
    Features:
    - Stereo interpolated Chorus (Lush/Analog)
    - Stereo Ping-Pong Delay with fractional reads (Smooth)
    - True Stereo Reverb (Wide)

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <array>
#include <vector>
#include <cmath>
#include "ZenithPolySynthDefs.h"

namespace zenith {

/**
    Per-Voice Effects Chain
*/
class ZenithEffects {
public:
  ZenithEffects() = default;

  void setSampleRate(double sampleRate);
  void initDelay();
  void reset() {
    // Clear delay buffers
    delayBufferL_.fill(0.0f);
    delayBufferR_.fill(0.0f);
    delayPos_ = 0;
    chorusPhase_ = 0.0f;
  }

  void setDistortion(float amount) { distortionAmount_ = amount; }
  void setChorus(float amount) { chorusAmount_ = amount; }
  void setReverb(float amount) { reverbAmount_ = amount; }
  void setDelay(float time, float feedback, float mix) {
      delayTime_ = time; delayFeedback_ = feedback; delayMix_ = mix;
  }
  void setBpm(double bpm) { bpm_ = bpm; }
  void setDelaySync(bool sync, SyncRate rate) { delaySync_ = sync; delaySyncRate_ = rate; }
  
  void setBlockSize(int blockSize) { 
    juce::ignoreUnused(blockSize);
  }

  void process(float &left, float &right);

  bool hasTail() const {
      return (reverbAmount_ > 0.0f || chorusAmount_ > 0.0f || delayMix_ > 0.0f);
  }

private:
  double sampleRate_ = 44100.0;
  double bpm_ = 120.0;
  bool delaySync_ = false;
  SyncRate delaySyncRate_ = SyncRate::_1_4;
  float distortionAmount_ = 0.0f;
  float chorusAmount_ = 0.0f;
  float reverbAmount_ = 0.0f;
  
  float delayTime_ = 0.5f;     // Seconds
  float delayFeedback_ = 0.5f; // 0..1
  float delayMix_ = 0.0f;      // 0..1
  std::vector<float> echoBufferL_;
  std::vector<float> echoBufferR_;
  int echoPos_ = 0;

  // Chorus LFO
  float chorusPhase_ = 0.0f;

  // Real Chorus (Delay Line)
  std::array<float, 2048> delayBufferL_ = {0.0f};
  std::array<float, 2048> delayBufferR_ = {0.0f};
  int delayPos_ = 0;
  
  // Reverb (Comb filters + Allpass)
  // Simple implementation: 4 combs, 2 allpass per channel (True Stereo)
  struct Comb {
      std::vector<float> buffer;
      int pos = 0;
      float feedback = 0.84f;
      float damp = 0.2f;
      float val = 0.0f;
      
      void resize(int size) {
          buffer.resize(size, 0.0f);
          pos = 0;
          val = 0.0f;
      }
      float process(float input) {
          if (buffer.empty()) return input;
          float output = buffer[pos];
          val = output * (1.0f - damp) + val * damp;
          buffer[pos] = input + val * feedback;
          pos = (pos + 1) % buffer.size();
          return output;
      }
  };
  
  struct Allpass {
      std::vector<float> buffer;
      int pos = 0;
      float feedback = 0.5f;
      
      void resize(int size) {
          buffer.resize(size, 0.0f);
          pos = 0;
      }
      float process(float input) {
          if (buffer.empty()) return input;
          float bufOut = buffer[pos];
          float output = -input + bufOut;
          buffer[pos] = input + (bufOut * feedback);
          pos = (pos + 1) % buffer.size();
          return output;
      }
  };
  
  // True Stereo Reverb Tank
  std::array<Comb, 4> combsL_;
  std::array<Comb, 4> combsR_;
  std::array<Allpass, 2> allpassesL_;
  std::array<Allpass, 2> allpassesR_;
  bool reverbInit_ = false;
  
  void initReverb();

  // Helper for Linear Interpolation
  // Reads from a circular buffer using a floating point position
  template <typename Container>
  inline float getInterpolatedSample(const Container& buffer, float readPos) const {
      if (buffer.empty()) return 0.0f;

      int size = static_cast<int>(buffer.size());

      // Wrap readPos to [0, size)
      while (readPos < 0.0f) readPos += size;
      while (readPos >= size) readPos -= size;

      int index0 = static_cast<int>(readPos);
      int index1 = (index0 + 1) % size;
      float frac = readPos - index0;

      return buffer[index0] * (1.0f - frac) + buffer[index1] * frac;
  }
};

} // namespace zenith
