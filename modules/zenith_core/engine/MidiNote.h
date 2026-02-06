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

#include <array>
#include <vector>
#include <juce_core/juce_core.h>

namespace zenith {

enum class NoteExpressionType {
    PitchBend = 0,

    Pressure,
    Slide,
    Expression,
    Count
};

struct NoteExpressionPoint {
    double timeOffset = 0.0; ///< Offset from note start in beats
    float value = 0.0f;      ///< 0.0 to 1.0
    float tension = 0.0f;    ///< -1.0 to 1.0
};

constexpr size_t kNoteExpressionTypeCount =
    static_cast<size_t>(NoteExpressionType::Count);

using NoteExpressionSeries = std::vector<NoteExpressionPoint>;
using NoteExpressionMap = std::array<NoteExpressionSeries, kNoteExpressionTypeCount>;

/**
 * @brief Canonical MIDI note representation for Zenith DAW.
 * 
 * Standardizes velocity to float [0.0, 1.0] to support high-res MIDI 2.0
 * and precise DSP scaling.
 */
struct MidiNote {
    juce::String id;        ///< Unique identifier for tracking/UI
    int pitch = 60;         ///< MIDI note number (0-127)
    double startBeats = 0.0;///< Start time relative to clip start
    double lengthBeats = 1.0;///< Duration in beats
    float velocity = 0.8f;  ///< Normalized velocity [0.0, 1.0]
    bool muted = false;     ///< If true, note is ignored in playback

    // AI/Generative Properties (Phase 8+)
    float probability = 1.0f; ///< 0.0 (never) to 1.0 (always)
    juce::String condition;   ///< Logic condition (e.g. "PreviousPlayed")
    juce::String recurrence;  ///< Loop recurrence (e.g. "1:4")
    int articulationId = 0;   ///< Articulation/Keyswitch ID
    float tension = 0.0f;     ///< -1.0 to 1.0 (swing/timing curve)
    NoteExpressionMap expressions; ///< Per-note expression data (MPE)

    MidiNote() = default;

    /**
     * @brief Utility to convert from standard MIDI 0-127 velocity
     */
    static float fromMidiVelocity(int v) {
        return juce::jlimit(0.0f, 1.0f, static_cast<float>(v) / 127.0f);
    }

    /**
     * @brief Utility to convert to standard MIDI 0-127 velocity
     */
    static int toMidiVelocity(float v) {
        return juce::jlimit(0, 127, static_cast<int>(std::round(v * 127.0f)));
    }
};

} // namespace zenith
