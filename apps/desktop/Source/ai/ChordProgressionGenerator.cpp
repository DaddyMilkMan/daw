/*
  ==============================================================================

    ChordProgressionGenerator.cpp
    Created: 2026-02-03
    Author:  Zenith DAW Team

    Implementation of AI-powered chord progression generator.

  ==============================================================================
*/

#include "ChordProgressionGenerator.h"
#include <algorithm>
#include <cmath>

namespace zenith {
namespace ai {

//==============================================================================
ChordProgressionGenerator::ChordProgressionGenerator()
    : rng_(static_cast<unsigned int>(std::time(nullptr)))
{
}

ChordProgressionGenerator::~ChordProgressionGenerator() = default;

//==============================================================================
GeneratedProgression ChordProgressionGenerator::generate(const ProgressionParams& params) {
    GeneratedProgression result;
    result.scale = Scale(params.rootNote, params.scaleType);
    
    // Find best matching pattern for the parameters
    ProgressionPattern pattern = findBestPattern(params);
    
    // If we have a reference progression, parse it
    if (params.referenceProgression.isNotEmpty()) {
        auto degrees = MusicTheory::parseRomanNumerals(params.referenceProgression);
        if (!degrees.empty()) {
            pattern.degrees = degrees;
            pattern.qualities.clear();
            for (int degree : degrees) {
                auto chord = result.scale.getDiatonicChord(degree);
                pattern.qualities.push_back(chord.quality);
            }
        }
    }
    
    // Adjust pattern length to match requested number of chords
    while (static_cast<int>(pattern.degrees.size()) < params.numChords) {
        // Repeat the pattern
        size_t origSize = pattern.degrees.size();
        for (size_t i = 0; i < origSize && static_cast<int>(pattern.degrees.size()) < params.numChords; ++i) {
            pattern.degrees.push_back(pattern.degrees[i]);
            if (i < pattern.qualities.size()) {
                pattern.qualities.push_back(pattern.qualities[i]);
            }
        }
    }
    
    // Generate chords from pattern
    for (int i = 0; i < params.numChords && i < static_cast<int>(pattern.degrees.size()); ++i) {
        int degree = pattern.degrees[static_cast<size_t>(i)];
        Chord chord = selectChordForDegree(degree, result.scale, params);
        
        // Override with pattern quality if available
        if (static_cast<size_t>(i) < pattern.qualities.size()) {
            chord.quality = pattern.qualities[static_cast<size_t>(i)];
        }
        
        result.chords.push_back(chord);
        result.startBeats.push_back(static_cast<double>(i) * params.beatsPerChord);
        result.durations.push_back(params.beatsPerChord);
    }
    
    // Apply voice leading and generate MIDI
    result.midiNotes = chordsToMidi(result.chords, result.startBeats, result.durations,
                                     params.voiceLeading, params.baseOctave, params.velocityBase);
    
    // Generate analysis
    result.romanNumerals = MusicTheory::toRomanNumerals(pattern.degrees, 
        std::vector<ChordQuality>(pattern.qualities.begin(), 
                                   pattern.qualities.begin() + 
                                   std::min(pattern.qualities.size(), 
                                            static_cast<size_t>(params.numChords))));
    result.analysis = analyzeProgression(result.chords, result.scale);
    result.estimatedTension = calculateProgressionTension(result.chords, result.scale);
    
    return result;
}

void ChordProgressionGenerator::generateWithAI(const ProgressionParams& params,
                                                const ProjectContext* projectContext,
                                                ProgressionCallback callback) {
    // Generate basic progression first
    auto progression = generate(params);
    
    // Build AI insight based on context
    juce::String insight;
    
    if (projectContext != nullptr) {
        auto info = projectContext->getProjectInfo();
        insight = "Generated " + progression.romanNumerals + " progression in " 
                  + progression.scale.getName() + " for " + params.genre + " style. ";
        
        if (info.tempo > 0) {
            if (info.tempo > 120) {
                insight += "At " + juce::String(info.tempo, 0) + " BPM, consider tighter voicings for clarity. ";
            } else if (info.tempo < 80) {
                insight += "The slower tempo allows for richer extensions and sustained chords. ";
            }
        }
    } else {
        insight = "This " + progression.romanNumerals + " progression works well for " 
                  + params.genre + " music. ";
    }
    
    // Add genre-specific advice
    juce::String lowerGenre = params.genre.toLowerCase();
    if (lowerGenre.contains("jazz")) {
        insight += "Consider adding extensions (9ths, 13ths) for a jazzier sound.";
    } else if (lowerGenre.contains("pop") || lowerGenre.contains("rock")) {
        insight += "This classic progression has been used in countless hits.";
    } else if (lowerGenre.contains("electronic") || lowerGenre.contains("edm")) {
        insight += "Try arpeggiating these chords or using a sidechain compressor for rhythmic interest.";
    } else if (lowerGenre.contains("hip") || lowerGenre.contains("r&b")) {
        insight += "Layer with a pad or rhodes for that smooth R&B feel.";
    }
    
    progression.aiInsight = insight;
    
    // Call callback on message thread
    juce::MessageManager::callAsync([callback, progression]() {
        callback(progression, true);
    });
}

GeneratedProgression ChordProgressionGenerator::generateFromPattern(
    const ProgressionPattern& pattern, int rootNote, double beatsPerChord) {
    
    ProgressionParams params;
    params.rootNote = rootNote;
    params.numChords = static_cast<int>(pattern.degrees.size());
    params.beatsPerChord = beatsPerChord;
    params.genre = pattern.genre;
    params.tension = pattern.tension;
    params.referenceProgression = pattern.name;
    
    GeneratedProgression result;
    result.scale = Scale(rootNote, ScaleType::Major); // Default to major
    
    for (size_t i = 0; i < pattern.degrees.size(); ++i) {
        Chord chord;
        chord.root = (rootNote + result.scale.getNoteAtDegree(pattern.degrees[i], 0) % 12 - (rootNote + 12)) % 12;
        if (chord.root < 0) chord.root += 12;
        chord.root = (rootNote + Scale::getIntervals(ScaleType::Major)[static_cast<size_t>(pattern.degrees[i] - 1)]) % 12;
        
        if (i < pattern.qualities.size()) {
            chord.quality = pattern.qualities[i];
        } else {
            chord = result.scale.getDiatonicChord(pattern.degrees[i]);
        }
        
        result.chords.push_back(chord);
        result.startBeats.push_back(static_cast<double>(i) * beatsPerChord);
        result.durations.push_back(beatsPerChord);
    }
    
    result.midiNotes = chordsToMidi(result.chords, result.startBeats, result.durations);
    result.romanNumerals = MusicTheory::toRomanNumerals(pattern.degrees, pattern.qualities);
    result.analysis = pattern.mood + " - " + pattern.name;
    result.estimatedTension = pattern.tension;
    
    return result;
}

GeneratedProgression ChordProgressionGenerator::generateRandom(
    const ProgressionParams& params, int seed) {
    
    if (seed >= 0) {
        rng_.seed(static_cast<unsigned int>(seed));
    }
    
    // Get genre-appropriate patterns
    auto patterns = MusicTheory::getProgressionsForGenre(params.genre);
    
    if (patterns.empty()) {
        patterns = MusicTheory::getAllProgressions();
    }
    
    // Randomly select a pattern
    std::uniform_int_distribution<size_t> dist(0, patterns.size() - 1);
    auto selectedPattern = patterns[dist(rng_)];
    
    // Apply some randomization
    ProgressionParams modifiedParams = params;
    
    // Occasionally substitute chords
    std::uniform_real_distribution<float> chanceDist(0.0f, 1.0f);
    if (params.complexity > 0.5f && chanceDist(rng_) < params.complexity - 0.5f) {
        modifiedParams.useSevenths = true;
    }
    
    return generateFromPattern(selectedPattern, params.rootNote, params.beatsPerChord);
}

std::vector<std::pair<Chord, float>> ChordProgressionGenerator::suggestNextChord(
    const std::vector<Chord>& currentChords,
    const Scale& scale,
    const juce::String& genre) {
    
    std::vector<std::pair<Chord, float>> suggestions;
    
    if (currentChords.empty()) {
        // Start with tonic
        suggestions.push_back({scale.getDiatonicChord(1), 1.0f});
        suggestions.push_back({scale.getDiatonicChord(6), 0.7f});
        return suggestions;
    }
    
    const Chord& lastChord = currentChords.back();
    int lastDegree = 0;
    
    // Find the scale degree of the last chord
    for (int d = 1; d <= 7; ++d) {
        if (scale.getDiatonicChord(d).root == lastChord.root) {
            lastDegree = d;
            break;
        }
    }
    
    // Common chord progressions based on last chord
    // Using functional harmony principles
    switch (lastDegree) {
        case 1: // I -> IV, V, vi, ii
            suggestions.push_back({scale.getDiatonicChord(4), 0.9f});
            suggestions.push_back({scale.getDiatonicChord(5), 0.85f});
            suggestions.push_back({scale.getDiatonicChord(6), 0.8f});
            suggestions.push_back({scale.getDiatonicChord(2), 0.6f});
            break;
            
        case 2: // ii -> V, vii
            suggestions.push_back({scale.getDiatonicChord(5), 0.95f});
            suggestions.push_back({scale.getDiatonicChord(7), 0.5f});
            break;
            
        case 3: // iii -> vi, IV
            suggestions.push_back({scale.getDiatonicChord(6), 0.85f});
            suggestions.push_back({scale.getDiatonicChord(4), 0.7f});
            break;
            
        case 4: // IV -> V, I, ii
            suggestions.push_back({scale.getDiatonicChord(5), 0.9f});
            suggestions.push_back({scale.getDiatonicChord(1), 0.85f});
            suggestions.push_back({scale.getDiatonicChord(2), 0.6f});
            break;
            
        case 5: // V -> I, vi
            suggestions.push_back({scale.getDiatonicChord(1), 0.95f});
            suggestions.push_back({scale.getDiatonicChord(6), 0.7f}); // Deceptive cadence
            break;
            
        case 6: // vi -> IV, ii, V
            suggestions.push_back({scale.getDiatonicChord(4), 0.9f});
            suggestions.push_back({scale.getDiatonicChord(2), 0.75f});
            suggestions.push_back({scale.getDiatonicChord(5), 0.7f});
            break;
            
        case 7: // vii° -> I, iii
            suggestions.push_back({scale.getDiatonicChord(1), 0.95f});
            suggestions.push_back({scale.getDiatonicChord(3), 0.6f});
            break;
            
        default:
            // Default: suggest tonic
            suggestions.push_back({scale.getDiatonicChord(1), 0.8f});
            break;
    }
    
    // Sort by confidence
    std::sort(suggestions.begin(), suggestions.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    return suggestions;
}

//==============================================================================
juce::Array<MidiNote> ChordProgressionGenerator::chordsToMidi(
    const std::vector<Chord>& chords,
    const std::vector<double>& startBeats,
    const std::vector<double>& durations,
    VoiceLeadingStyle style,
    int baseOctave,
    float velocity) {
    
    juce::Array<MidiNote> notes;
    std::vector<int> previousVoices;
    
    for (size_t i = 0; i < chords.size(); ++i) {
        double start = (i < startBeats.size()) ? startBeats[i] : static_cast<double>(i) * 4.0;
        double duration = (i < durations.size()) ? durations[i] : 4.0;
        
        // Apply voice leading
        std::vector<int> voicedNotes = applyVoiceLeading(previousVoices, chords[i], 
                                                          style, baseOctave);
        previousVoices = voicedNotes;
        
        // Create MIDI notes for each voice
        for (size_t v = 0; v < voicedNotes.size(); ++v) {
            MidiNote note;
            note.id = "chord_" + juce::String(i) + "_v" + juce::String(v);
            note.pitch = voicedNotes[v];
            note.startBeats = start;
            note.lengthBeats = duration;
            note.velocity = velocity;
            notes.add(note);
        }
    }
    
    return notes;
}

juce::String ChordProgressionGenerator::analyzeProgression(
    const std::vector<Chord>& chords, const Scale& scale) {
    
    if (chords.empty()) return "No chords to analyze";
    
    juce::String analysis;
    
    // Identify the progression pattern
    analysis += "Key: " + scale.getName() + "\n";
    analysis += "Chords: ";
    
    for (size_t i = 0; i < chords.size(); ++i) {
        if (i > 0) analysis += " - ";
        analysis += chords[i].getName();
    }
    analysis += "\n";
    
    // Analyze cadences
    if (chords.size() >= 2) {
        const Chord& secondLast = chords[chords.size() - 2];
        const Chord& last = chords.back();
        
        bool isV = (secondLast.root == (scale.getRoot() + 7) % 12);
        bool isIV = (secondLast.root == (scale.getRoot() + 5) % 12);
        bool isI = (last.root == scale.getRoot());
        bool isVI = (last.root == (scale.getRoot() + 9) % 12);
        
        if (isV && isI) {
            analysis += "Ends with: Authentic cadence (V-I) - Strong resolution\n";
        } else if (isIV && isI) {
            analysis += "Ends with: Plagal cadence (IV-I) - 'Amen' cadence\n";
        } else if (isV && isVI) {
            analysis += "Ends with: Deceptive cadence (V-vi) - Unexpected twist\n";
        }
    }
    
    // Check for common patterns
    if (chords.size() == 4) {
        // Check for I-V-vi-IV
        bool isPopProgression = 
            chords[0].root == scale.getRoot() &&
            chords[1].root == (scale.getRoot() + 7) % 12 &&
            chords[2].root == (scale.getRoot() + 9) % 12 &&
            chords[3].root == (scale.getRoot() + 5) % 12;
        
        if (isPopProgression) {
            analysis += "Pattern: Classic Pop progression (I-V-vi-IV) - Used in countless hits!\n";
        }
    }
    
    return analysis;
}

//==============================================================================
juce::StringArray ChordProgressionGenerator::getAvailableGenres() {
    return juce::StringArray{
        "Pop", "Rock", "Jazz", "Blues", "Electronic", "EDM",
        "Hip-Hop", "R&B", "Country", "Folk", "Classical", 
        "Cinematic", "Ambient", "Metal", "Funk", "Soul",
        "Reggae", "Latin", "World", "Experimental"
    };
}

std::vector<ProgressionPattern> ChordProgressionGenerator::getSuggestionsForGenre(
    const juce::String& genre) {
    return MusicTheory::getProgressionsForGenre(genre);
}

//==============================================================================
std::vector<int> ChordProgressionGenerator::applyVoiceLeading(
    const std::vector<int>& previousVoices,
    const Chord& chord,
    VoiceLeadingStyle style,
    int baseOctave) {
    
    auto rawNotes = chord.getMidiNotes(baseOctave);
    
    if (previousVoices.empty() || style == VoiceLeadingStyle::RootPosition) {
        return rawNotes;
    }
    
    std::vector<int> result;
    
    switch (style) {
        case VoiceLeadingStyle::Close:
            // Keep all notes within an octave
            for (int note : rawNotes) {
                while (note > baseOctave * 12 + 24) note -= 12;
                while (note < baseOctave * 12) note += 12;
                result.push_back(note);
            }
            break;
            
        case VoiceLeadingStyle::Open:
            // Spread notes across two octaves
            for (size_t i = 0; i < rawNotes.size(); ++i) {
                int note = rawNotes[i];
                if (i == 1 && rawNotes.size() >= 3) note += 12; // Spread middle voice
                result.push_back(note);
            }
            break;
            
        case VoiceLeadingStyle::Drop2:
            // Drop the second highest voice down an octave
            result = rawNotes;
            if (result.size() >= 4) {
                std::sort(result.begin(), result.end());
                result[result.size() - 2] -= 12;
            }
            break;
            
        case VoiceLeadingStyle::Drop3:
            // Drop the third highest voice down an octave
            result = rawNotes;
            if (result.size() >= 4) {
                std::sort(result.begin(), result.end());
                if (result.size() >= 3) {
                    result[result.size() - 3] -= 12;
                }
            }
            break;
            
        case VoiceLeadingStyle::Smooth:
        default:
            // Minimize voice movement from previous chord
            std::vector<int> available = rawNotes;
            // Add octave variants
            for (int note : rawNotes) {
                if (note + 12 <= 127) available.push_back(note + 12);
                if (note - 12 >= 0) available.push_back(note - 12);
            }
            
            for (int prev : previousVoices) {
                int closest = findClosestVoice(prev, available);
                result.push_back(closest);
                // Remove used note to avoid duplicates
                available.erase(std::remove(available.begin(), available.end(), closest), 
                               available.end());
            }
            
            // Add any remaining voices if we have fewer than before
            while (result.size() < rawNotes.size() && !available.empty()) {
                result.push_back(available.front());
                available.erase(available.begin());
            }
            break;
    }
    
    // Sort result for consistent voicing
    std::sort(result.begin(), result.end());
    
    return result;
}

int ChordProgressionGenerator::findClosestVoice(int target, const std::vector<int>& available) {
    if (available.empty()) return target;
    
    int closest = available[0];
    int minDistance = std::abs(target - closest);
    
    for (int note : available) {
        int distance = std::abs(target - note);
        if (distance < minDistance) {
            minDistance = distance;
            closest = note;
        }
    }
    
    return closest;
}

float ChordProgressionGenerator::calculateChordTension(const Chord& chord, const Scale& scale) {
    float tension = 0.0f;
    
    // Diminished chords have high tension
    if (chord.quality == ChordQuality::Diminished || 
        chord.quality == ChordQuality::Diminished7) {
        tension += 0.8f;
    }
    // Dominant 7ths create moderate tension
    else if (chord.quality == ChordQuality::Dominant7 ||
             chord.quality == ChordQuality::Dominant9) {
        tension += 0.5f;
    }
    // Minor chords have slight tension
    else if (chord.quality == ChordQuality::Minor ||
             chord.quality == ChordQuality::Minor7) {
        tension += 0.3f;
    }
    // Augmented chords are tense
    else if (chord.quality == ChordQuality::Augmented) {
        tension += 0.7f;
    }
    
    // Non-diatonic chords add tension
    bool isDiatonic = false;
    for (int d = 1; d <= 7; ++d) {
        if (scale.getDiatonicChord(d).root == chord.root) {
            isDiatonic = true;
            break;
        }
    }
    if (!isDiatonic) tension += 0.2f;
    
    return juce::jlimit(0.0f, 1.0f, tension);
}

float ChordProgressionGenerator::calculateProgressionTension(
    const std::vector<Chord>& chords, const Scale& scale) {
    
    if (chords.empty()) return 0.0f;
    
    float totalTension = 0.0f;
    for (const auto& chord : chords) {
        totalTension += calculateChordTension(chord, scale);
    }
    
    return totalTension / static_cast<float>(chords.size());
}

ProgressionPattern ChordProgressionGenerator::findBestPattern(const ProgressionParams& params) {
    auto patterns = MusicTheory::getProgressionsForGenre(params.genre);
    
    if (patterns.empty()) {
        // Return default I-IV-V-I
        return {
            "Basic I-IV-V-I", {1, 4, 5, 1},
            {ChordQuality::Major, ChordQuality::Major, ChordQuality::Major, ChordQuality::Major},
            "Universal", 0.3f, "Classic, resolved"
        };
    }
    
    // Find pattern closest to desired tension
    ProgressionPattern best = patterns[0];
    float bestDiff = std::abs(patterns[0].tension - params.tension);
    
    for (const auto& pattern : patterns) {
        float diff = std::abs(pattern.tension - params.tension);
        if (diff < bestDiff) {
            bestDiff = diff;
            best = pattern;
        }
    }
    
    return best;
}

Chord ChordProgressionGenerator::selectChordForDegree(int degree, const Scale& scale,
                                                       const ProgressionParams& params) {
    Chord chord = scale.getDiatonicChord(degree);
    
    // Apply complexity settings
    if (params.useSevenths) {
        switch (chord.quality) {
            case ChordQuality::Major:
                chord.quality = ChordQuality::Major7;
                break;
            case ChordQuality::Minor:
                chord.quality = ChordQuality::Minor7;
                break;
            case ChordQuality::Diminished:
                chord.quality = ChordQuality::HalfDiminished7;
                break;
            default:
                break;
        }
    }
    
    if (params.useExtensions && params.complexity > 0.7f) {
        // Add 9th
        chord.extensions.push_back(14); // 9th = 14 semitones
    }
    
    return chord;
}

} // namespace ai
} // namespace zenith
