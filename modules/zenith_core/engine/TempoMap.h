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

// TempoMap.h - RT-safe tempo map for beat/time conversions

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_events/juce_events.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_data_structures/juce_data_structures.h>
#include <atomic>
#include <memory>
#include <vector>

namespace zenith {

//==============================================================================
/**
 * @struct TempoPoint
 // Brief: Single tempo change point
 */
struct TempoPoint
{
    double timeBeats;    // Position in beats from session start
    double bpm;          // Tempo at/after this point

    TempoPoint(double beats, double tempoBpm)
        : timeBeats(beats), bpm(tempoBpm) {}
};

//==============================================================================
/**
 * @struct TempoMapSnapshot
 // Brief: Immutable snapshot of tempo map data
 *
 * This structure is built on the message thread and shared to the audio thread.
 * Once created, it is never modified (immutable).
 */
struct TempoMapSnapshot
{
    std::vector<TempoPoint> points;  // Sorted by timeBeats
    int timeSigNumerator = 4;
    int timeSigDenominator = 4;

    TempoMapSnapshot() = default;

    // Pre-calculate cumulative time for each tempo point for fast lookups
    void prepare();

    struct CachedPoint
    {
        double timeBeats;
        double bpm;
        double cumulativeSeconds;  // Total time from start to this point
    };

    std::vector<CachedPoint> cachedPoints;
};

//==============================================================================
/**
 * @class TempoMap
 // Brief: RT-safe tempo map for beat/time conversions
 *
 * This class provides beat↔time/sample conversions based on a variable tempo map.
 * It uses an atomic shared_ptr to allow lock-free reads from the audio thread.
 */
class TempoMap
{
public:
    //==========================================================================
    TempoMap();
    ~TempoMap();

    //==========================================================================
    // Message Thread API
    //==========================================================================

    /**
     // Brief: Update tempo map from ValueTree
     * @param tempoMapTree ValueTree containing TEMPO_POINT children
     // Note: MESSAGE THREAD ONLY
     */
    void updateFromValueTree(const juce::ValueTree& tempoMapTree);

    /**
     // Brief: Set a single static tempo
     * @param bpm Tempo in BPM
     // Note: MESSAGE THREAD ONLY
     */
    void setSingleTempo(double bpm) [[maybe_unused]];

    //==========================================================================
    // RT-Safe Conversion API (Audio Thread Safe)
    //==========================================================================

    /**
     // Brief: Convert beats to seconds
     * @param beats Time in beats
     * @param sampleRate Current sample rate (informational, not used in this method)
     * @return Time in seconds
     // Note: RT-SAFE (audio thread safe)
     */
    double beatsToSeconds(double beats, double sampleRate) const;

    /**
     // Brief: Convert seconds to beats
     * @param seconds Time in seconds
     * @param sampleRate Current sample rate (informational, not used in this method)
     * @return Time in beats
     // Note: RT-SAFE (audio thread safe)
     */
    double secondsToBeats(double seconds, double sampleRate) const;

    /**
     // Brief: Convert beats to samples
     * @param beats Time in beats
     * @param sampleRate Current sample rate
     * @return Time in samples
     // Note: RT-SAFE (audio thread safe)
     */
    int64_t beatsToSamples(double beats, double sampleRate) const;

    /**
     // Brief: Convert samples to beats
     * @param samples Time in samples
     * @param sampleRate Current sample rate
     * @return Time in beats
     // Note: RT-SAFE (audio thread safe)
     */
    double samplesToBeats(int64_t samples, double sampleRate) const;

    /**
     // Brief: Get current tempo at a given beat position
     * @param beats Time in beats
     * @return Tempo in BPM
     // Note: RT-SAFE (audio thread safe)
     */
    double getTempoAt(double beats) const;

    /**
     // Brief: Get current time signature numerator
     * @return Numerator (e.g. 4 for 4/4)
     */
    int getTimeSignatureNumerator() const;

    /**
     // Brief: Get current time signature denominator
     * @return Denominator (e.g. 4 for 4/4)
     */
    int getTimeSignatureDenominator() const;

private:
    //==========================================================================
    // Atomic shared_ptr for lock-free access from audio thread
    //==========================================================================

    // Current snapshot (read by audio thread, swapped by message thread)
    // RT-safe: Uses C++20 atomic shared_ptr for lock-free access
    std::atomic<std::shared_ptr<const TempoMapSnapshot>> snapshot_;

    //==========================================================================
    // Helper Methods
    //==========================================================================

    /**
     // Brief: Swap snapshot atomically
     * @param newSnapshot New snapshot to install
     // Note: MESSAGE THREAD ONLY
     */
    void swapSnapshot(std::shared_ptr<const TempoMapSnapshot> newSnapshot);

    /**
     // Brief: Load current snapshot for reading
     * @return Shared pointer to current snapshot
     // Note: RT-SAFE (audio thread safe)
     */
    std::shared_ptr<const TempoMapSnapshot> loadSnapshot() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TempoMap)
};

} // namespace zenith

