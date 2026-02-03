/*
  ==============================================================================

    MusicTheoryTests.cpp
    Created: 2026-02-03
    Author:  Zenith DAW Team

    Unit tests for MusicTheory and ChordProgressionGenerator classes.

  ==============================================================================
*/

#include "../ai/MusicTheory.h"
#include "../ai/ChordProgressionGenerator.h"
#include <juce_core/juce_core.h>

namespace zenith {
namespace tests {

/**
 * @class MusicTheoryTests
 * @brief Tests for music theory utilities and chord progression generation
 */
class MusicTheoryTests : public juce::UnitTest {
public:
    MusicTheoryTests() : juce::UnitTest("Music Theory & Chord Generation", "AI") {}

    void runTest() override {
        
        beginTest("NoteName Parsing");
        {
            // Test note name parsing
            expectEquals(ai::NoteName::parse("C4"), 60);
            expectEquals(ai::NoteName::parse("A4"), 69);
            expectEquals(ai::NoteName::parse("C#4"), 61);
            expectEquals(ai::NoteName::parse("Db4"), 61);
            expectEquals(ai::NoteName::parse("C0"), 12);
            expectEquals(ai::NoteName::parse("C-1"), 0);
            
            // Test note name generation
            expectEquals(ai::NoteName::get(60), juce::String("C4"));
            expectEquals(ai::NoteName::get(69), juce::String("A4"));
            expectEquals(ai::NoteName::get(61, false), juce::String("C#4"));
            expectEquals(ai::NoteName::get(61, true), juce::String("Db4"));
        }

        beginTest("Scale Construction");
        {
            // C Major scale
            ai::Scale cMajor(0, ai::ScaleType::Major);
            expect(cMajor.containsNote(60));  // C
            expect(cMajor.containsNote(62));  // D
            expect(cMajor.containsNote(64));  // E
            expect(cMajor.containsNote(65));  // F
            expect(cMajor.containsNote(67));  // G
            expect(cMajor.containsNote(69));  // A
            expect(cMajor.containsNote(71));  // B
            expect(!cMajor.containsNote(61)); // C# not in C major
            
            // A Minor scale
            ai::Scale aMinor(9, ai::ScaleType::NaturalMinor);
            expect(aMinor.containsNote(69));  // A
            expect(aMinor.containsNote(71));  // B
            expect(aMinor.containsNote(72));  // C
            expect(!aMinor.containsNote(73)); // C# not in A minor
        }

        beginTest("Scale Degrees");
        {
            ai::Scale cMajor(0, ai::ScaleType::Major);
            
            // C is the 1st degree
            expectEquals(cMajor.getScaleDegree(60), 1);
            // D is the 2nd degree
            expectEquals(cMajor.getScaleDegree(62), 2);
            // G is the 5th degree
            expectEquals(cMajor.getScaleDegree(67), 5);
            // C# is not in the scale
            expectEquals(cMajor.getScaleDegree(61), 0);
            
            // Get note at degree
            expectEquals(cMajor.getNoteAtDegree(1, 4), 60);  // C4
            expectEquals(cMajor.getNoteAtDegree(5, 4), 67);  // G4
        }

        beginTest("Chord Construction");
        {
            // C Major chord
            ai::Chord cMaj;
            cMaj.root = 0;
            cMaj.quality = ai::ChordQuality::Major;
            
            auto notes = cMaj.getMidiNotes(4);
            expectEquals(static_cast<int>(notes.size()), 3);
            expectEquals(notes[0], 60);  // C4
            expectEquals(notes[1], 64);  // E4
            expectEquals(notes[2], 67);  // G4
            
            expectEquals(cMaj.getName(), juce::String("C"));
            
            // A minor 7 chord
            ai::Chord am7;
            am7.root = 9;
            am7.quality = ai::ChordQuality::Minor7;
            
            auto am7Notes = am7.getMidiNotes(4);
            expectEquals(static_cast<int>(am7Notes.size()), 4);
            expectEquals(am7Notes[0], 69);  // A4
            expectEquals(am7Notes[1], 72);  // C5
            expectEquals(am7Notes[2], 76);  // E5
            expectEquals(am7Notes[3], 79);  // G5
            
            expectEquals(am7.getName(), juce::String("Am7"));
        }

        beginTest("Diatonic Chords");
        {
            ai::Scale cMajor(0, ai::ScaleType::Major);
            
            // I chord (C major)
            auto chord1 = cMajor.getDiatonicChord(1);
            expectEquals(chord1.root, 0);
            expect(chord1.quality == ai::ChordQuality::Major);
            
            // ii chord (D minor)
            auto chord2 = cMajor.getDiatonicChord(2);
            expectEquals(chord2.root, 2);
            expect(chord2.quality == ai::ChordQuality::Minor);
            
            // V chord (G major)
            auto chord5 = cMajor.getDiatonicChord(5);
            expectEquals(chord5.root, 7);
            expect(chord5.quality == ai::ChordQuality::Major);
            
            // vii chord (B diminished)
            auto chord7 = cMajor.getDiatonicChord(7);
            expectEquals(chord7.root, 11);
            expect(chord7.quality == ai::ChordQuality::Diminished);
        }

        beginTest("MusicTheory Utilities");
        {
            // Interval calculation
            expectEquals(ai::MusicTheory::getInterval(60, 67), 7);  // C to G = P5
            expectEquals(ai::MusicTheory::getInterval(60, 64), 4);  // C to E = M3
            
            // Transposition
            expectEquals(ai::MusicTheory::transpose(60, 7), 67);  // C + P5 = G
            expectEquals(ai::MusicTheory::transpose(60, -3), 57); // C - m3 = A
            
            // Relative keys
            expectEquals(ai::MusicTheory::getRelativeKey(0, true), 9);   // C major -> A minor
            expectEquals(ai::MusicTheory::getRelativeKey(9, false), 0);  // A minor -> C major
            
            // Scale quantization
            ai::Scale cMajor(0, ai::ScaleType::Major);
            expectEquals(ai::MusicTheory::quantizeToScale(61, cMajor), 60);  // C# -> C
            expectEquals(ai::MusicTheory::quantizeToScale(66, cMajor), 67);  // F# -> G
        }

        beginTest("Progression Patterns");
        {
            auto patterns = ai::MusicTheory::getAllProgressions();
            expect(!patterns.empty());
            
            // Find Pop pattern
            bool foundPopPattern = false;
            for (const auto& pattern : patterns) {
                if (pattern.name.contains("Pop") && pattern.degrees.size() == 4) {
                    foundPopPattern = true;
                    // I-V-vi-IV should have degrees 1, 5, 6, 4
                    if (pattern.name.contains("I-V-vi-IV")) {
                        expectEquals(pattern.degrees[0], 1);
                        expectEquals(pattern.degrees[1], 5);
                        expectEquals(pattern.degrees[2], 6);
                        expectEquals(pattern.degrees[3], 4);
                    }
                }
            }
            expect(foundPopPattern);
            
            // Get patterns for genre
            auto jazzPatterns = ai::MusicTheory::getProgressionsForGenre("Jazz");
            expect(!jazzPatterns.empty());
        }

        beginTest("Roman Numeral Parsing");
        {
            auto degrees = ai::MusicTheory::parseRomanNumerals("I-V-vi-IV");
            expectEquals(static_cast<int>(degrees.size()), 4);
            expectEquals(degrees[0], 1);
            expectEquals(degrees[1], 5);
            expectEquals(degrees[2], 6);
            expectEquals(degrees[3], 4);
            
            // Jazz progression
            auto jazzDegrees = ai::MusicTheory::parseRomanNumerals("ii-V-I");
            expectEquals(static_cast<int>(jazzDegrees.size()), 3);
            expectEquals(jazzDegrees[0], 2);
            expectEquals(jazzDegrees[1], 5);
            expectEquals(jazzDegrees[2], 1);
        }

        beginTest("ChordProgressionGenerator Basic Generation");
        {
            ai::ChordProgressionGenerator generator;
            
            ai::ProgressionParams params;
            params.rootNote = 0;  // C
            params.scaleType = ai::ScaleType::Major;
            params.numChords = 4;
            params.beatsPerChord = 4.0;
            params.genre = "Pop";
            
            auto progression = generator.generate(params);
            
            // Should have 4 chords
            expectEquals(static_cast<int>(progression.chords.size()), 4);
            
            // Should have MIDI notes
            expect(progression.midiNotes.size() > 0);
            
            // Should have analysis
            expect(progression.analysis.isNotEmpty());
            
            // Should have roman numerals
            expect(progression.romanNumerals.isNotEmpty());
            
            // Check timing
            expectEquals(progression.startBeats.size(), progression.chords.size());
            expectEquals(progression.durations.size(), progression.chords.size());
            expectEquals(progression.startBeats[0], 0.0);
            expectEquals(progression.startBeats[1], 4.0);
        }

        beginTest("ChordProgressionGenerator Pattern Generation");
        {
            ai::ChordProgressionGenerator generator;
            
            // Get a known pattern
            auto patterns = ai::MusicTheory::getProgressionsForGenre("Pop");
            expect(!patterns.empty());
            
            auto progression = generator.generateFromPattern(patterns[0], 0, 4.0);
            
            expect(!progression.chords.empty());
            expect(progression.midiNotes.size() > 0);
        }

        beginTest("ChordProgressionGenerator Next Chord Suggestions");
        {
            ai::ChordProgressionGenerator generator;
            ai::Scale cMajor(0, ai::ScaleType::Major);
            
            // After V chord, I should be the top suggestion
            std::vector<ai::Chord> currentChords;
            ai::Chord vChord;
            vChord.root = 7;  // G
            vChord.quality = ai::ChordQuality::Major;
            currentChords.push_back(vChord);
            
            auto suggestions = generator.suggestNextChord(currentChords, cMajor, "Pop");
            
            expect(!suggestions.empty());
            // I chord (C) should be a top suggestion after V
            bool foundTonic = false;
            for (const auto& [chord, confidence] : suggestions) {
                if (chord.root == 0 && chord.quality == ai::ChordQuality::Major) {
                    foundTonic = true;
                    expect(confidence > 0.8f);  // Should have high confidence
                    break;
                }
            }
            expect(foundTonic);
        }

        beginTest("ChordProgressionGenerator Voice Leading");
        {
            ai::ChordProgressionGenerator generator;
            
            std::vector<ai::Chord> chords;
            
            // C major
            ai::Chord c;
            c.root = 0;
            c.quality = ai::ChordQuality::Major;
            chords.push_back(c);
            
            // G major
            ai::Chord g;
            g.root = 7;
            g.quality = ai::ChordQuality::Major;
            chords.push_back(g);
            
            std::vector<double> starts = {0.0, 4.0};
            std::vector<double> durations = {4.0, 4.0};
            
            auto notes = generator.chordsToMidi(chords, starts, durations,
                                                 ai::VoiceLeadingStyle::Smooth, 4, 0.8f);
            
            // Should have 6 notes (3 per chord)
            expectEquals(notes.size(), 6);
            
            // Check note properties
            for (const auto& note : notes) {
                expect(note.pitch >= 48 && note.pitch <= 84);  // Reasonable range
                expectEquals(note.velocity, 0.8f);
                expect(note.lengthBeats == 4.0);
            }
        }

        beginTest("Available Genres");
        {
            auto genres = ai::ChordProgressionGenerator::getAvailableGenres();
            expect(genres.size() > 0);
            expect(genres.contains("Pop"));
            expect(genres.contains("Jazz"));
            expect(genres.contains("Rock"));
            expect(genres.contains("Electronic"));
        }

        beginTest("Random Generation Reproducibility");
        {
            ai::ChordProgressionGenerator gen1;
            ai::ChordProgressionGenerator gen2;
            
            ai::ProgressionParams params;
            params.rootNote = 0;
            params.numChords = 4;
            params.genre = "Jazz";
            
            // With same seed, should get same result
            auto prog1 = gen1.generateRandom(params, 42);
            auto prog2 = gen2.generateRandom(params, 42);
            
            expectEquals(prog1.chords.size(), prog2.chords.size());
            
            // Chords should match
            for (size_t i = 0; i < prog1.chords.size(); ++i) {
                expectEquals(prog1.chords[i].root, prog2.chords[i].root);
            }
        }
    }
};

static MusicTheoryTests musicTheoryTests;

} // namespace tests
} // namespace zenith
