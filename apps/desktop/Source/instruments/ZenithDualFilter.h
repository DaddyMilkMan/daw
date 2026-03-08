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

#include "ZenithFilter.h"
#include "ZenithPolySynthDefs.h"
#include <juce_audio_basics/juce_audio_basics.h>

namespace zenith {

//==============================================================================
// DUAL FILTER ARCHITECTURE
//==============================================================================
/**
 * Dual filter routing matching Serum 2:
 *
 * Routing Modes:
 * - Serial: F1 → F2
 * - Parallel: F1 || F2
 * - Split: F1 (Low) + F2 (High)
 * - Stereo: Left → F1, Right → F2
 * - Wet/Dry: Dry (blend) → F1 → Wet → F2
 *
 * Each filter is independent ZenithFilter with:
 * - Separate type, model, cutoff, resonance
 * - Individual drive
 * - Stereo/mono processing
 */
class ZenithDualFilter {
public:
    //==========================================================================
    // Routing Modes
    //==========================================================================

    enum class RoutingMode {
        Serial,        ///< F1 → F2 → Output
        Parallel,       ///< F1 → Output, F2 → Output, mix
        Split,         ///< F1 (Low) → Output, F2 (High) → Output, mix
        Stereo,        ///< Left → F1, Right → F2
        WetDry         ///< Dry → F1 → F2 → Output, wet/dry mix
    };

    ZenithDualFilter();
    ~ZenithDualFilter() = default;

    //==========================================================================
    // Filter Access
    //==========================================================================

    ZenithFilter& getFilter1() { return filter1_; }
    ZenithFilter& getFilter2() { return filter2_; }

    //==========================================================================
    // Configuration
    //==========================================================================

    void setRouting(RoutingMode mode) { routing_ = mode; }
    RoutingMode getRouting() const { return routing_; }

    void setMix(float mix) { mix_ = juce::jlimit(0.0f, 1.0f, mix); }
    void setBalance(float balance) { balance_ = juce::jlimit(-1.0f, 1.0f, balance); }
    void setStereoSpread(float spread) { stereoSpread_ = juce::jlimit(0.0f, 1.0f, spread); }
    void setSplitFrequency(float freqHz) { splitFreq_ = juce::jlimit(20.0f, 20000.0f, freqHz); }

    //==========================================================================
    // Sample Rate
    //==========================================================================

    void setSampleRate(double sr);
    void reset();

    //==========================================================================
    // Processing
    //==========================================================================

    /**
     * @brief Process stereo sample through dual filters
     * @param left Left input sample
     * @param right Right input sample
     * @return Left output sample (for mono in, returns same as left)
     */
    float processSample(float& left, float& right);

    /**
     * @brief Process block of stereo samples
     */
    void process(juce::AudioBuffer<float>& buffer);

private:
    //==========================================================================
    // Filters
    //==========================================================================

    ZenithFilter filter1_, filter2_;
    double sampleRate_ = 44100.0;

    //==========================================================================
    // Routing Parameters
    //==========================================================================

    RoutingMode routing_ = RoutingMode::Serial;
    float mix_ = 0.5f;              ///< Dry/F1/F2 mix
    float balance_ = 0.0f;            ///< -1 = F1, +1 = F2
    float stereoSpread_ = 0.0f;      ///< 0 = mono, 1 = wide
    float splitFreq_ = 1000.0f;        ///< Frequency for split mode

    //==========================================================================
    // Internal State
    //==========================================================================

    juce::AudioBuffer<float> tempBuffer_;  // For parallel processing
    float lastLeft_ = 0.0f;
    float lastRight_ = 0.0f;

    //==========================================================================
    // Internal Helpers
    //==========================================================================

    float processSerial(float input, float midiNote);
    float processParallel(float input, float midiNote);
    void processSplit(float input, float midiNote, float& outLow, float& outHigh);
    void processStereo(float left, float right, float midiNote);
    void processWetDry(float input, float midiNote);

};

} // namespace zenith
