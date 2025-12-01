/*
  ==============================================================================

    AiChordAnalyzer.h
    Created: 2025-11-30
    Authors: Kenji Nakamura (Music Theory Lead)

    FEATURE 2: Chord Progression Analyzer
    Detects chords from MIDI note data using music theory.

  ==============================================================================
*/

#pragma once

#include "AiDataStructures.h"
#include <juce_core/juce_core.h>
#include <vector>
#include <map>
#include <set>

namespace zenith {
namespace ai {

class ChordAnalyzer {
public:
    /**
     * Analyzes MIDI notes and detects chord progression.
     * Uses a sliding window to detect simultaneous notes.
     */
    static ChordProgression analyzeChords(const std::vector<AiMidiNote>& notes, double windowSize = 0.5) {
        ChordProgression progression;
        
        if (notes.empty()) {
            return progression;
        }
        
        // Sort notes by start time
        auto sortedNotes = notes;
        std::sort(sortedNotes.begin(), sortedNotes.end(), 
            [](const AiMidiNote& a, const AiMidiNote& b) {
                return a.startBeat < b.startBeat;
            });
        
        // Find the time range
        double maxTime = 0.0;
        for (const auto& note : sortedNotes) {
            maxTime = std::max(maxTime, note.startBeat + note.duration);
        }
        
        // Slide through time in windows
        for (double time = 0.0; time < maxTime; time += windowSize) {
            std::set<int> activeNotes;
            
            // Find all notes active in this window
            for (const auto& note : sortedNotes) {
                if (note.startBeat <= time && (note.startBeat + note.duration) > time) {
                    activeNotes.insert(note.pitch % 12); // Reduce to pitch class
                }
            }
            
            // Need at least 3 notes for a chord
            if (activeNotes.size() >= 3) {
                ChordInfo chord = detectChord(activeNotes, time, windowSize);
                
                // Merge with previous chord if it's the same
                if (!progression.chords.empty() && 
                    progression.chords.back().name == chord.name) {
                    progression.chords.back().duration += windowSize;
                } else {
                    progression.chords.push_back(chord);
                }
            }
        }
        
        return progression;
    }

private:
    static ChordInfo detectChord(const std::set<int>& pitchClasses, double startBeat, double duration) {
        ChordInfo chord;
        chord.startBeat = startBeat;
        chord.duration = duration;
        
        // Convert set to vector for easier indexing
        std::vector<int> pcs(pitchClasses.begin(), pitchClasses.end());
        chord.notes = pcs;
        
        // Find root (lowest note)
        int root = *std::min_element(pcs.begin(), pcs.end());
        
        // Calculate intervals from root
        std::set<int> intervals;
        for (int pc : pcs) {
            intervals.insert((pc - root + 12) % 12);
        }
        
        // Detect chord type based on intervals
        chord.name = getRootName(root) + getChordQuality(intervals);
        
        return chord;
    }
    
    static juce::String getRootName(int pitchClass) {
        static const char* noteNames[] = {
            "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
        };
        return noteNames[pitchClass];
    }
    
    static juce::String getChordQuality(const std::set<int>& intervals) {
        // Major triad: 0, 4, 7
        if (intervals == std::set<int>{0, 4, 7}) {
            return "maj";
        }
        // Minor triad: 0, 3, 7
        if (intervals == std::set<int>{0, 3, 7}) {
            return "min";
        }
        // Dominant 7th: 0, 4, 7, 10
        if (intervals == std::set<int>{0, 4, 7, 10}) {
            return "7";
        }
        // Major 7th: 0, 4, 7, 11
        if (intervals == std::set<int>{0, 4, 7, 11}) {
            return "maj7";
        }
        // Minor 7th: 0, 3, 7, 10
        if (intervals == std::set<int>{0, 3, 7, 10}) {
            return "min7";
        }
        // Diminished: 0, 3, 6
        if (intervals == std::set<int>{0, 3, 6}) {
            return "dim";
        }
        // Augmented: 0, 4, 8
        if (intervals == std::set<int>{0, 4, 8}) {
            return "aug";
        }
        // Sus4: 0, 5, 7
        if (intervals == std::set<int>{0, 5, 7}) {
            return "sus4";
        }
        // Sus2: 0, 2, 7
        if (intervals == std::set<int>{0, 2, 7}) {
            return "sus2";
        }
        
        // Default: just list the intervals
        return "?";
    }
};

} // namespace ai
} // namespace zenith
