/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux
    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>
    SPDX-License-Identifier: Apache-2.0 
*/
#include "ScaleAutoDetector.h"
#include "PitchDetector.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <juce_audio_basics/juce_audio_basics.h> // Ensure JUCE
namespace zenith {
namespace dsp {
//==============================================================================
// Scale profiles (Krumhansl-Schmuckler)
const std::array<float, 12> ScaleAutoDetector::majorProfile_ = {6.35f, 2.23f, 3.48f, 2.33f, 4.38f, 4.09f, 2.52f, 5.19f, 2.39f, 3.66f, 2.29f, 2.88f};
const std::array<float, 12> ScaleAutoDetector::minorProfile_ = {6.33f, 2.68f, 3.52f, 5.38f, 2.60f, 3.53f, 2.54f, 4.75f, 3.98f, 2.69f, 3.34f, 3.17f};
const std::array<float, 12> ScaleAutoDetector::minorHarmonicProfile_ = {6.33f, 2.68f, 3.52f, 5.38f, 2.60f, 3.53f, 2.54f, 4.75f, 3.98f, 2.69f, 6.0f, 3.17f}; // Boosted major 7th
const std::array<float, 12> ScaleAutoDetector::dorianProfile_ = {6.0f, 2.5f, 3.5f, 5.0f, 4.0f, 4.0f, 2.5f, 5.0f, 2.5f, 3.5f, 5.0f, 2.5f};
const std::array<float, 12> ScaleAutoDetector::mixolydianProfile_ = {6.0f, 2.5f, 3.5f, 2.5f, 4.5f, 4.0f, 2.5f, 5.0f, 2.5f, 4.0f, 2.5f, 2.5f};
//==============================================================================
ScaleAutoDetector::ScaleAutoDetector() {
    reset();
}
ScaleAutoDetector::~ScaleAutoDetector() {}
//==============================================================================
void ScaleAutoDetector::reset() {
    pitchHistogram_.fill(0.0f);
    totalWeight_ = 0.0f;
    sampleCount_.store(0);
}
void ScaleAutoDetector::addPitchSample(float pitchHz, float confidence) {
    if (confidence < minConfidence_ || pitchHz <= 0.0f) return;
    int noteClass = freqToNoteClass(pitchHz);
    pitchHistogram_[noteClass] += confidence;
    totalWeight_ += confidence;
    sampleCount_++;
}
int ScaleAutoDetector::freqToNoteClass(float freqHz) const {
    if (freqHz <= 0.0f) return -1;
    float midi = 12.0f * std::log2(freqHz / 440.0f) + 69.0f;
    return static_cast<int>(std::round(midi)) % 12;
}
void ScaleAutoDetector::analyzeAudio(const juce::AudioBuffer<float>& audio, double sampleRate) {
    PitchDetector detector;
    detector.prepare(sampleRate);
    detector.setMinFrequency(80.0f);
    detector.setMaxFrequency(1000.0f);
    detector.setConfidenceThreshold(minConfidence_);
    const int numSamples = audio.getNumSamples();
    const float* data = audio.getReadPointer(0);
    for (int i = 0; i < numSamples; ++i) {
        float pitch = detector.processSample(data[i]);
        float conf = detector.getConfidence();
        addPitchSample(pitch, conf);
    }
}
ScaleDetectionResult ScaleAutoDetector::getResult() const {
    if (totalWeight_ < 1.0f) {
        return {};
    }
    // Normalize histogram
    std::array<float, 12> normHist;
    for (int i = 0; i < 12; ++i) {
        normHist[i] = pitchHistogram_[i] / totalWeight_;
    }
    // Find best match
    float bestMatch = -1.0f;
    Note bestRoot = Note::C;
    MusicalScale bestScale = MusicalScale::Major;
    float majorMatch = 0.0f;
    float minorMatch = 0.0f;
    std::vector<MusicalScale> scales = {MusicalScale::Major, MusicalScale::Minor, MusicalScale::MinorHarmonic, MusicalScale::Dorian, MusicalScale::Mixolydian};
    for (Note root = Note::C; root <= Note::B; (int&)root += 1) {
        for (auto scale : scales) {
            float match = calculateScaleMatch(normHist, root, scale);
            if (match > bestMatch) {
                bestMatch = match;
                bestRoot = root;
                bestScale = scale;
            }
            if (scale == MusicalScale::Major) majorMatch = std::max(majorMatch, match);
            if (scale == MusicalScale::Minor) minorMatch = std::max(minorMatch, match);
        }
    }
    ScaleDetectionResult result;
    result.rootNote = bestRoot;
    result.scaleType = bestScale;
    result.confidence = bestMatch;
    result.majorMinorConfidence = (majorMatch - minorMatch) / (majorMatch + minorMatch + 1e-6f); // -1 minor, 1 major
    return result;
}
float ScaleAutoDetector::calculateScaleMatch(const std::array<float, 12>& histogram, Note root, MusicalScale scale) const {
    const std::array<float, 12>* profile;
    switch (scale) {
        case MusicalScale::Major: profile = &majorProfile_; break;
        case MusicalScale::Minor: profile = &minorProfile_; break;
        case MusicalScale::MinorHarmonic: profile = &minorHarmonicProfile_; break;
        case MusicalScale::Dorian: profile = &dorianProfile_; break;
        case MusicalScale::Mixolydian: profile = &mixolydianProfile_; break;
        default: return 0.0f;
    }
    // Rotate profile to root
    std::array<float, 12> rotated;
    int rootInt = static_cast<int>(root);
    for (int i = 0; i < 12; ++i) {
        rotated[i] = (*profile)[(i + rootInt) % 12];
    }
    // Cosine similarity
    float dot = 0.0f;
    float normA = 0.0f;
    float normB = 0.0f;
    for (int i = 0; i < 12; ++i) {
        dot += histogram[i] * rotated[i];
        normA += histogram[i] * histogram[i];
        normB += rotated[i] * rotated[i];
    }
    return dot / std::sqrt(normA * normB + 1e-6f);
}
Scale ScaleAutoDetector::getDetectedScale() const {
    ScaleDetectionResult res = getResult();
    return {res.rootNote, res.scaleType};
}
std::array<float, 12> ScaleAutoDetector::getPitchHistogram() const {
    if (totalWeight_ < 1.0f) return {};
    std::array<float, 12> norm;
    for (int i = 0; i < 12; ++i) {
        norm[i] = pitchHistogram_[i] / totalWeight_;
    }
    return norm;
}
std::vector<std::pair<ScaleDetectionResult, float>> ScaleAutoDetector::getTopMatches(int numMatches) const {
    std::vector<std::pair<ScaleDetectionResult, float>> matches;

    if (totalWeight_ < 1.0f) {
        return matches;
    }

    // Normalize histogram
    std::array<float, 12> normHist;
    for (int i = 0; i < 12; ++i) {
        normHist[i] = pitchHistogram_[i] / totalWeight_;
    }

    // Try all combinations
    std::vector<MusicalScale> scales = {
        MusicalScale::Major, MusicalScale::Minor, MusicalScale::MinorHarmonic,
        MusicalScale::Dorian, MusicalScale::Mixolydian
    };

    for (Note root = Note::C; root <= Note::B; (int&)root += 1) {
        for (auto scale : scales) {
            ScaleDetectionResult result;
            result.rootNote = root;
            result.scaleType = scale;

            float match = calculateScaleMatch(normHist, root, scale);
            result.confidence = match;

            // Calculate major vs minor confidence for this root
            float majorMatch = calculateScaleMatch(normHist, root, MusicalScale::Major);
            float minorMatch = calculateScaleMatch(normHist, root, MusicalScale::Minor);
            result.majorMinorConfidence = (majorMatch - minorMatch) / (majorMatch + minorMatch + 1e-6f);

            matches.push_back({result, match});
        }
    }

    // Sort by correlation (descending)
    std::sort(matches.begin(), matches.end(),
        [](const auto& a, const auto& b) {
            return a.second > b.second;
        });

    // Return top N
    if (static_cast<int>(matches.size()) > numMatches) {
        matches.resize(numMatches);
    }

    return matches;
}

juce::String ScaleDetectionResult::getDescription() const {
    // Convert root note to string
    juce::String rootName;
    switch (rootNote) {
        case Note::C: rootName = "C"; break;
        case Note::CSharp: rootName = "C#"; break;
        case Note::D: rootName = "D"; break;
        case Note::DSharp: rootName = "D#"; break;
        case Note::E: rootName = "E"; break;
        case Note::F: rootName = "F"; break;
        case Note::FSharp: rootName = "F#"; break;
        case Note::G: rootName = "G"; break;
        case Note::GSharp: rootName = "G#"; break;
        case Note::A: rootName = "A"; break;
        case Note::ASharp: rootName = "A#"; break;
        case Note::B: rootName = "B"; break;
        default: rootName = "?"; break;
    }

    // Convert scale type to string
    juce::String scaleName;
    switch (scaleType) {
        case MusicalScale::Major: scaleName = "Major"; break;
        case MusicalScale::Minor: scaleName = "Minor"; break;
        case MusicalScale::MinorHarmonic: scaleName = "Harmonic Minor"; break;
        case MusicalScale::MinorMelodic: scaleName = "Melodic Minor"; break;
        case MusicalScale::Dorian: scaleName = "Dorian"; break;
        case MusicalScale::Phrygian: scaleName = "Phrygian"; break;
        case MusicalScale::Lydian: scaleName = "Lydian"; break;
        case MusicalScale::Mixolydian: scaleName = "Mixolydian"; break;
        case MusicalScale::Locrian: scaleName = "Locrian"; break;
        case MusicalScale::Chromatic: scaleName = "Chromatic"; break;
        case MusicalScale::PentatonicMajor: scaleName = "Pentatonic Major"; break;
        case MusicalScale::PentatonicMinor: scaleName = "Pentatonic Minor"; break;
        case MusicalScale::Blues: scaleName = "Blues"; break;
        default: scaleName = "Unknown"; break;
    }

    return rootName + " " + scaleName + juce::String::formatted(" (%.0f%%)", confidence * 100.0f);
}
