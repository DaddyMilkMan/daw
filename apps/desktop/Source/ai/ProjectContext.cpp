/*
  ==============================================================================
    ProjectContext.cpp
    Project context awareness implementation
  ==============================================================================
*/

#include "ProjectContext.h"
#include <algorithm>
#include <numeric>
#include <cmath>

namespace zenith {
namespace ai {

ProjectContext::ProjectContext() {
    visualAnalyzer = std::make_unique<VisualAnalyzer>();
}

ProjectContext::~ProjectContext() = default;

bool ProjectContext::analyzeProject(const juce::String& projectPath) {
    // Load project file (implementation depends on project format)
    // This is a placeholder - actual implementation would parse project files
    
    projectInfo.name = juce::File(projectPath).getFileNameWithoutExtension();
    projectInfo.sampleRate = 44100.0;
    projectInfo.bitDepth = 24;
    
    // Analyze project structure
    analyzeProjectStructure();
    analyzeTrackRelationships();
    generateContextualInsights();
    
    return true;
}

bool ProjectContext::analyzeFromDAWState(const juce::var& dawState) {
    // Extract project information from DAW state
    if (auto* stateObj = dawState.getDynamicObject()) {
        projectInfo.name = stateObj->hasProperty("projectName") ? stateObj->getProperty("projectName").toString() : "Untitled";
        projectInfo.tempo = stateObj->hasProperty("tempo") ? static_cast<double>(stateObj->getProperty("tempo")) : 120.0;
        projectInfo.sampleRate = stateObj->hasProperty("sampleRate") ? static_cast<double>(stateObj->getProperty("sampleRate")) : 44100.0;
        projectInfo.bitDepth = stateObj->hasProperty("bitDepth") ? static_cast<int>(stateObj->getProperty("bitDepth")) : 24;
        
        // Extract track information
        auto tracksVar = stateObj->hasProperty("tracks") ? stateObj->getProperty("tracks") : juce::var();
        if (tracksVar.isArray()) {
            projectInfo.tracks.clear();
            
            for (const auto& trackVar : *tracksVar.getArray()) {
                if (auto* trackObj = trackVar.getDynamicObject()) {
                    TrackInfo track;
                    track.name = trackObj->hasProperty("name") ? trackObj->getProperty("name").toString() : "Unknown";
                    track.type = trackObj->hasProperty("type") ? trackObj->getProperty("type").toString() : "";
                    track.role = trackObj->hasProperty("role") ? trackObj->getProperty("role").toString() : "";
                    track.volume = trackObj->hasProperty("volume") ? static_cast<double>(trackObj->getProperty("volume")) : 0.0;
                    track.pan = trackObj->hasProperty("pan") ? static_cast<double>(trackObj->getProperty("pan")) : 0.0;
                    track.isMuted = trackObj->hasProperty("muted") ? static_cast<bool>(trackObj->getProperty("muted")) : false;
                    track.isSoloed = trackObj->hasProperty("soloed") ? static_cast<bool>(trackObj->getProperty("soloed")) : false;
                    track.effects = trackObj->hasProperty("effects") ? trackObj->getProperty("effects") : juce::var();
                    
                    // Analyze track audio if available
                    auto audioVar = trackObj->hasProperty("audioData") ? trackObj->getProperty("audioData") : juce::var();
                    if (audioVar.isBinaryData()) {
                        juce::MemoryBlock audioBlock;
                        if (auto* data = audioVar.getBinaryData())
                            audioBlock.replaceAll(data->getData(), data->getSize());
                        
                        // Create audio buffer from binary data
                        juce::AudioBuffer<float> audioBuffer;
                        // ... (implementation depends on audio format)
                        
                        // Perform visual analysis
                        track.visualAnalysis = visualAnalyzer->analyzeAudio(audioBuffer, projectInfo.sampleRate);
                    }
                    
                    // Auto-classify if not specified
                    if (track.type.isEmpty()) {
                        track.type = classifyTrackType(track);
                    }
                    if (track.role.isEmpty()) {
                        track.role = classifyTrackRole(track);
                    }
                    
                    projectInfo.tracks.push_back(track);
                }
            }
        }
        
        analyzeProjectStructure();
        analyzeTrackRelationships();
        generateContextualInsights();
        
        return true;
    }
    
    return false;
}

ContextualInsights ProjectContext::getContextualInsights() const {
    return insights;
}

void ProjectContext::analyzeProjectStructure() {
    // Count track types
    std::unordered_map<juce::String, int> typeCounts;
    std::unordered_map<juce::String, int> roleCounts;
    
    for (const auto& track : projectInfo.tracks) {
        typeCounts[track.type]++;
        roleCounts[track.role]++;
    }
    
    // Detect arrangement density
    int activeTracks = 0;
    for (const auto& track : projectInfo.tracks) {
        if (!track.isMuted && track.volume > -60.0) {
            activeTracks++;
        }
    }
    
    if (activeTracks <= 4) {
        insights.arrangementType = "minimal";
    } else if (activeTracks <= 8) {
        insights.arrangementType = "sparse";
    } else if (activeTracks <= 16) {
        insights.arrangementType = "moderate";
    } else {
        insights.arrangementType = "dense";
    }
    
    // Detect genre from track composition
    insights.overallGenre = detectGenre();
    
    // Detect production style
    insights.productionStyle = detectProductionStyle();
}

void ProjectContext::analyzeTrackRelationships() {
    // Analyze frequency relationships between tracks
    for (size_t i = 0; i < projectInfo.tracks.size(); ++i) {
        for (size_t j = i + 1; j < projectInfo.tracks.size(); ++j) {
            float overlap = calculateFrequencyOverlap(projectInfo.tracks[i], projectInfo.tracks[j]);
            
            // Store relationship data (implementation would use this for recommendations)
            if (overlap > 0.7f) {
                // High frequency overlap - potential masking
                insights.recommendations.push_back(
                    "Consider frequency separation between " + projectInfo.tracks[i].name + 
                    " and " + projectInfo.tracks[j].name);
            }
        }
    }
}

void ProjectContext::generateContextualInsights() {
    // Energy analysis
    insights.energyLevel = analyzeEnergyLevel();
    
    // Frequency balance
    insights.frequencyBalance = analyzeFrequencyBalance();
    
    // Generate context features for AI
    insights.contextFeatures = extractContextFeatures();
    
    // Generate additional recommendations
    auto freqRecs = getFrequencyRecommendations();
    auto dynRecs = getDynamicsRecommendations();
    
    insights.recommendations.insert(insights.recommendations.end(), 
                                   freqRecs.begin(), freqRecs.end());
    insights.recommendations.insert(insights.recommendations.end(), 
                                   dynRecs.begin(), dynRecs.end());
}

juce::String ProjectContext::classifyTrackType(const TrackInfo& track) const {
    // Use visual analysis and audio features to classify track type
    const auto& visual = track.visualAnalysis.waveform;
    
    // Kick drum: strong low frequency, fast attack, short decay
    if (visual.attackTime < 0.05f && visual.decayTime < 0.2f && 
        visual.crestFactor > 10.0f) {
        return "kick";
    }
    
    // Snare: mid frequency, fast attack, medium decay
    if (visual.attackTime < 0.01f && visual.decayTime < 0.3f && 
        visual.crestFactor > 8.0f) {
        return "snare";
    }
    
    // Bass: low-mid frequency, slower attack
    if (visual.attackTime > 0.01f && visual.crestFactor > 6.0f) {
        return "bass";
    }
    
    // Vocal: complex waveform, moderate dynamics
    if (visual.crestFactor < 8.0f && visual.dynamicRange < 3.0f) {
        return "vocal";
    }
    
    // Synth/Pads: slow attack, long decay
    if (visual.attackTime > 0.1f && visual.decayTime > 0.5f) {
        return "synth";
    }
    
    return "unknown";
}

juce::String ProjectContext::classifyTrackRole(const TrackInfo& track) const {
    // Determine track role based on type and characteristics
    if (track.type == "kick" || track.type == "snare") {
        return "rhythm";
    }
    
    if (track.type == "bass") {
        return "foundation";
    }
    
    if (track.type == "vocal") {
        return "lead";
    }
    
    if (track.type == "synth") {
        const auto& visual = track.visualAnalysis.waveform;
        if (visual.attackTime > 0.2f) {
            return "pad";
        } else {
            return "lead";
        }
    }
    
    return "support";
}

juce::String ProjectContext::detectGenre() const {
    // Analyze track composition and characteristics to detect genre
    std::unordered_map<juce::String, int> instrumentCounts;
    
    for (const auto& track : projectInfo.tracks) {
        instrumentCounts[track.type]++;
    }
    
    // Electronic music: synths, electronic drums
    if (instrumentCounts["synth"] > instrumentCounts["vocal"] && 
        instrumentCounts["kick"] > 0 && instrumentCounts["snare"] > 0) {
        return "electronic";
    }
    
    // Rock: guitars, drums, bass, vocals
    if (instrumentCounts["vocal"] > 0 && instrumentCounts["kick"] > 0 && 
        instrumentCounts["bass"] > 0) {
        return "rock";
    }
    
    // Hip-hop: strong rhythm, vocal focus
    if (instrumentCounts["vocal"] > 0 && instrumentCounts["kick"] > 0 &&
        projectInfo.tempo < 100.0) {
        return "hip-hop";
    }
    
    // Classical: orchestral instruments, complex arrangements
    if (projectInfo.tracks.size() > 20 && insights.arrangementType == "dense") {
        return "classical";
    }
    
    return "unknown";
}

juce::String ProjectContext::detectProductionStyle() const {
    // Analyze production characteristics
    float avgCrestFactor = 0.0f;
    int analyzedTracks = 0;
    
    for (const auto& track : projectInfo.tracks) {
        if (!track.isMuted && track.visualAnalysis.waveform.crestFactor > 0) {
            avgCrestFactor += track.visualAnalysis.waveform.crestFactor;
            analyzedTracks++;
        }
    }
    
    if (analyzedTracks > 0) {
        avgCrestFactor /= analyzedTracks;
    }
    
    // Modern: lower crest factor (more compression)
    if (avgCrestFactor < 6.0f) {
        return "modern";
    }
    
    // Vintage: higher crest factor (more dynamic)
    if (avgCrestFactor > 10.0f) {
        return "vintage";
    }
    
    // Electronic: based on track types
    if (insights.overallGenre == "electronic") {
        return "electronic";
    }
    
    return "contemporary";
}

juce::String ProjectContext::detectArrangementType() const {
    return insights.arrangementType; // Already calculated in analyzeProjectStructure
}

juce::String ProjectContext::analyzeFrequencyBalance() const {
    // Analyze overall frequency balance across all tracks
    float lowEnergy = 0.0f, midEnergy = 0.0f, highEnergy = 0.0f;
    int trackCount = 0;
    
    for (const auto& track : projectInfo.tracks) {
        if (!track.isMuted && track.visualAnalysis.spectral.spectralCentroid > 0) {
            float centroid = track.visualAnalysis.spectral.spectralCentroid;
            
            if (centroid < 500.0f) {
                lowEnergy += track.volume;
            } else if (centroid < 4000.0f) {
                midEnergy += track.volume;
            } else {
                highEnergy += track.volume;
            }
            trackCount++;
        }
    }
    
    if (trackCount == 0) return "balanced";
    
    // Normalize
    lowEnergy /= trackCount;
    midEnergy /= trackCount;
    highEnergy /= trackCount;
    
    // Determine balance
    if (lowEnergy > midEnergy + 3.0f && lowEnergy > highEnergy + 3.0f) {
        return "bass-heavy";
    } else if (highEnergy > midEnergy + 3.0f && highEnergy > lowEnergy + 3.0f) {
        return "bright";
    } else if (midEnergy > lowEnergy + 3.0f && midEnergy > highEnergy + 3.0f) {
        return "mid-focused";
    }
    
    return "balanced";
}

std::vector<juce::String> ProjectContext::getFrequencyRecommendations() const {
    std::vector<juce::String> recommendations;
    
    if (insights.frequencyBalance == "bass-heavy") {
        recommendations.push_back("Consider reducing low frequencies or adding high-frequency content");
    } else if (insights.frequencyBalance == "bright") {
        recommendations.push_back("Consider reducing high frequencies or adding warmth");
    } else if (insights.frequencyBalance == "mid-focused") {
        recommendations.push_back("Consider enhancing low and high frequency content");
    }
    
    return recommendations;
}

juce::String ProjectContext::analyzeEnergyLevel() const {
    float totalEnergy = 0.0f;
    int trackCount = 0;
    
    for (const auto& track : projectInfo.tracks) {
        if (!track.isMuted && track.volume > -60.0) {
            totalEnergy += track.volume;
            trackCount++;
        }
    }
    
    if (trackCount == 0) return "low";
    
    float avgEnergy = totalEnergy / trackCount;
    
    if (avgEnergy > -6.0f) return "high";
    if (avgEnergy > -12.0f) return "medium";
    return "low";
}

std::vector<juce::String> ProjectContext::getDynamicsRecommendations() const {
    std::vector<juce::String> recommendations;
    
    if (insights.energyLevel == "high") {
        recommendations.push_back("Consider dynamic range processing to add movement");
    } else if (insights.energyLevel == "low") {
        recommendations.push_back("Consider increasing overall energy or adding dynamic contrast");
    }
    
    return recommendations;
}

float ProjectContext::calculateFrequencyOverlap(const TrackInfo& track1, const TrackInfo& track2) const {
    // Simplified frequency overlap calculation
    float centroid1 = track1.visualAnalysis.spectral.spectralCentroid;
    float centroid2 = track2.visualAnalysis.spectral.spectralCentroid;
    
    if (centroid1 <= 0 || centroid2 <= 0) return 0.0f;
    
    float ratio = std::min(centroid1, centroid2) / std::max(centroid1, centroid2);
    return ratio;
}

juce::var ProjectContext::extractContextFeatures() const {
    auto features = new juce::DynamicObject();
    
    // Project metadata
    features->setProperty("projectName", projectInfo.name);
    features->setProperty("tempo", projectInfo.tempo);
    features->setProperty("sampleRate", projectInfo.sampleRate);
    features->setProperty("trackCount", static_cast<int>(projectInfo.tracks.size()));
    
    // Arrangement characteristics
    features->setProperty("arrangementType", insights.arrangementType);
    features->setProperty("energyLevel", insights.energyLevel);
    features->setProperty("frequencyBalance", insights.frequencyBalance);
    features->setProperty("genre", insights.overallGenre);
    features->setProperty("productionStyle", insights.productionStyle);
    
    // Track composition
    auto trackTypes = new juce::DynamicObject();
    for (const auto& track : projectInfo.tracks) {
        juce::String type = track.type;
        int count = trackTypes->hasProperty(type) ? static_cast<int>(trackTypes->getProperty(type)) : 0;
        trackTypes->setProperty(type, count + 1);
    }
    features->setProperty("trackTypes", juce::var(trackTypes));
    
    return juce::var(features);
}

void ProjectContext::updateFromLearning(const GrokJourney& journey) {
    // Update project insights based on learned preferences
    // This would integrate with the learning system to refine context understanding
}

void ProjectContext::storeContextualLearning(const ContextualInsights& insights, double userSatisfaction) {
    // Store contextual learning for future reference
    // Implementation would save this to the learning database
}

} // namespace ai
} // namespace zenith
