/*
  ==============================================================================

    MusicTheory.h
    Created: 2026-02-03
    Author:  Zenith DAW Team

    Comprehensive music theory utilities for AI-powered composition assistance.
    Provides scales, chord types, intervals, and music theory calculations.

    Competitive Feature: Matches and exceeds Logic Pro 11's Chord Track,
    Ableton Live 12's scale awareness, and FL Studio's scale highlighting.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <array>
#include <vector>
#include <unordered_map>

namespace zenith {
namespace ai {

//==============================================================================
/**
    Musical interval definitions (in semitones)
*/
enum class Interval : int {
    Unison = 0,
    MinorSecond = 1,
    MajorSecond = 2,
    MinorThird = 3,
    MajorThird = 4,
    PerfectFourth = 5,
    Tritone = 6,
    PerfectFifth = 7,
    MinorSixth = 8,
    MajorSixth = 9,
    MinorSeventh = 10,
    MajorSeventh = 11,
    Octave = 12
};

//==============================================================================
/**
    Note names for display and parsing
*/
struct NoteName {
    static constexpr std::array<const char*, 12> names = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };
    
    static constexpr std::array<const char*, 12> flatNames = {
        "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"
    };
    
    static juce::String get(int midiNote, bool useFlats = false) {
        int noteIndex = midiNote % 12;
        int octave = (midiNote / 12) - 1;
        return juce::String(useFlats ? flatNames[static_cast<size_t>(noteIndex)] 
                                      : names[static_cast<size_t>(noteIndex)]) 
               + juce::String(octave);
    }
    
    static int parse(const juce::String& noteName);
};

//==============================================================================
/**
    Scale types with their interval patterns
*/
enum class ScaleType {
    Major,
    NaturalMinor,
    HarmonicMinor,
    MelodicMinor,
    Dorian,
    Phrygian,
    Lydian,
    Mixolydian,
    Locrian,
    MajorPentatonic,
    MinorPentatonic,
    Blues,
    WholeTone,
    Chromatic
};

//==============================================================================
/**
    Chord quality types
*/
enum class ChordQuality {
    Major,
    Minor,
    Diminished,
    Augmented,
    Major7,
    Minor7,
    Dominant7,
    Diminished7,
    HalfDiminished7,
    MinorMajor7,
    Augmented7,
    Sus2,
    Sus4,
    Add9,
    Major9,
    Minor9,
    Dominant9,
    Power5
};

//==============================================================================
/**
    Represents a chord with root, quality, and optional bass note
*/
struct Chord {
    int root = 0;                       ///< Root note (0-11, C=0)
    ChordQuality quality = ChordQuality::Major;
    int bassNote = -1;                  ///< Bass note for slash chords (-1 = root)
    std::vector<int> extensions;        ///< Additional extensions (9, 11, 13)
    
    /** Get all MIDI notes for this chord at a given octave */
    std::vector<int> getMidiNotes(int octave = 4) const;
    
    /** Get chord name for display */
    juce::String getName(bool useFlats = false) const;
    
    /** Get intervals that define this chord quality */
    static std::vector<int> getIntervalsForQuality(ChordQuality q);
};

//==============================================================================
/**
    Scale representation with utilities for note generation
*/
class Scale {
public:
    Scale(int root = 0, ScaleType type = ScaleType::Major);
    
    /** Get all notes in this scale within a MIDI range */
    std::vector<int> getNotesInRange(int lowNote, int highNote) const;
    
    /** Check if a MIDI note is in this scale */
    bool containsNote(int midiNote) const;
    
    /** Get the scale degree (1-7) of a note, or 0 if not in scale */
    int getScaleDegree(int midiNote) const;
    
    /** Get the note at a specific scale degree */
    int getNoteAtDegree(int degree, int octave = 4) const;
    
    /** Get the diatonic chord at a scale degree */
    Chord getDiatonicChord(int degree) const;
    
    /** Get all diatonic triads in this scale */
    std::vector<Chord> getDiatonicTriads() const;
    
    /** Get all diatonic 7th chords in this scale */
    std::vector<Chord> getDiatonicSeventhChords() const;
    
    /** Get scale name for display */
    juce::String getName(bool useFlats = false) const;
    
    /** Get the interval pattern for a scale type */
    static std::vector<int> getIntervals(ScaleType type);
    
    int getRoot() const { return root_; }
    ScaleType getType() const { return type_; }

private:
    int root_;                          ///< Root note (0-11, C=0)
    ScaleType type_;
    std::vector<int> intervals_;        ///< Cached intervals
};

//==============================================================================
/**
    Common chord progression patterns used across genres
*/
struct ProgressionPattern {
    juce::String name;                  ///< Display name (e.g., "Pop I-V-vi-IV")
    std::vector<int> degrees;           ///< Scale degrees (1-based)
    std::vector<ChordQuality> qualities;///< Chord quality for each degree
    juce::String genre;                 ///< Primary genre association
    float tension;                      ///< Tension level (0.0 = calm, 1.0 = intense)
    juce::String mood;                  ///< Mood description
};

//==============================================================================
/**
    Music theory calculation utilities
*/
class MusicTheory {
public:
    //==========================================================================
    /** Get the interval between two notes in semitones */
    static int getInterval(int fromNote, int toNote);
    
    /** Transpose a note by a number of semitones */
    static int transpose(int midiNote, int semitones);
    
    /** Get the relative minor of a major key (or vice versa) */
    static int getRelativeKey(int root, bool toMinor);
    
    /** Get the parallel minor/major of a key */
    static int getParallelKey(int root);
    
    /** Quantize a note to the nearest note in a scale */
    static int quantizeToScale(int midiNote, const Scale& scale);
    
    //==========================================================================
    /** Get common chord progressions for a genre */
    static std::vector<ProgressionPattern> getProgressionsForGenre(const juce::String& genre);
    
    /** Get all available progression patterns */
    static std::vector<ProgressionPattern> getAllProgressions();
    
    /** Convert Roman numeral notation to scale degrees */
    static std::vector<int> parseRomanNumerals(const juce::String& notation);
    
    /** Convert scale degrees to Roman numeral notation */
    static juce::String toRomanNumerals(const std::vector<int>& degrees, 
                                         const std::vector<ChordQuality>& qualities);

private:
    static void initializeProgressions();
    static std::vector<ProgressionPattern> progressions_;
    static bool progressionsInitialized_;
};

} // namespace ai
} // namespace zenith
