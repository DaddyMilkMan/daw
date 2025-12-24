#include "MeteringSystem.h"

namespace zenith {

MeteringSystem::MeteringSystem() {
  analysisFifo = std::make_unique<StereoAudioFifo>(16384);
}

void MeteringSystem::prepare(const juce::dsp::ProcessSpec &spec) {
  sampleRate_ = spec.sampleRate;

  // Prepare K-Weighting filters for LUFS (Stage 1: High shelf, Stage 2: High
  // pass) Rec. ITU-R BS.1770-4
  auto &filter1 = kWeightingFilter_.get<0>(); // High shelf
  auto &filter2 = kWeightingFilter_.get<1>(); // High pass

  filter1.prepare(spec);
  // Shelf: +4dB at 1.5kHz approx
  // Coefficients usually hardcoded for 48kHz, but we use JUCE helpers
  // approximation
  *filter1.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
      sampleRate_, 1500.0f, 1.0f, juce::Decibels::decibelsToGain(4.0f));

  filter2.prepare(spec);
  *filter2.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(
      sampleRate_, 100.0f); // ~100Hz HPF

  reset();
}

void MeteringSystem::reset() {
  masterPeak.store(0.0f);
  rmsLevel.store(0.0f);
  vuLevel.store(0.0f);
  ppmLevel.store(0.0f);
  lufsMomentary.store(-100.0f);

  vuEnvelope_ = 0.0f;
  ppmEnvelope_ = 0.0f;

  kWeightingFilter_.reset();
}

void MeteringSystem::process(const juce::AudioBuffer<float> &buffer) {
  // 1. Peak & RMS
  float magnitude = buffer.getMagnitude(0, buffer.getNumSamples());
  float rms = buffer.getRMSLevel(0, 0, buffer.getNumSamples());

  // Peak hold
  float currentPeak = masterPeak.load();
  if (magnitude > currentPeak) {
    masterPeak.store(magnitude);
  }

  rmsLevel.store(rms);

  // 2. Ballistics (VU & PPM)
  // Simple digital simulation of needle ballistics
  float targetVU = rms; // VU follows RMS mostly
  float targetPPM = magnitude;

  // VU Ballistics (300ms integration)
  float vuCoeff =
      std::exp(-1.0f / (sampleRate_ * VU_RISE_TIME / buffer.getNumSamples()));
  vuEnvelope_ = targetVU * (1.0f - vuCoeff) + vuEnvelope_ * vuCoeff;
  vuLevel.store(vuEnvelope_);

  // PPM Ballistics (Fast attack, slow release)
  float ppmAttack =
      std::exp(-1.0f / (sampleRate_ * PPM_RISE_TIME / buffer.getNumSamples()));
  float ppmRelease =
      std::exp(-1.0f / (sampleRate_ * PPM_FALL_TIME / buffer.getNumSamples()));

  if (targetPPM > ppmEnvelope_) {
    ppmEnvelope_ = targetPPM * (1.0f - ppmAttack) + ppmEnvelope_ * ppmAttack;
  } else {
    ppmEnvelope_ = targetPPM * (1.0f - ppmRelease) + ppmEnvelope_ * ppmRelease;
  }
  ppmLevel.store(ppmEnvelope_);

  // 3. LUFS Momentary (K-Weighted)
  // We need a scratch buffer for filtering to not affect output
  juce::AudioBuffer<float> scratch(buffer);
  juce::dsp::AudioBlock<float> block(scratch);
  juce::dsp::ProcessContextReplacing<float> context(block);
  kWeightingFilter_.process(context);

  // Mean Square of K-weighted signal (400ms window usually, here block-wise for
  // momentary)
  float kRms = scratch.getRMSLevel(0, 0, scratch.getNumSamples());
  // Gating not strictly implemented for simple momentary
  float lufs = (kRms > 0.000001f) ? (20.0f * std::log10(kRms) - 0.691f)
                                  : -100.0f; // -0.691 offset? Standard says
                                             // just K-weighted mean square.
  // Actually full LUFS is more complex, but this is a decent "K-weighted RMS"
  // approximation. Standard LUFS is K-weighted, then Mean Square, then -0.691
  // dB offset.

  lufsMomentary.store(lufs);

  // Update Visualizer FIFO
  analysisFifo->push(buffer, buffer.getNumSamples());
}

float MeteringSystem::getLevel(MeterMode mode) const {
  switch (mode) {
  case MeterMode::Peak:
    return masterPeak.load();
  case MeterMode::RMS:
    return rmsLevel.load();
  case MeterMode::VU:
    return vuLevel.load();
  case MeterMode::PPM:
    return ppmLevel.load();

  // K-System: RMS value shifted by reference level
  // K-20: 0dB = -20dBFS. Display range -20 to +4.
  case MeterMode::K20:
    return rmsLevel.load();
  case MeterMode::K14:
    return rmsLevel.load();
  case MeterMode::K12:
    return rmsLevel.load();

  case MeterMode::LUFS_Momentary:
    return lufsMomentary.load(); // Returns dB value, others return linear 0-1
  default:
    return 0.0f;
  }
}

} // namespace zenith
