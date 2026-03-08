/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <complex>
#include <array>
#include <memory>

namespace zenith {
namespace dsp {

//==============================================================================
/**
    Detected note from polyphonic audio.
*/
struct PolyphonicNote
{
    double startTime = 0.0;
    double endTime = 0.0;
    float pitchHz = 0.0f;
    int midiNote = 0;
    float amplitude = 0.0f;
    float confidence = 0.0f;
    int voiceIndex = 0;          // Which voice in the chord
    int harmonicSeries = 0;      // Fundamental = 0

    // For editing
    bool isSelected = false;
    float correctedPitch = 0.0f;  // User-edited pitch
};

//==============================================================================
/**
    Time-frequency bin with source information.
*/
struct TFBin
{
    float frequency = 0.0f;
    float magnitude = 0.0f;
    float phase = 0.0f;
    int sourceId = -1;           // Which source (note) this belongs to
    bool isAssigned = false;
};

//==============================================================================
/**
    Source (note) track through time.
*/
struct SourceTrack
{
    int id = 0;
    float fundamentalFreq = 0.0f;
    std::vector<std::pair<double, float>> pitchOverTime;  // (time, pitch)
    std::vector<std::pair<double, float>> amplitudeOverTime;
    double birthTime = 0.0;
    double deathTime = 0.0;
    bool isActive = false;
};

//==============================================================================
/**
    Polyphonic pitch detector for Melodyne DNA-like functionality.

    Uses Constant-Q Transform for musical frequency resolution and
    harmonic clustering to separate individual notes within chords.

    Algorithm:
    1. Time-frequency analysis using Constant-Q Transform (CQT)
    2. Harmonic clustering - group harmonics by fundamental
    3. Onset detection - find note beginnings
    4. Source tracking - follow each note through time
    5. Pitch extraction - extract pitch curve for each note

    This is a simplified implementation compared to full Melodyne DNA,
    but provides functional polyphonic pitch detection and editing.
*/
class PolyphonicPitchDetector
{
public:
    //==============================================================================
    PolyphonicPitchDetector();
    ~PolyphonicPitchDetector();

    //==============================================================================
    /**
     * @brief Prepare for processing
     * @param sampleRate Sample rate in Hz
     * @param maxBlockSize Maximum expected buffer size
     */
    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    //==============================================================================
    /**
     * @brief Analyze audio for polyphonic pitches
     * @param audio Input audio (can be mono or stereo)
     * @return Vector of detected notes
     */
    std::vector<PolyphonicNote> analyze(const juce::AudioBuffer<float>& audio);

    //==============================================================================
    /**
     * @brief Analyze audio block in streaming mode
     * @param audio Input audio block
     */
    void processBlock(const juce::AudioBuffer<float>& audio);

    /**
     * @brief Get final note results after streaming analysis
     */
    std::vector<PolyphonicNote> getResults() const;

    //==============================================================================
    /**
     * @brief Set frequency range for detection
     */
    void setFrequencyRange(float minFreq, float maxFreq);
    void setMinFrequency(float freq) { minFrequency_ = freq; }
    void setMaxFrequency(float freq) { maxFrequency_ = freq; }

    //==============================================================================
    /**
     * @brief Configure analysis parameters
     */
    void setOnsetThreshold(float threshold) { onsetThreshold_ = threshold; }
    void setHarmonicThreshold(float threshold) { harmonicThreshold_ = threshold; }
    void setMinNoteLength(double seconds) { minNoteLength_ = seconds; }

    //==============================================================================
    /**
     * @brief Get current time-frequency representation
     */
    const std::vector<std::vector<TFBin>>& getTimeFrequencyRepresentation() const
    { return timeFrequencyGrid_; }

    //==============================================================================
    /**
     * @brief Get detected sources
     */
    const std::vector<SourceTrack>& getSourceTracks() const
    { return sourceTracks_; }

private:
    //==============================================================================
    // Analysis steps
    void computeConstantQTransform(const juce::AudioBuffer<float>& audio);
    void detectOnsets();
    void performHarmonicClustering();
    void trackSources();
    void extractPitchCurves();
    void finalizeNotes();

    //==============================================================================
    // Helper functions
    float frequencyToBin(float freq) const;
    float binToFrequency(int bin) const;
    std::vector<int> findHarmonics(float fundamental, int numHarmonics) const;
    float calculateHarmonicScore(const std::vector<TFBin>& frame, float fundamental) const;
    bool isOnset(const std::vector<TFBin>& currentFrame,
                 const std::vector<TFBin>& previousFrame) const;

    //==============================================================================
    // Configuration
    double sampleRate_ = 44100.0;
    float minFrequency_ = 80.0f;      // C2
    float maxFrequency_ = 1000.0f;    // B5
    float onsetThreshold_ = 0.3f;
    float harmonicThreshold_ = 0.5f;
    double minNoteLength_ = 0.05;     // 50ms minimum

    // Constant-Q Transform parameters
    int numBins_ = 72;               // 6 octaves * 12 notes
    int binsPerOctave_ = 12;
    float qFactor_ = 34.0f;          // Quality factor for CQT
    int fftSize_ = 4096;
    int hopSize_ = 1024;

    // Analysis results
    std::vector<std::vector<TFBin>> timeFrequencyGrid_;
    std::vector<double> onsetTimes_;
    std::vector<SourceTrack> sourceTracks_;
    std::vector<PolyphonicNote> detectedNotes_;

    // Processing state
    double currentTime_ = 0.0;
    std::vector<TFBin> previousFrame_;
    std::unique_ptr<juce::dsp::FFT> fft_;
    std::vector<std::complex<float>> fftBuffer_;
    std::vector<float> windowBuffer_;

    // Source ID counter
    int nextSourceId_ = 0;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PolyphonicPitchDetector)
};

//==============================================================================
/**
    Polyphonic pitch corrector for editing individual notes in chords.
*/
class PolyphonicPitchCorrector
{
public:
    //==============================================================================
    PolyphonicPitchCorrector();
    ~PolyphonicPitchCorrector();

    //==============================================================================
    /**
     * @brief Prepare for processing
     */
    void prepare(double sampleRate, int maxBlockSize);

    //==============================================================================
    /**
     * @brief Correct pitches based on edited notes
     * @param audio Input audio
     * @param notes Notes with correctedPitch values
     * @return Corrected audio
     */
    juce::AudioBuffer<float> process(
        const juce::AudioBuffer<float>& audio,
        const std::vector<PolyphonicNote>& notes);

    //==============================================================================
    /**
     * @brief Apply scale correction to all notes
     */
    void applyScaleCorrection(std::vector<PolyphonicNote>& notes,
                               int rootNote, int scaleType);

    //==============================================================================
    /**
     * @brief Set correction amount (0-100%)
     */
    void setCorrectionAmount(float amount) { correctionAmount_ = amount; }

private:
    //==============================================================================
    // Rubber Band library integration for high-quality pitch shifting
    struct VoiceProcessor
    {
        int sourceId = 0;
        float originalPitch = 0.0f;
        float targetPitch = 0.0f;
        juce::AudioBuffer<float> buffer;
    };

    std::vector<VoiceProcessor> voiceProcessors_;
    float correctionAmount_ = 1.0f;
    double sampleRate_ = 44100.0;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PolyphonicPitchCorrector)
};

} // namespace dsp
} // namespace zenith
