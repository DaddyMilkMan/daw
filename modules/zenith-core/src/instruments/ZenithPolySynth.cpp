/*
  ==============================================================================

    ZenithPolySynth.cpp
    Created: 2025-11-18
    Author:  Zenith DAW

    Implementation of ZenithPolySynth - multi-oscillator subtractive
    synthesizer.

  ==============================================================================
*/

#include "ZenithPolySynth.h"
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
}

bool ZenithPolySynthVoice::canPlaySound(juce::SynthesiserSound *sound) {
  return dynamic_cast<ZenithPolySynthSound *>(sound) != nullptr;
}

void ZenithPolySynthVoice::startNote(int midiNoteNumber, float velocity,
                                     juce::SynthesiserSound * /*sound*/,
                                     int /*currentPitchWheelPosition*/) {
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

void ZenithPolySynthVoice::pitchWheelMoved(int /*newPitchWheelValue*/) {
  // Handle pitch wheel if needed
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
    float modCutoff =
        filterCutoff_ +
        modulationState_.get(ModulationDestination::FilterCutoff) * 10000.0f;
    filter1_.setCutoff(modCutoff);

    float modResonance =
        filter1_.getResonance() +
        modulationState_.get(
            ModulationDestination::FilterResonance); // Need getter? Assuming
                                                     // handled by setResonance
    // Actually setResonance is on filter, we should use base + mod.
    // For simplicity, we'll just re-set resonance with mod

    // Render audio
    for (int i = 0; i < blockSize; ++i) {
      if (!ampEnvelope_.isActive()) {
        clearCurrentNote();
        return;
      }

      float ampEnv = ampEnvelope_.getNextSample();
      float modEnv = modEnvelope_.getNextSample(); // Advance mod env

      // Oscillators
      float osc1 = osc1_.getNextSample(currentFrequency_, oscShape_);
      float osc2 = osc2_.getNextSample(currentFrequency_, oscShape_);
      float osc3 = osc3_.getNextSample(currentFrequency_, oscShape_);

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
              unisonOscillators_[v].getNextSample(currentFrequency_, oscShape_);
        }
        unisonOutput /= effectiveUnisonVoices; // Normalize
        osc1 = (osc1 + unisonOutput) * 0.5f;
      }

      // Mix
      float mixed = osc1 * osc1Mix_ + osc2 * osc2Mix_ + osc3 * osc3Mix_;

      // Filter
      float filtered = filter1_.processSample(mixed);
      if (!filterSerial_) {
        // Parallel routing logic could go here
      }

      // Amp Envelope
      filtered *= ampEnv * velocity_;

      // Effects
      float left = filtered;
      float right = filtered;
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

  // LFOs
  lfo1Value_ = std::sin(lfo1Phase_ * juce::MathConstants<double>::twoPi);
  lfo1Phase_ += lfo1Rate_ / getSampleRate();
  if (lfo1Phase_ >= 1.0)
    lfo1Phase_ -= 1.0;

  lfo2Value_ = std::sin(lfo2Phase_ * juce::MathConstants<double>::twoPi);
  lfo2Phase_ += lfo2Rate_ / getSampleRate();
  if (lfo2Phase_ >= 1.0)
    lfo2Phase_ -= 1.0;

  // Apply matrix
  for (const auto &slot : modulationMatrix_) {
    if (slot.isActive()) {
      float sourceVal = getModulationSourceValue(slot.source);
      modulationState_.add(slot.destination, sourceVal * slot.amount);
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
    return 0.0f; // Amp env not easily accessible here without peeking
  case ModulationSource::Env2:
    return 0.0f; // Mod env
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

void ZenithPolySynthVoice::setModulationSlot(int slotIndex,
                                             ModulationSource source,
                                             ModulationDestination destination,
                                             float amount) {
  if (slotIndex >= 0 && slotIndex < 8) {
    modulationMatrix_[slotIndex].source = source;
    modulationMatrix_[slotIndex].destination = destination;
    modulationMatrix_[slotIndex].amount = amount;
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
const juce::String ZenithPolySynthProcessor::LFO1Target = "lfo1_target";

const juce::String ZenithPolySynthProcessor::LFO2Rate = "lfo2_rate";
const juce::String ZenithPolySynthProcessor::LFO2Amount = "lfo2_amount";
const juce::String ZenithPolySynthProcessor::LFO2Target = "lfo2_target";

const juce::String ZenithPolySynthProcessor::GlideTime = "glide_time";
const juce::String ZenithPolySynthProcessor::MonoMode = "mono_mode";
const juce::String ZenithPolySynthProcessor::MasterGain = "master_gain";

const juce::String ZenithPolySynthProcessor::MaxVoices = "max_voices";
const juce::String ZenithPolySynthProcessor::QualitySetting = "quality";

ZenithPolySynthProcessor::ZenithPolySynthProcessor()
    : AudioProcessor(BusesProperties().withOutput(
          "Output", juce::AudioChannelSet::stereo(), true)),
      parameters_(*this, nullptr, "Parameters", createParameterLayout()) {

  // Initialize visualizer buffer (2048 samples for waveform display)
  visualizerBuffer_.resize(2048, 0.0f);

  // Initialize modulation matrix to zero
  std::memset(modulationMatrix_, 0, sizeof(modulationMatrix_));

  updateVoiceCount();
}

ZenithPolySynthProcessor::~ZenithPolySynthProcessor() {}

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

  // Update visualizer buffer with audio data (for UI display)
  if (buffer.getNumChannels() > 0 && buffer.getNumSamples() > 0) {
    const float* channelData = buffer.getReadPointer(0);
    int writePos = visualizerWritePos_.load();
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
      visualizerBuffer_[writePos] = channelData[i];
      writePos = (writePos + 1) % visualizerBuffer_.size();
    }
    visualizerWritePos_.store(writePos);
  }
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

      voice->setFilterType((zenith::FilterType)getChoice(FilterType));
      voice->setFilterCutoff(getVal(FilterCutoff));
      voice->setFilterResonance(getVal(FilterResonance));
      voice->setFilterDrive(getVal(FilterDrive));

      voice->setAmpEnvelope(getVal(AmpAttack), getVal(AmpDecay),
                            getVal(AmpSustain), getVal(AmpRelease));
      voice->setModEnvelope(getVal(ModAttack), getVal(ModDecay),
                            getVal(ModSustain), getVal(ModRelease));

      voice->setLFO1(getVal(LFO1Rate), getVal(LFO1Amount),
                     (LFOTarget)getChoice(LFO1Target));
      voice->setLFO2(getVal(LFO2Rate), getVal(LFO2Amount),
                     (LFOTarget)getChoice(LFO2Target));

      voice->setGlideTime(getVal(GlideTime));
      voice->setMonoMode(getVal(MonoMode) > 0.5f);
      voice->setQualityPreset((QualityPreset)getChoice(QualitySetting));
    }
  }
}

juce::AudioProcessorValueTreeState::ParameterLayout
ZenithPolySynthProcessor::createParameterLayout() {
  juce::AudioProcessorValueTreeState::ParameterLayout layout;

  // Oscillators
  auto oscWaveforms = juce::StringArray{"Sine",     "Saw",   "Square",
                                        "Triangle", "Noise", "Supersaw"};
  layout.add(std::make_unique<juce::AudioParameterChoice>(
      Osc1Wave, "Osc 1 Wave", oscWaveforms, 1));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      Osc1Detune, "Osc 1 Detune", -100.0f, 100.0f, 0.0f));
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

  // Filter
  auto filterTypes = juce::StringArray{"Lowpass", "Bandpass", "Highpass"};
  layout.add(std::make_unique<juce::AudioParameterChoice>(
      FilterType, "Filter Type", filterTypes, 0));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      FilterCutoff, "Cutoff", 20.0f, 20000.0f, 1000.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      FilterResonance, "Resonance", 0.0f, 1.0f, 0.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(FilterDrive, "Drive",
                                                         1.0f, 5.0f, 1.0f));

  // Envelopes
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      AmpAttack, "Amp Attack", 0.001f, 5.0f, 0.01f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(AmpDecay, "Amp Decay",
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
  layout.add(std::make_unique<juce::AudioParameterFloat>(LFO1Rate, "LFO 1 Rate",
                                                         0.1f, 20.0f, 1.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      LFO1Amount, "LFO 1 Amount", 0.0f, 1.0f, 0.0f));
  layout.add(std::make_unique<juce::AudioParameterChoice>(
      LFO1Target, "LFO 1 Target", lfoTargets, 0));

  layout.add(std::make_unique<juce::AudioParameterFloat>(LFO2Rate, "LFO 2 Rate",
                                                         0.1f, 20.0f, 1.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      LFO2Amount, "LFO 2 Amount", 0.0f, 1.0f, 0.0f));
  layout.add(std::make_unique<juce::AudioParameterChoice>(
      LFO2Target, "LFO 2 Target", lfoTargets, 0));

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
  mapParameter("lfo1_target",
               getParamIndex(proc, ZenithPolySynthProcessor::LFO1Target));

  mapParameter("lfo2_rate",
               getParamIndex(proc, ZenithPolySynthProcessor::LFO2Rate));
  mapParameter("lfo2_amount",
               getParamIndex(proc, ZenithPolySynthProcessor::LFO2Amount));
  mapParameter("lfo2_target",
               getParamIndex(proc, ZenithPolySynthProcessor::LFO2Target));

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
