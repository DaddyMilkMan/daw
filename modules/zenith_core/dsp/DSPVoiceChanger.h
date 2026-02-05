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

    DSPVoiceChanger.h
    Created: 2025-11-29
    Author:  Zenith DAW - Efficient C++ Team
    
    Real-time voice changing using WSOLA (Waveform Similarity Overlap-Add)
    pitch shifting with optional formant preservation.


  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>

namespace zenith {

//==============================================================================
// DSP Constants - All documented with units
//==============================================================================

/** Maximum buffer size in samples (~4 seconds at 48kHz) */
static constexpr int kMaxBufferSamples = 192000;

/** Default WSOLA window size in milliseconds */
static constexpr float kDefaultWindowSizeMs = 30.0f;

/** Minimum window size in milliseconds */
static constexpr float kMinWindowSizeMs = 20.0f;

/** Maximum window size in milliseconds */
static constexpr float kMaxWindowSizeMs = 50.0f;

/** Overlap ratio between grains (0.75 = 75% overlap) */
static constexpr float kOverlapRatio = 0.75f;

/** Tolerance window for WSOLA similarity search in milliseconds */
static constexpr float kWsolaToleranceMs = 10.0f;

/** Transition crossfade duration in milliseconds */
static constexpr float kTransitionFadeMs = 10.0f;

/** Ring modulator frequency for Robot effect in Hz */
static constexpr float kRobotRingModFreqHz = 50.0f;

/** Minimum pitch shift in semitones (one octave down) */
static constexpr float kMinSemitones = -12.0f;

/** Maximum pitch shift in semitones (one octave up) */
static constexpr float kMaxSemitones = 12.0f;

//==============================================================================
// Voice Character Pitch Mappings (in semitones)
//==============================================================================

/** DeepMale pitch shift in semitones */
static constexpr float kDeepMaleSemitones = -5.0f;

/** Chipmunk pitch shift in semitones */
static constexpr float kChipmunkSemitones = 7.0f;

//==============================================================================
// Formant Filter Bank Constants
//==============================================================================

/** Number of formant bands for preservation */
static constexpr int kFormantBandCount = 5;

/** Formant center frequencies in Hz (vowel formants F1-F5) */
static constexpr std::array<float, kFormantBandCount> kFormantFrequencies = {
    500.0f,   // F1 - First formant
    1500.0f,  // F2 - Second formant
    2500.0f,  // F3 - Third formant
    3500.0f,  // F4 - Fourth formant
    4500.0f   // F5 - Fifth formant
};

/** Formant filter Q values */
static constexpr std::array<float, kFormantBandCount> kFormantQValues = {
    5.0f, 6.0f, 7.0f, 8.0f, 10.0f
};

//==============================================================================
// DSPVoiceChanger Class
//==============================================================================

class DSPVoiceChanger {
public:
    enum class VoiceCharacter {
        DeepMale,
        Chipmunk,
        Robot,
        Ethereal
    };

    DSPVoiceChanger();
    ~DSPVoiceChanger();

    /**
     * Prepare the DSP for processing.
     * @param spec JUCE ProcessSpec with sample rate, block size, and channel count
     */
    void prepare(const juce::dsp::ProcessSpec& spec);
    
    /**
     * Reset all internal state (buffers, phases, filters).
     */
    void reset();

    /**
     * Process audio through the voice changer.
     * @param inputBlock Const audio block to read from
     * @param outputBlock Audio block to write processed audio to
     * @param character Voice character to apply
     */
    void process(const juce::dsp::AudioBlock<const float>& inputBlock,
                 juce::dsp::AudioBlock<float>& outputBlock,
                 VoiceCharacter character);

    /**
     * Enable or disable formant preservation.
     * When enabled, voice characters maintain natural vocal quality.
     * @param enabled True to enable formant preservation
     */
    void setFormantPreservation(bool enabled) { formantPreservation_ = enabled; }
    
    /**
     * Check if formant preservation is enabled.
     * @return True if formant preservation is enabled
     */
    bool isFormantPreservationEnabled() const { return formantPreservation_; }

    /**
     * Set the WSOLA window size.
     * @param windowSizeMs Window size in milliseconds (clamped to min/max)
     */
    void setWindowSize(float windowSizeMs);
    
    /**
     * Get current window size in milliseconds.
     * @return Window size in ms
     */
    float getWindowSizeMs() const { return windowSizeMs_; }

    /**
     * Convert semitones to pitch ratio.
     * @param semitones Pitch shift in semitones (+/- values)
     * @return Linear pitch ratio (e.g., 2.0 for +12 semitones)
     */
    static float semitonesToRatio(float semitones) {
        return std::pow(2.0f, semitones / 12.0f);
    }

    /**
     * Convert pitch ratio to semitones.
     * @param ratio Linear pitch ratio
     * @return Pitch shift in semitones
     */
    static float ratioToSemitones(float ratio) {
        return 12.0f * std::log2(ratio);
    }

private:
    //==========================================================================
    // Core State
    //==========================================================================
    
    double sampleRate_ = 44100.0;
    
    //==========================================================================
    // WSOLA Pitch Shifter State
    //==========================================================================
    
    /** Circular input buffer for grain extraction */
    std::vector<float> inputBuffer_;
    
    /** Write position in input buffer (samples) */
    int inputWritePos_ = 0;
    
    /** Fractional read position in input buffer (samples) */
    float inputReadPos_ = 0.0f;
    
    /** Precomputed Hann window */
    std::vector<float> hannWindow_;
    
    /** Current grain storage (windowed samples) */
    std::vector<float> currentGrain_;
    
    /** Previous grain storage for overlap */
    std::vector<float> prevGrain_;
    
    /** Overlap-add output accumulator */
    std::vector<float> overlapBuffer_;
    
    /** Position within current overlap buffer output (samples) */
    int overlapOutputPos_ = 0;
    
    /** Window size in samples */
    int windowSizeSamples_ = 0;
    
    /** Hop size for output (samples) - windowSize * (1 - overlap) */
    int hopSizeSamples_ = 0;
    
    /** Tolerance for WSOLA search in samples */
    int toleranceSamples_ = 0;
    
    /** Current window size in milliseconds */
    float windowSizeMs_ = kDefaultWindowSizeMs;
    
    /** Current pitch ratio (1.0 = no change) */
    float currentPitchRatio_ = 1.0f;
    
    //==========================================================================
    // Transition Fade State
    //==========================================================================
    
    /** Previous voice character for transition detection */
    VoiceCharacter prevCharacter_ = VoiceCharacter::DeepMale;
    
    /** Transition fade progress (0.0 = old character, 1.0 = new character) */
    float transitionFade_ = 1.0f;
    
    /** Fade increment per sample */
    float fadeIncrement_ = 0.0f;
    
    /** Stored output for crossfade during transitions */
    std::vector<float> transitionBuffer_;
    
    /** Flag indicating a transition is in progress */
    bool isTransitioning_ = false;
    
    /** Flag to detect first block processing */
    bool firstBlock_ = true;
    
    //==========================================================================
    // Ring Modulator State (Robot Effect)
    //==========================================================================
    
    /** Ring modulator phase (normalized 0-1) */
    double ringModPhase_ = 0.0;
    
    //==========================================================================
    // Formant Preservation State
    //==========================================================================
    
    /** Enable formant preservation processing */
    bool formantPreservation_ = false;
    
    /** Analysis filter bank (extract formants from input) */
    std::array<juce::dsp::IIR::Filter<float>, kFormantBandCount> analysisFilters_;
    
    /** Synthesis filter bank (apply formants to output) */
    std::array<juce::dsp::IIR::Filter<float>, kFormantBandCount> synthesisFilters_;
    
    /** Formant envelope followers (one per band) */
    std::array<float, kFormantBandCount> formantEnvelopes_;
    
    /** Envelope follower attack coefficient */
    float envelopeAttack_ = 0.0f;
    
    /** Envelope follower release coefficient */
    float envelopeRelease_ = 0.0f;
    
    //==========================================================================
    // Private Methods
    //==========================================================================
    
    /** 
     * Recalculate WSOLA parameters based on current sample rate and settings.
     */
    void recalculateWsolaParameters();
    
    /**
     * Generate Hann window of specified size.
     * @param windowSize Size of window in samples
     */
    void generateHannWindow(int windowSize);
    
    /**
     * Extract and window a grain from the input buffer.
     * @param startPos Starting position in input buffer (fractional)
     */
    void extractGrain(float startPos);
    
    /**
     * Find best match position using WSOLA cross-correlation.
     * @param targetPos Target position in input buffer
     * @return Best matching position within tolerance
     */
    int findBestMatchPosition(int targetPos);
    
    /**
     * Calculate cross-correlation between two buffer positions.
     * @param pos1 First position
     * @param pos2 Second position
     * @param length Correlation length
     * @return Normalized cross-correlation value (-1 to 1)
     */
    float calculateCrossCorrelation(int pos1, int pos2, int length);
    
    /**
     * Process a single sample through pitch shifting.
     * @param input Input sample
     * @return Pitch-shifted output sample
     */
    float processPitchShift(float input);
    
    /**
     * Process formant preservation for a sample.
     * @param input Original input sample
     * @param pitchShifted Pitch-shifted sample
     * @return Formant-corrected output sample
     */
    float processFormantPreservation(float input, float pitchShifted);
    
    /**
     * Initialize formant filter bank.
     */
    void initFormantFilters();
    
    /**
     * Handle transition between voice characters.
     * @param newCharacter New voice character
     */
    void handleCharacterTransition(VoiceCharacter newCharacter);
    
    /**
     * Get pitch ratio for a voice character.
     * @param character Voice character
     * @return Pitch ratio
     */
    float getPitchRatioForCharacter(VoiceCharacter character) const;
};

} // namespace zenith
