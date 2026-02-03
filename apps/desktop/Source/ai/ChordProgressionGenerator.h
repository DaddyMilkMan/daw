/*
  ==============================================================================

    ChordProgressionGenerator.h
    Created: 2026-02-03
    Author:  Zenith DAW Team

    AI-powered chord progression generator for creative music composition.
    
    Competitive Feature: Combines the best of Logic Pro 11's Chord Track,
    Ableton Live 12's generative tools, and FL Studio's harmonic assistance
    with AI-powered suggestions via Grok integration.

    Key Features:
    - Genre-aware chord progression generation
    - AI-powered creative suggestions (via Grok API)
    - Music theory-based progression analysis
    - MIDI output compatible with Zenith's clip system
    - Tension/resolution curve control
    - Voice leading optimization

  ==============================================================================
*/

#pragma once

#include "MusicTheory.h"
#include "GenreDetector.h"
#include "ProjectContext.h"
#include "../engine/MidiNote.h"
#include <juce_core/juce_core.h>
#include <functional>
#include <memory>
#include <random>

namespace zenith {
namespace ai {

//==============================================================================
/**
    Voice leading preferences for chord voicings
*/
enum class VoiceLeadingStyle {
    Close,          ///< Close position voicings
    Open,           ///< Open/spread voicings
    Drop2,          ///< Drop 2 jazz voicings
    Drop3,          ///< Drop 3 voicings
    RootPosition,   ///< Root always in bass
    Smooth          ///< Minimize voice movement
};

//==============================================================================
/**
    Parameters for generating a chord progression
*/
struct ProgressionParams {
    int rootNote = 0;                           ///< Key root (0-11, C=0)
    ScaleType scaleType = ScaleType::Major;     ///< Scale/mode
    int numChords = 4;                          ///< Number of chords to generate
    double beatsPerChord = 4.0;                 ///< Duration of each chord
    juce::String genre = "Pop";                 ///< Target genre
    float tension = 0.5f;                       ///< Tension level (0.0-1.0)
    float complexity = 0.5f;                    ///< Harmonic complexity (0.0-1.0)
    VoiceLeadingStyle voiceLeading = VoiceLeadingStyle::Smooth;
    int baseOctave = 3;                         ///< Base octave for voicings
    float velocityBase = 0.7f;                  ///< Base velocity (0.0-1.0)
    bool useSevenths = false;                   ///< Use 7th chords when appropriate
    bool useExtensions = false;                 ///< Use 9ths, 11ths, 13ths
    juce::String mood = "";                     ///< Optional mood hint
    juce::String referenceProgression = "";     ///< Optional reference (e.g., "I-V-vi-IV")
};

//==============================================================================
/**
    Result of chord progression generation
*/
struct GeneratedProgression {
    std::vector<Chord> chords;                  ///< Generated chords
    std::vector<double> startBeats;             ///< Start time for each chord
    std::vector<double> durations;              ///< Duration of each chord
    juce::Array<MidiNote> midiNotes;            ///< MIDI notes for playback
    Scale scale;                                ///< Scale used for generation
    juce::String analysis;                      ///< Music theory analysis
    juce::String romanNumerals;                 ///< Roman numeral representation
    float estimatedTension = 0.0f;              ///< Calculated tension curve
    juce::String aiInsight;                     ///< AI-generated insight (if available)
};

//==============================================================================
/**
    Callback for AI-enhanced generation completion
*/
using ProgressionCallback = std::function<void(const GeneratedProgression&, bool success)>;

//==============================================================================
/**
    AI-powered chord progression generator
    
    Provides intelligent chord progression generation with genre awareness,
    music theory analysis, and optional AI enhancement via Grok API.
    
    Usage:
    @code
    ChordProgressionGenerator generator;
    
    ProgressionParams params;
    params.rootNote = 0;  // C
    params.scaleType = ScaleType::Major;
    params.genre = "Pop";
    params.numChords = 4;
    
    auto progression = generator.generate(params);
    
    // Use the MIDI notes directly
    for (const auto& note : progression.midiNotes) {
        // Add to clip...
    }
    @endcode
*/
class ChordProgressionGenerator {
public:
    //==========================================================================
    ChordProgressionGenerator();
    ~ChordProgressionGenerator();

    //==========================================================================
    /**
     * @brief Generate a chord progression based on parameters
     * @param params Generation parameters
     * @return Generated progression with MIDI notes
     */
    GeneratedProgression generate(const ProgressionParams& params);

    /**
     * @brief Generate with AI enhancement (async)
     * @param params Generation parameters
     * @param projectContext Optional project context for better suggestions
     * @param callback Completion callback
     * 
     * This method generates a basic progression immediately, then
     * optionally enhances it with AI suggestions.
     */
    void generateWithAI(const ProgressionParams& params,
                        const ProjectContext* projectContext,
                        ProgressionCallback callback);

    //==========================================================================
    /**
     * @brief Generate a progression based on an existing pattern
     * @param pattern Pattern to use (e.g., from MusicTheory::getAllProgressions)
     * @param rootNote Key root
     * @param beatsPerChord Duration per chord
     * @return Generated progression
     */
    GeneratedProgression generateFromPattern(const ProgressionPattern& pattern,
                                              int rootNote,
                                              double beatsPerChord = 4.0);

    /**
     * @brief Generate a random progression within genre constraints
     * @param params Parameters with genre hint
     * @param seed Random seed (-1 for random)
     * @return Generated progression
     */
    GeneratedProgression generateRandom(const ProgressionParams& params, 
                                         int seed = -1);

    /**
     * @brief Suggest the next chord based on current progression
     * @param currentChords Chords played so far
     * @param scale Scale context
     * @param genre Genre hint
     * @return Vector of suggested next chords with confidence scores
     */
    std::vector<std::pair<Chord, float>> suggestNextChord(
        const std::vector<Chord>& currentChords,
        const Scale& scale,
        const juce::String& genre);

    //==========================================================================
    /**
     * @brief Convert chords to MIDI notes with voice leading
     * @param chords Chords to voice
     * @param startBeats Start time for each chord
     * @param durations Duration of each chord
     * @param style Voice leading style
     * @param baseOctave Base octave
     * @param velocity Base velocity
     * @return Array of MIDI notes
     */
    juce::Array<MidiNote> chordsToMidi(
        const std::vector<Chord>& chords,
        const std::vector<double>& startBeats,
        const std::vector<double>& durations,
        VoiceLeadingStyle style = VoiceLeadingStyle::Smooth,
        int baseOctave = 3,
        float velocity = 0.7f);

    /**
     * @brief Analyze a chord progression
     * @param chords Chords to analyze
     * @param scale Scale context
     * @return Analysis string
     */
    juce::String analyzeProgression(const std::vector<Chord>& chords,
                                     const Scale& scale);

    //==========================================================================
    /**
     * @brief Get available genres for generation
     * @return List of supported genres
     */
    static juce::StringArray getAvailableGenres();

    /**
     * @brief Get suggested progressions for a genre
     * @param genre Genre name
     * @return Available progression patterns
     */
    static std::vector<ProgressionPattern> getSuggestionsForGenre(
        const juce::String& genre);

private:
    //==========================================================================
    std::mt19937 rng_;
    
    // Voice leading helpers
    std::vector<int> applyVoiceLeading(const std::vector<int>& previousVoices,
                                        const Chord& chord,
                                        VoiceLeadingStyle style,
                                        int baseOctave);
    
    int findClosestVoice(int target, const std::vector<int>& available);
    
    // Tension calculation
    float calculateChordTension(const Chord& chord, const Scale& scale);
    float calculateProgressionTension(const std::vector<Chord>& chords,
                                       const Scale& scale);
    
    // Pattern matching
    ProgressionPattern findBestPattern(const ProgressionParams& params);
    
    // Chord selection
    Chord selectChordForDegree(int degree, const Scale& scale,
                                const ProgressionParams& params);
    
    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordProgressionGenerator)
};

} // namespace ai
} // namespace zenith
