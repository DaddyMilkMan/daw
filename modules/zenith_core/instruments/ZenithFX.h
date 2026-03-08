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

#pragma once

#include "ZenithPolySynthDefs.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

namespace zenith {

//==============================================================================
// RING MODULATOR
//==============================================================================
/**
 * Professional ring modulator matching Serum 2:
 * - Configurable carrier/modulator routing
 * - Polarity control (positive/negative ring)
 * - Wet/dry mix with stereo spread
 * - Frequency tracking (carrier tracks modulator)
 */
class ZenithRingModulator {
public:
    ZenithRingModulator();

    enum class Routing {
        CarrierToMod,      ///< Carrier → Modulator → Output
        ModulatorToCarrier,  ///< Modulator → Carrier → Output
        Parallel,           ///< Carrier → Output, Modulator → Output, mix
        Difference,         ///< (Carrier × Modulator) → Output
        XOR                ///< (Carrier ⊕ Modulator) → Output
    };

    void setRouting(Routing r) { routing_ = r; }
    Routing getRouting() const { return routing_; }

    void setCarrierOscillator(int index) { carrierIndex_ = index; }
    void setModulatorOscillator(int index) { modulatorIndex_ = index; }

    void setPolarity(bool positive) { polarity_ = positive; }

    /**
     * @brief Process ring modulation
     * @param carrier Carrier sample
     * @param modulator Modulator sample
     * @return Modulated output
     */
    float processSample(float carrier, float modulator);

private:
    int carrierIndex_ = 0;
    int modulatorIndex_ = 1;
    bool polarity_ = false;
    juce::Random random_;

    float applyRouting(float c, float m);
};

//==============================================================================
// FREQUENCY SHIFTER
//==============================================================================
/**
 * Professional frequency shifter:
 * - Through-zero (linear) and ring (modulated) modes
 * - Pitch shift in semitones (-12 to +12)
 * - Feedback with stereo widening
 * - Formant filter
 */
class ZenithFrequencyShifter {
public:
    ZenithFrequencyShifter();

    enum class Mode {
        Through,          ///< Linear pitch shift
        Ring,            ///< Ring modulation style
        Formant,          ///< Vocal formant enhancement
        Shift,            ///< Simple frequency shift
    };

    void setMode(Mode mode) { mode_ = mode; }
    void setShiftSemitones(int semitones) { shiftSemitones_ = juce::jlimit(-12, 12, semitones); }
    void setFeedback(float feedback) { feedback_ = juce::jlimit(0.0f, 0.95f, feedback); }
    void setStereoWidth(float width) { stereoWidth_ = juce::jlimit(0.0f, 1.0f, width); }

    /**
     * @brief Process stereo sample
     */
    void processSample(float& left, float& right);

private:
    Mode mode_ = Mode::Through;
    int shiftSemitones_ = 0;
    float feedback_ = 0.0f;
    float stereoWidth_ = 0.5f;

    juce::Random random_;

    struct {
        double phase[2] = {0.0, 0.0};
        double lfoPhase = 0.0;
    } state_;

    void processThrough(float& left, float& right);
    void processRing(float& left, float& right);
    void processFormant(float& left, float& right);
    void processShift(float& left, float& right);
};

//==============================================================================
// VOCAL FORMANT FILTER
//==============================================================================
/**
 * Formant filter for vocal synthesis:
 * - 5 formant vowels (A, E, I, O, U)
 * - 24-band filter bank
 * - Morphing between vowels
 * - Bright/throat controls
 */
class ZenithVocalFormant {
public:
    ZenithVocalFormant();

    enum class Vowel {
        A = 0,  ///< Ah vowel (bright)
        E = 1,  ///< Eh vowel (mid)
        I = 2,  ///< Ih vowel (dark)
        O = 3,  ///< Oh vowel (mid)
        U = 4,  ///< Oo vowel (bright)
    };

    void setVowel(Vowel vowel) { vowel_ = vowel; }
    void setMorph(float morph) { morph_ = juce::jlimit(0.0f, 1.0f, morph); }
    void setBright(float bright) { bright_ = juce::jlimit(0.0f, 1.0f, bright); }
    void setThroat(float throat) { throat_ = juce::jlimit(0.0f, 1.0f, throat); }

    /**
     * @brief Process sample through formant filter
     * @param input Input sample
     * @param pitch Pitch (0-1, where 0.5 = no shift)
     * @return Filtered sample
     */
    float processSample(float input, float pitch = 0.5f);

private:
    Vowel vowel_ = Vowel::A;
    float morph_ = 0.0f;
    float bright_ = 0.5f;
    float throat_ = 0.5f;

    static constexpr int NUM_BANDS = 24;

    // 24-band formant filters (5 vowels × different peaks)
    static constexpr float formantFilters[NUM_BANDS][5] = {
        // A - bright, focused
        {0.0f, 1.0f, 0.5f, 0.8f, 0.2f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
         {0.3f, 0.9f, 1.0f, 0.7f, 0.5f, 0.3f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
         {0.5f, 0.7f, 0.8f, 0.5f, 0.2f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},

        // E - mid, balanced
        {0.0f, 0.2f, 0.8f, 0.5f, 0.8f, 0.2f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
         {0.3f, 0.5f, 0.8f, 0.6f, 0.5f, 0.2f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
         {0.5f, 0.6f, 0.7f, 0.5f, 0.2f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},

        // I - dark, spread
        {0.0f, 0.3f, 0.6f, 0.7f, 0.4f, 0.1f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
         {0.0f, 0.2f, 0.4f, 0.6f, 0.7f, 0.4f, 0.1f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},

        // O - bright, focused
        {0.0f, 0.0f, 0.2f, 0.5f, 0.6f, 0.8f, 0.4f, 0.1f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
         {0.0f, 0.1f, 0.4f, 0.5f, 0.6f, 0.7f, 0.4f, 0.2f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},

        // U - bright, open
        {0.0f, 0.0f, 0.0f, 0.2f, 0.5f, 0.6f, 0.8f, 0.5f, 0.2f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
         {0.0f, 0.0f, 0.1f, 0.3f, 0.5f, 0.5f, 0.4f, 0.2f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
    };
};

} // namespace zenith
