#include "MIDIPatternGenerator.h"
#include <cmath>
#include <algorithm>

namespace zenith {
namespace ai {

//==============================================================================
// Static counter for unique seed generation
//==============================================================================
static std::atomic<uint64_t> s_seedCounter{0};

//==============================================================================
// Constructor
//==============================================================================

MIDIPatternGenerator::MIDIPatternGenerator() {
    initializeRNG(generateUniqueSeed());
}

//==============================================================================
// Unique Seed Generation - CRITICAL FOR UNIQUENESS
//==============================================================================

uint64_t MIDIPatternGenerator::generateUniqueSeed() {
    std::random_device rd;
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    uint64_t pid = static_cast<uint64_t>(getpid());
    uint64_t counter = ++s_seedCounter;
    
    // Combine multiple entropy sources
    uint64_t seed = rd();
    seed ^= (now << 16);
    seed ^= (pid << 32);
    seed ^= (counter << 48);
    seed ^= rd(); // Extra entropy
    
    return seed;
}

void MIDIPatternGenerator::initializeRNG(uint64_t seed) {
    if (seed == 0) {
        seed = generateUniqueSeed();
    }
    rng_.seed(seed);
    // Warm up the RNG
    for (int i = 0; i < 100; ++i) {
        rng_();
    }
}

//==============================================================================
// Scale Database - 50+ Scales
//==============================================================================

const std::map<juce::String, std::vector<int>>& MIDIPatternGenerator::getScaleDatabase() {
    static const std::map<juce::String, std::vector<int>> scales = {
        // Modal scales
        {"ionian",          {0, 2, 4, 5, 7, 9, 11}},
        {"major",           {0, 2, 4, 5, 7, 9, 11}},
        {"dorian",          {0, 2, 3, 5, 7, 9, 10}},
        {"phrygian",        {0, 1, 3, 5, 7, 8, 10}},
        {"lydian",          {0, 2, 4, 6, 7, 9, 11}},
        {"mixolydian",      {0, 2, 4, 5, 7, 9, 10}},
        {"aeolian",         {0, 2, 3, 5, 7, 8, 10}},
        {"natural_minor",   {0, 2, 3, 5, 7, 8, 10}},
        {"minor",           {0, 2, 3, 5, 7, 8, 10}},
        {"locrian",         {0, 1, 3, 5, 6, 8, 10}},
        
        // Minor variants
        {"harmonic_minor",      {0, 2, 3, 5, 7, 8, 11}},
        {"melodic_minor",       {0, 2, 3, 5, 7, 9, 11}},
        {"hungarian_minor",     {0, 2, 3, 6, 7, 8, 11}},
        {"neapolitan_minor",    {0, 1, 3, 5, 7, 8, 11}},
        {"romanian_minor",      {0, 2, 3, 6, 7, 9, 10}},
        
        // Major variants
        {"harmonic_major",          {0, 2, 4, 5, 7, 8, 11}},
        {"double_harmonic_major",   {0, 1, 4, 5, 7, 8, 11}},
        {"neapolitan_major",        {0, 1, 3, 5, 7, 9, 11}},
        {"lydian_augmented",        {0, 2, 4, 6, 8, 9, 11}},
        
        // Pentatonic & Blues
        {"pentatonic_major",    {0, 2, 4, 7, 9}},
        {"pentatonic_minor",    {0, 3, 5, 7, 10}},
        {"pentatonic",          {0, 3, 5, 7, 10}},
        {"blues",               {0, 3, 5, 6, 7, 10}},
        {"blues_major",         {0, 2, 3, 4, 7, 9}},
        {"blues_minor",         {0, 3, 5, 6, 7, 10}},
        
        // Bebop scales
        {"bebop_dominant",  {0, 2, 4, 5, 7, 9, 10, 11}},
        {"bebop_major",     {0, 2, 4, 5, 7, 8, 9, 11}},
        {"bebop_minor",     {0, 2, 3, 5, 7, 8, 9, 10}},
        {"bebop_dorian",    {0, 2, 3, 4, 5, 7, 9, 10}},
        
        // Exotic/World scales
        {"arabic",          {0, 1, 4, 5, 7, 8, 11}},
        {"persian",         {0, 1, 4, 5, 6, 8, 11}},
        {"japanese",        {0, 1, 5, 7, 8}},
        {"hirajoshi",       {0, 2, 3, 7, 8}},
        {"hindu",           {0, 2, 4, 5, 7, 8, 10}},
        {"gypsy",           {0, 2, 3, 6, 7, 8, 10}},
        {"romanian",        {0, 2, 3, 6, 7, 9, 10}},
        {"spanish",         {0, 1, 4, 5, 7, 8, 10}},
        {"jewish",          {0, 1, 4, 5, 7, 8, 10}},
        {"freygish",        {0, 1, 4, 5, 7, 8, 10}},
        {"egyptian",        {0, 2, 5, 7, 10}},
        {"chinese",         {0, 4, 6, 7, 11}},
        {"indian",          {0, 1, 4, 5, 7, 8, 10}},
        {"balinese",        {0, 1, 3, 7, 8}},
        {"javanese",        {0, 1, 3, 5, 7, 9, 10}},
        {"ethiopian",       {0, 2, 4, 5, 7, 8, 11}},
        {"hawaiian",        {0, 2, 3, 7, 9}},
        
        // Symmetric scales
        {"chromatic",               {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}},
        {"whole_tone",              {0, 2, 4, 6, 8, 10}},
        {"diminished",              {0, 2, 3, 5, 6, 8, 9, 11}},
        {"diminished_whole_half",   {0, 2, 3, 5, 6, 8, 9, 11}},
        {"diminished_half_whole",   {0, 1, 3, 4, 6, 7, 9, 10}},
        {"augmented",               {0, 3, 4, 7, 8, 11}},
        
        // Jazz scales
        {"lydian_dominant",     {0, 2, 4, 6, 7, 9, 10}},
        {"altered",             {0, 1, 3, 4, 6, 8, 10}},
        {"super_locrian",       {0, 1, 3, 4, 6, 8, 10}},
        {"mixolydian_b6",       {0, 2, 4, 5, 7, 8, 10}},
        {"dorian_b2",           {0, 1, 3, 5, 7, 9, 10}},
        {"phrygian_dominant",   {0, 1, 4, 5, 7, 8, 10}},
        {"locrian_natural2",    {0, 2, 3, 5, 6, 8, 10}},
        {"lydian_b7",           {0, 2, 4, 6, 7, 9, 10}},
        
        // Other unique scales
        {"prometheus",          {0, 2, 4, 6, 9, 10}},
        {"tritone",             {0, 1, 4, 6, 7, 10}},
        {"enigmatic",           {0, 1, 4, 6, 8, 10, 11}},
        {"double_harmonic",     {0, 1, 4, 5, 7, 8, 11}},
        {"ukrainian_dorian",    {0, 2, 3, 6, 7, 9, 10}},
        {"algerian",            {0, 2, 3, 5, 6, 7, 8, 11}},
        {"flamenco",            {0, 1, 4, 5, 7, 8, 11}},
        {"byzantine",           {0, 1, 4, 5, 7, 8, 11}},
        {"oriental",            {0, 1, 4, 5, 6, 9, 10}},
        {"kumoi",               {0, 2, 3, 7, 9}},
        {"iwato",               {0, 1, 5, 6, 10}},
        {"in_sen",              {0, 1, 5, 7, 10}},
        {"yo",                  {0, 2, 5, 7, 9}},
        {"ritusen",             {0, 2, 5, 7, 9}},
        {"pelog",               {0, 1, 3, 7, 8}},
        {"slendro",             {0, 2, 5, 7, 9}},
        {"hungarian_gypsy",     {0, 2, 3, 6, 7, 8, 11}},
        {"spanish_gypsy",       {0, 1, 4, 5, 7, 8, 10}},
        {"maqam_hijaz",         {0, 1, 4, 5, 7, 8, 10}},
        {"maqam_bayati",        {0, 2, 3, 5, 7, 8, 10}}, // Approximated for MIDI
        {"raga_bhairav",        {0, 1, 4, 5, 7, 8, 11}},
    };
    return scales;
}

std::vector<int> MIDIPatternGenerator::getScaleIntervals(const juce::String& scale) {
    const auto& db = getScaleDatabase();
    auto it = db.find(scale.toLowerCase());
    if (it != db.end()) {
        return it->second;
    }
    // Default to minor if scale not found
    return db.at("minor");
}

//==============================================================================
// Note and Scale Utilities
//==============================================================================

int MIDIPatternGenerator::noteNameToMIDI(const juce::String& noteName) {
    static const std::map<juce::String, int> noteMap = {
        {"C", 0}, {"C#", 1}, {"Db", 1}, {"D", 2}, {"D#", 3}, {"Eb", 3},
        {"E", 4}, {"Fb", 4}, {"E#", 5}, {"F", 5}, {"F#", 6}, {"Gb", 6}, 
        {"G", 7}, {"G#", 8}, {"Ab", 8}, {"A", 9}, {"A#", 10}, {"Bb", 10}, 
        {"B", 11}, {"Cb", 11}, {"B#", 0}
    };
    
    juce::String note = noteName.trimCharactersAtEnd("0123456789-");
    int octave = noteName.getTrailingIntValue();
    if (octave == 0 && !noteName.containsChar('0')) octave = 4;
    
    auto it = noteMap.find(note);
    if (it != noteMap.end()) {
        return (octave + 1) * 12 + it->second;
    }
    return 60; // Default to middle C
}

std::vector<int> MIDIPatternGenerator::getScaleNotes(const juce::String& root, const juce::String& scale) {
    auto intervals = getScaleIntervals(scale);
    int rootNote = noteNameToMIDI(root + "0") % 12;
    
    std::vector<int> scaleNotes;
    for (int interval : intervals) {
        scaleNotes.push_back((rootNote + static_cast<int>(interval)) % 12);
    }
    return scaleNotes;
}

//==============================================================================
// Quantize Utilities
//==============================================================================

double MIDIPatternGenerator::getGridSize(const juce::String& grid) {
    if (grid == "1/4" || grid == "quarter") return 1.0;
    if (grid == "1/8" || grid == "eighth") return 0.5;
    if (grid == "1/16" || grid == "sixteenth") return 0.25;
    if (grid == "1/32" || grid == "thirtysecond") return 0.125;
    if (grid == "1/64") return 0.0625;
    if (grid == "1/8T" || grid == "eighth_triplet") return 1.0 / 3.0;
    if (grid == "1/16T" || grid == "sixteenth_triplet") return 1.0 / 6.0;
    if (grid == "1/4T" || grid == "quarter_triplet") return 2.0 / 3.0;
    return 0.25; // Default to 1/16
}

double MIDIPatternGenerator::quantizePosition(double position, const juce::String& grid, float strength) {
    if (strength <= 0.0f) return position;
    
    double gridSize = getGridSize(grid);
    double quantized = std::round(position / gridSize) * gridSize;
    
    // Blend between original and quantized based on strength
    return position + (quantized - position) * strength;
}

void MIDIPatternGenerator::applyQuantize(std::vector<MIDINote>& notes, const PatternSpec& spec) {
    if (!spec.autoQuantize || spec.quantizeStrength <= 0.0f) return;
    
    for (auto& note : notes) {
        note.startBeats = quantizePosition(note.startBeats, spec.quantizeGrid, spec.quantizeStrength);
        // Ensure note doesn't start before 0
        if (note.startBeats < 0) note.startBeats = 0;
    }
}

//==============================================================================
// Variation Helpers - Inject Uniqueness
//==============================================================================

float MIDIPatternGenerator::varyProbability(float baseProbability, float variation) {
    std::uniform_real_distribution<float> dist(-0.3f, 0.3f);
    float varied = baseProbability + (dist(rng_) * variation);
    return juce::jlimit(0.0f, 1.0f, varied);
}

int MIDIPatternGenerator::varyVelocity(int baseVelocity, float variation) {
    std::uniform_int_distribution<int> dist(-20, 20);
    int varied = baseVelocity + static_cast<int>(dist(rng_) * variation);
    return juce::jlimit(1, 127, varied);
}

//==============================================================================
// Humanization
//==============================================================================

float MIDIPatternGenerator::applySwing(float position, float swingAmount) {
    int step = static_cast<int>(position * 4) % 4;
    if (step == 1 || step == 3) {
        return position + (swingAmount * 0.08f);
    }
    return position;
}

int MIDIPatternGenerator::applyHumanize(int velocity, float amount) {
    std::uniform_int_distribution<int> dist(-15, 15);
    int variation = static_cast<int>(dist(rng_) * amount);
    return juce::jlimit(1, 127, velocity + variation);
}

double MIDIPatternGenerator::applyTimingHumanize(double position, float amount) {
    std::uniform_real_distribution<double> dist(-0.03, 0.03);
    return position + (dist(rng_) * amount);
}

//==============================================================================
// Drum Pattern Templates with VARIATION
//==============================================================================

MIDIPatternGenerator::DrumTemplate MIDIPatternGenerator::getTrapTemplate(float variation) {
    DrumTemplate t;
    
    // Base patterns
    std::vector<float> baseKick = {1.0f, 0.0f, 0.0f, 0.3f, 0.0f, 0.0f, 0.7f, 0.0f,
                                    0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.0f, 0.2f, 0.0f};
    std::vector<float> baseSnare = {0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
                                     0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.3f};
    std::vector<float> baseHat = {0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f,
                                   0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f};
    std::vector<float> baseOpen = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.3f,
                                    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.3f};
    
    // Apply variation to each step
    for (size_t i = 0; i < 16; ++i) {
        t.kickPattern.push_back(varyProbability(baseKick[i], variation));
        t.snarePattern.push_back(varyProbability(baseSnare[i], variation));
        t.hatPattern.push_back(varyProbability(baseHat[i], variation));
        t.openHatPattern.push_back(varyProbability(baseOpen[i], variation));
    }
    
    // Inject some random ghost notes based on variation
    std::uniform_int_distribution<int> stepDist(0, 15);
    int extraHits = static_cast<int>(variation * 4);
    for (int i = 0; i < extraHits; ++i) {
        int step = stepDist(rng_);
        t.kickPattern[step] = varyProbability(0.3f, variation);
    }
    
    t.hatVelocityVariation = 0.3f + variation * 0.2f;
    return t;
}

MIDIPatternGenerator::DrumTemplate MIDIPatternGenerator::getHouseTemplate(float variation) {
    DrumTemplate t;
    
    // Four-on-the-floor base
    std::vector<float> baseKick = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
                                    1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    std::vector<float> baseSnare = {0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
                                     0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    std::vector<float> baseHat = {0.0f, 0.0f, 0.9f, 0.0f, 0.0f, 0.0f, 0.9f, 0.0f,
                                   0.0f, 0.0f, 0.9f, 0.0f, 0.0f, 0.0f, 0.9f, 0.0f};
    std::vector<float> baseOpen = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                                    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.6f, 0.0f};
    
    for (size_t i = 0; i < 16; ++i) {
        t.kickPattern.push_back(varyProbability(baseKick[i], variation));
        t.snarePattern.push_back(varyProbability(baseSnare[i], variation));
        t.hatPattern.push_back(varyProbability(baseHat[i], variation));
        t.openHatPattern.push_back(varyProbability(baseOpen[i], variation));
    }
    
    // Add some offbeat kicks with variation
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    if (dist(rng_) < variation) {
        t.kickPattern[6] = varyProbability(0.5f, variation);
    }
    if (dist(rng_) < variation) {
        t.kickPattern[14] = varyProbability(0.4f, variation);
    }
    
    return t;
}

MIDIPatternGenerator::DrumTemplate MIDIPatternGenerator::getJazzTemplate(float variation) {
    DrumTemplate t;
    
    // Swing ride pattern
    std::vector<float> baseKick = {0.6f, 0.0f, 0.0f, 0.3f, 0.0f, 0.0f, 0.4f, 0.0f,
                                    0.0f, 0.2f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f};
    std::vector<float> baseSnare = {0.0f, 0.0f, 0.2f, 0.0f, 0.0f, 0.0f, 0.3f, 0.0f,
                                     0.0f, 0.0f, 0.2f, 0.0f, 0.0f, 0.0f, 0.2f, 0.0f};
    std::vector<float> baseRide = {0.9f, 0.0f, 0.5f, 0.9f, 0.0f, 0.5f, 0.9f, 0.0f,
                                    0.5f, 0.9f, 0.0f, 0.5f, 0.9f, 0.0f, 0.5f, 0.0f};
    
    for (size_t i = 0; i < 16; ++i) {
        t.kickPattern.push_back(varyProbability(baseKick[i], variation));
        t.snarePattern.push_back(varyProbability(baseSnare[i], variation));
        t.hatPattern.push_back(varyProbability(baseRide[i], variation));
        t.openHatPattern.push_back(0.0f);
    }
    
    // Jazz has more ghost notes with high variation
    std::uniform_int_distribution<int> stepDist(0, 15);
    int ghosts = static_cast<int>(variation * 6);
    for (int i = 0; i < ghosts; ++i) {
        int step = stepDist(rng_);
        t.snarePattern[step] = std::max(t.snarePattern[step], varyProbability(0.2f, variation));
    }
    
    t.hatVelocityVariation = 0.25f;
    return t;
}

MIDIPatternGenerator::DrumTemplate MIDIPatternGenerator::getLofiTemplate(float variation) {
    DrumTemplate t;
    
    // Laid-back boom bap
    std::vector<float> baseKick = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f,
                                    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.4f, 0.0f, 0.0f};
    std::vector<float> baseSnare = {0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
                                     0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.2f};
    std::vector<float> baseHat = {0.7f, 0.0f, 0.7f, 0.0f, 0.7f, 0.0f, 0.7f, 0.0f,
                                   0.7f, 0.0f, 0.7f, 0.0f, 0.7f, 0.0f, 0.7f, 0.0f};
    
    for (size_t i = 0; i < 16; ++i) {
        t.kickPattern.push_back(varyProbability(baseKick[i], variation));
        t.snarePattern.push_back(varyProbability(baseSnare[i], variation));
        t.hatPattern.push_back(varyProbability(baseHat[i], variation));
        t.openHatPattern.push_back(i == 7 ? varyProbability(0.4f, variation) : 0.0f);
    }
    
    t.hatVelocityVariation = 0.2f;
    return t;
}

MIDIPatternGenerator::DrumTemplate MIDIPatternGenerator::getRockTemplate(float variation) {
    DrumTemplate t;
    
    std::vector<float> baseKick = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.8f, 0.0f,
                                    1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    std::vector<float> baseSnare = {0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
                                     0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    std::vector<float> baseHat = {0.9f, 0.0f, 0.9f, 0.0f, 0.9f, 0.0f, 0.9f, 0.0f,
                                   0.9f, 0.0f, 0.9f, 0.0f, 0.9f, 0.0f, 0.9f, 0.0f};
    
    for (size_t i = 0; i < 16; ++i) {
        t.kickPattern.push_back(varyProbability(baseKick[i], variation));
        t.snarePattern.push_back(varyProbability(baseSnare[i], variation));
        t.hatPattern.push_back(varyProbability(baseHat[i], variation));
        t.openHatPattern.push_back(0.0f);
    }
    
    return t;
}

MIDIPatternGenerator::DrumTemplate MIDIPatternGenerator::getEDMTemplate(float variation) {
    DrumTemplate t;
    
    // Big room / EDM
    std::vector<float> baseKick = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
                                    1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.5f, 0.0f};
    std::vector<float> baseClap = {0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
                                    0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    std::vector<float> baseHat = {1.0f, 0.5f, 1.0f, 0.5f, 1.0f, 0.5f, 1.0f, 0.5f,
                                   1.0f, 0.5f, 1.0f, 0.5f, 1.0f, 0.5f, 1.0f, 0.5f};
    
    for (size_t i = 0; i < 16; ++i) {
        t.kickPattern.push_back(varyProbability(baseKick[i], variation));
        t.snarePattern.push_back(varyProbability(baseClap[i], variation));
        t.hatPattern.push_back(varyProbability(baseHat[i], variation));
        t.openHatPattern.push_back(0.0f);
    }
    
    // Add build-up variations
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    if (dist(rng_) < variation * 0.5f) {
        // Snare roll variation
        for (int i = 12; i < 16; ++i) {
            t.snarePattern[i] = varyProbability(0.7f, variation);
        }
    }
    
    t.hatVelocityVariation = 0.15f;
    return t;
}

MIDIPatternGenerator::DrumTemplate MIDIPatternGenerator::getRnBTemplate(float variation) {
    DrumTemplate t;
    
    // R&B / Neo-soul groove
    std::vector<float> baseKick = {1.0f, 0.0f, 0.0f, 0.3f, 0.0f, 0.0f, 0.6f, 0.0f,
                                    0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    std::vector<float> baseSnare = {0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.2f,
                                     0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    std::vector<float> baseHat = {0.8f, 0.4f, 0.8f, 0.4f, 0.8f, 0.4f, 0.8f, 0.4f,
                                   0.8f, 0.4f, 0.8f, 0.4f, 0.8f, 0.4f, 0.8f, 0.4f};
    
    for (size_t i = 0; i < 16; ++i) {
        t.kickPattern.push_back(varyProbability(baseKick[i], variation));
        t.snarePattern.push_back(varyProbability(baseSnare[i], variation));
        t.hatPattern.push_back(varyProbability(baseHat[i], variation));
        t.openHatPattern.push_back(0.0f);
    }
    
    t.hatVelocityVariation = 0.25f;
    return t;
}

//==============================================================================
// Drum Pattern Generation
//==============================================================================

std::vector<MIDINote> MIDIPatternGenerator::generateDrumPattern(const PatternSpec& spec) {
    // Initialize with unique seed for this generation
    initializeRNG(spec.seed);
    
    std::vector<MIDINote> notes;
    
    // Select template based on style with variation
    DrumTemplate templ;
    juce::String style = spec.style.toLowerCase();
    
    if (style == "trap")       templ = getTrapTemplate(spec.variation);
    else if (style == "house") templ = getHouseTemplate(spec.variation);
    else if (style == "jazz")  templ = getJazzTemplate(spec.variation);
    else if (style == "lofi")  templ = getLofiTemplate(spec.variation);
    else if (style == "rock")  templ = getRockTemplate(spec.variation);
    else if (style == "edm")   templ = getEDMTemplate(spec.variation);
    else if (style == "rnb" || style == "r&b") templ = getRnBTemplate(spec.variation);
    else                       templ = getTrapTemplate(spec.variation);
    
    std::uniform_real_distribution<float> probDist(0.0f, 1.0f);
    
    int stepsPerBar = 16;
    int totalSteps = stepsPerBar * spec.bars;
    
    for (int step = 0; step < totalSteps; ++step) {
        int patternStep = step % stepsPerBar;
        double beatPosition = step / 4.0;
        
        // Apply swing
        beatPosition = applySwing(static_cast<float>(beatPosition), spec.swing);
        
        // Kick
        if (patternStep < static_cast<int>(templ.kickPattern.size())) {
            float prob = templ.kickPattern[patternStep] * (0.5f + spec.complexity * 0.5f);
            if (probDist(rng_) < prob) {
                MIDINote note;
                note.pitch = KICK;
                note.startBeats = applyTimingHumanize(beatPosition, spec.humanize);
                note.lengthBeats = 0.25;
                note.velocity = applyHumanize(100, spec.humanize);
                notes.push_back(note);
            }
        }
        
        // Snare/Clap
        if (patternStep < static_cast<int>(templ.snarePattern.size())) {
            float prob = templ.snarePattern[patternStep];
            if (probDist(rng_) < prob) {
                MIDINote note;
                note.pitch = (style == "trap" || style == "house" || style == "edm") ? CLAP : SNARE;
                note.startBeats = applyTimingHumanize(beatPosition, spec.humanize);
                note.lengthBeats = 0.25;
                note.velocity = applyHumanize(110, spec.humanize);
                notes.push_back(note);
            }
        }
        
        // Hi-hat
        if (patternStep < static_cast<int>(templ.hatPattern.size())) {
            float prob = templ.hatPattern[patternStep] * (0.7f + spec.complexity * 0.3f);
            if (probDist(rng_) < prob) {
                MIDINote note;
                note.pitch = (style == "jazz") ? RIDE : CLOSED_HAT;
                note.startBeats = applyTimingHumanize(beatPosition, spec.humanize);
                note.lengthBeats = 0.125;
                int baseVel = (patternStep % 4 == 0) ? 100 : 70;
                note.velocity = applyHumanize(baseVel, spec.humanize + templ.hatVelocityVariation);
                notes.push_back(note);
            }
        }
        
        // Open hi-hat
        if (patternStep < static_cast<int>(templ.openHatPattern.size()) && 
            templ.openHatPattern[patternStep] > 0) {
            float prob = templ.openHatPattern[patternStep];
            if (probDist(rng_) < prob) {
                MIDINote note;
                note.pitch = OPEN_HAT;
                note.startBeats = applyTimingHumanize(beatPosition, spec.humanize);
                note.lengthBeats = 0.5;
                note.velocity = applyHumanize(80, spec.humanize);
                notes.push_back(note);
            }
        }
        
        // Trap hi-hat rolls (complexity-based)
        if (style == "trap" && spec.complexity > 0.6f) {
            if (patternStep == 14 || patternStep == 15) {
                for (int r = 0; r < 2; ++r) {
                    if (probDist(rng_) < spec.complexity * 0.5f) {
                        MIDINote note;
                        note.pitch = CLOSED_HAT;
                        note.startBeats = beatPosition + (r * 0.0625);
                        note.lengthBeats = 0.0625;
                        note.velocity = applyHumanize(60 + r * 10, spec.humanize);
                        notes.push_back(note);
                    }
                }
            }
        }
    }
    
    // Apply auto-quantize
    applyQuantize(notes, spec);
    
    return notes;
}

//==============================================================================
// Bass Line Generation
//==============================================================================

std::vector<MIDINote> MIDIPatternGenerator::generateBassLine(const PatternSpec& spec) {
    initializeRNG(spec.seed);
    
    std::vector<MIDINote> notes;
    
    auto scaleNotes = getScaleNotes(spec.key, spec.scale);
    int rootMIDI = noteNameToMIDI(spec.key + "2");
    
    std::uniform_real_distribution<float> probDist(0.0f, 1.0f);
    std::uniform_int_distribution<int> noteDist(0, static_cast<int>(scaleNotes.size()) - 1);
    
    // Generate unique rhythm pattern for this call
    std::vector<float> rhythmPattern(16);
    std::vector<float> lengthPattern(16);
    
    juce::String style = spec.style.toLowerCase();
    
    // Base patterns that get varied
    if (style == "trap" || style == "lofi") {
        std::vector<float> base = {1.0f, 0.0f, 0.0f, 0.3f, 0.0f, 0.0f, 0.5f, 0.0f,
                                   0.0f, 0.0f, 0.3f, 0.0f, 0.0f, 0.0f, 0.4f, 0.0f};
        std::vector<float> lengths = {2.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.5f, 0.0f,
                                      0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f};
        for (int i = 0; i < 16; ++i) {
            rhythmPattern[i] = varyProbability(base[i], spec.variation);
            lengthPattern[i] = lengths[i] * (0.8f + probDist(rng_) * 0.4f);
        }
    } else if (style == "house" || style == "edm") {
        std::vector<float> base = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
                                   1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.5f, 0.0f};
        for (int i = 0; i < 16; ++i) {
            rhythmPattern[i] = varyProbability(base[i], spec.variation);
            lengthPattern[i] = 0.5f;
        }
    } else if (style == "jazz") {
        std::vector<float> base = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
                                   1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
        for (int i = 0; i < 16; ++i) {
            rhythmPattern[i] = varyProbability(base[i], spec.variation);
            lengthPattern[i] = 0.9f;
        }
    } else {
        std::vector<float> base = {1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
                                   1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f};
        for (int i = 0; i < 16; ++i) {
            rhythmPattern[i] = varyProbability(base[i], spec.variation);
            lengthPattern[i] = 0.4f;
        }
    }
    
    // Get unique chord progression
    auto chords = getChordProgression(spec.style, spec.key, spec.variation);
    
    int stepsPerBar = 16;
    int totalSteps = stepsPerBar * spec.bars;
    
    int currentChordIndex = 0;
    
    for (int step = 0; step < totalSteps; ++step) {
        int patternStep = step % stepsPerBar;
        int bar = step / stepsPerBar;
        double beatPosition = step / 4.0;
        
        if (step % stepsPerBar == 0 && !chords.empty()) {
            currentChordIndex = bar % static_cast<int>(chords.size());
        }
        
        float prob = rhythmPattern[patternStep] * (0.6f + spec.complexity * 0.4f);
        if (probDist(rng_) < prob && lengthPattern[patternStep] > 0) {
            MIDINote note;
            
            int chordRoot = chords.empty() ? 0 : chords[currentChordIndex][0];
            
            // Note selection with variation
            std::vector<int> bassOptions = {chordRoot, chordRoot, (chordRoot + 7) % 12, chordRoot + 12};
            
            // Add more options based on variation
            if (spec.variation > 0.3f) {
                bassOptions.push_back((chordRoot + 5) % 12); // 4th
            }
            if (spec.variation > 0.6f) {
                bassOptions.push_back((chordRoot + 10) % 12); // 7th
            }
            
            int selectedInterval = bassOptions[noteDist(rng_) % static_cast<int>(bassOptions.size())];
            
            note.pitch = rootMIDI + selectedInterval;
            note.startBeats = applyTimingHumanize(beatPosition, spec.humanize);
            note.lengthBeats = lengthPattern[patternStep];
            note.velocity = applyHumanize(90, spec.humanize);
            
            notes.push_back(note);
        }
    }
    
    applyQuantize(notes, spec);
    return notes;
}

//==============================================================================
// Chord Progression Generation
//==============================================================================

std::vector<std::vector<int>> MIDIPatternGenerator::getChordProgression(
    const juce::String& style, const juce::String& key, float variation) {
    
    // Multiple progression options per style
    std::vector<std::vector<std::vector<int>>> progressionOptions;
    
    juce::String s = style.toLowerCase();
    
    if (s == "trap" || s == "lofi") {
        progressionOptions = {
            {{0, 3, 7}, {8, 0, 3}, {3, 7, 10}, {10, 2, 5}},     // i - VI - III - VII
            {{0, 3, 7}, {5, 8, 0}, {7, 10, 2}, {3, 7, 10}},     // i - iv - v - III
            {{0, 3, 7}, {10, 2, 5}, {8, 0, 3}, {5, 8, 0}},      // i - VII - VI - iv
            {{0, 3, 7}, {3, 7, 10}, {5, 8, 0}, {7, 10, 2}},     // i - III - iv - v
        };
    } else if (s == "house" || s == "edm") {
        progressionOptions = {
            {{0, 3, 7}, {5, 8, 0}, {10, 2, 5}, {3, 7, 10}},     // i - iv - VII - III
            {{0, 4, 7}, {7, 11, 2}, {9, 0, 4}, {5, 9, 0}},      // I - V - vi - IV
            {{0, 3, 7}, {8, 0, 3}, {5, 8, 0}, {10, 2, 5}},      // i - VI - iv - VII
        };
    } else if (s == "jazz") {
        progressionOptions = {
            {{2, 5, 9}, {7, 11, 2}, {0, 4, 7}, {9, 0, 4}},      // ii - V - I - vi
            {{0, 4, 7}, {9, 0, 4}, {2, 5, 9}, {7, 11, 2}},      // I - vi - ii - V
            {{0, 4, 7, 11}, {5, 9, 0, 4}, {2, 5, 9, 0}, {7, 11, 2, 5}}, // Imaj7 - IVmaj7 - ii7 - V7
        };
    } else if (s == "rock") {
        progressionOptions = {
            {{0, 4, 7}, {5, 9, 0}, {7, 11, 2}, {0, 4, 7}},      // I - IV - V - I
            {{0, 4, 7}, {7, 11, 2}, {5, 9, 0}, {0, 4, 7}},      // I - V - IV - I
            {{9, 0, 4}, {5, 9, 0}, {0, 4, 7}, {7, 11, 2}},      // vi - IV - I - V
        };
    } else if (s == "rnb" || s == "r&b") {
        progressionOptions = {
            {{0, 4, 7, 11}, {9, 0, 4, 7}, {2, 5, 9, 0}, {7, 11, 2, 5}}, // Imaj7 - vi7 - ii7 - V7
            {{0, 3, 7, 10}, {5, 8, 0, 3}, {10, 2, 5, 8}, {3, 7, 10, 2}}, // i7 - iv7 - VII7 - III7
        };
    } else {
        // Pop default
        progressionOptions = {
            {{0, 4, 7}, {7, 11, 2}, {9, 0, 4}, {5, 9, 0}},      // I - V - vi - IV
            {{0, 4, 7}, {5, 9, 0}, {9, 0, 4}, {7, 11, 2}},      // I - IV - vi - V
            {{9, 0, 4}, {5, 9, 0}, {0, 4, 7}, {7, 11, 2}},      // vi - IV - I - V
        };
    }
    
    // Select a random progression based on seed
    std::uniform_int_distribution<size_t> dist(0, progressionOptions.size() - 1);
    size_t selected = dist(rng_);
    
    auto progression = progressionOptions[selected];
    
    // Apply variation to chord voicings
    if (variation > 0.3f) {
        std::uniform_int_distribution<int> invDist(0, 2);
        std::uniform_real_distribution<float> probDist(0.0f, 1.0f);
        for (auto& chord : progression) {
            if (probDist(rng_) < variation) {
                // Apply random inversion
                int inv = invDist(rng_);
                for (int i = 0; i < inv && chord.size() > 1; ++i) {
                    chord.push_back(chord[0] + 12);
                    chord.erase(chord.begin());
                }
            }
        }
    }
    
    return progression;
}

std::vector<MIDINote> MIDIPatternGenerator::generateChordProgression(const PatternSpec& spec) {
    initializeRNG(spec.seed);
    
    std::vector<MIDINote> notes;
    
    int rootMIDI = noteNameToMIDI(spec.key + "4");
    auto chords = getChordProgression(spec.style, spec.key, spec.variation);
    
    std::uniform_real_distribution<float> probDist(0.0f, 1.0f);
    
    // Generate unique rhythm pattern
    std::vector<float> rhythmPattern(16);
    juce::String style = spec.style.toLowerCase();
    
    if (style == "trap" || style == "lofi") {
        std::vector<float> base = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                                   0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
        for (int i = 0; i < 16; ++i) {
            rhythmPattern[i] = varyProbability(base[i], spec.variation);
        }
    } else if (style == "house" || style == "edm") {
        std::vector<float> base = {0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                                   0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f};
        for (int i = 0; i < 16; ++i) {
            rhythmPattern[i] = varyProbability(base[i], spec.variation);
        }
    } else if (style == "jazz") {
        std::vector<float> base = {0.0f, 0.0f, 0.8f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f,
                                   0.0f, 0.0f, 0.0f, 0.7f, 0.0f, 0.0f, 0.5f, 0.0f};
        for (int i = 0; i < 16; ++i) {
            rhythmPattern[i] = varyProbability(base[i], spec.variation);
        }
    } else {
        std::vector<float> base = {1.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f,
                                   1.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f};
        for (int i = 0; i < 16; ++i) {
            rhythmPattern[i] = varyProbability(base[i], spec.variation);
        }
    }
    
    int stepsPerBar = 16;
    int totalSteps = stepsPerBar * spec.bars;
    
    for (int step = 0; step < totalSteps; ++step) {
        int patternStep = step % stepsPerBar;
        int bar = step / stepsPerBar;
        double beatPosition = step / 4.0;
        
        int chordIndex = bar % static_cast<int>(chords.size());
        
        float prob = rhythmPattern[patternStep];
        if (probDist(rng_) < prob) {
            for (int interval : chords[chordIndex]) {
                MIDINote note;
                note.pitch = rootMIDI + interval;
                note.startBeats = applyTimingHumanize(beatPosition, spec.humanize * 0.3f);
                
                if (style == "trap" || style == "lofi") {
                    note.lengthBeats = 4.0;
                } else if (style == "house" || style == "edm") {
                    note.lengthBeats = 0.5;
                } else {
                    note.lengthBeats = 1.0;
                }
                
                note.velocity = applyHumanize(75, spec.humanize);
                notes.push_back(note);
            }
        }
    }
    
    applyQuantize(notes, spec);
    return notes;
}

//==============================================================================
// Melody Generation with Motif Development
//==============================================================================

std::vector<int> MIDIPatternGenerator::generateMotif(const std::vector<int>& scaleNotes, int length) {
    std::vector<int> motif;
    std::uniform_int_distribution<int> scaleDist(0, static_cast<int>(scaleNotes.size()) - 1);
    std::uniform_int_distribution<int> dirDist(-1, 1);
    
    int currentIndex = scaleDist(rng_);
    
    for (int i = 0; i < length; ++i) {
        motif.push_back(scaleNotes[currentIndex]);
        
        // Move stepwise or leap
        int move = dirDist(rng_);
        currentIndex = juce::jlimit(0, static_cast<int>(scaleNotes.size()) - 1, currentIndex + move);
    }
    
    return motif;
}

std::vector<int> MIDIPatternGenerator::developMotif(const std::vector<int>& motif, float variation) {
    std::vector<int> developed = motif;
    std::uniform_real_distribution<float> probDist(0.0f, 1.0f);
    std::uniform_int_distribution<int> transposeDist(-5, 7);
    
    // Transpose
    if (probDist(rng_) < variation) {
        int transpose = transposeDist(rng_);
        for (auto& note : developed) {
            note = (note + transpose) % 12;
            if (note < 0) note += 12;
        }
    }
    
    // Invert
    if (probDist(rng_) < variation * 0.5f) {
        std::reverse(developed.begin(), developed.end());
    }
    
    // Augment/diminish (change some notes)
    for (auto& note : developed) {
        if (probDist(rng_) < variation * 0.3f) {
            note = (note + (probDist(rng_) < 0.5f ? 1 : -1)) % 12;
            if (note < 0) note += 12;
        }
    }
    
    return developed;
}

std::vector<MIDINote> MIDIPatternGenerator::generateMelody(const PatternSpec& spec) {
    initializeRNG(spec.seed);
    
    std::vector<MIDINote> notes;
    
    auto scaleNotes = getScaleNotes(spec.key, spec.scale);
    int rootMIDI = noteNameToMIDI(spec.key + "5");
    
    std::uniform_real_distribution<float> probDist(0.0f, 1.0f);
    std::uniform_int_distribution<int> scaleDist(0, static_cast<int>(scaleNotes.size()) - 1);
    std::uniform_int_distribution<int> directionDist(-1, 1);
    std::uniform_int_distribution<int> rhythmDist(1, 4);
    
    // Generate a seed motif (3-5 notes)
    std::uniform_int_distribution<int> motifLengthDist(3, 5);
    auto seedMotif = generateMotif(scaleNotes, motifLengthDist(rng_));
    
    // Generate unique rhythm pattern
    std::vector<float> rhythmPattern(16);
    juce::String style = spec.style.toLowerCase();
    
    if (style == "trap") {
        std::vector<float> base = {0.8f, 0.0f, 0.6f, 0.3f, 0.7f, 0.0f, 0.5f, 0.0f,
                                   0.6f, 0.0f, 0.4f, 0.3f, 0.7f, 0.0f, 0.5f, 0.4f};
        for (int i = 0; i < 16; ++i) {
            rhythmPattern[i] = varyProbability(base[i], spec.variation);
        }
    } else if (style == "lofi") {
        std::vector<float> base = {0.6f, 0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.5f, 0.0f,
                                   0.0f, 0.3f, 0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.3f};
        for (int i = 0; i < 16; ++i) {
            rhythmPattern[i] = varyProbability(base[i], spec.variation);
        }
    } else if (style == "jazz") {
        std::vector<float> base = {0.7f, 0.0f, 0.5f, 0.4f, 0.0f, 0.6f, 0.3f, 0.0f,
                                   0.5f, 0.0f, 0.4f, 0.3f, 0.0f, 0.5f, 0.4f, 0.0f};
        for (int i = 0; i < 16; ++i) {
            rhythmPattern[i] = varyProbability(base[i], spec.variation);
        }
    } else {
        std::vector<float> base = {0.8f, 0.0f, 0.5f, 0.0f, 0.7f, 0.0f, 0.4f, 0.0f,
                                   0.6f, 0.0f, 0.5f, 0.0f, 0.7f, 0.0f, 0.4f, 0.3f};
        for (int i = 0; i < 16; ++i) {
            rhythmPattern[i] = varyProbability(base[i], spec.variation);
        }
    }
    
    int stepsPerBar = 16;
    int totalSteps = stepsPerBar * spec.bars;
    
    int currentScaleIndex = scaleDist(rng_);
    int direction = 1;
    int motifIndex = 0;
    bool useMotif = true;
    
    for (int step = 0; step < totalSteps; ++step) {
        int patternStep = step % stepsPerBar;
        int bar = step / stepsPerBar;
        double beatPosition = step / 4.0;
        
        // Develop motif for new bars
        if (step % stepsPerBar == 0 && bar > 0) {
            seedMotif = developMotif(seedMotif, spec.variation);
            motifIndex = 0;
        }
        
        float prob = rhythmPattern[patternStep] * spec.complexity;
        if (probDist(rng_) < prob) {
            MIDINote note;
            
            // Use motif or free movement
            if (useMotif && motifIndex < static_cast<int>(seedMotif.size())) {
                note.pitch = rootMIDI + seedMotif[motifIndex];
                motifIndex++;
                if (motifIndex >= static_cast<int>(seedMotif.size())) {
                    useMotif = probDist(rng_) < 0.6f; // 60% chance to repeat motif
                    motifIndex = 0;
                }
            } else {
                // Free melodic movement
                if (probDist(rng_) < 0.7f) {
                    currentScaleIndex += direction;
                    if (currentScaleIndex >= static_cast<int>(scaleNotes.size())) {
                        currentScaleIndex = static_cast<int>(scaleNotes.size()) - 2;
                        direction = -1;
                    } else if (currentScaleIndex < 0) {
                        currentScaleIndex = 1;
                        direction = 1;
                    }
                } else {
                    currentScaleIndex = scaleDist(rng_);
                    direction = directionDist(rng_);
                    if (direction == 0) direction = 1;
                }
                note.pitch = rootMIDI + scaleNotes[currentScaleIndex];
            }
            
            note.startBeats = applyTimingHumanize(beatPosition, spec.humanize);
            int lengthChoice = rhythmDist(rng_);
            note.lengthBeats = lengthChoice * 0.25;
            note.velocity = applyHumanize(85, spec.humanize);
            notes.push_back(note);
        }
    }
    
    applyQuantize(notes, spec);
    return notes;
}

//==============================================================================
// Arpeggio Generation
//==============================================================================

std::vector<MIDINote> MIDIPatternGenerator::generateArpeggio(const PatternSpec& spec) {
    initializeRNG(spec.seed);
    
    std::vector<MIDINote> notes;
    
    auto chords = getChordProgression(spec.style, spec.key, spec.variation);
    int rootMIDI = noteNameToMIDI(spec.key + "4");
    
    std::uniform_real_distribution<float> probDist(0.0f, 1.0f);
    
    // Select arpeggio pattern type based on variation
    enum class ArpPattern { Up, Down, UpDown, Random, Played };
    std::uniform_int_distribution<int> patternDist(0, 4);
    ArpPattern pattern = static_cast<ArpPattern>(patternDist(rng_));
    
    // Add some unique rhythm variation
    std::uniform_int_distribution<int> skipDist(0, 3);
    int skipPattern = skipDist(rng_);
    
    int stepsPerBar = 16;
    int totalSteps = stepsPerBar * spec.bars;
    
    for (int step = 0; step < totalSteps; ++step) {
        int bar = step / stepsPerBar;
        int stepInBar = step % stepsPerBar;
        double beatPosition = step / 4.0;
        
        // Skip some steps for variation
        if (skipPattern > 0 && stepInBar % (skipPattern + 1) == skipPattern) {
            continue;
        }
        
        int chordIndex = bar % static_cast<int>(chords.size());
        auto& chord = chords[chordIndex];
        int chordSize = static_cast<int>(chord.size());
        
        int noteIndex;
        
        switch (pattern) {
            case ArpPattern::Up:
                noteIndex = stepInBar % chordSize;
                break;
            case ArpPattern::Down:
                noteIndex = (chordSize - 1) - (stepInBar % chordSize);
                break;
            case ArpPattern::UpDown: {
                int cycle = stepInBar % (chordSize * 2 - 2);
                if (cycle < chordSize) {
                    noteIndex = cycle;
                } else {
                    noteIndex = chordSize - 2 - (cycle - chordSize);
                }
                break;
            }
            case ArpPattern::Random:
                noteIndex = static_cast<int>(probDist(rng_) * chordSize);
                break;
            case ArpPattern::Played:
                // Play in chord order
                noteIndex = stepInBar % chordSize;
                break;
        }
        
        noteIndex = juce::jlimit(0, chordSize - 1, noteIndex);
        
        MIDINote note;
        note.pitch = rootMIDI + chord[noteIndex];
        
        // Octave variation
        if (stepInBar >= 8 && probDist(rng_) < 0.3f * spec.variation) {
            note.pitch += 12;
        }
        if (probDist(rng_) < 0.1f * spec.variation) {
            note.pitch -= 12;
        }
        
        note.startBeats = applyTimingHumanize(beatPosition, spec.humanize);
        note.lengthBeats = 0.2 + probDist(rng_) * 0.1f * spec.variation;
        note.velocity = applyHumanize(70 + (noteIndex == 0 ? 15 : 0), spec.humanize);
        
        notes.push_back(note);
    }
    
    applyQuantize(notes, spec);
    return notes;
}

} // namespace ai
} // namespace zenith
