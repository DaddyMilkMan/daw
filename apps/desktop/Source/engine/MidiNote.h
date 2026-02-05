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
