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

    PianoRollTypes.h
    Extracted from PianoRollComponent.h
    
    Common types used by the Piano Roll MIDI editor.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <vector>

namespace zenith {

//==============================================================================
/**
 * @struct MidiClipContext
 * @brief Identifies which MIDI clip is being edited
 */
struct MidiClipContext {
  juce::String clipId;
  juce::String trackId;
  double clipStartBeats = 0.0;
  double clipLengthBeats = 4.0;
  juce::String clipName = "Untitled Clip";

  bool isValid() const { return clipId.isNotEmpty(); }
};

//==============================================================================
// Tool and Mode Enums
//==============================================================================

/** Available editing tools */
enum class PianoRollTool {
  Select, // Selection and manipulation of existing notes
  Draw,   // Create notes on click
  Erase,  // Delete notes on click
  Slice   // Split notes at cursor position
};

/** Velocity curve transformation types */
enum class VelocityCurve { RampUp, RampDown, Compress, Expand, Invert };

//==============================================================================
// Scale and Chord Types
//==============================================================================

enum class ScaleType {
  Chromatic,
  Major,
  Minor,
  HarmonicMinor,
  MelodicMinor,
  // Modes
  Dorian,
  Phrygian,
  Lydian,
  Mixolydian,
  Aeolian,
  Locrian,
  // Pentatonic
  MajorPentatonic,
  MinorPentatonic,
  Egyptian,
  ManGong,
  Ritusen,
  Hirajoshi,
  InSen,
  Iwato,
  YoScale,
  // Blues
  MajorBlues,
  MinorBlues,
  // Symmetrical
  WholeTone,
  DiminishedHalfWhole,
  DiminishedWholeHalf,
  Augmented,
  Prometheus,
  // Bebop
  BebopMajor,
  BebopMinor,
  BebopDominant,
  BebopDorian,
  // Exotic / World
  HungarianMinor,
  HungarianMajor,
  Bhairav,
  Byzantine,
  Persian,
  Arabian,
  Japanese,
  Chinese,
  Balinese,
  NeapolitanMajor,
  NeapolitanMinor,
  Enigmatic,
  DoubleHarmonic,
  SpanishGypsy,
  Algierian
};

enum class ChordType {
  Major,
  Minor,
  Diminished,
  Augmented,
  Major7,
  Minor7,
  Dominant7,
  Diminished7,
  Sus2,
  Sus4,
  Add9,
  Minor9,
  Power,
  Sixth,
  Minor6
};

//==============================================================================
// Groove and Pattern Types
//==============================================================================

enum class GrooveTemplate {
  Straight,  // No swing
  Swing8th,  // 8th note swing (57%)
  Swing16th, // 16th note swing (57%)
  Shuffle,   // Triplet feel (66%)
  MPC,       // MPC-style swing
  JDilla,    // J Dilla feel (slight late)
  HipHop,    // Hip hop groove
  Funk       // Funk groove
};

enum class ArpPattern {
  Up,
  Down,
  UpDown,
  DownUp,
  Random,
  Order // As played
};

enum class StrumDirection {
  Down,     // High pitch to low
  Up,       // Low pitch to high
  Alternate // Alternates based on beat
};

enum class HarmonyType {
  Thirds,  // Add 3rds
  Fifths,  // Add 5ths
  Octaves, // Add octaves
  Power,   // Root + 5th
  Triad,   // Full triad
  Drop2    // Jazz voicing (Open Tetrad)
};

//==============================================================================
// Quantize Options
//==============================================================================

struct QuantizeOptions {
  double gridSize = 0.25;    // Grid size in beats (0 = use current)
  float strength = 1.0f;     // 0.0-1.0 quantize strength
  float swingAmount = 0.0f;  // 0.0-1.0 swing amount
  bool useTriplets = false;  // Use triplet grid
  bool quantizeStart = true; // Quantize note start positions
  bool quantizeEnd = false;  // Quantize note end positions
};

//==============================================================================
// Riff Generator Settings
//==============================================================================

struct RiffSettings {
  ScaleType scale = ScaleType::Minor;
  int rootNote = 0;
  float density = 0.5f;   // Note density
  float variation = 0.3f; // Randomness
  bool useChords = false;
  bool useArps = true;
};

//==============================================================================
// MPE / Expression Types
//==============================================================================

enum class ExpressionType {
  PitchBend, // Per-note pitch bend
  Pressure,  // Aftertouch/Pressure
  Slide,     // Slide/Timbre (CC74)
  Expression // Expression pedal (CC11)
};

/** Expression automation point for a note */
struct ExpressionPoint {
  double timeOffset;    // Offset from note start (0.0 = note start)
  float value;          // 0.0 to 1.0
  float tension = 0.0f; // Bezier curve tension (-1.0 to 1.0, 0 = linear)
};

//==============================================================================
// MIDI CC Types
//==============================================================================

/** MIDI CC point (time, value) */
struct CCPoint {
  double timeBeats;
  int value;       // 0-127
  juce::String id; // Unique ID for undo/redo
};

//==============================================================================
// MIDI CC Constants
//==============================================================================

namespace MidiCC {
constexpr int MODULATION = 1;
constexpr int VOLUME = 7;
constexpr int PAN = 10;
constexpr int EXPRESSION = 11;
constexpr int SUSTAIN = 64;
constexpr int PORTAMENTO = 65;
constexpr int SOSTENUTO = 66;
constexpr int SOFT_PEDAL = 67;
constexpr int FILTER_RESONANCE = 71;
constexpr int RELEASE = 72;
constexpr int ATTACK = 73;
constexpr int CUTOFF = 74;
constexpr int REVERB = 91;
constexpr int CHORUS = 93;
} // namespace MidiCC

} // namespace zenith
