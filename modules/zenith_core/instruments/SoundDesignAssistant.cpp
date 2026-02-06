/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

    ==============================================================================

    SoundDesignAssistant.cpp
    Created: 2025-02-05
    Author:  Zenith DAW

    Implementation of AI-powered sound design assistant.

    ==============================================================================
*/

#include "SoundDesignAssistant.h"
#include <juce_core/juce_core.h>
#include <random>
#include <algorithm>
#include <cmath>

namespace zenith {

//==============================================================================
// Construction/Destruction
//==============================================================================

SoundDesignAssistant::SoundDesignAssistant() {
    // Initialize preference weights
    preferenceWeights_.resize(32, 0.5f); // Default neutral weights

    // Initialize category tracking
    recentCategories_.ensureStorageAllocated(10);

    // Create timbre analyzer
    timbreAnalyzer_ = std::make_unique<TimbreTransfer>();

    DBG("SoundDesignAssistant: Initialized");
}

SoundDesignAssistant::~SoundDesignAssistant() {
    release();
}

//==============================================================================
// Initialization
//==============================================================================

bool SoundDesignAssistant::initialize(const juce::File& configPath) {
    configPath_ = configPath;

    // Create config directory if needed
    if (!configPath_.exists()) {
        configPath_.createDirectory();
    }

    // Initialize timbre analyzer
    if (timbreAnalyzer_) {
        timbreAnalyzer_->initialize(44100.0, 512);
    }

    // Load existing data
    loadPresets();
    loadLearningData();

    initialized_ = true;
    DBG("SoundDesignAssistant: Ready");

    return true;
}

void SoundDesignAssistant::release() {
    juce::ScopedLock scopedLock(lock_);

    // Save data
    savePresets();
    saveLearningData();

    timbreAnalyzer_.reset();
    initialized_ = false;
}

//==============================================================================
// Suggestion Generation
//==============================================================================

SuggestionResult SoundDesignAssistant::getSuggestions(const juce::String& description) {
    juce::ScopedLock scopedLock(lock_);

    SuggestionResult result;

    // Parse description
    auto keywords = parseDescription(description);

    // Extract characteristics
    auto baseParams = extractCharacteristics(keywords);

    // Apply learned preferences
    auto personalizedParams = applyLearnedWeights(baseParams);

    result.parameters = personalizedParams;
    result.description = "Generated from: " + description;
    result.confidence = 0.7f; // Base confidence
    result.isNeuralSuggestion = false;
    result.requiresNeuralModel = false;

    // Add tags based on keywords
    for (const auto& keyword : keywords) {
        if (keyword.isNotEmpty() && keyword.length() > 2) {
            result.tags.add(keyword);
        }
    }

    return result;
}

SuggestionResult SoundDesignAssistant::getSuggestionsForCategory(SoundCategory category,
                                                                 float variation) {
    juce::ScopedLock scopedLock(lock_);

    SuggestionResult result;

    // Get base parameters for category
    auto baseParams = getDefaultForCategory(category);

    // Add variation if requested
    if (variation > 0.0f) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::normal_distribution<float> dist(0.0f, variation);

        baseParams.brightness = juce::jlimit(0.0f, 1.0f, baseParams.brightness + dist(gen));
        baseParams.warmth = juce::jlimit(0.0f, 1.0f, baseParams.warmth + dist(gen));
        baseParams.richness = juce::jlimit(0.0f, 1.0f, baseParams.richness + dist(gen));
        baseParams.attack = juce::jlimit(0.001f, 2.0f, baseParams.attack * (1.0f + dist(gen) * 0.5f));
        baseParams.decay = juce::jlimit(0.01f, 2.0f, baseParams.decay * (1.0f + dist(gen) * 0.5f));
        baseParams.filterCutoff = juce::jlimit(100.0f, 10000.0f,
                                            baseParams.filterCutoff * (1.0f + dist(gen) * 0.3f));
    }

    // Apply learned preferences
    result.parameters = applyLearnedWeights(baseParams);
    result.description = "Generated for: " + categoryToString(category);
    result.confidence = 0.8f;
    result.isNeuralSuggestion = false;
    result.requiresNeuralModel = false;
    result.tags.add(categoryToString(category));

    // Track category usage
    recentCategories_.add((int)category);
    if (recentCategories_.size() > 10) {
        recentCategories_.remove(0);
    }

    return result;
}

SuggestionResult SoundDesignAssistant::getSuggestionsFromReference(const juce::File& referenceAudio) {
    juce::ScopedLock scopedLock(lock_);

    SuggestionResult result;

    if (!timbreAnalyzer_ || !referenceAudio.existsAsFile()) {
        result.parameters = getDefaultForCategory(SoundCategory::Lead);
        result.confidence = 0.0f;
        return result;
    }

    // Analyze reference audio
    auto timbre = timbreAnalyzer_->analyzeTimbreFromFile(referenceAudio);

    if (!timbre.valid) {
        result.parameters = getDefaultForCategory(SoundCategory::Lead);
        result.confidence = 0.0f;
        return result;
    }

    // Convert timbre analysis to parameters
    SoundDesignParams params;

    // Map spectral centroid to brightness
    if (!timbre.spectralCentroid.empty()) {
        float centroid = timbre.spectralCentroid[0];
        params.brightness = juce::jmap(centroid, 200.0f, 8000.0f, 0.0f, 1.0f);
    }

    // Map harmonicity to warmth/richness
    params.warmth = timbre.harmonicity;
    params.richness = 1.0f - timbre.inharmonicity;

    // Map spectral contrast to filter resonance
    if (!timbre.spectralContrast.empty()) {
        params.filterResonance = timbre.spectralContrast[0] * 0.7f;
    }

    // Map RMS to overall level (via attack/decay)
    params.attack = juce::jmap(timbre.rms, 0.0f, 0.5f, 0.01f, 0.1f);
    params.decay = juce::jmap(timbre.rms, 0.0f, 0.5f, 0.1f, 0.5f);

    // Classify category
    auto category = classifySound(params);

    result.parameters = applyLearnedWeights(params);
    result.description = "Generated from reference: " + referenceAudio.getFileName();
    result.confidence = 0.85f;
    result.isNeuralSuggestion = true;
    result.requiresNeuralModel = false;
    result.tags.add(categoryToString(category));
    result.tags.add("from_reference");

    return result;
}

SuggestionResult SoundDesignAssistant::exploreVariations(const SoundDesignParams& currentParams,
                                                        const juce::String& direction,
                                                        float amount) {
    juce::ScopedLock scopedLock(lock_);

    SuggestionResult result;
    result.parameters = currentParams;

    // Parse direction
    juce::String dir = direction.toLowerCase();

    if (dir.contains("bright") || dir.contains("brighter")) {
        result.parameters.brightness = juce::jmin(1.0f, currentParams.brightness + amount);
        result.parameters.warmth = juce::jmax(0.0f, currentParams.warmth - amount * 0.3f);
        result.parameters.filterCutoff = juce::jmin(10000.0f, currentParams.filterCutoff * (1.0f + amount));
        result.description = "Brighter variation";
    }
    else if (dir.contains("warm") || dir.contains("warmer")) {
        result.parameters.warmth = juce::jmin(1.0f, currentParams.warmth + amount);
        result.parameters.brightness = juce::jmax(0.0f, currentParams.brightness - amount * 0.3f);
        result.parameters.filterCutoff = juce::jmax(100.0f, currentParams.filterCutoff * (1.0f - amount * 0.3f));
        result.description = "Warmer variation";
    }
    else if (dir.contains("rich") || dir.contains("richer")) {
        result.parameters.richness = juce::jmin(1.0f, currentParams.richness + amount);
        result.parameters.harmonicity = juce::jmin(1.0f, currentParams.harmonicity + amount * 0.5f);
        result.description = "Richer variation";
    }
    else if (dir.contains("soft") || dir.contains("softer")) {
        result.parameters.attack = juce::jmax(0.001f, currentParams.attack * (1.0f - amount * 0.5f));
        result.parameters.distortionAmount = juce::jmax(0.0f, currentParams.distortionAmount - amount);
        result.parameters.filterResonance = juce::jmax(0.0f, currentParams.filterResonance - amount * 0.5f);
        result.description = "Softer variation";
    }
    else if (dir.contains("sharp") || dir.contains("sharper")) {
        result.parameters.attack = juce::jmin(2.0f, currentParams.attack * (1.0f + amount));
        result.parameters.filterResonance = juce::jmin(1.0f, currentParams.filterResonance + amount * 0.5f);
        result.parameters.brightness = juce::jmin(1.0f, currentParams.brightness + amount * 0.3f);
        result.description = "Sharper variation";
    }
    else if (dir.contains("modulate") || dir.contains("modulation")) {
        result.parameters.vibratoDepth = juce::jmin(1.0f, currentParams.vibratoDepth + amount);
        result.parameters.tremoloDepth = juce::jmin(1.0f, currentParams.tremoloDepth + amount * 0.5f);
        result.parameters.complexity = juce::jmin(1.0f, currentParams.complexity + amount * 0.3f);
        result.description = "More modulation";
    }
    else {
        // Random exploration
        std::random_device rd;
        std::mt19937 gen(rd());
        std::normal_distribution<float> dist(0.0f, amount * 0.5f);

        result.parameters.brightness = juce::jlimit(0.0f, 1.0f, currentParams.brightness + dist(gen));
        result.parameters.warmth = juce::jlimit(0.0f, 1.0f, currentParams.warmth + dist(gen));
        result.parameters.richness = juce::jlimit(0.0f, 1.0f, currentParams.richness + dist(gen));
        result.description = "Random exploration";
    }

    result.confidence = 0.6f;
    result.isNeuralSuggestion = false;
    result.requiresNeuralModel = false;
    result.tags.add("variation");

    return result;
}

//==============================================================================
// Machine Learning
//==============================================================================

void SoundDesignAssistant::provideFeedback(const SuggestionResult& result, float rating) {
    juce::ScopedLock scopedLock(lock_);

    UserPreference pref;
    pref.params = result.parameters;
    pref.rating = rating;
    pref.context = result.description;
    pref.timestamp = juce::Time::getMillisecondCounter();

    userPreferences_.add(pref);

    // Keep only recent history
    if (userPreferences_.size() > 1000) {
        userPreferences_.remove(0);
    }

    // Update preference model
    updatePreferenceModel(pref);

    // Save periodically
    if (userPreferences_.size() % 10 == 0) {
        saveLearningData();
    }

    DBG("SoundDesignAssistant: Recorded feedback - Rating: " + juce::String(rating));
}

void SoundDesignAssistant::learnFromPreset(const juce::String& presetName,
                                          const SoundDesignParams& params,
                                          float rating) {
    juce::ScopedLock scopedLock(lock_);

    UserPreference pref;
    pref.params = params;
    pref.rating = rating;
    pref.context = "Preset: " + presetName;
    pref.timestamp = juce::Time::getMillisecondCounter();

    userPreferences_.add(pref);

    updatePreferenceModel(pref);

    DBG("SoundDesignAssistant: Learned from preset '" + presetName + "' - Rating: " + juce::String(rating));
}

SuggestionResult SoundDesignAssistant::getPersonalizedSuggestion() {
    juce::ScopedLock scopedLock(lock_);

    if (userPreferences_.isEmpty()) {
        // No learning data yet, return default
        return getSuggestionsForCategory(SoundCategory::Lead);
    }

    // Generate personalized suggestion based on preferences
    auto result = generatePersonalizedSuggestion();

    return result;
}

//==============================================================================
// Preset Generation
//==============================================================================

SoundDesignParams SoundDesignAssistant::generatePreset(SoundCategory baseCategory,
                                                      float creativity) {
    juce::ScopedLock scopedLock(lock_);

    // Get base parameters
    auto params = getDefaultForCategory(baseCategory);

    // Apply creativity (amount of random variation)
    if (creativity > 0.0f) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::normal_distribution<float> dist(0.0f, creativity);

        // Vary all parameters
        params.brightness = juce::jlimit(0.0f, 1.0f, params.brightness + dist(gen));
        params.warmth = juce::jlimit(0.0f, 1.0f, params.warmth + dist(gen));
        params.richness = juce::jlimit(0.0f, 1.0f, params.richness + dist(gen));
        params.attack = juce::jlimit(0.001f, 2.0f, params.attack * (1.0f + dist(gen) * 0.5f));
        params.decay = juce::jlimit(0.01f, 2.0f, params.decay * (1.0f + dist(gen) * 0.5f));
        params.sustain = juce::jlimit(0.0f, 1.0f, params.sustain + dist(gen) * 0.3f);
        params.release = juce::jlimit(0.01f, 3.0f, params.release * (1.0f + dist(gen) * 0.5f));

        params.vibratoRate = juce::jlimit(0.5f, 15.0f, params.vibratoRate * (1.0f + dist(gen) * 0.5f));
        params.vibratoDepth = juce::jlimit(0.0f, 1.0f, params.vibratoDepth + dist(gen) * 0.3f);

        params.filterCutoff = juce::jlimit(100.0f, 10000.0f, params.filterCutoff * (1.0f + dist(gen) * 0.5f));
        params.filterResonance = juce::jlimit(0.0f, 1.0f, params.filterResonance + dist(gen) * 0.3f);

        // Add effects based on creativity
        if (creativity > 0.5f) {
            params.reverbAmount = juce::jlimit(0.0f, 1.0f, creativity * 0.5f);
            params.delayAmount = juce::jlimit(0.0f, 1.0f, creativity * 0.3f);
        }
    }

    return params;
}

juce::Array<SoundDesignParams> SoundDesignAssistant::generatePresetBatch(SoundCategory category,
                                                                        int count) {
    juce::Array<SoundDesignParams> batch;

    for (int i = 0; i < count; ++i) {
        float creativity = (float)i / (float)count; // Increasing creativity
        batch.add(generatePreset(category, creativity));
    }

    return batch;
}

SoundDesignParams SoundDesignAssistant::optimizeParameters(const SoundDesignParams& startingParams,
                                                           const juce::String& goal) {
    juce::ScopedLock scopedLock(lock_);

    // Parse goal
    auto keywords = parseDescription(goal);
    auto targetParams = extractCharacteristics(keywords);

    // Gradient descent-like optimization
    SoundDesignParams optimized = startingParams;
    float learningRate = 0.1f;
    int iterations = 10;

    for (int i = 0; i < iterations; ++i) {
        // Move towards target
        optimized.brightness += (targetParams.brightness - optimized.brightness) * learningRate;
        optimized.warmth += (targetParams.warmth - optimized.warmth) * learningRate;
        optimized.richness += (targetParams.richness - optimized.richness) * learningRate;
        optimized.attack += (targetParams.attack - optimized.attack) * learningRate;
        optimized.decay += (targetParams.decay - optimized.decay) * learningRate;
        optimized.filterCutoff += (targetParams.filterCutoff - optimized.filterCutoff) * learningRate;
        optimized.filterResonance += (targetParams.filterResonance - optimized.filterResonance) * learningRate;

        // Decay learning rate
        learningRate *= 0.9f;
    }

    // Clamp values
    optimized.brightness = juce::jlimit(0.0f, 1.0f, optimized.brightness);
    optimized.warmth = juce::jlimit(0.0f, 1.0f, optimized.warmth);
    optimized.richness = juce::jlimit(0.0f, 1.0f, optimized.richness);
    optimized.attack = juce::jlimit(0.001f, 2.0f, optimized.attack);
    optimized.decay = juce::jlimit(0.01f, 2.0f, optimized.decay);
    optimized.sustain = juce::jlimit(0.0f, 1.0f, optimized.sustain);
    optimized.release = juce::jlimit(0.01f, 3.0f, optimized.release);
    optimized.filterCutoff = juce::jlimit(100.0f, 10000.0f, optimized.filterCutoff);
    optimized.filterResonance = juce::jlimit(0.0f, 1.0f, optimized.filterResonance);

    return optimized;
}

//==============================================================================
// Analysis
//==============================================================================

juce::String SoundDesignAssistant::analyzeParameters(const SoundDesignParams& params) {
    juce::String description;

    // Analyze brightness
    if (params.brightness > 0.7f) {
        description << "Bright, ";
    } else if (params.brightness < 0.3f) {
        description << "Dark, ";
    } else {
        description << "Balanced, ";
    }

    // Analyze warmth
    if (params.warmth > 0.7f) {
        description << "warm, ";
    } else if (params.warmth < 0.3f) {
        description << "cold, ";
    }

    // Analyze richness
    if (params.richness > 0.7f) {
        description << "rich harmonics, ";
    } else if (params.richness < 0.3f) {
        description << "thin, ";
    }

    // Analyze envelope
    if (params.attack > 0.3f) {
        description << "slow attack, ";
    } else {
        description << "fast attack, ";
    }

    if (params.decay > 0.5f || params.sustain > 0.5f) {
        description << "long release";
    } else {
        description << "short release";
    }

    // Analyze effects
    if (params.reverbAmount > 0.3f) {
        description << ", with reverb";
    }
    if (params.delayAmount > 0.3f) {
        description << ", with delay";
    }
    if (params.distortionAmount > 0.3f) {
        description << ", distorted";
    }

    return description.trim();
}

float SoundDesignAssistant::calculateSimilarity(const SoundDesignParams& paramsA,
                                               const SoundDesignParams& paramsB) {
    // Calculate weighted similarity
    float sumDiff = 0.0f;
    float sumWeights = 0.0f;

    // Core parameters (higher weight)
    sumDiff += std::abs(paramsA.brightness - paramsB.brightness) * 2.0f;
    sumWeights += 2.0f;

    sumDiff += std::abs(paramsA.warmth - paramsB.warmth) * 1.5f;
    sumWeights += 1.5f;

    sumDiff += std::abs(paramsA.richness - paramsB.richness) * 1.5f;
    sumWeights += 1.5f;

    // Envelope (medium weight)
    sumDiff += std::abs(paramsA.attack - paramsB.attack) / 2.0f;
    sumWeights += 1.0f;

    sumDiff += std::abs(paramsA.decay - paramsB.decay) / 2.0f;
    sumWeights += 1.0f;

    sumDiff += std::abs(paramsA.sustain - paramsB.sustain);
    sumWeights += 1.0f;

    sumDiff += std::abs(paramsA.release - paramsB.release) / 3.0f;
    sumWeights += 1.0f;

    // Filter (lower weight)
    sumDiff += std::abs(paramsA.filterCutoff - paramsB.filterCutoff) / 10000.0f;
    sumWeights += 1.0f;

    sumDiff += std::abs(paramsA.filterResonance - paramsB.filterResonance);
    sumWeights += 1.0f;

    // Effects (lowest weight)
    sumDiff += std::abs(paramsA.reverbAmount - paramsB.reverbAmount) * 0.5f;
    sumWeights += 0.5f;

    sumDiff += std::abs(paramsA.delayAmount - paramsB.delayAmount) * 0.5f;
    sumWeights += 0.5f;

    sumDiff += std::abs(paramsA.distortionAmount - paramsB.distortionAmount) * 0.5f;
    sumWeights += 0.5f;

    if (sumWeights < 0.001f) {
        return 0.0f;
    }

    float similarity = 1.0f - (sumDiff / sumWeights);
    return juce::jlimit(0.0f, 1.0f, similarity);
}

juce::String SoundDesignAssistant::findMostSimilar(const SoundDesignParams& target) {
    juce::ScopedLock scopedLock(lock_);

    float maxSimilarity = -1.0f;
    juce::String mostSimilar;

    for (const auto& entry : presetDatabase_) {
        float similarity = calculateSimilarity(target, entry.params);
        if (similarity > maxSimilarity) {
            maxSimilarity = similarity;
            mostSimilar = entry.name;
        }
    }

    return mostSimilar;
}

SoundCategory SoundDesignAssistant::classifySound(const SoundDesignParams& params) {
    // Simple rule-based classification
    // In production, would use actual ML classifier

    float score = 0.0f;
    SoundCategory bestCategory = SoundCategory::Lead;
    float bestScore = 0.0f;

    // Bass score
    score = 0.0f;
    score += (1.0f - params.brightness) * 2.0f;
    score += params.warmth * 1.5f;
    score += (params.attack < 0.05f) ? 1.0f : 0.0f;
    if (score > bestScore) {
        bestScore = score;
        bestCategory = SoundCategory::Bass;
    }

    // Lead score
    score = 0.0f;
    score += params.brightness * 1.5f;
    score += (params.attack < 0.1f) ? 1.0f : 0.0f;
    score += (params.sustain > 0.5f) ? 0.5f : 0.0f;
    if (score > bestScore) {
        bestScore = score;
        bestCategory = SoundCategory::Lead;
    }

    // Pad score
    score = 0.0f;
    score += (params.attack > 0.2f) ? 2.0f : 0.0f;
    score += (params.sustain > 0.6f) ? 1.5f : 0.0f;
    score += (params.release > 0.5f) ? 1.0f : 0.0f;
    score += params.reverbAmount * 1.0f;
    if (score > bestScore) {
        bestScore = score;
        bestCategory = SoundCategory::Pad;
    }

    // Pluck score
    score = 0.0f;
    score += (params.attack < 0.01f) ? 2.0f : 0.0f;
    score += (params.decay < 0.3f && params.sustain < 0.3f) ? 2.0f : 0.0f;
    score += params.brightness * 1.0f;
    if (score > bestScore) {
        bestScore = score;
        bestCategory = SoundCategory::Pluck;
    }

    return bestCategory;
}

//==============================================================================
// Database Management
//==============================================================================

void SoundDesignAssistant::savePreset(const juce::String& name,
                                     const SoundDesignParams& params,
                                     SoundCategory category) {
    juce::ScopedLock scopedLock(lock_);

    // Check if preset already exists
    for (auto& entry : presetDatabase_) {
        if (entry.name == name) {
            entry.params = params;
            entry.category = category;
            entry.description = analyzeParameters(params);
            savePresets();
            return;
        }
    }

    // Add new preset
    PresetEntry entry;
    entry.name = name;
    entry.params = params;
    entry.category = category;
    entry.description = analyzeParameters(params);
    entry.useCount = 0;
    entry.avgRating = 0.0f;

    presetDatabase_.add(entry);
    savePresets();

    DBG("SoundDesignAssistant: Saved preset '" + name + "'");
}

SoundDesignParams SoundDesignAssistant::loadPreset(const juce::String& name) {
    juce::ScopedLock scopedLock(lock_);

    for (const auto& entry : presetDatabase_) {
        if (entry.name == name) {
            // Update use count
            const_cast<PresetEntry&>(entry).useCount++;
            savePresets();
            return entry.params;
        }
    }

    // Return default if not found
    return getDefaultForCategory(SoundCategory::Lead);
}

juce::StringArray SoundDesignAssistant::getPresetsInCategory(SoundCategory category) {
    juce::ScopedLock scopedLock(lock_);

    juce::StringArray presets;

    for (const auto& entry : presetDatabase_) {
        if (entry.category == category) {
            presets.add(entry.name);
        }
    }

    return presets;
}

bool SoundDesignAssistant::exportDatabase(const juce::File& outputFile) {
    juce::ScopedLock scopedLock(lock_);

    auto xml = std::make_unique<juce::XmlElement>("SoundDesignDatabase");

    // Save presets
    auto presetsXml = xml->createNewChildElement("Presets");
    for (const auto& entry : presetDatabase_) {
        auto presetXml = presetsXml->createNewChildElement("Preset");
        presetXml->setAttribute("name", entry.name);
        presetXml->setAttribute("category", (int)entry.category);
        presetXml->setAttribute("description", entry.description);
        presetXml->setAttribute("useCount", entry.useCount);
        presetXml->setAttribute("avgRating", entry.avgRating);

        // Save parameters
        auto paramsXml = presetXml->createNewChildElement("Parameters");
        paramsXml->setAttribute("fundamentalFreq", entry.params.fundamentalFreq);
        paramsXml->setAttribute("brightness", entry.params.brightness);
        paramsXml->setAttribute("richness", entry.params.richness);
        paramsXml->setAttribute("warmth", entry.params.warmth);
        paramsXml->setAttribute("attack", entry.params.attack);
        paramsXml->setAttribute("decay", entry.params.decay);
        paramsXml->setAttribute("sustain", entry.params.sustain);
        paramsXml->setAttribute("release", entry.params.release);
        // ... save other parameters
    }

    // Save learning data
    auto learningXml = xml->createNewChildElement("LearningData");
    learningXml->setAttribute("preferenceCount", userPreferences_.size());

    juce::FileOutputStream stream(outputFile);
    if (stream.openedOk()) {
        stream.setPosition(0);
        stream.truncate();
        stream.writeText(xml->toString(), false, false, nullptr);
        return true;
    }

    return false;
}

bool SoundDesignAssistant::importDatabase(const juce::File& inputFile) {
    juce::ScopedLock scopedLock(lock_);

    auto xml = juce::XmlDocument::parse(inputFile);
    if (!xml || xml->getTagName() != "SoundDesignDatabase") {
        return false;
    }

    // Load presets
    auto* presetsXml = xml->getChildByName("Presets");
    if (presetsXml) {
        presetDatabase_.clear();

        for (auto* presetXml : presetsXml->getChildWithTagNameIterator("Preset")) {
            PresetEntry entry;
            entry.name = presetXml->getStringAttribute("name");
            entry.category = (SoundCategory)presetXml->getIntAttribute("category");
            entry.description = presetXml->getStringAttribute("description");
            entry.useCount = presetXml->getIntAttribute("useCount");
            entry.avgRating = (float)presetXml->getDoubleAttribute("avgRating");

            auto* paramsXml = presetXml->getChildByName("Parameters");
            if (paramsXml) {
                entry.params.fundamentalFreq = (float)paramsXml->getDoubleAttribute("fundamentalFreq", 440.0);
                entry.params.brightness = (float)paramsXml->getDoubleAttribute("brightness", 0.5);
                entry.params.richness = (float)paramsXml->getDoubleAttribute("richness", 0.5);
                entry.params.warmth = (float)paramsXml->getDoubleAttribute("warmth", 0.5);
                entry.params.attack = (float)paramsXml->getDoubleAttribute("attack", 0.01);
                entry.params.decay = (float)paramsXml->getDoubleAttribute("decay", 0.1);
                entry.params.sustain = (float)paramsXml->getDoubleAttribute("sustain", 0.7);
                entry.params.release = (float)paramsXml->getDoubleAttribute("release", 0.2);
            }

            presetDatabase_.add(entry);
        }
    }

    return true;
}

//==============================================================================
// Utilities
//==============================================================================

NeuralSynthParameters SoundDesignAssistant::toNeuralParams(const SoundDesignParams& params) {
    NeuralSynthParameters neuralParams;

    neuralParams.fundamentalFreq = params.fundamentalFreq;
    neuralParams.brightness = params.brightness;
    neuralParams.spectralContrast = params.richness;
    neuralParams.modulationDepth = params.vibratoDepth;
    neuralParams.harmonicity = params.warmth;
    neuralParams.complexity = params.complexity;
    neuralParams.temperature = params.temperature;
    neuralParams.morphPosition = params.morphPosition;

    // Map envelope
    neuralParams.timbreVector[0] = params.attack;
    neuralParams.timbreVector[1] = params.decay;
    neuralParams.timbreVector[2] = params.sustain;
    neuralParams.timbreVector[3] = params.release;

    // Map filter
    neuralParams.timbreVector[4] = params.filterCutoff / 10000.0f;
    neuralParams.timbreVector[5] = params.filterResonance;

    // Map effects
    neuralParams.timbreVector[6] = params.reverbAmount;
    neuralParams.timbreVector[7] = params.delayAmount;
    neuralParams.timbreVector[8] = params.distortionAmount;

    return neuralParams;
}

SoundDesignParams SoundDesignAssistant::fromNeuralParams(const NeuralSynthParameters& neuralParams) {
    SoundDesignParams params;

    params.fundamentalFreq = neuralParams.fundamentalFreq;
    params.brightness = neuralParams.brightness;
    params.richness = neuralParams.spectralContrast;
    params.vibratoDepth = neuralParams.modulationDepth;
    params.warmth = neuralParams.harmonicity;
    params.complexity = neuralParams.complexity;
    params.temperature = neuralParams.temperature;
    params.morphPosition = neuralParams.morphPosition;

    // Map envelope
    params.attack = neuralParams.timbreVector[0];
    params.decay = neuralParams.timbreVector[1];
    params.sustain = neuralParams.timbreVector[2];
    params.release = neuralParams.timbreVector[3];

    // Map filter
    params.filterCutoff = neuralParams.timbreVector[4] * 10000.0f;
    params.filterResonance = neuralParams.timbreVector[5];

    // Map effects
    params.reverbAmount = neuralParams.timbreVector[6];
    params.delayAmount = neuralParams.timbreVector[7];
    params.distortionAmount = neuralParams.timbreVector[8];

    return params;
}

SoundDesignParams SoundDesignAssistant::getDefaultForCategory(SoundCategory category) {
    SoundDesignParams params;

    switch (category) {
        case SoundCategory::Bass:
            params.brightness = 0.2f;
            params.warmth = 0.8f;
            params.richness = 0.9f;
            params.attack = 0.005f;
            params.decay = 0.1f;
            params.sustain = 0.8f;
            params.release = 0.1f;
            params.filterCutoff = 800.0f;
            params.filterResonance = 0.2f;
            break;

        case SoundCategory::Lead:
            params.brightness = 0.7f;
            params.warmth = 0.4f;
            params.richness = 0.6f;
            params.attack = 0.01f;
            params.decay = 0.1f;
            params.sustain = 0.7f;
            params.release = 0.2f;
            params.filterCutoff = 3000.0f;
            params.filterResonance = 0.3f;
            break;

        case SoundCategory::Pad:
            params.brightness = 0.4f;
            params.warmth = 0.7f;
            params.richness = 0.8f;
            params.attack = 0.4f;
            params.decay = 0.3f;
            params.sustain = 0.8f;
            params.release = 1.5f;
            params.filterCutoff = 2000.0f;
            params.filterResonance = 0.1f;
            params.reverbAmount = 0.4f;
            break;

        case SoundCategory::Pluck:
            params.brightness = 0.6f;
            params.warmth = 0.3f;
            params.richness = 0.5f;
            params.attack = 0.001f;
            params.decay = 0.2f;
            params.sustain = 0.1f;
            params.release = 0.3f;
            params.filterCutoff = 4000.0f;
            params.filterResonance = 0.4f;
            break;

        default:
            // Generic lead
            params.brightness = 0.5f;
            params.warmth = 0.5f;
            params.richness = 0.5f;
            params.attack = 0.01f;
            params.decay = 0.1f;
            params.sustain = 0.7f;
            params.release = 0.2f;
            params.filterCutoff = 2000.0f;
            params.filterResonance = 0.2f;
            break;
    }

    return params;
}

SoundCategory SoundDesignAssistant::stringToCategory(const juce::String& categoryStr) {
    juce::String str = categoryStr.toLowerCase();

    if (str.contains("bass")) return SoundCategory::Bass;
    if (str.contains("lead")) return SoundCategory::Lead;
    if (str.contains("pad")) return SoundCategory::Pad;
    if (str.contains("pluck")) return SoundCategory::Pluck;
    if (str.contains("key")) return SoundCategory::Keys;
    if (str.contains("brass")) return SoundCategory::Brass;
    if (str.contains("string")) return SoundCategory::Strings;
    if (str.contains("wind")) return SoundCategory::Wind;
    if (str.contains("perc")) return SoundCategory::Percussion;
    if (str.contains("fx") || str.contains("effect")) return SoundCategory::FX;
    if (str.contains("ambient")) return SoundCategory::Ambient;
    if (str.contains("experimental")) return SoundCategory::Experimental;

    return SoundCategory::Lead; // Default
}

juce::String SoundDesignAssistant::categoryToString(SoundCategory category) {
    switch (category) {
        case SoundCategory::Bass: return "Bass";
        case SoundCategory::Lead: return "Lead";
        case SoundCategory::Pad: return "Pad";
        case SoundCategory::Pluck: return "Pluck";
        case SoundCategory::Keys: return "Keys";
        case SoundCategory::Brass: return "Brass";
        case SoundCategory::Strings: return "Strings";
        case SoundCategory::Wind: return "Wind";
        case SoundCategory::Percussion: return "Percussion";
        case SoundCategory::FX: return "FX";
        case SoundCategory::Ambient: return "Ambient";
        case SoundCategory::Experimental: return "Experimental";
        default: return "Lead";
    }
}

//==============================================================================
// Internal Learning
//==============================================================================

void SoundDesignAssistant::updatePreferenceModel(const UserPreference& preference) {
    // Simple preference learning: adjust weights based on feedback

    float learningRate = 0.1f * (0.5f - std::abs(preference.rating - 0.5f));

    // Adjust weights for each parameter dimension
    preferenceWeights_[0] += (preference.params.brightness - 0.5f) * preference.rating * learningRate;
    preferenceWeights_[1] += (preference.params.warmth - 0.5f) * preference.rating * learningRate;
    preferenceWeights_[2] += (preference.params.richness - 0.5f) * preference.rating * learningRate;
    preferenceWeights_[3] += ((preference.params.attack / 2.0f) - 0.5f) * preference.rating * learningRate;
    preferenceWeights_[4] += ((preference.params.decay / 2.0f) - 0.5f) * preference.rating * learningRate;
    preferenceWeights_[5] += (preference.params.sustain - 0.5f) * preference.rating * learningRate;
    preferenceWeights_[6] += ((preference.params.filterCutoff / 10000.0f) - 0.5f) * preference.rating * learningRate;

    // Normalize weights
    calculatePreferenceWeights();
}

void SoundDesignAssistant::calculatePreferenceWeights() {
    // Normalize weights to [0, 1]
    float minWeight = *std::min_element(preferenceWeights_.begin(), preferenceWeights_.end());
    float maxWeight = *std::max_element(preferenceWeights_.begin(), preferenceWeights_.end());

    float range = maxWeight - minWeight;
    if (range < 0.001f) range = 1.0f;

    for (auto& weight : preferenceWeights_) {
        weight = (weight - minWeight) / range;
    }
}

SuggestionResult SoundDesignAssistant::generatePersonalizedSuggestion() {
    SuggestionResult result;

    // Calculate average parameters from high-rated presets
    float totalRating = 0.0f;
    SoundDesignParams avgParams;
    int count = 0;

    for (const auto& pref : userPreferences_) {
        if (pref.rating > 0.6f) {
            avgParams.brightness += pref.params.brightness;
            avgParams.warmth += pref.params.warmth;
            avgParams.richness += pref.params.richness;
            avgParams.attack += pref.params.attack;
            avgParams.decay += pref.params.decay;
            avgParams.sustain += pref.params.sustain;
            avgParams.release += pref.params.release;
            avgParams.filterCutoff += pref.params.filterCutoff;
            totalRating += pref.rating;
            count++;
        }
    }

    if (count > 0) {
        float invCount = 1.0f / count;
        avgParams.brightness *= invCount;
        avgParams.warmth *= invCount;
        avgParams.richness *= invCount;
        avgParams.attack *= invCount;
        avgParams.decay *= invCount;
        avgParams.sustain *= invCount;
        avgParams.release *= invCount;
        avgParams.filterCutoff *= invCount;

        result.parameters = avgParams;
        result.description = "Personalized suggestion based on your preferences";
        result.confidence = juce::jmin(1.0f, totalRating / count);
        result.isNeuralSuggestion = false;
        result.requiresNeuralModel = false;
        result.tags.add("personalized");
    } else {
        // Fallback to category-based
        return getSuggestionsForCategory(SoundCategory::Lead);
    }

    return result;
}

//==============================================================================
// NLP Processing
//==============================================================================

juce::StringArray SoundDesignAssistant::parseDescription(const juce::String& description) {
    juce::StringArray keywords;
    juce::String desc = description.toLowerCase();

    // Common sound descriptor keywords
    static const char* descriptors[] = {
        "bright", "dark", "warm", "cold", "soft", "hard", "sharp", "smooth",
        "rich", "thin", "fat", "thin", "clean", "dirty", "crunchy", "smooth",
        "pluck", "pad", "lead", "bass", "ambient", "evolving", "static",
        "attack", "decay", "sustain", "release", "envelope",
        "reverb", "delay", "chorus", "flanger", "phaser", "distortion",
        "filter", "cutoff", "resonance", "sweep", "modulation",
        "vibrato", "tremolo", "pitch", "detune", "unison",
        "analog", "digital", "vintage", "modern", "retro",
        "aggressive", "gentle", "harsh", "mellow", "crisp",
        "deep", "shallow", "wide", "narrow", "stereo", "mono",
        nullptr
    };

    for (int i = 0; descriptors[i] != nullptr; ++i) {
        if (desc.contains(descriptors[i])) {
            keywords.add(descriptors[i]);
        }
    }

    return keywords;
}

SoundDesignParams SoundDesignAssistant::extractCharacteristics(const juce::StringArray& keywords) {
    SoundDesignParams params;

    // Start from neutral
    params.brightness = 0.5f;
    params.warmth = 0.5f;
    params.richness = 0.5f;
    params.attack = 0.01f;
    params.decay = 0.1f;
    params.sustain = 0.7f;
    params.release = 0.2f;
    params.filterCutoff = 2000.0f;
    params.filterResonance = 0.2f;

    // Process keywords
    for (const auto& keyword : keywords) {
        if (keyword == "bright") {
            params.brightness = juce::jmin(1.0f, params.brightness + 0.3f);
            params.filterCutoff = juce::jmin(10000.0f, params.filterCutoff * 1.5f);
        }
        else if (keyword == "dark") {
            params.brightness = juce::jmax(0.0f, params.brightness - 0.3f);
            params.filterCutoff = juce::jmax(100.0f, params.filterCutoff * 0.7f);
        }
        else if (keyword == "warm") {
            params.warmth = juce::jmin(1.0f, params.warmth + 0.3f);
        }
        else if (keyword == "cold") {
            params.warmth = juce::jmax(0.0f, params.warmth - 0.3f);
        }
        else if (keyword == "rich" || keyword == "fat") {
            params.richness = juce::jmin(1.0f, params.richness + 0.3f);
        }
        else if (keyword == "thin") {
            params.richness = juce::jmax(0.0f, params.richness - 0.3f);
        }
        else if (keyword == "pluck") {
            params.attack = 0.001f;
            params.decay = 0.2f;
            params.sustain = 0.1f;
        }
        else if (keyword == "pad") {
            params.attack = 0.4f;
            params.decay = 0.3f;
            params.sustain = 0.8f;
            params.release = 1.5f;
            params.reverbAmount = 0.4f;
        }
        else if (keyword == "lead") {
            params.brightness = 0.7f;
            params.attack = 0.01f;
            params.sustain = 0.7f;
        }
        else if (keyword == "bass") {
            params.brightness = 0.2f;
            params.warmth = 0.8f;
            params.filterCutoff = 800.0f;
        }
        else if (keyword == "reverb") {
            params.reverbAmount = 0.5f;
        }
        else if (keyword == "delay") {
            params.delayAmount = 0.4f;
        }
        else if (keyword == "distortion") {
            params.distortionAmount = 0.5f;
        }
        else if (keyword == "aggressive") {
            params.brightness = juce::jmin(1.0f, params.brightness + 0.2f);
            params.filterResonance = juce::jmin(1.0f, params.filterResonance + 0.3f);
            params.distortionAmount = juce::jmin(1.0f, params.distortionAmount + 0.2f);
        }
        else if (keyword == "gentle" || keyword == "soft") {
            params.brightness = juce::jmax(0.0f, params.brightness - 0.2f);
            params.attack = juce::jmax(0.001f, params.attack + 0.05f);
            params.filterResonance = juce::jmax(0.0f, params.filterResonance - 0.2f);
        }
    }

    return params;
}

//==============================================================================
// Machine Learning Helpers
//==============================================================================

SoundDesignParams SoundDesignAssistant::applyLearnedWeights(const SoundDesignParams& baseParams) {
    if (preferenceWeights_.empty()) {
        return baseParams;
    }

    SoundDesignParams params = baseParams;

    // Apply learned preferences
    params.brightness = juce::jlimit(0.0f, 1.0f, baseParams.brightness * 0.7f + preferenceWeights_[0] * 0.3f);
    params.warmth = juce::jlimit(0.0f, 1.0f, baseParams.warmth * 0.7f + preferenceWeights_[1] * 0.3f);
    params.richness = juce::jlimit(0.0f, 1.0f, baseParams.richness * 0.7f + preferenceWeights_[2] * 0.3f);
    params.attack = juce::jlimit(0.001f, 2.0f, baseParams.attack * (0.7f + preferenceWeights_[3] * 0.3f));
    params.decay = juce::jlimit(0.01f, 2.0f, baseParams.decay * (0.7f + preferenceWeights_[4] * 0.3f));
    params.sustain = juce::jlimit(0.0f, 1.0f, baseParams.sustain * 0.7f + preferenceWeights_[5] * 0.3f);
    params.filterCutoff = juce::jlimit(100.0f, 10000.0f,
                                    baseParams.filterCutoff * (0.7f + preferenceWeights_[6] * 0.3f));

    return params;
}

int SoundDesignAssistant::clusterParameters(const SoundDesignParams& params) {
    // Simple clustering based on brightness and warmth
    int brightnessBin = (int)(params.brightness * 3.0f); // 0-2
    int warmthBin = (int)(params.warmth * 3.0f); // 0-2

    return brightnessBin * 3 + warmthBin; // 0-8
}

//==============================================================================
// Database I/O
//==============================================================================

bool SoundDesignAssistant::loadPresets() {
    juce::File presetFile = configPath_.getChildFile("sound_design_presets.xml");

    if (!presetFile.existsAsFile()) {
        // Create default presets
        auto defaults = SoundDesignFactory::createGenerator(SoundCategory::Lead);
        return true;
    }

    return importDatabase(presetFile);
}

bool SoundDesignAssistant::savePresets() {
    juce::File presetFile = configPath_.getChildFile("sound_design_presets.xml");
    return exportDatabase(presetFile);
}

bool SoundDesignAssistant::loadLearningData() {
    juce::File learningFile = configPath_.getChildFile("sound_design_learning.xml");

    if (!learningFile.existsAsFile()) {
        return true; // No learning data yet, that's OK
    }

    auto xml = juce::XmlDocument::parse(learningFile);
    if (!xml) return false;

    auto* learningXml = xml->getChildByName("LearningData");
    if (learningXml) {
        int count = learningXml->getIntAttribute("preferenceCount");
        // In production, would load actual preferences
    }

    return true;
}

bool SoundDesignAssistant::saveLearningData() {
    juce::File learningFile = configPath_.getChildFile("sound_design_learning.xml");

    auto xml = std::make_unique<juce::XmlElement>("SoundDesignLearning");
    auto* learningXml = xml->createNewChildElement("LearningData");
    learningXml->setAttribute("preferenceCount", userPreferences_.size());

    juce::FileOutputStream stream(learningFile);
    if (stream.openedOk()) {
        stream.setPosition(0);
        stream.truncate();
        stream.writeText(xml->toString(), false, false, nullptr);
        return true;
    }

    return false;
}

} // namespace zenith
