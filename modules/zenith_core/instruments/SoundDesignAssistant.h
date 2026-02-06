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

    SoundDesignAssistant.h
    Created: 2025-02-05
    Author:  Zenith DAW

    AI-powered sound design suggestions and parameter recommendations.
    Features machine learning-based preset generation and intelligent
    parameter mapping.

    ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include "NeuralSynthEngine.h"
#include "TimbreTransfer.h"
#include <memory>
#include <vector>

namespace zenith {

//==============================================================================
// Sound Design Parameters
//==============================================================================

struct SoundDesignParams {
    // Synthesis parameters
    float fundamentalFreq{440.0f};
    float brightness{0.5f};
    float richness{0.5f};
    float warmth{0.5f};
    float attack{0.01f};
    float decay{0.1f};
    float sustain{0.7f};
    float release{0.2f};

    // Modulation
    float vibratoRate{5.0f};
    float vibratoDepth{0.0f};
    float tremoloRate{5.0f};
    float tremoloDepth{0.0f};

    // Effects
    float reverbAmount{0.0f};
    float delayAmount{0.0f};
    float distortionAmount{0.0f};
    float filterCutoff{1000.0f};
    float filterResonance{0.0f};

    // Neural-specific
    float complexity{0.5f};
    float temperature{0.7f};
    float morphPosition{0.5f};
};

//==============================================================================
// Sound Category
//==============================================================================

enum class SoundCategory {
    Bass,
    Lead,
    Pad,
    Pluck,
    Keys,
    Brass,
    Strings,
    Wind,
    Percussion,
    FX,
    Ambient,
    Experimental
};

//==============================================================================
// Suggestion Result
//==============================================================================

struct SuggestionResult {
    SoundDesignParams parameters;
    juce::String description;
    float confidence{0.0f};
    juce::StringArray tags;
    bool isNeuralSuggestion{false};
    bool requiresNeuralModel{false};
};

//==============================================================================
// Learning Data
//==============================================================================

struct UserPreference {
    SoundDesignParams params;
    float rating; // 0-1
    juce::String context;
    juce::uint64 timestamp;
};

//==============================================================================
/**
    AI-powered sound design assistant

    This class provides:
    - Intelligent parameter suggestions based on descriptions
    - Learning from user preferences
    - Preset generation and optimization
    - Sound similarity analysis
*/
class SoundDesignAssistant {
public:
    //==========================================================================
    // Construction/Destruction
    //==========================================================================

    SoundDesignAssistant();
    ~SoundDesignAssistant();

    //==========================================================================
    // Initialization
    //==========================================================================

    /**
        Initialize the assistant

        @param configPath Path to store/load learning data
        @return true if successful
    */
    bool initialize(const juce::File& configPath);

    /**
        Release resources
    */
    void release();

    /**
        Check if ready
    */
    bool isReady() const { return initialized_; }

    //==========================================================================
    // Suggestion Generation
    //==========================================================================

    /**
        Get parameter suggestions based on natural language description

        @param description Description of desired sound
        @return Suggested parameters
    */
    SuggestionResult getSuggestions(const juce::String& description);

    /**
        Get suggestions for specific sound category

        @param category Sound category
        @param variation Amount of random variation (0-1)
        @return Suggested parameters
    */
    SuggestionResult getSuggestionsForCategory(SoundCategory category,
                                              float variation = 0.0f);

    /**
        Get suggestions based on reference audio

        @param referenceAudio Reference audio file
        @return Suggested parameters matching the reference
    */
    SuggestionResult getSuggestionsFromReference(const juce::File& referenceAudio);

    /**
        Get suggestions based on current parameters (variations)

        @param currentParams Current parameters
        @param direction Direction to explore ("brighter", "warmer", etc.)
        @param amount Amount of change (0-1)
        @return Suggested parameters
    */
    SuggestionResult exploreVariations(const SoundDesignParams& currentParams,
                                      const juce::String& direction,
                                      float amount);

    //==========================================================================
    // Machine Learning
    //==========================================================================

    /**
        Provide feedback on suggestions (reinforcement learning)

        @param result The suggestion that was used
        @param rating User rating (0-1)
    */
    void provideFeedback(const SuggestionResult& result, float rating);

    /**
        Learn from user-created presets

        @param presetName Name of preset
        @param params Parameters user created
        @param rating User satisfaction (0-1)
    */
    void learnFromPreset(const juce::String& presetName,
                        const SoundDesignParams& params,
                        float rating);

    /**
        Get personalized suggestions based on learning

        @return Suggestion tailored to user preferences
    */
    SuggestionResult getPersonalizedSuggestion();

    //==========================================================================
    // Preset Generation
    //==========================================================================

    /**
        Generate new preset using AI

        @param baseCategory Base category to build on
        @param creativity How creative/experimental (0-1)
        @return Generated preset parameters
    */
    SoundDesignParams generatePreset(SoundCategory baseCategory,
                                    float creativity = 0.5f);

    /**
        Batch generate presets

        @param category Category
        @param count Number of presets to generate
        @return Array of generated presets
    */
    juce::Array<SoundDesignParams> generatePresetBatch(SoundCategory category,
                                                       int count);

    /**
        Optimize parameters for target goal

        @param startingParams Initial parameters
        @param goal Target description
        @return Optimized parameters
    */
    SoundDesignParams optimizeParameters(const SoundDesignParams& startingParams,
                                       const juce::String& goal);

    //==========================================================================
    // Analysis
    //==========================================================================

    /**
        Analyze parameter characteristics

        @param params Parameters to analyze
        @return Description of the sound
    */
    juce::String analyzeParameters(const SoundDesignParams& params);

    /**
        Calculate similarity between two parameter sets

        @param paramsA First parameter set
        @param paramsB Second parameter set
        @return Similarity score (0-1)
    */
    float calculateSimilarity(const SoundDesignParams& paramsA,
                             const SoundDesignParams& paramsB);

    /**
        Find most similar preset in database

        @param target Target parameters
        @return Name of most similar preset
    */
    juce::String findMostSimilar(const SoundDesignParams& target);

    /**
        Classify sound category from parameters

        @param params Parameters to classify
        @return Most likely category
    */
    SoundCategory classifySound(const SoundDesignParams& params);

    //==========================================================================
    // Database Management
    //==========================================================================

    /**
        Save preset to database

        @param name Preset name
        @param params Parameter values
        @param category Sound category
    */
    void savePreset(const juce::String& name,
                   const SoundDesignParams& params,
                   SoundCategory category);

    /**
        Load preset from database

        @param name Preset name
        @return Loaded parameters (or default if not found)
    */
    SoundDesignParams loadPreset(const juce::String& name);

    /**
        Get all presets in category

        @param category Category to filter by
        @return Array of preset names
    */
    juce::StringArray getPresetsInCategory(SoundCategory category);

    /**
        Export learning data and presets

        @param outputFile Output file path
        @return true if successful
    */
    bool exportDatabase(const juce::File& outputFile);

    /**
        Import learning data and presets

        @param inputFile Input file path
        @return true if successful
    */
    bool importDatabase(const juce::File& inputFile);

    //==========================================================================
    // Utilities
    //==========================================================================

    /**
        Convert parameters to NeuralSynthParameters

        @param params Sound design parameters
        @return Neural synth parameters
    */
    static NeuralSynthParameters toNeuralParams(const SoundDesignParams& params);

    /**
        Convert NeuralSynthParameters to SoundDesignParams

        @param neuralParams Neural synth parameters
        @return Sound design parameters
    */
    static SoundDesignParams fromNeuralParams(const NeuralSynthParameters& neuralParams);

    /**
        Get default parameters for category

        @param category Sound category
        @return Default parameters
    */
    static SoundDesignParams getDefaultForCategory(SoundCategory category);

    /**
        Get category from string

        @param categoryStr Category string
        @return Sound category enum
    */
    static SoundCategory stringToCategory(const juce::String& categoryStr);

    /**
        Get string from category

        @param category Sound category enum
        @return Category string
    */
    static juce::String categoryToString(SoundCategory category);

private:
    //==========================================================================
    // Internal Learning
    //==========================================================================

    /**
        Update user preference model
    */
    void updatePreferenceModel(const UserPreference& preference);

    /**
        Calculate user preference weights
    */
    void calculatePreferenceWeights();

    /**
        Generate personalized suggestion
    */
    SuggestionResult generatePersonalizedSuggestion();

    //==========================================================================
    // NLP Processing
    //==========================================================================

    /**
        Parse description into keywords and concepts

        @param description Input description
        @return Array of relevant keywords
    */
    juce::StringArray parseDescription(const juce::String& description);

    /**
        Extract sound characteristics from keywords

        @param keywords Keywords from description
        @return Parameter hints
    */
    SoundDesignParams extractCharacteristics(const juce::StringArray& keywords);

    //==========================================================================
    // Machine Learning Helpers
    //==========================================================================

    /**
        Apply learned weights to parameters

        @param baseParams Base parameters
        @return Personalized parameters
    */
    SoundDesignParams applyLearnedWeights(const SoundDesignParams& baseParams);

    /**
        Cluster similar parameters

        @param params Parameters to cluster
        @return Cluster ID
    */
    int clusterParameters(const SoundDesignParams& params);

    //==========================================================================
    // Database I/O
    //==========================================================================

    /**
        Load presets from file
    */
    bool loadPresets();

    /**
        Save presets to file
    */
    bool savePresets();

    /**
        Load learning data
    */
    bool loadLearningData();

    /**
        Save learning data
    */
    bool saveLearningData();

    //==========================================================================
    // Member Variables
    //==========================================================================

    // Initialization
    bool initialized_{false};
    juce::File configPath_;

    // Database
    struct PresetEntry {
        juce::String name;
        SoundDesignParams params;
        SoundCategory category;
        juce::String description;
        int useCount{0};
        float avgRating{0.0f};
    };
    juce::Array<PresetEntry> presetDatabase_;

    // Learning data
    juce::Array<UserPreference> userPreferences_;
    std::vector<float> preferenceWeights_;
    juce::Array<int> recentCategories_;

    // Analysis
    std::unique_ptr<TimbreTransfer> timbreAnalyzer_;

    // Thread safety
    juce::CriticalSection lock_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoundDesignAssistant)
};

//==============================================================================
/**
    Factory for creating preset generators
*/
class SoundDesignFactory {
public:
    /**
        Create procedural generator for category

        @param category Sound category
        @return Generator function
    */
    static std::function<SoundDesignParams()> createGenerator(SoundCategory category);

    /**
        Create morphing generator between two sounds

        @param paramsA First sound
        @param paramsB Second sound
        @return Generator that morphs between sounds
    */
    static std::function<SoundDesignParams(float)> createMorphGenerator(
        const SoundDesignParams& paramsA,
        const SoundDesignParams& paramsB);

    /**
        Create evolutionary generator

        @param population Initial population
        @return Generator that evolves parameters
    */
    static std::function<SoundDesignParams(int)> createEvolutionaryGenerator(
        const juce::Array<SoundDesignParams>& population);
};

} // namespace zenith
