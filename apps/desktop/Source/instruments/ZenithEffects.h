/*
  ==============================================================================

    ZenithEffects.h
    Created: 2025-12-06
    Author:  Zenith DAW

    Global Effects Processor for ZenithPolySynth.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <array>
#include <vector>

namespace zenith {

/**
    Per-Voice Effects Chain
*/
class ZenithEffects {
public:
  ZenithEffects() = default;

  void setSampleRate(double sampleRate) { sampleRate_ = sampleRate; }
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

  void process(float &left, float &right);

private:
  double sampleRate_ = 44100.0;
  float distortionAmount_ = 0.0f;
  float chorusAmount_ = 0.0f;
  float reverbAmount_ = 0.0f;

  // Chorus LFO
  float chorusPhase_ = 0.0f;

  // Real Chorus (Delay Line)
  std::array<float, 2048> delayBufferL_ = {0.0f};
  std::array<float, 2048> delayBufferR_ = {0.0f};
  int delayPos_ = 0;
  
  // Reverb (Comb filters + Allpass)
  // Simple implementation: 4 combs, 2 allpass
  struct Comb {
      std::vector<float> buffer;
      int pos = 0;
      float feedback = 0.84f;
      float damp = 0.2f;
      float val = 0.0f;
      
      void resize(int size) { buffer.resize(size, 0.0f); }
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
      
      void resize(int size) { buffer.resize(size, 0.0f); }
      float process(float input) {
          if (buffer.empty()) return input;
          float bufOut = buffer[pos];
          float output = -input + bufOut;
          buffer[pos] = input + (bufOut * feedback);
          pos = (pos + 1) % buffer.size();
          return output;
      }
  };
  
  std::array<Comb, 4> combs_;
  std::array<Allpass, 2> allpasses_;
  bool reverbInit_ = false;
  
  void initReverb();
  
public:
    bool hasTail() const {
        // Simple check
        return (reverbAmount_ > 0.0f || chorusAmount_ > 0.0f);
    }
};

} // namespace zenith
