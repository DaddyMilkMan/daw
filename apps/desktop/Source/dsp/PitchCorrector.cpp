/*
  ==============================================================================

    PitchCorrector.cpp
    Created: 2026-01-29
    Author:  Zenith DAW

    Professional pitch correction implementation with Rubber Band.

  ==============================================================================
*/

#include "PitchCorrector.h"
#include <cmath>

namespace zenith {
namespace dsp {

//==============================================================================
// Scale Implementation
//==============================================================================

static const std::array<bool, 12> majorScale = {true, false, true, false, true, true, false, true, false, true, false, true};
static const std::array<bool, 12> minorScale = {true, false, true, true, false, true, false, true, true, false, true, false};
static const std::array<bool, 12> minorHarmonicScale = {true, false, true, true, false, true, false, true, true, false, false, true};
static const std::array<bool, 12> minorMelodicScale = {true, false, true, true, false, true, false, true, false, true, false, true};
static const std::array<bool, 12> pentatonicMajor = {true, false, true, false, true, false, false, true, false, true, false, false};
static const std::array<bool, 12> pentatonicMinor = {true, false, false, true, false, true, false, true, false, false, true, false};
static const std::array<bool, 12> bluesScale = {true, false, false, true, false, true, true, true, false, false, true, false};
static const std::array<bool, 12> dorianScale = {true, false, true, true, false, true, false, true, false, true, false, true};
static const std::array<bool, 12> phrygianScale = {true, true, false, true, false, true, false, true, true, false, true, false};
static const std::array<bool, 12> lydianScale = {true, false, true, false, true, false, true, true, false, true, false, true};
static const std::array<bool, 12> mixolydianScale = {true, false, true, false, true, true, false, true, false, true, true, false};

bool Scale::isNoteInScale(int midiNote) const
{
    int noteInOctave = midiNote % 12;
    if (noteInOctave < 0) noteInOctave += 12;
    
    switch (scaleType)
    {
        case MusicalScale::Chromatic: return true;
        case MusicalScale::Major: return majorScale[noteInOctave];
        case MusicalScale::Minor: return minorScale[noteInOctave];
        case MusicalScale::MinorHarmonic: return minorHarmonicScale[noteInOctave];
        case MusicalScale::MinorMelodic: return minorMelodicScale[noteInOctave];
        case MusicalScale::PentatonicMajor: return pentatonicMajor[noteInOctave];
        case MusicalScale::PentatonicMinor: return pentatonicMinor[noteInOctave];
        case MusicalScale::Blues: return bluesScale[noteInOctave];
        case MusicalScale::Dorian: return dorianScale[noteInOctave];
        case MusicalScale::Phrygian: return phrygianScale[noteInOctave];
        case MusicalScale::Lydian: return lydianScale[noteInOctave];
        case MusicalScale::Mixolydian: return mixolydianScale[noteInOctave];
        case MusicalScale::Custom: return customNotes[noteInOctave];
    }
    return true;
}

float Scale::getNearestScalePitch(float frequencyHz) const
{
    if (frequencyHz <= 0.0f) return 0.0f;
    
    // Convert frequency to MIDI note
    float midiNote = 69.0f + 12.0f * std::log2(frequencyHz / 440.0f);
    int nearestNote = static_cast<int>(std::round(midiNote));
    
    // Find nearest note in scale
    float minDistance = 100.0f;
    int bestNote = nearestNote;
    
    for (int offset = -12; offset <= 12; ++offset)
    {
        int testNote = nearestNote + offset;
        if (isNoteInScale(testNote))
        {
            float distance = std::abs(midiNote - testNote);
            if (distance < minDistance)
            {
                minDistance = distance;
                bestNote = testNote;
            }
        }
    }
    
    // Convert back to frequency
    return 440.0f * std::pow(2.0f, (bestNote - 69) / 12.0f);
}

float Scale::snapToScale(float frequencyHz, float correctionAmount) const
{
    if (frequencyHz <= 0.0f || correctionAmount <= 0.0f) return frequencyHz;
    
    float targetFreq = getNearestScalePitch(frequencyHz);
    if (targetFreq <= 0.0f) return frequencyHz;
    
    // Apply correction amount
    float diff = targetFreq - frequencyHz;
    return frequencyHz + diff * correctionAmount;
}

//==============================================================================
// PitchCorrector Implementation
//==============================================================================

PitchCorrector::PitchCorrector()
{
    // Default to C Major
    scale_.rootNote = Note::C;
    scale_.scaleType = MusicalScale::Chromatic;
    
    // Initialize pitch history for vibrato detection
    pitchHistory_.resize(kVibratoHistorySize, 0.0f);
}

PitchCorrector::~PitchCorrector()
{
}

void PitchCorrector::prepare(double sampleRate, int samplesPerBlock)
{
    sampleRate_ = sampleRate;
    
    // Create professional pitch shifter
    proShifter_ = std::make_unique<ProPitchShifter>();
    proShifter_->prepare(sampleRate, 1, samplesPerBlock);
    
    // Prepare temp buffer
    tempBuffer_.setSize(1, samplesPerBlock, false, false, true);
    
    reset();
}

void PitchCorrector::reset()
{
    smoothedCorrection_ = 0.0f;
    currentSemitoneShift_ = 0.0f;
    lastTargetSemitones_ = 0.0f;
    
    std::fill(pitchHistory_.begin(), pitchHistory_.end(), 0.0f);
    
    if (proShifter_)
    {
        proShifter_->reset();
    }
}

void PitchCorrector::setScale(Note root, MusicalScale type)
{
    scale_.rootNote = root;
    scale_.scaleType = type;
}

void PitchCorrector::setQuality(ProPitchShifter::Quality quality)
{
    quality_ = quality;
    if (proShifter_)
    {
        proShifter_->setQuality(quality);
    }
}

int PitchCorrector::getLatencySamples() const
{
    if (proShifter_)
        return proShifter_->getLatencySamples();
    return 0;
}

float PitchCorrector::getLatencyMs() const
{
    if (proShifter_)
        return proShifter_->getLatencyMs();
    return 0.0f;
}

float PitchCorrector::frequencyToSemitones(float freqHz) const
{
    if (freqHz <= 0.0f) return 0.0f;
    return 12.0f * std::log2(freqHz / 440.0f);
}

float PitchCorrector::semitonesToFrequency(float semitones) const
{
    return 440.0f * std::pow(2.0f, semitones / 12.0f);
}

void PitchCorrector::detectVibrato(float currentPitch)
{
    // Add to history
    pitchHistory_.push_back(currentPitch);
    if (pitchHistory_.size() > kVibratoHistorySize)
        pitchHistory_.pop_front();
    
    if (pitchHistory_.size() < kVibratoHistorySize / 2)
        return;
    
    // Calculate vibrato using zero-crossing and amplitude
    float sum = 0.0f;
    float sumSq = 0.0f;
    int count = 0;
    
    for (float pitch : pitchHistory_)
    {
        if (pitch > 0.0f)
        {
            sum += pitch;
            sumSq += pitch * pitch;
            count++;
        }
    }
    
    if (count < 10)
    {
        vibratoDepth_.store(0.0f);
        vibratoRate_.store(0.0f);
        return;
    }
    
    float mean = sum / count;
    float variance = (sumSq / count) - (mean * mean);
    float stdDev = std::sqrt(std::max(0.0f, variance));
    
    // Convert frequency deviation to cents
    float centsDeviation = 1200.0f * std::log2((mean + stdDev) / mean);
    vibratoDepth_.store(std::max(0.0f, centsDeviation));
    
    // Estimate vibrato rate (simplified - would need FFT for accuracy)
    // For now, use a reasonable default
    vibratoRate_.store(6.0f);  // Typical vocal vibrato ~6Hz
}

void PitchCorrector::applyCorrectionWithVibrato(float& targetSemitones, float detectedPitch)
{
    float vibratoPreserve = vibratoPreserve_.load();
    float vibratoDepth = vibratoDepth_.load();
    
    // If vibrato detected and preservation is enabled
    if (vibratoDepth > 10.0f && vibratoPreserve > 0.0f)
    {
        // Calculate how much to correct
        // At vibratoPreserve = 1.0, we correct less to preserve vibrato
        // At vibratoPreserve = 0.0, we fully correct (flatten vibrato)
        
        float correctionFactor = 1.0f - (vibratoPreserve * 0.5f);
        
        // Add back some of the original pitch variation
        float currentSemitones = frequencyToSemitones(detectedPitch);
        float variation = currentSemitones - targetSemitones;
        
        targetSemitones += variation * vibratoPreserve;
    }
}

float PitchCorrector::calculateCorrection(float inputPitch, float /*sampleRate*/)
{
    if (inputPitch <= 0.0f)
    {
        isCorrecting_.store(false);
        return 0.0f;
    }
    
    currentPitch_.store(inputPitch);
    
    // Detect vibrato
    detectVibrato(inputPitch);
    
    // Get target pitch from scale
    float targetPitch = scale_.snapToScale(inputPitch, correctionAmount_.load());
    targetPitch_.store(targetPitch);
    
    if (targetPitch <= 0.0f)
    {
        isCorrecting_.store(false);
        return 0.0f;
    }
    
    // Calculate semitone shift needed
    float currentSemitones = frequencyToSemitones(inputPitch);
    float targetSemitones = frequencyToSemitones(targetPitch);
    float desiredShift = targetSemitones - currentSemitones;
    
    // Store cents offset for UI
    currentCentsOffset_.store(desiredShift * 100.0f);
    
    // Check if we're transitioning between notes
    bool isTransition = std::abs(targetSemitones - lastTargetSemitones_) > 0.5f;
    lastTargetSemitones_ = targetSemitones;
    
    // Apply vibrato preservation
    applyCorrectionWithVibrato(desiredShift, inputPitch);
    
    // Apply humanize (don't correct 100% - allow some natural variation)
    float humanizeAmount = humanize_.load();
    float humanizedShift = desiredShift * (1.0f - humanizeAmount * 0.3f);
    
    // Apply retune speed smoothing
    float retuneSpeed = isTransition ? noteTransitionSpeedMs_.load() : retuneSpeedMs_.load();
    
    // Legato mode uses slower transitions
    if (legatoMode_.load() && isTransition)
    {
        retuneSpeed *= 2.0f;  // Slower = more legato
    }
    
    if (retuneSpeed <= 1.0f)
    {
        // Instant correction (T-Pain effect)
        smoothedCorrection_ = humanizedShift;
    }
    else
    {
        // Smooth transition
        float smoothingCoeff = 1.0f / (retuneSpeed * 0.001f * sampleRate_ / 64.0f);
        smoothingCoeff = juce::jlimit(0.01f, 1.0f, smoothingCoeff);
        smoothedCorrection_ += smoothingCoeff * (humanizedShift - smoothedCorrection_);
    }
    
    isCorrecting_.store(true);
    return smoothedCorrection_;
}

void PitchCorrector::process(const juce::AudioBuffer<float>& inputBuffer,
                             float detectedPitchHz,
                             juce::AudioBuffer<float>& outputBuffer)
{
    if (!enabled_.load() || detectedPitchHz <= 0.0f || !proShifter_)
    {
        // Pass through
        outputBuffer.copyFrom(0, 0, inputBuffer, 0, 0, inputBuffer.getNumSamples());
        isCorrecting_.store(false);
        return;
    }
    
    // Ensure temp buffer is large enough
    int numSamples = inputBuffer.getNumSamples();
    if (tempBuffer_.getNumSamples() < numSamples)
    {
        tempBuffer_.setSize(1, numSamples, false, false, true);
    }
    
    // Copy input to temp
    tempBuffer_.copyFrom(0, 0, inputBuffer, 0, 0, numSamples);
    
    // Calculate correction
    float semitoneShift = calculateCorrection(detectedPitchHz, sampleRate_);
    
    if (std::abs(semitoneShift) < 0.01f)
    {
        // No correction needed - pass through
        outputBuffer.copyFrom(0, 0, tempBuffer_, 0, 0, numSamples);
        return;
    }
    
    // Apply pitch shift using Rubber Band
    proShifter_->setPitchSemitones(semitoneShift);
    proShifter_->process(tempBuffer_, outputBuffer);
    
    // Output is mono - if stereo input, copy to both channels
    if (inputBuffer.getNumChannels() > 1 && outputBuffer.getNumChannels() > 1)
    {
        for (int ch = 1; ch < outputBuffer.getNumChannels(); ++ch)
        {
            outputBuffer.copyFrom(ch, 0, outputBuffer, 0, 0, numSamples);
        }
    }
}

} // namespace dsp
} // namespace zenith
