/*
  ==============================================================================
    CreativePartner.cpp
    AI creative partnership implementation
  ==============================================================================
*/

#include "CreativePartner.h"
#include <algorithm>
#include <random>
#include <cmath>

namespace zenith {
namespace ai {

CreativePartner::CreativePartner() {
    visualAnalyzer = std::make_unique<VisualAnalyzer>();
    genreDetector = std::make_unique<GenreDetector>();
    
    // Initialize genre expertise
    genreExpertise["electronic"] = 0.9f;
    genreExpertise["rock"] = 0.8f;
    genreExpertise["hip-hop"] = 0.85f;
    genreExpertise["pop"] = 0.75f;
    genreExpertise["classical"] = 0.7f;
    genreExpertise["jazz"] = 0.8f;
}

CreativePartner::~CreativePartner() = default;

CreativeInsight CreativePartner::analyzeCreatively(const juce::AudioBuffer<float>& audio,
                                                  const ProjectContext& context,
                                                  const GenrePrediction& genre) {
    CreativeInsight insight;
    
    // Perform visual analysis
    auto visualResult = visualAnalyzer->analyzeAudio(audio, context.getProjectInfo().sampleRate);
    
    // Generate observations
    insight.observation = analyzeArtisticIntent(visualResult, context);
    insight.explanation = analyzeProductionTechniques(visualResult, genre);
    insight.creativeAdvice = generateCreativeResponse("What creative direction should I take?", insight);
    auto advice = getGenreSpecificAdvice(genre.genre);
    insight.genreContext = juce::StringArray(advice.data(), (int)advice.size()).joinIntoString(", ");
    
    // Generate suggestions
    insight.suggestions = generateSuggestions(insight);
    
    return insight;
}

std::vector<CreativeSuggestion> CreativePartner::generateSuggestions(const CreativeInsight& insight) {
    std::vector<CreativeSuggestion> suggestions;
    
    // Generate EQ suggestion
    // (This would use the visual analysis data - simplified for this example)
    CreativeSuggestion eqSuggestion;
    eqSuggestion.type = "eq";
    eqSuggestion.action = "boost";
    eqSuggestion.target = "high frequencies";
    eqSuggestion.parameters = "8kHz +2dB with Q=0.7";
    eqSuggestion.reasoning = "Adding air and presence to enhance clarity";
    eqSuggestion.confidence = 0.8f;
    eqSuggestion.context = "Based on spectral analysis showing slight high-frequency roll-off";
    suggestions.push_back(eqSuggestion);
    
    // Generate compression suggestion
    CreativeSuggestion compSuggestion;
    compSuggestion.type = "compression";
    compSuggestion.action = "add";
    compSuggestion.target = "master bus";
    compSuggestion.parameters = "ratio 2:1, threshold -18dB, attack 5ms";
    compSuggestion.reasoning = "Gentle glue compression to bring elements together";
    compSuggestion.confidence = 0.7f;
    compSuggestion.context = "Dynamic range analysis suggests moderate compression would help";
    suggestions.push_back(compSuggestion);
    
    // Generate arrangement suggestion
    CreativeSuggestion arrSuggestion;
    arrSuggestion.type = "arrangement";
    arrSuggestion.action = "adjust";
    arrSuggestion.target = "instrument balance";
    arrSuggestion.parameters = "reduce rhythm section by 1.5dB, increase vocals by 1dB";
    arrSuggestion.reasoning = "Better balance for vocal clarity while maintaining energy";
    arrSuggestion.confidence = 0.75f;
    arrSuggestion.context = "Frequency analysis shows slight masking in mid-range";
    suggestions.push_back(arrSuggestion);
    
    // Sort by confidence
    std::sort(suggestions.begin(), suggestions.end(),
              [](const CreativeSuggestion& a, const CreativeSuggestion& b) {
                  return a.confidence > b.confidence;
              });
    
    return suggestions;
}

CreativeSuggestion CreativePartner::suggestEQ(const VisualAnalysisResult& visual, 
                                             const ProjectContext& context) {
    CreativeSuggestion suggestion;
    suggestion.type = "eq";
    
    // Analyze spectral content - use direct struct access
    float spectralCentroid = visual.spectral.spectralCentroid > 0.0f ? visual.spectral.spectralCentroid : 2000.0f;
    
    if (spectralCentroid < 1500.0f) {
        // Dark sound - suggest high-frequency boost
        suggestion.action = "boost";
        suggestion.target = "air frequencies";
        suggestion.parameters = "10kHz +3dB, shelving";
        suggestion.reasoning = "Adding brightness to counteract dark tonal balance";
        suggestion.confidence = 0.8f;
    } else if (spectralCentroid > 4000.0f) {
        // Bright sound - suggest low-frequency warmth
        suggestion.action = "boost";
        suggestion.target = "low-mid warmth";
        suggestion.parameters = "200Hz +2dB, Q=0.5";
        suggestion.reasoning = "Adding warmth to balance bright high frequencies";
        suggestion.confidence = 0.75f;
    } else {
        // Balanced - suggest subtle enhancement
        suggestion.action = "enhance";
        suggestion.target = "presence";
        suggestion.parameters = "3kHz +1.5dB, Q=1.0";
        suggestion.reasoning = "Subtle presence enhancement for vocal clarity";
        suggestion.confidence = 0.6f;
    }
    
    return suggestion;
}

CreativeSuggestion CreativePartner::suggestCompression(const VisualAnalysisResult& visual,
                                                      const ProjectContext& context) {
    CreativeSuggestion suggestion;
    suggestion.type = "compression";
    
    float dynamicRange = visual.waveform.dynamicRange;
    float crestFactor = visual.waveform.crestFactor;
    
    if (dynamicRange > 6.0f) {
        // High dynamic range - suggest moderate compression
        suggestion.action = "add";
        suggestion.target = "master bus";
        suggestion.parameters = "ratio 3:1, threshold -20dB, attack 10ms, release 100ms";
        suggestion.reasoning = "Reducing dynamic range for better commercial compatibility";
        suggestion.confidence = 0.85f;
    } else if (dynamicRange < 2.0f) {
        // Low dynamic range - suggest parallel compression
        suggestion.action = "add";
        suggestion.target = "parallel compression";
        suggestion.parameters = "ratio 4:1, threshold -30dB, mix 30%";
        suggestion.reasoning = "Adding dynamics through parallel compression while preserving transients";
        suggestion.confidence = 0.8f;
    } else {
        // Moderate dynamic range - suggest gentle compression
        suggestion.action = "add";
        suggestion.target = "bus compression";
        suggestion.parameters = "ratio 2:1, threshold -18dB, attack 5ms, release 50ms";
        suggestion.reasoning = "Gentle glue compression to enhance cohesion";
        suggestion.confidence = 0.7f;
    }
    
    return suggestion;
}

CreativeSuggestion CreativePartner::suggestReverb(const VisualAnalysisResult& visual,
                                                const ProjectContext& context) {
    CreativeSuggestion suggestion;
    suggestion.type = "reverb";
    
    // Suggest reverb based on project context
    juce::String arrangementType = context.getContextualInsights().arrangementType;
    
    if (arrangementType == "minimal" || arrangementType == "sparse") {
        suggestion.action = "add";
        suggestion.target = "ambience";
        suggestion.parameters = "hall reverb, decay 2.5s, predelay 40ms, wet 15%";
        suggestion.reasoning = "Creating space in sparse arrangement";
        suggestion.confidence = 0.8f;
    } else if (arrangementType == "dense") {
        suggestion.action = "add";
        suggestion.target = "depth";
        suggestion.parameters = "plate reverb, decay 1.2s, predelay 20ms, wet 8%";
        suggestion.reasoning = "Subtle depth enhancement without cluttering dense mix";
        suggestion.confidence = 0.75f;
    } else {
        suggestion.action = "enhance";
        suggestion.target = "spatial imaging";
        suggestion.parameters = "room reverb, decay 1.8s, early reflections 25%, wet 12%";
        suggestion.reasoning = "Adding natural space and imaging";
        suggestion.confidence = 0.7f;
    }
    
    return suggestion;
}

CreativeSuggestion CreativePartner::suggestArrangement(const ProjectContext& context) {
    CreativeSuggestion suggestion;
    suggestion.type = "arrangement";
    
    auto insights = context.getContextualInsights();
    juce::String energyLevel = insights.energyLevel;
    juce::String frequencyBalance = insights.frequencyBalance;
    
    if (energyLevel == "low") {
        suggestion.action = "increase";
        suggestion.target = "energy";
        suggestion.parameters = "boost rhythm section by 2dB, add subtle saturation";
        suggestion.reasoning = "Increasing energy to engage listeners";
        suggestion.confidence = 0.8f;
    } else if (energyLevel == "high") {
        suggestion.action = "create";
        suggestion.target = "dynamic contrast";
        suggestion.parameters = "reduce chorus levels by 3dB, automate verse builds";
        suggestion.reasoning = "Creating dynamic interest through contrast";
        suggestion.confidence = 0.85f;
    }
    
    if (frequencyBalance == "bass-heavy") {
        suggestion.action = "balance";
        suggestion.target = "frequency spectrum";
        suggestion.parameters = "cut 60-80Hz by 2dB, boost 8kHz by 1.5dB";
        suggestion.reasoning = "Balancing bass-heavy mix for better translation";
        suggestion.confidence = 0.75f;
    }
    
    return suggestion;
}

juce::String CreativePartner::explainReasoning(const CreativeSuggestion& suggestion) {
    juce::String explanation = "I suggested " + suggestion.action + " on " + suggestion.target + " because:\n\n";
    explanation += suggestion.reasoning + "\n\n";
    
    explanation += "Technical context: " + suggestion.context + "\n\n";
    
    // Add genre-specific explanation
    if (suggestion.type == "eq") {
        explanation += "EQ adjustments affect the frequency balance and can enhance clarity, warmth, or air depending on the target frequencies. ";
        explanation += "The parameters I suggested are carefully chosen to address specific characteristics in your audio.";
    } else if (suggestion.type == "compression") {
        explanation += "Compression controls dynamic range by reducing the difference between loud and quiet parts. ";
        explanation += "The settings I recommended balance transparency with effective dynamic control.";
    } else if (suggestion.type == "reverb") {
        explanation += "Reverb creates spatial context and can make tracks feel more cohesive or add depth. ";
        explanation += "The parameters I chose will enhance the sense of space without overwhelming the mix.";
    } else if (suggestion.type == "arrangement") {
        explanation += "Arrangement adjustments affect the overall balance and flow of the music. ";
        explanation += "My suggestions aim to improve the musical impact and listener engagement.";
    }
    
    return explanation;
}

juce::String CreativePartner::explainGenreContext(const juce::String& genre, const juce::String& suggestion) {
    juce::String explanation = "In " + genre + " music:\n\n";
    
    if (genre == "electronic") {
        explanation += "Precision and impact are key. EQ should maintain clarity between electronic elements. ";
        explanation += "Compression often uses faster attack times to control transients. ";
        explanation += "Reverb is typically used more subtly to maintain the electronic aesthetic.";
    } else if (genre == "rock") {
        explanation += "Energy and attitude are paramount. EQ often emphasizes mid-range for instrument presence. ";
        explanation += "Compression helps glue the band while preserving performance dynamics. ";
        explanation += "Reverb creates space without losing the raw rock feel.";
    } else if (genre == "hip-hop") {
        explanation += "Rhythm and vocal clarity are crucial. Low-end control is essential for sub-bass and kick. ";
        explanation += "Compression shapes the vocal delivery and controls dynamics. ";
        explanation += "Reverb adds space while maintaining rhythmic precision.";
    } else if (genre == "pop") {
        explanation += "Polish and accessibility drive production decisions. EQ creates vocal clarity and commercial brightness. ";
        explanation += "Compression ensures consistent levels and radio readiness. ";
        explanation += "Reverb adds professional polish without overwhelming the arrangement.";
    }
    
    explanation += "\n\nMy suggestion aligns with these " + genre + " production conventions while addressing your specific audio characteristics.";
    
    return explanation;
}

juce::String CreativePartner::respondToQuestion(const juce::String& question,
                                                  const CreativeInsight& currentInsight) {
    // Simple pattern matching for common questions
    if (question.containsIgnoreCase("eq") || question.containsIgnoreCase("frequency")) {
        return explainTechnicalConcept("EQ");
    } else if (question.containsIgnoreCase("compress") || question.containsIgnoreCase("dynamic")) {
        return explainTechnicalConcept("Compression");
    } else if (question.containsIgnoreCase("reverb") || question.containsIgnoreCase("space")) {
        return explainTechnicalConcept("Reverb");
    } else if (question.containsIgnoreCase("why") || question.containsIgnoreCase("reason")) {
        return "My suggestions are based on analyzing your audio's spectral content, dynamic characteristics, and production context. "
               "I consider genre conventions, technical requirements, and creative intent to provide recommendations that enhance your music.";
    } else if (question.containsIgnoreCase("genre")) {
        return "Genre context is crucial because different styles have different production expectations. "
               "I adapt my suggestions based on the detected genre to ensure they're stylistically appropriate.";
    } else {
        return "That's an interesting question! Based on my analysis, I notice " + currentInsight.observation + ". "
               "This suggests we should focus on " + currentInsight.creativeAdvice + ". "
               "Would you like me to elaborate on any specific aspect?";
    }
}

juce::String CreativePartner::explainTechnicalConcept(const juce::String& topicName) {
    if (topicName == "EQ") {
        return "EQ (Equalization) shapes frequency content by boosting or cutting specific frequency bands. "
               "Key parameters: Gain adjusts level, Frequency sets target band, Q controls width. "
               "It helps balance mixes and create space for instruments.";
    } else if (topicName == "Compression") {
        return "Compression reduces dynamic range by attenuating signals above a threshold. "
               "Key parameters: Threshold sets activation level, Ratio controls reduction amount, "
               "Attack determines response speed, Release controls recovery time. "
               "It creates consistency and punch.";
    } else if (topicName == "Reverb") {
        return "Reverb simulates acoustic spaces by creating many delayed, filtered copies of a sound. "
               "Key parameters: Decay time controls reverb length, Pre-delay adds gap before reverb starts, "
               "Early reflections simulate first room reflections, Wet/Dry mixes reverb with dry signal. "
               "It creates space, depth, and cohesion when used appropriately.";
    }
    
    return "I'd be happy to explain that concept in more detail. Could you specify what aspect you'd like to understand better?";
}

void CreativePartner::updateFromUserFeedback(const CreativeSuggestion& suggestion, 
                                             bool wasHelpful, 
                                             const juce::String& userComment) {
    updateSuggestionConfidence(suggestion, wasHelpful);
    
    // Store feedback for learning (would integrate with GrokJourney)
    if (wasHelpful) {
        // Positive feedback reinforces this type of suggestion
    } else {
        // Negative feedback reduces confidence and triggers refinement
        refineGenreKnowledge("", userComment);
    }
}

void CreativePartner::setCreativityLevel(float level) {
    creativityLevel = juce::jlimit(0.0f, 1.0f, level);
}

void CreativePartner::setGenreExpertise(const juce::String& genre, float expertise) {
    genreExpertise[genre] = juce::jlimit(0.0f, 1.0f, expertise);
}

// Private helper methods
juce::String CreativePartner::analyzeArtisticIntent(const VisualAnalysisResult& visual,
                                                   const ProjectContext& context) {
    juce::String intent = "I detect ";
    
    float crestFactor = visual.waveform.crestFactor;
    float dynamicRange = visual.waveform.dynamicRange;
    
    if (crestFactor > 10.0f && dynamicRange > 5.0f) {
        intent += "high dynamic range with strong transients, suggesting an energetic, impactful mix";
    } else if (crestFactor < 6.0f && dynamicRange < 3.0f) {
        intent += "controlled dynamics with smooth transients, suggesting a polished, commercial sound";
    } else {
        intent += "moderate dynamics with balanced transients, suggesting versatility and musicality";
    }
    
    intent += ". The spectral characteristics indicate ";
    
    float spectralCentroid = visual.spectral.spectralCentroid > 0.0f ? visual.spectral.spectralCentroid : 2000.0f;
    
    if (spectralCentroid > 3000.0f) {
        intent += "bright, detailed high-frequency content";
    } else if (spectralCentroid < 1500.0f) {
        intent += "warm, low-mid focused content";
    } else {
        intent += "balanced frequency content across the spectrum";
    }
    
    return intent + ".";
}

juce::String CreativePartner::analyzeProductionTechniques(const VisualAnalysisResult& visual,
                                                         const GenrePrediction& genre) {
    juce::String techniques = "The production shows ";
    
    // Analyze compression characteristics
    float crestFactor = visual.waveform.crestFactor;
    if (crestFactor < 8.0f) {
        techniques += "evidence of compression or limiting, creating controlled dynamics";
    } else {
        techniques += "natural dynamics with minimal compression, preserving performance nuance";
    }
    
    techniques += ". Frequency analysis reveals ";
    
    float spectralCentroid = visual.spectral.spectralCentroid > 0.0f ? visual.spectral.spectralCentroid : 2000.0f;
    
    if (spectralCentroid > 2500.0f) {
        techniques += "bright EQ treatment or high-frequency enhancement";
    } else if (spectralCentroid < 2000.0f) {
        techniques += "warm EQ curve with possible low-frequency emphasis";
    } else {
        techniques += "balanced EQ treatment with natural frequency response";
    }
    
    techniques += ", which aligns with " + genre.genre + " production conventions.";
    
    return techniques;
}

std::vector<juce::String> CreativePartner::getGenreSpecificAdvice(const juce::String& genre) {
    std::vector<juce::String> advice;
    
    if (genre == "electronic") {
        advice.push_back("Focus on low-frequency control and transient clarity");
        advice.push_back("Maintain separation between electronic elements");
        advice.push_back("Use compression for punch and impact");
    } else if (genre == "rock") {
        advice.push_back("Preserve performance dynamics while adding control");
        advice.push_back("Focus on mid-range presence for instrument character");
        advice.push_back("Use compression to glue the band without losing energy");
    } else if (genre == "hip-hop") {
        advice.push_back("Prioritize vocal clarity and rhythm section punch");
        advice.push_back("Control low frequencies for sub-bass and kick compatibility");
        advice.push_back("Use parallel processing for modern character");
    } else if (genre == "pop") {
        advice.push_back("Focus on vocal brightness and commercial appeal");
        advice.push_back("Use compression for consistent levels and radio readiness");
        advice.push_back("Create space for each element to shine");
    }
    
    return advice;
}

void CreativePartner::updateSuggestionConfidence(const CreativeSuggestion& suggestion, bool wasHelpful) {
    // Update confidence based on user feedback
    // In practice, this would modify internal models
}

void CreativePartner::refineGenreKnowledge(const juce::String& genre, const juce::String& feedback) {
    // Refine genre-specific knowledge based on feedback
    // In practice, this would update the learning system
}

juce::String CreativePartner::generateCreativeResponse(const juce::String& question, const CreativeInsight& insight) {
    // Generate creative, contextual responses
    if (question.containsIgnoreCase("direction") || question.containsIgnoreCase("what should")) {
        return "Based on my analysis, I'd recommend focusing on " + insight.genreContext + ". "
               "Your track shows " + insight.observation + ", which suggests we should enhance "
               + insight.creativeAdvice + ". This will help achieve a more polished and impactful result.";
    }
    
    return "Let me analyze your audio and provide creative guidance based on what I hear...";
}

} // namespace ai
} // namespace zenith
