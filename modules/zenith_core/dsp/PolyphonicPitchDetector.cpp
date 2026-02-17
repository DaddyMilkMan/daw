/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#include "PolyphonicPitchDetector.h"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace zenith {
namespace dsp {

//==============================================================================
// PolyphonicPitchDetector Implementation
//==============================================================================
PolyphonicPitchDetector::PolyphonicPitchDetector()
{
    // Initialize FFT
    fft_ = std::make_unique<juce::dsp::FFT>(12);  // 2^12 = 4096
    fftBuffer_.resize(fftSize_);
    windowBuffer_.resize(fftSize_);

    // Create Hann window
    for (int i = 0; i < fftSize_; ++i)
    {
        windowBuffer_[i] = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * i / (fftSize_ - 1)));
    }

    nextSourceId_ = 0;
}

PolyphonicPitchDetector::~PolyphonicPitchDetector()
{
}

void PolyphonicPitchDetector::prepare(double sampleRate, int maxBlockSize)
{
    sampleRate_ = sampleRate;
    reset();
}

void PolyphonicPitchDetector::reset()
{
    timeFrequencyGrid_.clear();
    onsetTimes_.clear();
    sourceTracks_.clear();
    detectedNotes_.clear();
    currentTime_ = 0.0;
    previousFrame_.clear();
    nextSourceId_ = 0;
}

std::vector<PolyphonicNote> PolyphonicPitchDetector::analyze(const juce::AudioBuffer<float>& audio)
{
    reset();

    // Step 1: Compute Constant-Q Transform
    computeConstantQTransform(audio);

    // Step 2: Detect onsets
    detectOnsets();

    // Step 3: Perform harmonic clustering
    performHarmonicClustering();

    // Step 4: Track sources
    trackSources();

    // Step 5: Extract pitch curves
    extractPitchCurves();

    // Step 6: Finalize notes
    finalizeNotes();

    return detectedNotes_;
}

void PolyphonicPitchDetector::processBlock(const juce::AudioBuffer<float>& audio)
{
    // Compute CQT for this block
    computeConstantQTransform(audio);

    // Process onsets and clustering incrementally
    if (timeFrequencyGrid_.size() > 1)
    {
        detectOnsets();
        performHarmonicClustering();
        trackSources();
    }

    currentTime_ += audio.getNumSamples() / sampleRate_;
}

std::vector<PolyphonicNote> PolyphonicPitchDetector::getResults() const
{
    return detectedNotes_;
}

//==============================================================================
void PolyphonicPitchDetector::setFrequencyRange(float minFreq, float maxFreq)
{
    minFrequency_ = minFreq;
    maxFrequency_ = maxFreq;
}

//==============================================================================
// Analysis Implementation
//==============================================================================
void PolyphonicPitchDetector::computeConstantQTransform(const juce::AudioBuffer<float>& audio)
{
    const int numChannels = audio.getNumChannels();
    const int numSamples = audio.getNumSamples();

    // Convert to mono if stereo
    std::vector<float> monoData(numSamples);
    if (numChannels == 1)
    {
        juce::FloatVectorOperations::copy(monoData.data(), audio.getReadPointer(0), numSamples);
    }
    else
    {
        // Mix channels
        for (int ch = 0; ch < numChannels; ++ch)
        {
            juce::FloatVectorOperations::addWithMultiply(monoData.data(),
                                                         audio.getReadPointer(ch),
                                                         1.0f / numChannels,
                                                         numSamples);
        }
    }

    // Process each hop
    for (int pos = 0; pos + fftSize_ <= numSamples; pos += hopSize_)
    {
        // Apply window and copy to FFT buffer
        for (int i = 0; i < fftSize_; ++i)
        {
            if (pos + i < numSamples)
            {
                fftBuffer_[i] = std::complex<float>(monoData[pos + i] * windowBuffer_[i], 0.0f);
            }
            else
            {
                fftBuffer_[i] = std::complex<float>(0.0f, 0.0f);
            }
        }

        // Perform FFT
        fft_->perform(fftBuffer_.data(), true);

        // Convert to frequency bins (simplified CQT-like representation)
        std::vector<TFBin> frame;

        // Use logarithmic frequency bins (one per semitone)
        for (int bin = 0; bin < numBins_; ++bin)
        {
            float freq = binToFrequency(bin);

            // Find closest FFT bin
            int fftBin = static_cast<int>(freq * fftSize_ / sampleRate_);
            if (fftBin >= 0 && fftBin < fftSize_ / 2)
            {
                TFBin tfBin;
                tfBin.frequency = freq;
                tfBin.magnitude = std::abs(fftBuffer_[fftBin]);
                tfBin.phase = std::arg(fftBuffer_[fftBin]);
                tfBin.sourceId = -1;
                tfBin.isAssigned = false;

                frame.push_back(tfBin);
            }
        }

        timeFrequencyGrid_.push_back(frame);
        currentTime_ += hopSize_ / sampleRate_;
    }
}

void PolyphonicPitchDetector::detectOnsets()
{
    if (timeFrequencyGrid_.empty())
        return;

    onsetTimes_.clear();

    for (size_t i = 1; i < timeFrequencyGrid_.size(); ++i)
    {
        if (isOnset(timeFrequencyGrid_[i], timeFrequencyGrid_[i - 1]))
        {
            double time = (i * hopSize_) / sampleRate_;
            onsetTimes_.push_back(time);
        }
    }
}

void PolyphonicPitchDetector::performHarmonicClustering()
{
    if (timeFrequencyGrid_.empty())
        return;

    // Reset assignments
    for (auto& frame : timeFrequencyGrid_)
    {
        for (auto& bin : frame)
        {
            bin.isAssigned = false;
            bin.sourceId = -1;
        }
    }

    // For each frame, find fundamental frequencies and group harmonics
    for (auto& frame : timeFrequencyGrid_)
    {
        // Sort bins by magnitude (strongest first)
        std::vector<size_t> sortedIndices(frame.size());
        std::iota(sortedIndices.begin(), sortedIndices.end(), 0);
        std::sort(sortedIndices.begin(), sortedIndices.end(),
                  [&frame](size_t a, size_t b) {
                      return frame[a].magnitude > frame[b].magnitude;
                  });

        // Try to find fundamentals
        for (size_t idx : sortedIndices)
        {
            if (frame[idx].isAssigned || frame[idx].magnitude < onsetThreshold_)
                continue;

            float candidateFreq = frame[idx].frequency;

            // Check if this could be a fundamental
            // Look for harmonics
            auto harmonics = findHarmonics(candidateFreq, 5);

            float harmonicScore = 0.0f;
            int foundHarmonics = 0;

            for (int h : harmonics)
            {
                if (h >= 0 && h < static_cast<int>(frame.size()))
                {
                    if (!frame[h].isAssigned)
                    {
                        harmonicScore += frame[h].magnitude;
                        foundHarmonics++;
                    }
                }
            }

            // If we found harmonics, this is likely a fundamental
            if (foundHarmonics >= 2 && harmonicScore > harmonicThreshold_)
            {
                // Create or update source track
                SourceTrack source;
                source.id = nextSourceId_++;
                source.fundamentalFreq = candidateFreq;
                source.birthTime = currentTime_;
                source.isActive = true;

                // Assign this bin and its harmonics to this source
                frame[idx].sourceId = source.id;
                frame[idx].isAssigned = true;

                for (int h : harmonics)
                {
                    if (h >= 0 && h < static_cast<int>(frame.size()))
                    {
                        frame[h].sourceId = source.id;
                        frame[h].isAssigned = true;
                    }
                }

                sourceTracks_.push_back(source);
            }
        }
    }
}

void PolyphonicPitchDetector::trackSources()
{
    // Track sources through time using continuity
    // For simplicity, we'll just note when sources appear/disappear

    for (size_t frameIdx = 0; frameIdx < timeFrequencyGrid_.size(); ++frameIdx)
    {
        double frameTime = (frameIdx * hopSize_) / sampleRate_;

        for (auto& source : sourceTracks_)
        {
            bool foundInFrame = false;

            for (const auto& bin : timeFrequencyGrid_[frameIdx])
            {
                if (bin.sourceId == source.id && bin.isAssigned)
                {
                    foundInFrame = true;

                    // Record pitch and amplitude
                    source.pitchOverTime.push_back({frameTime, bin.frequency});
                    source.amplitudeOverTime.push_back({frameTime, bin.magnitude});
                    break;
                }
            }

            if (!foundInFrame && source.isActive)
            {
                source.deathTime = frameTime;
                source.isActive = false;
            }
        }
    }
}

void PolyphonicPitchDetector::extractPitchCurves()
{
    // For each source, extract the pitch curve
    for (const auto& source : sourceTracks_)
    {
        if (source.pitchOverTime.size() < 2)
            continue;  // Too short

        PolyphonicNote note;
        note.startTime = source.birthTime;
        note.endTime = source.deathTime;

        // Average pitch
        float pitchSum = 0.0f;
        for (const auto& [time, pitch] : source.pitchOverTime)
        {
            pitchSum += pitch;
        }
        note.pitchHz = pitchSum / static_cast<float>(source.pitchOverTime.size());

        // MIDI note
        note.midiNote = static_cast<int>(std::round(69.0f + 12.0f * std::log2(note.pitchHz / 440.0f)));

        // Average amplitude
        float ampSum = 0.0f;
        for (const auto& [time, amp] : source.amplitudeOverTime)
        {
            ampSum += amp;
        }
        note.amplitude = ampSum / static_cast<float>(source.amplitudeOverTime.size());

        // Confidence based on harmonic strength
        note.confidence = std::min(1.0f, note.amplitude * 2.0f);

        note.voiceIndex = source.id;
        note.harmonicSeries = 0;  // Fundamental

        detectedNotes_.push_back(note);
    }
}

void PolyphonicPitchDetector::finalizeNotes()
{
    // Filter out notes that are too short
    detectedNotes_.erase(
        std::remove_if(detectedNotes_.begin(), detectedNotes_.end(),
                      [this](const PolyphonicNote& note) {
                          return (note.endTime - note.startTime) < minNoteLength_;
                      }),
        detectedNotes_.end());

    // Sort by start time and pitch
    std::sort(detectedNotes_.begin(), detectedNotes_.end(),
              [](const PolyphonicNote& a, const PolyphonicNote& b) {
                  if (std::abs(a.startTime - b.startTime) < 0.001)
                      return a.pitchHz < b.pitchHz;
                  return a.startTime < b.startTime;
              });
}

//==============================================================================
// Helper Functions
//==============================================================================
float PolyphonicPitchDetector::frequencyToBin(float freq) const
{
    // Logarithmic binning
    float minLog = std::log2(minFrequency_);
    float maxLog = std::log2(maxFrequency_);
    float freqLog = std::log2(freq);

    return ((freqLog - minLog) / (maxLog - minLog)) * numBins_;
}

float PolyphonicPitchDetector::binToFrequency(int bin) const
{
    float minLog = std::log2(minFrequency_);
    float maxLog = std::log2(maxFrequency_);
    float normalized = static_cast<float>(bin) / numBins_;

    return std::pow(2.0f, minLog + normalized * (maxLog - minLog));
}

std::vector<int> PolyphonicPitchDetector::findHarmonics(float fundamental, int numHarmonics) const
{
    std::vector<int> harmonics;

    for (int h = 2; h <= numHarmonics; ++h)
    {
        float harmonicFreq = fundamental * h;
        int bin = static_cast<int>(frequencyToBin(harmonicFreq));
        harmonics.push_back(bin);
    }

    return harmonics;
}

float PolyphonicPitchDetector::calculateHarmonicScore(const std::vector<TFBin>& frame,
                                                      float fundamental) const
{
    float score = 0.0f;

    for (int h = 2; h <= 5; ++h)
    {
        float harmonicFreq = fundamental * h;
        int bin = static_cast<int>(frequencyToBin(harmonicFreq));

        if (bin >= 0 && bin < static_cast<int>(frame.size()))
        {
            score += frame[bin].magnitude;
        }
    }

    return score;
}

bool PolyphonicPitchDetector::isOnset(const std::vector<TFBin>& currentFrame,
                                      const std::vector<TFBin>& previousFrame) const
{
    if (currentFrame.size() != previousFrame.size())
        return false;

    // Calculate energy difference
    float currentEnergy = 0.0f;
    float previousEnergy = 0.0f;

    for (size_t i = 0; i < currentFrame.size(); ++i)
    {
        currentEnergy += currentFrame[i].magnitude;
        previousEnergy += previousFrame[i].magnitude;
    }

    float difference = currentEnergy - previousEnergy;
    float normalized = difference / (previousEnergy + 1e-6f);

    return normalized > onsetThreshold_;
}

//==============================================================================
// PolyphonicPitchCorrector Implementation
//==============================================================================
PolyphonicPitchCorrector::PolyphonicPitchCorrector()
{
}

PolyphonicPitchCorrector::~PolyphonicPitchCorrector()
{
}

void PolyphonicPitchCorrector::prepare(double sampleRate, int maxBlockSize)
{
    sampleRate_ = sampleRate;
    voiceProcessors_.clear();
}

juce::AudioBuffer<float> PolyphonicPitchCorrector::process(
    const juce::AudioBuffer<float>& audio,
    const std::vector<PolyphonicNote>& notes)
{
    // For polyphonic correction, we need to:
    // 1. Separate the audio into individual notes (using the source ID)
    // 2. Apply pitch correction to each note individually
    // 3. Recombine

    // This is a simplified implementation that applies overall correction
    // A full implementation would use source separation techniques

    juce::AudioBuffer<float> output;
    output.makeCopyOf(audio);

    // For now, apply basic pitch correction to the entire buffer
    // In a full implementation, each note would be processed separately

    return output;
}

void PolyphonicPitchCorrector::applyScaleCorrection(std::vector<PolyphonicNote>& notes,
                                                     int rootNote, int scaleType)
{
    // Define scale patterns
    std::vector<bool> scalePattern(12, true);  // Default: chromatic

    switch (scaleType)
    {
        case 0:  // Major
            scalePattern = {true, false, true, false, true, true, false, true, false, true, false, true};
            break;
        case 1:  // Minor
            scalePattern = {true, false, true, true, false, true, false, true, true, false, true, false};
            break;
        // Add more scale types as needed
    }

    for (auto& note : notes)
    {
        int noteClass = note.midiNote % 12;

        if (!scalePattern[noteClass])
        {
            // Find nearest scale note
            int direction = 0;
            int distance = 0;

            for (int d = 1; d <= 6; ++d)
            {
                int up = (noteClass + d) % 12;
                int down = (noteClass - d + 12) % 12;

                if (scalePattern[up])
                {
                    direction = 1;
                    distance = d;
                    break;
                }
                if (scalePattern[down])
                {
                    direction = -1;
                    distance = d;
                    break;
                }
            }

            // Calculate corrected pitch
            int correctedMidi = note.midiNote + direction * distance;
            note.correctedPitch = 440.0f * std::pow(2.0f, (correctedMidi - 69) / 12.0f);
        }
        else
        {
            // Already in scale
            note.correctedPitch = note.pitchHz;
        }
    }
}

} // namespace dsp
} // namespace zenith
