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

    PitchCorrector.h
    Created: 2026-01-29
    Author:  Zenith DAW

    Professional pitch correction engine with:
    - Scale-aware correction
    - Adjustable retune speed

    - Humanize/naturalize
    - Formant preservation
    - Vibrato detection and preservation

  ==============================================================================
*/

#pragma once

#include "ProPitchShifter.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <atomic>
#include <array>
#include <deque>

namespace zenith {
namespace dsp {

//==============================================================================
/**
    Musical scales for pitch correction.
*/
enum class MusicalScale
{
    Chromatic,      // All 12 notes
    Major,          // Major scale
    Minor,          // Natural minor
    MinorHarmonic,  // Harmonic minor
    MinorMelodic,   // Melodic minor
    PentatonicMajor,
    PentatonicMinor,
    Blues,
    Dorian,         // Common in electronic music
    Phrygian,
    Lydian,
    Mixolydian,
    Custom          // User-defined
};

//==============================================================================
/**
    Musical notes.
*/
enum class Note
{
    C = 0, CSharp, D, DSharp, E, F, FSharp, G, GSharp, A, ASharp, B,
    NumNotes = 12
};

//==============================================================================
/**
    Scale definition with root note.
*/
struct Scale
{
    Note rootNote = Note::C;
    MusicalScale scaleType = MusicalScale::Chromatic;
    std::array<bool, 12> customNotes{};  // For custom scale
    
    bool isNoteInScale(int midiNote) const;
    float getNearestScalePitch(float frequencyHz) const;
    float snapToScale(float frequencyHz, float correctionAmount) const;
};

//==============================================================================
/**
    Pitch correction processor with professional features.
    
    Features:
    - Real-time pitch correction
    - Adjustable retune speed (0ms - 800ms)
    - Humanize amount (preserves subtle pitch variation)
    - Formant preservation (maintains vocal character)
    - Vibrato detection (preserves intentional vibrato)
    - Scale/Key selection
*/
class PitchCorrector
{
public:
    //==============================================================================
    PitchCorrector();
    ~PitchCorrector();

    //==============================================================================
    /**
     * @brief Prepare for processing
     */
    void prepare(double sampleRate, int samplesPerBlock);
    
    /**
     * @brief Reset internal state
     */
    void reset();

    //==============================================================================
    /**
     * @brief Process audio with pitch correction
     * @param inputBuffer Input audio (mono)
     * @param detectedPitchHz Input pitch from detector (0 = no pitch)
     * @param outputBuffer Output buffer (corrected audio)
     */
    void process(const juce::AudioBuffer<float>& inputBuffer,
                 float detectedPitchHz,
                 juce::AudioBuffer<float>& outputBuffer);

    //==============================================================================
    // Parameter Setters
    //==============================================================================
    
    /**
     * @brief Enable/disable correction
     */
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_.load(); }
    
    /**
     * @brief Set correction amount (0.0 = none, 1.0 = full correction)
     */
    void setCorrectionAmount(float amount) { 
        correctionAmount_ = juce::jlimit(0.0f, 1.0f, amount); 
    }
    float getCorrectionAmount() const { return correctionAmount_.load(); }
    
    /**
     * @brief Set retune speed in milliseconds (0-800ms)
     * 0ms = instant (T-Pain effect)
     * 20-50ms = natural correction
     * 100ms+ = very subtle
     */
    void setRetuneSpeed(float ms) { 
        retuneSpeedMs_ = juce::jlimit(0.0f, 800.0f, ms); 
    }
    float getRetuneSpeed() const { return retuneSpeedMs_.load(); }
    
    /**
     * @brief Set humanize amount (0.0 = robotic, 1.0 = natural)
     * Preserves subtle pitch variations and vibrato
     */
    void setHumanize(float amount) { 
        humanize_ = juce::jlimit(0.0f, 1.0f, amount); 
    }
    float getHumanize() const { return humanize_.load(); }
    
    /**
     * @brief Set formant preservation (0.0 = none, 1.0 = full)
     * Prevents "chipmunk" effect when shifting pitch
     */
    void setFormantPreservation(float amount) { 
        formantPreservation_ = juce::jlimit(0.0f, 1.0f, amount);
        if (proShifter_)
            proShifter_->setFormantRatio(1.0f + (amount - 0.5f) * 0.4f);
    }
    float getFormantPreservation() const { return formantPreservation_.load(); }
    
    /**
     * @brief Set target scale
     */
    void setScale(const Scale& scale) { scale_ = scale; }
    const Scale& getScale() const { return scale_; }
    
    /**
     * @brief Quick scale setup
     */
    void setScale(Note root, MusicalScale type);
    
    //==============================================================================
    // Vibrato Control
    //==============================================================================
    
    /**
     * @brief Set vibrato preservation (0.0 = flatten, 1.0 = preserve natural)
     */
    void setVibratoPreservation(float amount) { 
        vibratoPreserve_ = juce::jlimit(0.0f, 1.0f, amount); 
    }
    float getVibratoPreservation() const { return vibratoPreserve_.load(); }
    
    /**
     * @brief Get detected vibrato depth (cents)
     */
    float getVibratoDepth() const { return vibratoDepth_.load(); }
    
    /**
     * @brief Get detected vibrato rate (Hz)
     */
    float getVibratoRate() const { return vibratoRate_.load(); }
    
    //==============================================================================
    // Note Transitions
    //==============================================================================
    
    /**
     * @brief Set different retune speed for note transitions vs sustained notes
     */
    void setNoteTransitionSpeed(float ms) {
        noteTransitionSpeedMs_ = juce::jlimit(0.0f, 800.0f, ms);
    }
    float getNoteTransitionSpeed() const { return noteTransitionSpeedMs_.load(); }
    
    /**
     * @brief Enable legato mode (smooth between notes)
     */
    void setLegatoMode(bool legato) { legatoMode_ = legato; }
    bool isLegatoMode() const { return legatoMode_.load(); }
    
    //==============================================================================
    // Quality Settings
    //==============================================================================
    
    /**
     * @brief Set processing quality
     */
    void setQuality(ProPitchShifter::Quality quality);
    ProPitchShifter::Quality getQuality() const { return quality_; }
    
    /**
     * @brief Get reported latency in samples
     */
    int getLatencySamples() const;
    
    /**
     * @brief Get reported latency in milliseconds
     */
    float getLatencyMs() const;
    
    //==============================================================================
    // Accessors for UI
    //==============================================================================
    float getCurrentPitch() const { return currentPitch_.load(); }
    float getTargetPitch() const { return targetPitch_.load(); }
    bool isPitchCorrecting() const { return isCorrecting_.load(); }
    float getCurrentCentsOffset() const { return currentCentsOffset_.load(); }

private:
    //==============================================================================
    // Internal processing
    float calculateCorrection(float inputPitch, float sampleRate);
    float frequencyToSemitones(float freqHz) const;
    float semitonesToFrequency(float semitones) const;
    void detectVibrato(float currentPitch);
    void applyCorrectionWithVibrato(float& targetSemitones, float detectedPitch);
    
    //==============================================================================
    // Parameters
    std::atomic<bool> enabled_{true};
    std::atomic<float> correctionAmount_{1.0f};
    std::atomic<float> retuneSpeedMs_{50.0f};  // Default: natural correction
    std::atomic<float> humanize_{0.5f};         // Default: some humanization
    std::atomic<float> formantPreservation_{0.8f};
    std::atomic<float> vibratoPreserve_{0.7f};  // Default: preserve most vibrato
    std::atomic<float> noteTransitionSpeedMs_{30.0f};
    std::atomic<bool> legatoMode_{false};
    ProPitchShifter::Quality quality_ = ProPitchShifter::Quality::Balanced;
    Scale scale_;
    
    // State
    double sampleRate_ = 44100.0;
    float smoothedCorrection_ = 0.0f;
    float currentSemitoneShift_ = 0.0f;
    float lastTargetSemitones_ = 0.0f;
    
    // Professional pitch shifter
    std::unique_ptr<ProPitchShifter> proShifter_;
    juce::AudioBuffer<float> tempBuffer_;
    
    // Vibrato detection
    std::deque<float> pitchHistory_;
    static constexpr int kVibratoHistorySize = 512;  // ~10ms at 48kHz
    std::atomic<float> vibratoDepth_{0.0f};
    std::atomic<float> vibratoRate_{0.0f};
    
    // For UI feedback
    std::atomic<float> currentPitch_{0.0f};
    std::atomic<float> targetPitch_{0.0f};
    std::atomic<bool> isCorrecting_{false};
    std::atomic<float> currentCentsOffset_{0.0f};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PitchCorrector)
};

} // namespace dsp
} // namespace zenith
