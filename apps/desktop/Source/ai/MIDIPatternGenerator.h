#pragma once

#include <juce_core/juce_core.h>
#include <vector>
#include <random>
#include <map>
#include <atomic>
#include <chrono>
#include <unistd.h>

namespace zenith {
namespace ai {

struct MIDINote {
    int pitch;           // 0-127
    double startBeats;   // Position in beats
    double lengthBeats;  // Duration in beats
    int velocity;        // 0-127
};

struct PatternSpec {
    juce::String style = "trap";      // "trap", "house", "jazz", "lofi", "rock", "edm", "rnb"
    juce::String key = "C";           // Any root: C, C#, Db, D, D#, Eb, E, F, F#, Gb, G, G#, Ab, A, A#, Bb, B
    juce::String scale = "minor";     // 50+ scales supported
    int bars = 4;
    double tempo = 120.0;
    float complexity = 0.5f;          // 0.0 - 1.0
    float swing = 0.0f;               // 0.0 - 1.0
    float humanize = 0.1f;            // Timing/velocity variation
    
    // Uniqueness control
    uint64_t seed = 0;                // 0 = auto-generate unique seed each call
    float variation = 0.5f;           // 0.0-1.0: how much to deviate from templates
    
    // Quantize control
    juce::String quantizeGrid = "1/16";   // "1/4", "1/8", "1/16", "1/32", "1/8T", "1/16T"
    float quantizeStrength = 1.0f;        // 0.0 = no quantize, 1.0 = full snap
    bool autoQuantize = true;             // Apply quantize after generation
};

class MIDIPatternGenerator {
public:
    MIDIPatternGenerator();
    ~MIDIPatternGenerator() = default;

    // Core generation methods - each call produces UNIQUE output
    std::vector<MIDINote> generateDrumPattern(const PatternSpec& spec);
    std::vector<MIDINote> generateBassLine(const PatternSpec& spec);
    std::vector<MIDINote> generateChordProgression(const PatternSpec& spec);
    std::vector<MIDINote> generateMelody(const PatternSpec& spec);
    std::vector<MIDINote> generateArpeggio(const PatternSpec& spec);

    // Utility
    static int noteNameToMIDI(const juce::String& noteName);
    static std::vector<int> getScaleNotes(const juce::String& root, const juce::String& scale);
    static std::vector<int> getScaleIntervals(const juce::String& scale);
    
    // Quantize utilities
    static double quantizePosition(double position, const juce::String& grid, float strength);
    static double getGridSize(const juce::String& grid);

private:
    std::mt19937_64 rng_;
    
    // Generate truly unique seed per call
    static uint64_t generateUniqueSeed();
    void initializeRNG(uint64_t seed);
    
    // Drum mapping (GM standard)
    static constexpr int KICK = 36;
    static constexpr int SNARE = 38;
    static constexpr int CLAP = 39;
    static constexpr int CLOSED_HAT = 42;
    static constexpr int OPEN_HAT = 46;
    static constexpr int RIMSHOT = 37;
    static constexpr int TOM_LOW = 45;
    static constexpr int TOM_MID = 47;
    static constexpr int TOM_HIGH = 50;
    static constexpr int CRASH = 49;
    static constexpr int RIDE = 51;
    static constexpr int SHAKER = 70;
    static constexpr int PERC_1 = 60;
    static constexpr int PERC_2 = 61;

    // Pattern templates with variation
    struct DrumTemplate {
        std::vector<float> kickPattern;
        std::vector<float> snarePattern;
        std::vector<float> hatPattern;
        std::vector<float> openHatPattern;
        std::vector<float> percPattern;
        float hatVelocityVariation = 0.2f;
    };

    DrumTemplate getTrapTemplate(float variation);
    DrumTemplate getHouseTemplate(float variation);
    DrumTemplate getJazzTemplate(float variation);
    DrumTemplate getLofiTemplate(float variation);
    DrumTemplate getRockTemplate(float variation);
    DrumTemplate getEDMTemplate(float variation);
    DrumTemplate getRnBTemplate(float variation);

    // Chord progressions with variation
    std::vector<std::vector<int>> getChordProgression(const juce::String& style, 
                                                       const juce::String& key,
                                                       float variation);
    
    // Generate unique motifs
    std::vector<int> generateMotif(const std::vector<int>& scaleNotes, int length);
    std::vector<int> developMotif(const std::vector<int>& motif, float variation);
    
    // Helper methods
    float applySwing(float position, float swingAmount);
    int applyHumanize(int velocity, float amount);
    double applyTimingHumanize(double position, float amount);
    void applyQuantize(std::vector<MIDINote>& notes, const PatternSpec& spec);
    
    // Variation injection
    float varyProbability(float baseProbability, float variation);
    int varyVelocity(int baseVelocity, float variation);
    
    // Scale database - 50+ scales
    static const std::map<juce::String, std::vector<int>>& getScaleDatabase();
};

} // namespace ai
} // namespace zenith
