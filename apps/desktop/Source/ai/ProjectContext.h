/*
  ==============================================================================
    ProjectContext.h
    Project context awareness system for AI mastering
    Phase 2: Context-Aware AI (9/10)
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "VisualAnalyzer.h"
#include "GrokJourney.h"
#include <memory>
#include <unordered_map>

namespace zenith {
namespace ai {

struct TrackInfo {
    juce::String name;
    juce::String type;           // "kick", "snare", "vocal", "synth", etc.
    juce::String role;           // "lead", "rhythm", "pad", "effect"
    bool isMuted = false;
    bool isSoloed = false;
    double volume = 0.0;        // dB
    double pan = 0.0;            // -1.0 to 1.0
    juce::var effects;           // Chain of effects
    VisualAnalysisResult visualAnalysis;
};

struct ProjectInfo {
    juce::String name;
    juce::String genre;
    double tempo = 120.0;
    int timeSignatureNumerator = 4;
    int timeSignatureDenominator = 4;
    double sampleRate = 44100.0;
    int bitDepth = 24;
    juce::String key;
    juce::String scale;          // "major", "minor", "pentatonic", etc.
    std::vector<TrackInfo> tracks;
    juce::String projectDescription;
    juce::var projectMetadata;
};

struct ContextualInsights {
    juce::String overallGenre;
    juce::String arrangementType;  // "sparse", "dense", "minimal", "complex"
    juce::String energyLevel;     // "low", "medium", "high", "dynamic"
    juce::String frequencyBalance; // "bass-heavy", "mid-focused", "bright", "balanced"
    juce::String productionStyle;  // "modern", "vintage", "electronic", "acoustic"
    std::vector<juce::String> recommendations;
    juce::var contextFeatures;
};

class ProjectContext {
public:
    ProjectContext();
    ~ProjectContext();

    // Project analysis
    bool analyzeProject(const juce::String& projectPath);
    bool analyzeFromDAWState(const juce::var& dawState);
    
    // Context queries
    ContextualInsights getContextualInsights() const;
    ProjectInfo getProjectInfo() const { return projectInfo; }
    
    // Track relationship analysis
    std::vector<juce::String> findConflictingTracks() const;
    std::vector<juce::String> findDominantTracks() const;
    juce::String getTrackRelationship(const juce::String& track1, const juce::String& track2) const;
    
    // Genre and style detection
    juce::String detectGenre() const;
    juce::String detectProductionStyle() const;
    juce::String detectArrangementType() const;
    
    // Frequency analysis
    juce::String analyzeFrequencyBalance() const;
    std::vector<juce::String> getFrequencyRecommendations() const;
    
    // Energy and dynamics
    juce::String analyzeEnergyLevel() const;
    std::vector<juce::String> getDynamicsRecommendations() const;
    
    // Integration with learning system
    void updateFromLearning(const GrokJourney& journey);
    void storeContextualLearning(const ContextualInsights& insights, double userSatisfaction);

private:
    ProjectInfo projectInfo;
    ContextualInsights insights;
    std::unique_ptr<VisualAnalyzer> visualAnalyzer;
    
    // Analysis helpers
    void analyzeTrackRelationships();
    void analyzeProjectStructure();
    void generateContextualInsights();
    
    // Track classification
    juce::String classifyTrackType(const TrackInfo& track) const;
    juce::String classifyTrackRole(const TrackInfo& track) const;
    
    // Frequency analysis helpers
    std::vector<float> computeTrackFrequencyMask(const TrackInfo& track) const;
    float calculateFrequencyOverlap(const TrackInfo& track1, const TrackInfo& track2) const;
    
    // Energy analysis
    float calculateTrackEnergy(const TrackInfo& track) const;
    float calculateProjectDynamics() const;
    
    // Genre detection using learned patterns
    juce::String matchGenreFromFeatures() const;
    juce::String matchProductionStyleFromFeatures() const;
    
    // Context feature extraction
    juce::var extractContextFeatures() const;
    juce::String generateContextualDescription() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProjectContext)
};

} // namespace ai
} // namespace zenith
