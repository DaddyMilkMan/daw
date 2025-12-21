/*
  ==============================================================================

    ZenithPolySynth.cpp
    Created: 2025-11-18
    Author:  Zenith DAW

    synthesizer.

  ==============================================================================
*/

#include "ZenithPolySynth.h"
#include "../ui/skia/ZenithPolySynthUI.h"
#include "../utils/PresetGenerator.h"
#include <cmath>

namespace zenith {

//==============================================================================
// ZenithOscillator Implementation
//==============================================================================

float ZenithOscillator::getNextSample(float frequency, float shape) {
  // Apply detune
  float detuneMultiplier = std::pow(2.0f, detuneCents_ / 1200.0f);
  float detunedFrequency = frequency * detuneMultiplier;

  switch (waveform_) {
  case OscillatorWaveform::Sine:
    return processSine(detunedFrequency);
  case OscillatorWaveform::Saw:
    return processSaw(detunedFrequency);
  case OscillatorWaveform::Square:
    return processSquare(detunedFrequency, shape);
  case OscillatorWaveform::Triangle:
    return processTriangle(detunedFrequency);
  case OscillatorWaveform::Noise:
    return processNoise();
  case OscillatorWaveform::Supersaw:
    return processSaw(detunedFrequency); // Same as saw for individual osc
  default:
    return 0.0f;
  }
}

float ZenithOscillator::processSine(float frequency) {
  float sample = std::sin(phase_ * juce::MathConstants<double>::twoPi);
  phase_ += frequency / sampleRate_;
  if (phase_ >= 1.0)
    phase_ -= 1.0;
  return sample;
}

float ZenithOscillator::processSaw(float frequency) {
  // PolyBLEP antialiasing for saw wave
  float naiveSaw = static_cast<float>(2.0 * phase_ - 1.0);

  // Simple PolyBLEP implementation
  float t = phase_;
  float dt = frequency / sampleRate_;

  // Correct discontinuity at phase wrap
  if (t < dt) {
    t = t / dt;
    naiveSaw -= (t + t - t * t - 1.0f);
  } else if (t > 1.0 - dt) {
    t = (t - 1.0) / dt;
    naiveSaw -= (t + t + t * t + 1.0f);
  }

  phase_ += dt;
  if (phase_ >= 1.0)
    phase_ -= 1.0;

  return naiveSaw;
}

float ZenithOscillator::processSquare(float frequency, float pulseWidth) {
  // Clamp pulse width to reasonable range (0.05 to 0.95)
  float pw = juce::jlimit(0.05f, 0.95f, pulseWidth);

  // PolyBLEP antialiasing for square wave with variable pulse width
  float naiveSquare = (phase_ < pw) ? 1.0f : -1.0f;

  float t = phase_;
  float dt = frequency / sampleRate_;

  // Correct discontinuity at rising edge (phase = 0)
  if (t < dt) {
    t = t / dt;
    naiveSquare += (t + t - t * t - 1.0f);
  } else if (t > 1.0 - dt) {
    t = (t - 1.0) / dt;
    naiveSquare += (t + t + t * t + 1.0f);
  }

  // Correct discontinuity at falling edge (phase = pw)
  t = phase_ - pw;
  if (t > 0.0 && t < dt) {
    t = t / dt;
    naiveSquare -= (t + t - t * t - 1.0f);
  } else if (t > -dt && t < 0.0) {
    t = (t + dt) / dt;
    naiveSquare -= (t + t + t * t + 1.0f);
  }

  phase_ += dt;
  if (phase_ >= 1.0)
    phase_ -= 1.0;

  return naiveSquare;
}

float ZenithOscillator::processTriangle(float frequency) {
  // Triangle wave from phase
  float triangle;
  if (phase_ < 0.25)
    triangle = 4.0f * static_cast<float>(phase_);
  else if (phase_ < 0.75)
    triangle = 2.0f - 4.0f * static_cast<float>(phase_);
  else
    triangle = -4.0f + 4.0f * static_cast<float>(phase_);

  phase_ += frequency / sampleRate_;
  if (phase_ >= 1.0)
    phase_ -= 1.0;

  return triangle;
}

float ZenithOscillator::processNoise() {
  return random_.nextFloat() * 2.0f - 1.0f;
}

//==============================================================================
// ZenithFilter Implementation
//==============================================================================

void ZenithFilter::setSampleRate(double sampleRate) {
  sampleRate_ = sampleRate;
  cutoffSmoothed_.reset(sampleRate, 0.02); // 20ms smoothing
  resonanceSmoothed_.reset(sampleRate, 0.02);
}

void ZenithFilter::setCutoff(float cutoffHz) {
  cutoffSmoothed_.setTargetValue(juce::jlimit(20.0f, 20000.0f, cutoffHz));
}

void ZenithFilter::setResonance(float resonance) {
  resonanceSmoothed_.setTargetValue(juce::jlimit(0.0f, 1.0f, resonance));
}

void ZenithFilter::reset() {
  v0_ = v1_ = v2_ = 0.0f;
  ic1eq_ = ic2eq_ = 0.0f;
  cutoffSmoothed_.setCurrentAndTargetValue(1000.0f);
  resonanceSmoothed_.setCurrentAndTargetValue(0.5f);
}

float ZenithFilter::processSample(float input) {
  // Get smoothed parameter values
  float cutoff = cutoffSmoothed_.getNextValue();
  float resonance = resonanceSmoothed_.getNextValue();

  // Apply drive/saturation
  input *= drive_;
  input = std::tanh(input);

  // State variable filter (Chamberlin/Hal formulation)
  float fc = cutoff / static_cast<float>(sampleRate_);
  fc = juce::jlimit(0.0001f, 0.45f, fc); // Prevent instability

  // Optimization: Only recompute tan if cutoff changed significantly
  float g;
  if (std::abs(fc - lastCutoff_) > 0.00001f) {
    g = std::tan(juce::MathConstants<float>::pi * fc);
    lastCutoff_ = fc;
    lastG_ = g;
  } else {
    g = lastG_;
  }

  float k = 2.0f - 2.0f * resonance; // Resonance (damping)

  float a1 = 1.0f / (1.0f + g * (g + k));
  float a2 = g * a1;
  float a3 = g * a2;

  v0_ = input;
  v1_ = a1 * ic1eq_ + a2 * (v0_ - ic2eq_);
  v2_ = ic2eq_ + a2 * ic1eq_ + a3 * (v0_ - ic2eq_);

  ic1eq_ = 2.0f * v1_ - ic1eq_;
  ic2eq_ = 2.0f * v2_ - ic2eq_;

  // Select output based on filter type
  switch (type_) {
  case FilterType::Lowpass:
    return v2_;
  case FilterType::Bandpass:
    return v1_;
  case FilterType::Highpass:
    return v0_ - k * v1_ - v2_;
  default:
    return v2_;
  }
}

//==============================================================================
// ZenithEffects Implementation
//==============================================================================

void ZenithEffects::process(float &left, float &right) {
  // 1. Distortion (tanh saturation)
  if (distortionAmount_ > 0.01f) {
    float drive = 1.0f + distortionAmount_ * 4.0f;
    left = std::tanh(left * drive);
    right = std::tanh(right * drive);
  }

  // 2. Chorus (Delay-based)
  if (chorusAmount_ > 0.01f) {
    // LFO for delay time modulation
    float lfo = std::sin(chorusPhase_ * juce::MathConstants<float>::twoPi);
    chorusPhase_ += 0.5f / static_cast<float>(sampleRate_); // 0.5 Hz rate
    if (chorusPhase_ >= 1.0f)
      chorusPhase_ -= 1.0f;

    // Modulate delay time: 10ms to 20ms
    float baseDelay = 0.015f * static_cast<float>(sampleRate_);
    float modDelay = 0.005f * static_cast<float>(sampleRate_) * lfo;

    float delaySamplesL = baseDelay + modDelay;
    float delaySamplesR = baseDelay - modDelay; // Stereo spread

    // Write to delay buffer
    delayBufferL_[delayPos_] = left;
    delayBufferR_[delayPos_] = right;

    // Read from delay buffer (Linear Interpolation)
    auto readDelay = [&](const std::array<float, 2048> &buffer,
                         float delay) -> float {
      float readPos = static_cast<float>(delayPos_) - delay;
      while (readPos < 0.0f)
        readPos += 2048.0f;

      int p0 = static_cast<int>(readPos);
      int p1 = (p0 + 1) % 2048;
      float frac = readPos - p0;

      return buffer[p0] * (1.0f - frac) + buffer[p1] * frac;
    };

    float wetL = readDelay(delayBufferL_, delaySamplesL);
    float wetR = readDelay(delayBufferR_, delaySamplesR);

    // Mix wet/dry
    left = left * (1.0f - chorusAmount_ * 0.5f) + wetL * chorusAmount_;
    right = right * (1.0f - chorusAmount_ * 0.5f) + wetR * chorusAmount_;

    // Increment write position
    delayPos_ = (delayPos_ + 1) % 2048;
  }
}

//==============================================================================
// ZenithPolySynthVoice Implementation
//==============================================================================

ZenithPolySynthVoice::ZenithPolySynthVoice() {
  osc1_.setWaveform(OscillatorWaveform::Saw);
  osc2_.setWaveform(OscillatorWaveform::Square);
  osc3_.setWaveform(OscillatorWaveform::Sine);
  
  // Clear matrix
  for (int i = 0; i < (int)ModulationSource::NumSources; ++i)
    for (int j = 0; j < (int)ModulationDestination::NumDestinations; ++j)
      modulationMatrix_[i][j] = 0.0f;
}

bool ZenithPolySynthVoice::canPlaySound(juce::SynthesiserSound *sound) {
  return dynamic_cast<ZenithPolySynthSound *>(sound) != nullptr;
}

void ZenithPolySynthVoice::startNote(int midiNoteNumber, float velocity,
                                     juce::SynthesiserSound * /*sound*/,
                                     int /*currentPitchWheelPosition*/) {
  currentMidiNote_ = midiNoteNumber; // Store for pitch wheel
  currentFrequency_ = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
  targetFrequency_ = currentFrequency_;
  velocity_ = velocity;

  ampEnvelope_.noteOn();
  modEnvelope_.noteOn();

  osc1_.reset();
  osc2_.reset();
  osc3_.reset();

  // Randomize phases for analog feel
  osc1_.randomizePhase();
  osc2_.randomizePhase();
  osc3_.randomizePhase();

  for (auto &osc : unisonOscillators_) {
    osc.reset();
    osc.randomizePhase();
  }

  // Reset sub oscillator phase for clean bass
  subOscPhase_ = 0.0;

  filter1_.reset();
  filter2_.reset();
  effects_.reset();
}

void ZenithPolySynthVoice::stopNote(float /*velocity*/, bool allowTailOff) {
  if (allowTailOff) {
    ampEnvelope_.noteOff();
    modEnvelope_.noteOff();
  } else {
    clearCurrentNote();
    ampEnvelope_.reset();
    modEnvelope_.reset();
  }
}

void ZenithPolySynthVoice::pitchWheelMoved(int newPitchWheelValue) {
  if (currentMidiNote_ < 0) return; // No note playing
  
  // Convert MIDI pitch wheel (0-16383, center=8192) to semitones
  // Typical range is ±2 semitones (configurable via pitchBendRange_)
  float pitchBendSemitones = ((newPitchWheelValue - 8192.0f) / 8192.0f) * pitchBendRange_;
  
  // Apply to target frequency (glide will smooth it)
  float baseFrequency = juce::MidiMessage::getMidiNoteInHertz(currentMidiNote_);
  float pitchBendMultiplier = std::pow(2.0f, pitchBendSemitones / 12.0f);
  targetFrequency_ = baseFrequency * pitchBendMultiplier;
}

void ZenithPolySynthVoice::controllerMoved(int controllerNumber,
                                           int newControllerValue) {
  if (controllerNumber == 1) {
    setModWheel(newControllerValue / 127.0f);
  }
}

void ZenithPolySynthVoice::channelPressureChanged(int newChannelPressureValue) {
  setAftertouch(newChannelPressureValue / 127.0f);
}

void ZenithPolySynthVoice::setSampleRate(double sampleRate) {
  setCurrentPlaybackSampleRate(sampleRate);
  osc1_.setSampleRate(sampleRate);
  osc2_.setSampleRate(sampleRate);
  osc3_.setSampleRate(sampleRate);

  for (auto &osc : unisonOscillators_) {
    osc.setSampleRate(sampleRate);
  }

  filter1_.setSampleRate(sampleRate);
  filter2_.setSampleRate(sampleRate);
  effects_.setSampleRate(sampleRate);

  ampEnvelope_.setSampleRate(sampleRate);
  modEnvelope_.setSampleRate(sampleRate);
}

void ZenithPolySynthVoice::renderNextBlock(
    juce::AudioBuffer<float> &outputBuffer, int startSample, int numSamples) {
  if (!isVoiceActive())
    return;

  // Process in small sub-blocks for modulation updates
  const int subBlockSize = 32;

  for (int start = 0; start < numSamples; start += subBlockSize) {
    int blockSize = std::min(subBlockSize, numSamples - start);

    // Update modulation
    computeModulation();

    // Apply modulation to parameters
    // Filter envelope: dedicated amount control for intuitive filter sweeps
    float filterEnvMod = currentModEnv_ * filterEnvAmount_ * 10000.0f;
    
    float modCutoff =
        filterCutoff_ +
        filterEnvMod +  // Direct filter envelope
        modulationState_.get(ModulationDestination::FilterCutoff) * 10000.0f;  // Matrix modulation
    filter1_.setCutoff(modCutoff);

    float modResonance = juce::jlimit(0.0f, 1.0f,
        filterResonance_ +
        modulationState_.get(ModulationDestination::FilterResonance));
    filter1_.setResonance(modResonance);

    // Render audio
    for (int i = 0; i < blockSize; ++i) {
      if (!ampEnvelope_.isActive()) {
        clearCurrentNote();
        return;
      }

      float ampEnv = ampEnvelope_.getNextSample();
      float modEnv = modEnvelope_.getNextSample();
      
      // Cache envelope values for modulation system
      currentAmpEnv_ = ampEnv;
      currentModEnv_ = modEnv;

      // Update frequency with glide/portamento
      updateFrequency();

      // Apply pitch modulation (in semitones, converted to frequency multiplier)
      float osc1PitchMod = modulationState_.get(ModulationDestination::Osc1Pitch);
      float osc2PitchMod = modulationState_.get(ModulationDestination::Osc2Pitch);
      float osc3PitchMod = modulationState_.get(ModulationDestination::Osc3Pitch);
      
      float osc1Freq = currentFrequency_ * std::pow(2.0f, osc1PitchMod / 12.0f);
      float osc2Freq = currentFrequency_ * std::pow(2.0f, osc2PitchMod / 12.0f);
      float osc3Freq = currentFrequency_ * std::pow(2.0f, osc3PitchMod / 12.0f);

      // Oscillators
      float osc1 = osc1_.getNextSample(osc1Freq, oscShape_);
      float osc2 = osc2_.getNextSample(osc2Freq, oscShape_);
      float osc3 = osc3_.getNextSample(osc3Freq, oscShape_);

      // Unison
      float unisonOutput = 0.0f;
      int effectiveUnisonVoices = unisonVoices_;
      if (qualityPreset_ == QualityPreset::Low)
        effectiveUnisonVoices = std::min(effectiveUnisonVoices, 3);
      else if (qualityPreset_ == QualityPreset::Medium)
        effectiveUnisonVoices = std::min(effectiveUnisonVoices, 5);

      if (effectiveUnisonVoices > 1) {
        for (int v = 0; v < effectiveUnisonVoices; ++v) {
          float detune = (v - effectiveUnisonVoices / 2.0f) * unisonDetune_;
          unisonOscillators_[v].setDetune(detune * 100.0f); // Scale up detune

          // Fix Issue 11: Sync unison waveform with Osc 1
          unisonOscillators_[v].setWaveform(osc1_.getWaveform());

          unisonOutput +=
              unisonOscillators_[v].getNextSample(osc1Freq, oscShape_);  // Use osc1Freq with pitch modulation
        }
        unisonOutput /= effectiveUnisonVoices; // Normalize
        osc1 = (osc1 + unisonOutput) * 0.5f;
      }

      // Apply mix modulation
      float osc1MixMod = juce::jlimit(0.0f, 1.0f, 
          osc1Mix_ + modulationState_.get(ModulationDestination::Osc1Mix));
      float osc2MixMod = juce::jlimit(0.0f, 1.0f,
          osc2Mix_ + modulationState_.get(ModulationDestination::Osc2Mix));
      float osc3MixMod = juce::jlimit(0.0f, 1.0f,
          osc3Mix_ + modulationState_.get(ModulationDestination::Osc3Mix));

      // Mix
      float mixed = osc1 * osc1MixMod + osc2 * osc2MixMod + osc3 * osc3MixMod;

      // Sub Oscillator (sine wave, -1 octave for fat bass)
      if (subOscLevel_ > 0.01f) {
        float subFreq = osc1Freq * 0.5f; // One octave below Osc1
        float subSample = std::sin(subOscPhase_ * juce::MathConstants<double>::twoPi);
        subOscPhase_ += subFreq / getSampleRate();
        if (subOscPhase_ >= 1.0) subOscPhase_ -= 1.0;
        
        mixed += subSample * subOscLevel_;
      }

      // Noise Generator
      if (noiseLevel_ > 0.01f) {
        float noise = (noiseRandom_.nextFloat() * 2.0f - 1.0f) * noiseLevel_;
        mixed += noise;
      }

      // Dual Filter Routing
      float filtered;
      if (filterSerial_) {
        // Serial: Filter1 -> Filter2
        float temp = filter1_.processSample(mixed);
        filtered = filter2_.processSample(temp);
      } else {
        // Parallel: (Filter1 + Filter2) / 2
        float filter1Out = filter1_.processSample(mixed);
        float filter2Out = filter2_.processSample(mixed);
        filtered = (filter1Out + filter2Out) * 0.5f;
      }

      // Amp Envelope
      filtered *= ampEnv * velocity_;

      // Apply volume modulation
      float volumeMod = 1.0f + modulationState_.get(ModulationDestination::Volume);
      filtered *= juce::jlimit(0.0f, 2.0f, volumeMod);

      // Apply pan modulation with equal-power panning
      float panMod = modulationState_.get(ModulationDestination::Pan);
      float panValue = juce::jlimit(-1.0f, 1.0f, panMod);
      
      // Equal power panning: -1 = left, 0 = center, +1 = right
      float panAngle = (panValue + 1.0f) * juce::MathConstants<float>::pi / 4.0f;
      float leftGain = std::cos(panAngle);
      float rightGain = std::sin(panAngle);

      // Effects
      float left = filtered * leftGain;
      float right = filtered * rightGain;
      effects_.process(left, right);

      // Output
      currentAmplitude_ = (std::abs(left) + std::abs(right)) * 0.5f;

      // Add to buffer
      outputBuffer.addSample(0, startSample + start + i, left);
      outputBuffer.addSample(1, startSample + start + i, right);
    }
  }
}

void ZenithPolySynthVoice::computeModulation() {
  modulationState_.reset();

  // LFO1
  switch (lfo1Waveform_) {
    case LFOWaveform::Sine:
      lfo1Value_ = std::sin(lfo1Phase_ * juce::MathConstants<double>::twoPi);
      break;
    case LFOWaveform::Triangle:
      lfo1Value_ = (lfo1Phase_ < 0.5) 
        ? (4.0f * (float)lfo1Phase_ - 1.0f)
        : (3.0f - 4.0f * (float)lfo1Phase_);
      break;
    case LFOWaveform::Square:
      lfo1Value_ = (lfo1Phase_ < 0.5) ? 1.0f : -1.0f;
      break;
    case LFOWaveform::SawUp:
      lfo1Value_ = 2.0f * (float)lfo1Phase_ - 1.0f;
      break;
    case LFOWaveform::SawDown:
      lfo1Value_ = 1.0f - 2.0f * (float)lfo1Phase_;
      break;
    case LFOWaveform::SampleHold:
      if (lfo1Phase_ < lastLfo1Phase_) { // Wrapped
        lfo1SampleHold_ = (float)noiseRandom_.nextFloat() * 2.0f - 1.0f;
      }
      lfo1Value_ = lfo1SampleHold_;
      break;
    default:
      lfo1Value_ = std::sin(lfo1Phase_ * juce::MathConstants<double>::twoPi);
      break;
  }
  
  lastLfo1Phase_ = lfo1Phase_;
  lfo1Phase_ += lfo1Rate_ / getSampleRate();
  if (lfo1Phase_ >= 1.0) lfo1Phase_ -= 1.0;

  // LFO2
  switch (lfo2Waveform_) {
    case LFOWaveform::Sine:
      lfo2Value_ = std::sin(lfo2Phase_ * juce::MathConstants<double>::twoPi);
      break;
    case LFOWaveform::Triangle:
      lfo2Value_ = (lfo2Phase_ < 0.5) 
        ? (4.0f * (float)lfo2Phase_ - 1.0f)
        : (3.0f - 4.0f * (float)lfo2Phase_);
      break;
    case LFOWaveform::Square:
      lfo2Value_ = (lfo2Phase_ < 0.5) ? 1.0f : -1.0f;
      break;
    case LFOWaveform::SawUp:
      lfo2Value_ = 2.0f * (float)lfo2Phase_ - 1.0f;
      break;
    case LFOWaveform::SawDown:
      lfo2Value_ = 1.0f - 2.0f * (float)lfo2Phase_;
      break;
    case LFOWaveform::SampleHold:
      if (lfo2Phase_ < lastLfo2Phase_) { // Wrapped
        lfo2SampleHold_ = (float)noiseRandom_.nextFloat() * 2.0f - 1.0f;
      }
      lfo2Value_ = lfo2SampleHold_;
      break;
    default:
      lfo2Value_ = std::sin(lfo2Phase_ * juce::MathConstants<double>::twoPi);
      break;
  }

  lastLfo2Phase_ = lfo2Phase_;
  lfo2Phase_ += lfo2Rate_ / getSampleRate();
  if (lfo2Phase_ >= 1.0) lfo2Phase_ -= 1.0;

  // Apply matrix (Dense)
  for (int src = 1; src < (int)ModulationSource::NumSources; ++src) {
    float sourceVal = getModulationSourceValue((ModulationSource)src);
    // Optimization: Skip if source is near zero (except for envelopes which might be active)
    // Actually, let's just process it.
    
    for (int dst = 1; dst < (int)ModulationDestination::NumDestinations; ++dst) {
       float amount = modulationMatrix_[src][dst];
       if (amount != 0.0f) {
         modulationState_.add((ModulationDestination)dst, sourceVal * amount);
       }
    }
  }

  // Fix Issue 12/13: Map WavetablePos to OscShape as fallback
  // If WavetablePos is modulated but not used (no wavetables yet), apply to
  // OscShape
  float shapeMod = modulationState_.get(ModulationDestination::OscShape) +
                   modulationState_.get(ModulationDestination::WavetablePos);
  oscShape_ = juce::jlimit(0.05f, 0.95f, 0.5f + shapeMod);
}

float ZenithPolySynthVoice::getModulationSourceValue(ModulationSource source) {
  switch (source) {
  case ModulationSource::LFO1:
    return lfo1Value_;
  case ModulationSource::LFO2:
    return lfo2Value_;
  case ModulationSource::Env1:
    return currentAmpEnv_; // Use cached amp envelope value
  case ModulationSource::Env2:
    return currentModEnv_; // Use cached mod envelope value
  case ModulationSource::Velocity:
    return velocity_;
  case ModulationSource::ModWheel:
    return modWheel_;
  case ModulationSource::Aftertouch:
    return aftertouch_;
  default:
    return 0.0f;
  }
}

void ZenithPolySynthVoice::setAmpEnvelope(float attack, float decay,
                                          float sustain, float release) {
  ampEnvParams_.attack = attack;
  ampEnvParams_.decay = decay;
  ampEnvParams_.sustain = sustain;
  ampEnvParams_.release = release;
  ampEnvelope_.setParameters(ampEnvParams_);
}

void ZenithPolySynthVoice::setModEnvelope(float attack, float decay,
                                          float sustain, float release) {
  modEnvParams_.attack = attack;
  modEnvParams_.decay = decay;
  modEnvParams_.sustain = sustain;
  modEnvParams_.release = release;
  modEnvelope_.setParameters(modEnvParams_);
}

void ZenithPolySynthVoice::setLFO1(float rate, float amount, LFOTarget target) {
  lfo1Rate_ = rate;
  lfo1Amount_ = amount;
  lfo1Target_ = target;
}

void ZenithPolySynthVoice::setLFO2(float rate, float amount, LFOTarget target) {
  lfo2Rate_ = rate;
  lfo2Amount_ = amount;
  lfo2Target_ = target;
}

void ZenithPolySynthVoice::setModulationAmount(ModulationSource source,
                                             ModulationDestination destination,
                                             float amount) {
  int src = (int)source;
  int dst = (int)destination;
  if (src >= 0 && src < (int)ModulationSource::NumSources &&
      dst >= 0 && dst < (int)ModulationDestination::NumDestinations) {
    modulationMatrix_[src][dst] = amount;
  }
}

float ZenithPolySynthVoice::getModulationAmount(ModulationSource source,
                                                ModulationDestination destination) const {
  int src = (int)source;
  int dst = (int)destination;
  if (src >= 0 && src < (int)ModulationSource::NumSources &&
      dst >= 0 && dst < (int)ModulationDestination::NumDestinations) {
    return modulationMatrix_[src][dst];
  }
  return 0.0f;
}

void ZenithPolySynthVoice::updateFrequency() {
  // Implement glide/portamento
  if (glideTime_ > 0.0f && getSampleRate() > 0.0) {
    // Calculate glide rate (time constant for exponential smoothing)
    // glideTime_ is in seconds, convert to samples
    float glideSamples = glideTime_ * static_cast<float>(getSampleRate());
    float glideCoeff = 1.0f - std::exp(-1.0f / glideSamples);
    
    // Exponential smoothing toward target frequency
    currentFrequency_ += (targetFrequency_ - currentFrequency_) * glideCoeff;
  } else {
    // No glide, jump directly to target
    currentFrequency_ = targetFrequency_;
  }
}

//==============================================================================
// ZenithPolySynthProcessor Implementation
//==============================================================================

const juce::String ZenithPolySynthProcessor::Osc1Wave = "osc1_wave";
const juce::String ZenithPolySynthProcessor::Osc1Detune = "osc1_detune";
const juce::String ZenithPolySynthProcessor::Osc1Mix = "osc1_mix";
const juce::String ZenithPolySynthProcessor::Osc2Wave = "osc2_wave";
const juce::String ZenithPolySynthProcessor::Osc2Detune = "osc2_detune";
const juce::String ZenithPolySynthProcessor::Osc2Mix = "osc2_mix";
const juce::String ZenithPolySynthProcessor::Osc3Wave = "osc3_wave";
const juce::String ZenithPolySynthProcessor::Osc3Detune = "osc3_detune";
const juce::String ZenithPolySynthProcessor::Osc3Mix = "osc3_mix";

const juce::String ZenithPolySynthProcessor::UnisonVoices = "unison_voices";
const juce::String ZenithPolySynthProcessor::UnisonDetune = "unison_detune";

const juce::String ZenithPolySynthProcessor::FilterType = "filter_type";
const juce::String ZenithPolySynthProcessor::FilterCutoff = "filter_cutoff";
const juce::String ZenithPolySynthProcessor::FilterResonance =
    "filter_resonance";
const juce::String ZenithPolySynthProcessor::FilterDrive = "filter_drive";

const juce::String ZenithPolySynthProcessor::AmpAttack = "amp_attack";
const juce::String ZenithPolySynthProcessor::AmpDecay = "amp_decay";
const juce::String ZenithPolySynthProcessor::AmpSustain = "amp_sustain";
const juce::String ZenithPolySynthProcessor::AmpRelease = "amp_release";

const juce::String ZenithPolySynthProcessor::ModAttack = "mod_attack";
const juce::String ZenithPolySynthProcessor::ModDecay = "mod_decay";
const juce::String ZenithPolySynthProcessor::ModSustain = "mod_sustain";
const juce::String ZenithPolySynthProcessor::ModRelease = "mod_release";

const juce::String ZenithPolySynthProcessor::LFO1Rate = "lfo1_rate";
const juce::String ZenithPolySynthProcessor::LFO1Amount = "lfo1_amount";

const juce::String ZenithPolySynthProcessor::LFO2Rate = "lfo2_rate";
const juce::String ZenithPolySynthProcessor::LFO2Amount = "lfo2_amount";

const juce::String ZenithPolySynthProcessor::GlideTime = "glide_time";
const juce::String ZenithPolySynthProcessor::MonoMode = "mono_mode";
const juce::String ZenithPolySynthProcessor::MasterGain = "master_gain";

ZenithPolySynthProcessor::~ZenithPolySynthProcessor() {}

//==============================================================================
juce::AudioProcessorEditor *ZenithPolySynthProcessor::createEditor() {
  return new ZenithPolySynthUI(*this);
}

bool ZenithPolySynthProcessor::hasEditor() const { return true; }

void ZenithPolySynthProcessor::prepareToPlay(double sampleRate,
                                             int samplesPerBlock) {
  setCurrentPlaybackSampleRate(sampleRate);
  for (int i = 0; i < getNumVoices(); ++i) {
    if (auto *voice = dynamic_cast<ZenithPolySynthVoice *>(getVoice(i))) {
      voice->setSampleRate(sampleRate);
    }
  }
}

void ZenithPolySynthProcessor::releaseResources() {}

void ZenithPolySynthProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                            juce::MidiBuffer &midiMessages) {
  juce::ScopedNoDenormals noDenormals;

  // Update parameters
  updateVoiceParameters();
  updateVoiceCount();

  // Clear buffer
  buffer.clear();

  // Render
  renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

  // Master Gain
  float masterGain = *parameters_.getRawParameterValue(MasterGain);
  buffer.applyGain(juce::Decibels::decibelsToGain(masterGain));
  
  // Visualizer
  pushToVisualizer(buffer);
}

void ZenithPolySynthProcessor::updateVoiceParameters() {
  // Helper to get float param
  auto getVal = [&](const juce::String &id) {
    return parameters_.getRawParameterValue(id)->load();
  };

  // Helper to get choice/int param
  auto getChoice = [&](const juce::String &id) {
    return (int)getVal(id);
  }; // Simplified

  for (int i = 0; i < getNumVoices(); ++i) {
    if (auto *voice = dynamic_cast<ZenithPolySynthVoice *>(getVoice(i))) {
      voice->setOsc1Waveform((OscillatorWaveform)getChoice(Osc1Wave));
      voice->setOsc1Detune(getVal(Osc1Detune));
      voice->setOsc1Mix(getVal(Osc1Mix));

      voice->setOsc2Waveform((OscillatorWaveform)getChoice(Osc2Wave));
      voice->setOsc2Detune(getVal(Osc2Detune));
      voice->setOsc2Mix(getVal(Osc2Mix));

      voice->setOsc3Waveform((OscillatorWaveform)getChoice(Osc3Wave));
      voice->setOsc3Detune(getVal(Osc3Detune));
      voice->setOsc3Mix(getVal(Osc3Mix));

      voice->setUnisonVoices((int)getVal(UnisonVoices));
      voice->setUnisonDetune(getVal(UnisonDetune));

      voice->setSubOscLevel(getVal("sub_level"));
      voice->setNoiseLevel(getVal("noise_level"));

      voice->setFilterType((zenith::FilterType)getChoice(FilterType));
      voice->setFilterCutoff(getVal(FilterCutoff));
      voice->setFilterResonance(getVal(FilterResonance));
      voice->setFilterDrive(getVal(FilterDrive));
      voice->setFilterEnvAmount(getVal("filter_env_amount"));

      voice->setFilter2Type((zenith::FilterType)getChoice("filter2_type"));
      voice->setFilter2Cutoff(getVal("filter2_cutoff"));
      voice->setFilter2Resonance(getVal("filter2_resonance"));
      voice->setFilter2Drive(getVal("filter2_drive"));
      voice->setFilterRouting(getVal("filter_serial") > 0.5f);

      voice->setDistortion(getVal("distortion"));
      voice->setChorus(getVal("chorus"));

      voice->setAmpEnvelope(getVal(AmpAttack), getVal(AmpDecay),
                            getVal(AmpSustain), getVal(AmpRelease));
      voice->setModEnvelope(getVal(ModAttack), getVal(ModDecay),
                            getVal(ModSustain), getVal(ModRelease));

      voice->setLFO1(getVal(LFO1Rate), getVal(LFO1Amount));
      voice->setLFO1Waveform((LFOWaveform)getChoice("lfo1_waveform"));

      voice->setLFO2(getVal(LFO2Rate), getVal(LFO2Amount));
      voice->setLFO2Waveform((LFOWaveform)getChoice("lfo2_waveform"));

      voice->setGlideTime(getVal(GlideTime));
      voice->setMonoMode(getVal(MonoMode) > 0.5f);
      voice->setQualityPreset((QualityPreset)getChoice(QualitySetting));
  layout.add(std::make_unique<juce::AudioParameterFloat>(Osc1Mix, "Osc 1 Mix",
                                                         0.0f, 1.0f, 1.0f));

  layout.add(std::make_unique<juce::AudioParameterChoice>(
      Osc2Wave, "Osc 2 Wave", oscWaveforms, 2));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      Osc2Detune, "Osc 2 Detune", -100.0f, 100.0f, 0.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(Osc2Mix, "Osc 2 Mix",
                                                         0.0f, 1.0f, 0.5f));

  layout.add(std::make_unique<juce::AudioParameterChoice>(
      Osc3Wave, "Osc 3 Wave", oscWaveforms, 0));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      Osc3Detune, "Osc 3 Detune", -100.0f, 100.0f, 0.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(Osc3Mix, "Osc 3 Mix",
                                                         0.0f, 1.0f, 0.0f));

  // Unison
  layout.add(std::make_unique<juce::AudioParameterInt>(
      UnisonVoices, "Unison Voices", 1, 7, 1));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      UnisonDetune, "Unison Detune", 0.0f, 100.0f, 10.0f));

  // Sub & Noise
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "sub_level", "Sub Level", 0.0f, 1.0f, 0.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "noise_level", "Noise Level", 0.0f, 1.0f, 0.0f));

  // Filter 1
  auto filterTypes = juce::StringArray{"Lowpass", "Bandpass", "Highpass"};
  layout.add(std::make_unique<juce::AudioParameterChoice>(
      FilterType, "Filter Type", filterTypes, 0));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      FilterCutoff, "Cutoff", 20.0f, 20000.0f, 1000.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      FilterResonance, "Resonance", 0.0f, 1.0f, 0.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(FilterDrive, "Drive",
                                                         1.0f, 5.0f, 1.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "filter_env_amount", "Filter Env Amount", 0.0f, 1.0f, 0.5f));

  // Filter 2
                                                         0.001f, 5.0f, 0.1f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      AmpSustain, "Amp Sustain", 0.0f, 1.0f, 0.7f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      AmpRelease, "Amp Release", 0.001f, 5.0f, 0.3f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      ModAttack, "Mod Attack", 0.001f, 5.0f, 0.01f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(ModDecay, "Mod Decay",
                                                         0.001f, 5.0f, 0.1f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      ModSustain, "Mod Sustain", 0.0f, 1.0f, 0.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      ModRelease, "Mod Release", 0.001f, 5.0f, 0.1f));

  // LFOs
  auto lfoTargets = juce::StringArray{"Cutoff", "Osc1 Pitch", "Osc2 Pitch",
                                      "Osc1 Mix", "Osc2 Mix"};
  auto lfoWaveforms = juce::StringArray{"Sine", "Triangle", "Square", "SawUp", "SawDown", "S&H"};

  layout.add(std::make_unique<juce::AudioParameterChoice>(
      "lfo1_waveform", "LFO1 Waveform", lfoWaveforms, 0));
  layout.add(std::make_unique<juce::AudioParameterFloat>(LFO1Rate, "LFO 1 Rate",
                                                         0.1f, 20.0f, 1.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      LFO1Amount, "LFO 1 Amount", 0.0f, 1.0f, 0.0f));

  layout.add(std::make_unique<juce::AudioParameterChoice>(
      "lfo2_waveform", "LFO2 Waveform", lfoWaveforms, 0));
  layout.add(std::make_unique<juce::AudioParameterFloat>(LFO2Rate, "LFO 2 Rate",
                                                         0.1f, 20.0f, 1.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      LFO2Amount, "LFO 2 Amount", 0.0f, 1.0f, 0.0f));

  // Global
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      GlideTime, "Glide Time", 0.0f, 1.0f, 0.0f));
  layout.add(
      std::make_unique<juce::AudioParameterBool>(MonoMode, "Mono Mode", false));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      MasterGain, "Master Gain", -60.0f, 6.0f, -3.0f));

  layout.add(std::make_unique<juce::AudioParameterInt>(MaxVoices, "Max Voices",
                                                       1, 32, 16));
  auto qualities = juce::StringArray{"Low", "Medium", "High"};
  layout.add(std::make_unique<juce::AudioParameterChoice>(
      QualitySetting, "Quality", qualities, 1));

  return layout;
}

void ZenithPolySynthProcessor::updateVoiceCount() {
  int newMaxVoices = (int)*parameters_.getRawParameterValue(MaxVoices);
  if (newMaxVoices != currentMaxVoices_) {
    clearVoices();
    for (int i = 0; i < newMaxVoices; ++i) {
      addVoice(new ZenithPolySynthVoice());
    }
    currentMaxVoices_ = newMaxVoices;
  }
}

juce::SynthesiserVoice *
ZenithPolySynthProcessor::findFreeVoice(juce::SynthesiserSound *soundToPlay,
                                        int midiChannel, int midiNoteNumber,
                                        bool stealIfNoneAvailable) const {
  // Check if mono mode is enabled
  bool monoMode = *parameters_.getRawParameterValue(MonoMode) > 0.5f;
  
  if (monoMode) {
    // In mono mode, always use the first voice and steal it if necessary
    if (getNumVoices() > 0) {
      auto* voice = getVoice(0);
      if (voice != nullptr) {
        // If voice is currently playing, stop it for legato transition
        if (voice->isVoiceActive()) {
          voice->stopNote(1.0f, false); // Hard stop for legato
        }
        return voice;
      }
    }
  }
  
  // Poly mode: use default JUCE behavior
  return juce::Synthesiser::findFreeVoice(soundToPlay, midiChannel,
                                          midiNoteNumber, stealIfNoneAvailable);
}

void ZenithPolySynthProcessor::getStateInformation(
    juce::MemoryBlock &destData) {
  auto state = parameters_.copyState();
  std::unique_ptr<juce::XmlElement> xml(state.createXml());
  copyXmlToBinary(*xml, destData);
}

void ZenithPolySynthProcessor::setStateInformation(const void *data,
                                                   int sizeInBytes) {
  std::unique_ptr<juce::XmlElement> xmlState(
      getXmlFromBinary(data, sizeInBytes));
  if (xmlState != nullptr) {
    if (xmlState->hasTagName(parameters_.state.getType())) {
      parameters_.replaceState(juce::ValueTree::fromXml(*xmlState));
    }
  }
}

//==============================================================================
void ZenithPolySynthProcessor::setModulationMatrix(ModulationSource src, ModulationDestination dst, float amount) {
  int s = (int)src;
  int d = (int)dst;
  if (s >= 0 && s < (int)ModulationSource::NumSources &&
      d >= 0 && d < (int)ModulationDestination::NumDestinations) {
    processorModulationMatrix_[s][d] = amount;
  }
}

float ZenithPolySynthProcessor::getModulationMatrix(ModulationSource src, ModulationDestination dst) const {
  int s = (int)src;
  int d = (int)dst;
  if (s >= 0 && s < (int)ModulationSource::NumSources &&
      d >= 0 && d < (int)ModulationDestination::NumDestinations) {
    return processorModulationMatrix_[s][d];
  }
  return 0.0f;
}

void ZenithPolySynthProcessor::pushToVisualizer(const juce::AudioBuffer<float>& buffer) {
    if (buffer.getNumChannels() > 0) {
        auto* channelData = buffer.getReadPointer(0);
        int numSamples = buffer.getNumSamples();
        
        int start1, size1, start2, size2;
        visualizerFifo_.prepareToWrite(numSamples, start1, size1, start2, size2);
        
        if (size1 > 0) {
            for (int i = 0; i < size1; ++i) visualizerBuffer_[start1 + i] = channelData[i];
        }
        if (size2 > 0) {
            for (int i = 0; i < size2; ++i) visualizerBuffer_[start2 + i] = channelData[size1 + i];
        }
        
        visualizerFifo_.finishedWrite(size1 + size2);
    }
}

int ZenithPolySynthProcessor::readFromVisualizer(float* dest, int numSamples) {
    int start1, size1, start2, size2;
    visualizerFifo_.prepareToRead(numSamples, start1, size1, start2, size2);
    
    if (size1 > 0) {
        for (int i = 0; i < size1; ++i) dest[i] = visualizerBuffer_[start1 + i];
    }
    if (size2 > 0) {
        for (int i = 0; i < size2; ++i) dest[size1 + i] = visualizerBuffer_[start2 + i];
    }
    
    visualizerFifo_.finishedRead(size1 + size2);
    return size1 + size2;
}

//==============================================================================
// ZenithPolySynth Instrument Wrapper
//==============================================================================

static int getParamIndex(juce::AudioProcessor *proc,
                         const juce::String &paramId) {
  if (!proc)
    return -1;
  const auto &params = proc->getParameters();
  for (int i = 0; i < params.size(); ++i) {
    if (auto *paramWithID =
            dynamic_cast<juce::AudioProcessorParameterWithID *>(params[i])) {
      if (paramWithID->getParameterID() == paramId)
        return i;
    }
  }
  return -1;
}

ZenithPolySynth::ZenithPolySynth()
    : InstrumentBase(std::make_unique<ZenithPolySynthProcessor>(),
                     createMetadata()) {
  // Map parameter IDs to JUCE indices
  // Map parameter IDs to JUCE indices
  auto *proc = getAudioProcessor();

  mapParameter("osc1_wave",
               getParamIndex(proc, ZenithPolySynthProcessor::Osc1Wave));
  mapParameter("osc1_detune",
               getParamIndex(proc, ZenithPolySynthProcessor::Osc1Detune));
  mapParameter("osc1_mix",
               getParamIndex(proc, ZenithPolySynthProcessor::Osc1Mix));
  mapParameter("osc2_wave",
               getParamIndex(proc, ZenithPolySynthProcessor::Osc2Wave));
  mapParameter("osc2_detune",
               getParamIndex(proc, ZenithPolySynthProcessor::Osc2Detune));
  mapParameter("osc2_mix",
               getParamIndex(proc, ZenithPolySynthProcessor::Osc2Mix));
  mapParameter("osc3_wave",
               getParamIndex(proc, ZenithPolySynthProcessor::Osc3Wave));
  mapParameter("osc3_detune",
               getParamIndex(proc, ZenithPolySynthProcessor::Osc3Detune));
  mapParameter("osc3_mix",
               getParamIndex(proc, ZenithPolySynthProcessor::Osc3Mix));

  mapParameter("unison_voices",
               getParamIndex(proc, ZenithPolySynthProcessor::UnisonVoices));
  mapParameter("unison_detune",
               getParamIndex(proc, ZenithPolySynthProcessor::UnisonDetune));

  mapParameter("filter_type",
               getParamIndex(proc, ZenithPolySynthProcessor::FilterType));
  mapParameter("filter_cutoff",
               getParamIndex(proc, ZenithPolySynthProcessor::FilterCutoff));
  mapParameter("filter_resonance",
               getParamIndex(proc, ZenithPolySynthProcessor::FilterResonance));
  mapParameter("filter_drive",
               getParamIndex(proc, ZenithPolySynthProcessor::FilterDrive));

  mapParameter("amp_attack",
               getParamIndex(proc, ZenithPolySynthProcessor::AmpAttack));
  mapParameter("amp_decay",
               getParamIndex(proc, ZenithPolySynthProcessor::AmpDecay));
  mapParameter("amp_sustain",
               getParamIndex(proc, ZenithPolySynthProcessor::AmpSustain));
  mapParameter("amp_release",
               getParamIndex(proc, ZenithPolySynthProcessor::AmpRelease));

  mapParameter("mod_attack",
               getParamIndex(proc, ZenithPolySynthProcessor::ModAttack));
  mapParameter("mod_decay",
               getParamIndex(proc, ZenithPolySynthProcessor::ModDecay));
  mapParameter("mod_sustain",
               getParamIndex(proc, ZenithPolySynthProcessor::ModSustain));
  mapParameter("mod_release",
               getParamIndex(proc, ZenithPolySynthProcessor::ModRelease));

  mapParameter("lfo1_rate",
               getParamIndex(proc, ZenithPolySynthProcessor::LFO1Rate));
  mapParameter("lfo1_amount",
               getParamIndex(proc, ZenithPolySynthProcessor::LFO1Amount));

  mapParameter("lfo2_rate",
               getParamIndex(proc, ZenithPolySynthProcessor::LFO2Rate));
  mapParameter("lfo2_amount",
               getParamIndex(proc, ZenithPolySynthProcessor::LFO2Amount));

  mapParameter("glide_time",
               getParamIndex(proc, ZenithPolySynthProcessor::GlideTime));
  mapParameter("mono_mode",
               getParamIndex(proc, ZenithPolySynthProcessor::MonoMode));
  mapParameter("master_gain",
               getParamIndex(proc, ZenithPolySynthProcessor::MasterGain));

  mapParameter("max_voices",
               getParamIndex(proc, ZenithPolySynthProcessor::MaxVoices));
  mapParameter("quality",
               getParamIndex(proc, ZenithPolySynthProcessor::QualitySetting));

  registerPresets();
}

InstrumentMetadata ZenithPolySynth::createMetadata() {
  InstrumentMetadata metadata;
  metadata.instrumentId = "zenith.poly_synth";
  metadata.name = "Zenith Poly Synth";
  metadata.category = "synth";
  metadata.description = "Multi-oscillator subtractive synthesizer optimized "
                         "for EDM/trap/future-bass";
  // ... (Add parameters here if needed, but for now we rely on the processor's
  // layout) In a real implementation, we would duplicate the parameter
  // definitions here for the UI
  return metadata;
}

void ZenithPolySynth::registerPresets() {
  PresetGenerator::generateFactoryPresets();
}

} // namespace zenith
