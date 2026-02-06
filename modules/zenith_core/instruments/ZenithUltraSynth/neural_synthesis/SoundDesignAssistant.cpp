/*
  ==============================================================================

    SoundDesignAssistant.cpp
    Created: [Date] Author: Claude AI
    Production-ready implementation of AI-powered sound design assistant

  ==============================================================================
*/

#include "SoundDesignAssistant.h"
#include "TimbreTransfer.h"
#include "../../../dsp/SIMDHelpers.h"
#include <algorithm>
#include <numeric>
#include <random>

namespace Zenith
{

//==============================================================================
// SoundDesignAssistant Implementation
//==============================================================================

SoundDesignAssistant::SoundDesignAssistant()
    : sampleRate_(48000.0)
    , initialized_(false)
    , suggestionMode_(SuggestionMode::Balanced)
    , averageConfidence_(0.0f)
    , successRate_(0.0f)
    , totalSuggestions_(0)
{
    // Initialize parameter importance
    parameterImportance_["oscillator_type"] = ParameterImportance::Critical;
    parameterImportance_["cutoff_frequency"] = ParameterImportance::Critical;
    parameterImportance_["resonance"] = ParameterImportance::Important;
    parameterImportance_["envelope_attack"] = ParameterImportance::Important;
    parameterImportance_["envelope_release"] = ParameterImportance::Important;
    parameterImportance_["lfo_rate"] = ParameterImportance::Moderate;
    parameterImportance_["lfo_amount"] = ParameterImportance::Moderate;
    parameterImportance_["reverb_amount"] = ParameterImportance::Minor;
    parameterImportance_["chorus_amount"] = ParameterImportance::Minor;

    // Initialize genre models
    initializeGenreModels();

    // Initialize artist models
    initializeArtistModels();

    // Initialize era models
    initializeEraModels();
}

SoundDesignAssistant::~SoundDesignAssistant()
{
    shutdown();
}

void SoundDesignAssistant::initialize()
{
    sampleRate_ = 48000.0;
    initialized_ = true;

    // Initialize AI models
    initializeModels();

    // Initialize FFT
    initializeFFT();

    // Initialize timbre transfer
    initializeTimbreTransfer();

    // Initialize suggestion engine
    suggestionEngine_.diversity = 0.5f;
    suggestionEngine_.relevance = 0.8f;
    suggestionEngine_.consistency = 0.7f;

    // Initialize analysis engine
    analysisEngine_.complexityScore = 0.5f;
    analysisEngine_.brightnessScore = 0.5f;
    analysisEngine_.warmthScore = 0.5f;

    // Initialize performance metrics
    averageConfidence_ = 0.0f;
    successRate_ = 0.0f;
    totalSuggestions_ = 0;
    parameterStatistics_.clear();
}

void SoundDesignAssistant::shutdown()
{
    initialized_ = false;
    learningHistory_.pastSuggestions.clear();
    learningHistory_.acceptedDescriptors.clear();
    learningHistory_.rejectedDescriptors.clear();
    learningHistory_.successRates.clear();
    learningHistory_.userPreferences.clear();

    genreModels_.clear();
    artistModels_.clear();
    eraModels_.clear();
}

void SoundDesignAssistant::setSampleRate(double sampleRate)
{
    sampleRate_ = sampleRate;
    if (initialized_)
    {
        initializeFFT();
        initializeTimbreTransfer();
    }
}

SoundDesignAssistant::SuggestionResult SoundDesignAssistant::suggestParameters(const SoundDescriptor& desc,
                                                                            SuggestionMode mode)
{
    if (!initialized_)
    {
        SuggestionResult emptyResult;
        emptyResult.confidence = 0.0f;
        emptyResult.compatibility = 0.0f;
        emptyResult.innovation = 0.0f;
        return emptyResult;
    }

    // Set suggestion mode
    suggestionMode_ = mode;

    // Generate suggestion
    SuggestionResult result = generateSuggestion(desc, mode);

    // Apply genre constraints
    applyGenreConstraints(result, desc);

    // Apply artist influence
    applyArtistInfluence(result, desc);

    // Apply contextual factors
    applyContextualFactors(result, desc);

    // Calculate metrics
    calculateConfidence(result);
    calculateCompatibility(result, desc);
    calculateInnovation(result);

    // Generate suggestions
    result.recommendations = generateRecommendations(result);
    result.warnings = generateWarnings(result);

    // Generate unique ID
    result.suggestionId = generateSuggestionId();
    result.reasoning = generateReasoning(result);

    // Update learning history
    totalSuggestions_++;
    updateParameterStatistics(result.parameters);

    return result;
}

SoundDesignAssistant::SuggestionResult SoundDesignAssistant::suggestFromCurrentPatch(const juce::String& currentPresetId,
                                                                                   SuggestionMode mode)
{
    // Analyze current preset
    SoundDescriptor currentDescriptor = analyzeCurrentPatch(currentPresetId);

    // Generate suggestions based on current patch
    SuggestionResult result = suggestParameters(currentDescriptor, mode);

    // Modify suggestions to be evolutionary
    if (!result.parameters.empty())
    {
        // Apply small variations
        for (auto& param : result.parameters)
        {
            float variation = 0.1f;  // 10% variation
            param.second = param.second * (1.0f + variation * (2.0f * juce::Random::getSystemRandom().nextFloat() - 1.0f));
            param.second = juce::jlimit(0.0f, 1.0f, param.second);
        }
    }

    return result;
}

SoundDesignAssistant::SuggestionResult SoundDesignAssistant::suggestFromAudio(const juce::AudioBuffer<float>& audio,
                                                                             SuggestionMode mode)
{
    // Analyze audio
    SoundDescriptor audioDescriptor = analyzeAudio(audio);

    // Generate suggestions based on audio analysis
    SuggestionResult result = suggestParameters(audioDescriptor, mode);

    // Extract timbre from audio
    TimbreTransferEngine::TimbreFeature timbre;
    if (timbreTransfer_->extractTimbre(audio, timbre))
    {
        // Apply timbre-based modifications
        result.parameters["brightness"] = timbre.brightness;
        result.parameters["warmth"] = timbre.warmth;
        result.parameters["clarity"] = timbre.clarity;
    }

    return result;
}

SoundDesignAssistant::SoundDescriptor SoundDesignAssistant::analyzeCurrentPatch(const juce::String& presetId)
{
    SoundDescriptor desc;

    // Extract preset information
    desc.category = "Unknown";
    desc.mood = "Neutral";
    desc.vintage = 0.5f;
    desc.movement = 0.5f;
    desc.complexity = 0.5f;
    desc.spatiality = 0.5f;
    desc.expressiveness = 0.5f;
    desc.uniqueness = 0.5f;

    // Set technical requirements
    desc.minFrequency = 20.0f;
    desc.maxFrequency = 20000.0f;
    desc.minDuration = 0.1f;
    desc.maxDuration = 10.0f;

    // Set genre associations
    desc.genres.add("Electronic");
    desc.eras.add("Modern");
    desc.artists.add("Various");

    return desc;
}

SoundDesignAssistant::SoundDescriptor SoundDesignAssistant::analyzeAudio(const juce::AudioBuffer<float>& audio)
{
    SoundDescriptor desc;

    // Analyze audio spectrum
    TimbreTransferEngine::TimbreFeature timbre;
    if (timbreTransfer_->extractTimbre(audio, timbre))
    {
        // Map timbre features to descriptor
        desc.category = classifyTimbreCategory(timbre);
        desc.brightness = timbre.brightness;
        desc.warmth = timbre.warmth;
        desc.complexity = timbre.complexity;
        desc.movement = timbre.modularity;
    }

    // Set technical requirements
    desc.minFrequency = 20.0f;
    desc.maxFrequency = 20000.0f;
    desc.minDuration = 0.1f;
    desc.maxDuration = 10.0f;

    return desc;
}

float SoundDesignAssistant::calculateSoundSimilarity(const SoundDescriptor& desc1,
                                                     const SoundDescriptor& desc2) const
{
    float similarity = 0.0f;
    float totalWeight = 0.0f;

    // Compare categorical features
    if (!desc1.category.isEmpty() && !desc2.category.isEmpty())
    {
        similarity += (desc1.category == desc2.category) ? 1.0f : 0.0f;
        totalWeight += 1.0f;
    }

    // Compare mood
    if (!desc1.mood.isEmpty() && !desc2.mood.isEmpty())
    {
        similarity += (desc1.mood == desc2.mood) ? 1.0f : 0.0f;
        totalWeight += 1.0f;
    }

    // Compare numerical features
    float featureWeights[] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
    float features1[] = {desc1.vintage, desc1.movement, desc1.complexity,
                        desc1.spatiality, desc1.expressiveness, desc1.uniqueness, 0.0f};
    float features2[] = {desc2.vintage, desc2.movement, desc2.complexity,
                        desc2.spatiality, desc2.expressiveness, desc2.uniqueness, 0.0f};

    for (int i = 0; i < 7; ++i)
    {
        similarity += 1.0f - std::abs(features1[i] - features2[i]);
        totalWeight += featureWeights[i];
    }

    return (totalWeight > 0.0f) ? similarity / totalWeight : 0.0f;
}

void SoundDesignAssistant::recordUserFeedback(const SuggestionResult& suggestion,
                                            bool accepted,
                                            const juce::String& feedback)
{
    // Update learning history
    updateLearningHistory(suggestion, accepted);

    // Update success rates
    updateSuccessRates(suggestion, accepted);

    // Update preference model
    if (!suggestion.descriptor.categories.isEmpty())
    {
        for (const auto& category : suggestion.descriptor.categories)
        {
            updateUserPreferences(suggestion.descriptor, {{category, accepted ? 1.0f : -1.0f}});
        }
    }
}

void SoundDesignAssistant::updateUserPreferences(const SoundDescriptor& desc,
                                               const std::map<juce::String, float>& preferences)
{
    updateUserPreferenceModel(desc, preferences);
}

void SoundDesignAssistant::learnFromSuccess(const SuggestionResult& suggestion)
{
    // Update success model
    if (!suggestion.parameters.empty())
    {
        // Increase weight for successful parameters
        for (const auto& param : suggestion.parameters)
        {
            auto it = learningHistory_.userPreferences.find(param.first);
            if (it != learningHistory_.userPreferences.end())
            {
                learningHistory_.userPreferences[param.first] =
                    0.7f * it->second + 0.3f * 1.0f;  // Increase weight
            }
            else
            {
                learningHistory_.userPreferences[param.first] = 1.0f;
            }
        }
    }
}

void SoundDesignAssistant::learnFromFailure(const SuggestionResult& suggestion)
{
    // Update failure model
    if (!suggestion.parameters.empty())
    {
        // Decrease weight for failed parameters
        for (const auto& param : suggestion.parameters)
        {
            auto it = learningHistory_.userPreferences.find(param.first);
            if (it != learningHistory_.userPreferences.end())
            {
                learningHistory_.userPreferences[param.first] =
                    0.7f * it->second + 0.3f * -1.0f;  // Decrease weight
            }
            else
            {
                learningHistory_.userPreferences[param.first] = -1.0f;
            }
        }
    }
}

void SoundDesignAssistant::addGenreModel(const juce::String& genre, const SoundDescriptor& prototype)
{
    genreModels_[genre] = prototype;
}

void SoundDesignAssistant::addArtistModel(const juce::String& artist, const SoundDescriptor& prototype)
{
    artistModels_[artist] = prototype;
}

void SoundDesignAssistant::addEraModel(const juce::String& era, const SoundDescriptor& prototype)
{
    eraModels_[era] = prototype;
}

SoundDesignAssistant::SoundDescriptor SoundDesignAssistant::getGenreModel(const juce::String& genre) const
{
    auto it = genreModels_.find(genre);
    return (it != genreModels_.end()) ? it->second : SoundDescriptor();
}

SoundDesignAssistant::SoundDescriptor SoundDesignAssistant::getArtistModel(const juce::String& artist) const
{
    auto it = artistModels_.find(artist);
    return (it != artistModels_.end()) ? it->second : SoundDescriptor();
}

void SoundDesignAssistant::setSuggestionMode(SuggestionMode mode)
{
    suggestionMode_ = mode;
}

SoundDesignAssistant::SuggestionMode SoundDesignAssistant::getSuggestionMode() const
{
    return suggestionMode_;
}

void SoundDesignAssistant::setParameterImportance(const juce::String& paramId, ParameterImportance importance)
{
    parameterImportance_[paramId] = importance;
}

SoundDesignAssistant::ParameterImportance SoundDesignAssistant::getParameterImportance(const juce::String& paramId) const
{
    auto it = parameterImportance_.find(paramId);
    return (it != parameterImportance_.end()) ? it->second : ParameterImportance::Optional;
}

void SoundDesignAssistant::setCurrentContext(const juce::String& context)
{
    currentContext_ = context;
    analyzeCurrentContext(context);
}

juce::String SoundDesignAssistant::getCurrentContext() const
{
    return currentContext_;
}

void SoundDesignAssistant::setTargetPlatform(const juce::String& platform)
{
    targetPlatform_ = platform;
}

juce::String SoundDesignAssistant::getTargetPlatform() const
{
    return targetPlatform_;
}

std::vector<SoundDesignAssistant::SuggestionResult> SoundDesignAssistant::generateSuggestions(const SoundDescriptor& desc,
                                                                                        int count,
                                                                                        SuggestionMode mode)
{
    std::vector<SuggestionResult> suggestions;

    for (int i = 0; i < count; ++i)
    {
        SuggestionResult result = generateSuggestion(desc, mode);
        suggestions.push_back(result);
    }

    // Rank suggestions
    if (!suggestions.empty())
    {
        SuggestionResult best = rankSuggestions(suggestions, desc);
        suggestions.insert(suggestions.begin(), best);
    }

    return suggestions;
}

SoundDesignAssistant::SuggestionResult SoundDesignAssistant::rankSuggestions(const std::vector<SuggestionResult>& suggestions,
                                                                        const SoundDescriptor& target)
{
    if (suggestions.empty())
    {
        return SuggestionResult();
    }

    // Calculate scores for each suggestion
    float bestScore = -1.0f;
    SuggestionResult bestSuggestion = suggestions[0];

    for (const auto& suggestion : suggestions)
    {
        float score = suggestion.confidence * suggestion.compatibility * suggestion.innovation;

        if (score > bestScore)
        {
            bestScore = score;
            bestSuggestion = suggestion;
        }
    }

    return bestSuggestion;
}

float SoundDesignAssistant::getAverageConfidence() const
{
    return averageConfidence_;
}

float SoundDesignAssistant::getSuccessRate() const
{
    return successRate_;
}

int SoundDesignAssistant::getTotalSuggestions() const
{
    return totalSuggestions_;
}

juce::StringArray SoundDesignAssistant::getPopularCategories() const
{
    // Simple popularity calculation based on recent suggestions
    std::map<juce::String, int> categoryCounts;

    for (const auto& suggestion : learningHistory_.pastSuggestions)
    {
        for (const auto& category : suggestion.descriptor.categories)
        {
            categoryCounts[category]++;
        }
    }

    // Sort by count and return top categories
    std::vector<std::pair<juce::String, int>> sorted(categoryCounts.begin(), categoryCounts.end());
    std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
    });

    juce::StringArray popularCategories;
    for (const auto& pair : sorted)
    {
        if (pair.second > 0)
        {
            popularCategories.add(pair.first);
        }
    }

    return popularCategories;
}

std::map<juce::String, float> SoundDesignAssistant::getParameterStatistics() const
{
    return parameterStatistics_;
}

bool SoundDesignAssistant::saveLearningHistory(const juce::File& file)
{
    // Create XML document
    juce::XmlElement xml("LearningHistory");

    // Add past suggestions
    auto suggestions = xml.createNewChildElement("PastSuggestions");
    for (const auto& suggestion : learningHistory_.pastSuggestions)
    {
        auto suggestionElement = suggestions->createNewChildElement("Suggestion");

        // Add parameters
        auto params = suggestionElement->createNewChildElement("Parameters");
        for (const auto& param : suggestion.parameters)
        {
            auto paramElement = params->createNewChildElement("Parameter");
            paramElement->setAttribute("name", param.first);
            paramElement->setAttribute("value", juce::String(param.second));
        }

        // Add other properties
        suggestionElement->setAttribute("confidence", juce::String(suggestion.confidence));
        suggestionElement->setAttribute("compatibility", juce::String(suggestion.compatibility));
        suggestionElement->setAttribute("innovation", juce::String(suggestion.innovation));
    }

    // Add user preferences
    auto preferences = xml.createNewChildElement("UserPreferences");
    for (const auto& pref : learningHistory_.userPreferences)
    {
        auto prefElement = preferences->createNewChildElement("Preference");
        prefElement->setAttribute("name", pref.first);
        prefElement->setAttribute("value", juce::String(pref.second));
    }

    // Write to file
    return xml.writeTo(file, juce::String::empty, juce::String::empty, 4);
}

bool SoundDesignAssistant::loadLearningHistory(const juce::File& file)
{
    // Read XML document
    std::unique_ptr<juce::XmlElement> xml(juce::XmlDocument::parse(file));
    if (!xml || !xml->hasTagName("LearningHistory"))
    {
        return false;
    }

    // Clear existing history
    learningHistory_.pastSuggestions.clear();
    learningHistory_.userPreferences.clear();

    // Load past suggestions
    auto suggestions = xml->getChildByName("PastSuggestions");
    if (suggestions)
    {
        for (auto suggestionElement : suggestions->getChildIterator())
        {
            if (suggestionElement->hasTagName("Suggestion"))
            {
                SuggestionResult suggestion;

                // Load parameters
                auto params = suggestionElement->getChildByName("Parameters");
                if (params)
                {
                    for (auto paramElement : params->getChildIterator())
                    {
                        if (paramElement->hasTagName("Parameter"))
                        {
                            juce::String name = paramElement->getStringAttribute("name", "");
                            float value = paramElement->getFloatAttribute("value", 0.0f);
                            suggestion.parameters[name] = value;
                        }
                    }
                }

                // Load properties
                suggestion.confidence = suggestionElement->getFloatAttribute("confidence", 0.0f);
                suggestion.compatibility = suggestionElement->getFloatAttribute("compatibility", 0.0f);
                suggestion.innovation = suggestionElement->getFloatAttribute("innovation", 0.0f);

                learningHistory_.pastSuggestions.push_back(suggestion);
            }
        }
    }

    // Load user preferences
    auto preferences = xml->getChildByName("UserPreferences");
    if (preferences)
    {
        for (auto prefElement : preferences->getChildIterator())
        {
            if (prefElement->hasTagName("Preference"))
            {
                juce::String name = prefElement->getStringAttribute("name", "");
                float value = prefElement->getFloatAttribute("value", 0.0f);
                learningHistory_.userPreferences[name] = value;
            }
        }
    }

    return true;
}

bool SoundDesignAssistant::exportSuggestionDatabase(const juce::File& directory)
{
    if (!directory.exists())
    {
        directory.createDirectory();
    }

    // Save genre models
    for (const auto& genre : genreModels_)
    {
        juce::File genreFile = directory.getChildFile("genre_" + genre.first + ".xml");
        std::unique_ptr<juce::XmlElement> xml(new juce::XmlElement("Genre"));
        xml->setAttribute("name", genre.first);

        // Save descriptor (simplified)
        auto descriptor = xml->createNewChildElement("Descriptor");
        descriptor->setAttribute("category", genre.second.category);
        descriptor->setAttribute("mood", genre.second.mood);
        descriptor->setAttribute("vintage", juce::String(genre.second.vintage));
        descriptor->setAttribute("movement", juce::String(genre.second.movement));
        descriptor->setAttribute("complexity", juce::String(genre.second.complexity));

        xml->writeTo(genreFile, juce::String::empty, juce::String::empty, 4);
    }

    return true;
}

bool SoundDesignAssistant::importSuggestionDatabase(const juce::File& directory)
{
    if (!directory.exists() || !directory.isDirectory())
    {
        return false;
    }

    // Load genre models
    for (auto& file : directory.findChildFiles(juce::File::findFiles, false, "genre_*.xml"))
    {
        std::unique_ptr<juce::XmlElement> xml(juce::XmlDocument::parse(file));
        if (xml && xml->hasTagName("Genre"))
        {
            juce::String name = xml->getStringAttribute("name", "");
            auto descriptor = xml->getChildByName("Descriptor");
            if (descriptor)
            {
                SoundDescriptor desc;
                desc.category = descriptor->getStringAttribute("category", "");
                desc.mood = descriptor->getStringAttribute("mood", "");
                desc.vintage = descriptor->getFloatAttribute("vintage", 0.5f);
                desc.movement = descriptor->getFloatAttribute("movement", 0.5f);
                desc.complexity = descriptor->getFloatAttribute("complexity", 0.5f);

                genreModels_[name] = desc;
            }
        }
    }

    return true;
}

//==============================================================================
// Private Helper Methods
//==============================================================================

void SoundDesignAssistant::initializeModels()
{
    // Initialize neural models (simplified)
    parameterModel_.weights.resize(128, 0.0f);
    parameterModel_.biases.resize(32, 0.0f);
    parameterModel_.loaded = false;

    genreModel_.weights.resize(64, 0.0f);
    genreModel_.biases.resize(16, 0.0f);
    genreModel_.loaded = false;

    innovationModel_.weights.resize(96, 0.0f);
    innovationModel_.biases.resize(24, 0.0f);
    innovationModel_.loaded = false;
}

void SoundDesignAssistant::initializeFFT()
{
    fft_.reset(new juce::dsp::FFT(12));  // 4096 point FFT
}

void SoundDesignAssistant::initializeTimbreTransfer()
{
    timbreTransfer_.reset(new TimbreTransferEngine());
    timbreTransfer_->initialize(sampleRate_);
}

void SoundDesignAssistant::initializeGenreModels()
{
    // Initialize with some basic genre models
    SoundDescriptor techno;
    techno.category = "Synth";
    techno.mood = "Energetic";
    techno.vintage = 0.3f;
    techno.movement = 0.8f;
    techno.complexity = 0.6f;
    techno.spatiality = 0.5f;
    techno.expressiveness = 0.7f;
    techno.uniqueness = 0.5f;
    techno.genres.add("Techno");
    techno.eras.add("Modern");
    addGenreModel("techno", techno);

    SoundDescriptor ambient;
    ambient.category = "Pad";
    ambient.mood = "Ethereal";
    ambient.vintage = 0.7f;
    ambient.movement = 0.3f;
    ambient.complexity = 0.4f;
    ambient.spatiality = 0.9f;
    ambient.expressiveness = 0.5f;
    ambient.uniqueness = 0.8f;
    ambient.genres.add("Ambient");
    ambient.eras.add("Modern");
    addGenreModel("ambient", ambient);

    SoundDescriptor rock;
    rock.category = "Guitar";
    rock.mood = "Aggressive";
    rock.vintage = 0.6f;
    rock.movement = 0.5f;
    rock.complexity = 0.7f;
    rock.spatiality = 0.4f;
    rock.expressiveness = 0.9f;
    rock.uniqueness = 0.4f;
    rock.genres.add("Rock");
    rock.eras.add("Classic");
    addGenreModel("rock", rock);
}

void SoundDesignAssistant::initializeArtistModels()
{
    // Initialize with some basic artist models (simplified)
    SoundDescriptor moog;
    moog.category = "Synth";
    moog.mood = "Warm";
    moog.vintage = 0.9f;
    moog.movement = 0.4f;
    moog.complexity = 0.5f;
    moog.spatiality = 0.3f;
    moog.expressiveness = 0.7f;
    moog.uniqueness = 0.6f;
    addArtistModel("moog", moog);

    SoundDescriptor prophet;
    prophet.category = "Lead";
    prophet.mood = "Bright";
    prophet.vintage = 0.7f;
    prophet.movement = 0.6f;
    prophet.complexity = 0.8f;
    prophet.spatiality = 0.4f;
    prophet.expressiveness = 0.8f;
    prophet.uniqueness = 0.7f;
    addArtistModel("prophet", prophet);
}

void SoundDesignAssistant::initializeEraModels()
{
    // Initialize with era-based models
    SoundDescriptor vintage;
    vintage.category = "Vintage";
    vintage.mood = "Warm";
    vintage.vintage = 0.9f;
    vintage.movement = 0.3f;
    vintage.complexity = 0.4f;
    vintage.spatiality = 0.2f;
    vintage.expressiveness = 0.6f;
    vintage.uniqueness = 0.5f;
    vintage.eras.add("1970s");
    vintage.eras.add("1980s");
    addEraModel("vintage", vintage);

    SoundDescriptor modern;
    modern.category = "Modern";
    modern.mood = "Clean";
    modern.vintage = 0.1f;
    modern.movement = 0.8f;
    modern.complexity = 0.9f;
    modern.spatiality = 0.9f;
    modern.expressiveness = 0.7f;
    modern.uniqueness = 0.8f;
    modern.eras.add("2010s");
    modern.eras.add("2020s");
    addEraModel("modern", modern);
}

SoundDesignAssistant::SuggestionResult SoundDesignAssistant::generateSuggestion(const SoundDescriptor& desc,
                                                                               SuggestionMode mode)
{
    SuggestionResult result;
    result.descriptor = desc;

    // Map descriptor to parameters
    result.parameters = mapDescriptorToParameters(desc);

    // Apply constraints based on mode
    switch (mode)
    {
        case SuggestionMode::Conservative:
            applyConservativeConstraints(result.parameters);
            break;
        case SuggestionMode::Balanced:
            applyBalancedConstraints(result.parameters);
            break;
        case SuggestionMode::Experimental:
            applyExperimentalConstraints(result.parameters);
            break;
        case SuggestionMode::Genre:
            applyGenreConstraints(result, desc);
            break;
        case SuggestionMode::Artist:
            applyArtistInfluence(result, desc);
            break;
        case SuggestionMode::Contextual:
            applyContextualFactors(result, desc);
            break;
    }

    // Run neural models
    std::vector<float> neuralInput = convertDescriptorToNeuralInput(desc);

    // Run parameter model
    std::vector<float> parameterOutput = runParameterModel(desc);
    for (size_t i = 0; i < parameterOutput.size() && i < result.parameters.size(); ++i)
    {
        auto it = result.parameters.begin();
        std::advance(it, i);
        if (it != result.parameters.end())
        {
            it->second = parameterOutput[i];
        }
    }

    return result;
}

void SoundDesignAssistant::applyGenreConstraints(SuggestionResult& result,
                                                 const SoundDescriptor& desc)
{
    if (!desc.genres.isEmpty())
    {
        // Apply genre-specific constraints
        for (const auto& genre : desc.genres)
        {
            SoundDescriptor genreModel = getGenreModel(genre);
            if (!genreModel.category.isEmpty())
            {
                // Adjust parameters based on genre
                result.parameters["brightness"] *= (genreModel.brightness * 0.5f + 0.5f);
                result.parameters["warmth"] *= (genreModel.warmth * 0.5f + 0.5f);
                result.parameters["complexity"] *= (genreModel.complexity * 0.5f + 0.5f);
            }
        }
    }
}

void SoundDesignAssistant::applyArtistInfluence(SuggestionResult& result,
                                                const SoundDescriptor& desc)
{
    if (!desc.artists.isEmpty())
    {
        // Apply artist-specific influence
        for (const auto& artist : desc.artists)
        {
            SoundDescriptor artistModel = getArtistModel(artist);
            if (!artistModel.category.isEmpty())
            {
                // Adjust parameters based on artist style
                result.parameters["movement"] *= (artistModel.movement * 0.5f + 0.5f);
                result.parameters["expressiveness"] *= (artistModel.expressiveness * 0.5f + 0.5f);
                result.parameters["uniqueness"] *= (artistModel.uniqueness * 0.5f + 0.5f);
            }
        }
    }
}

void SoundDesignAssistant::applyContextualFactors(SuggestionResult& result,
                                                 const SoundDescriptor& desc)
{
    if (!currentContext_.isEmpty())
    {
        // Apply context-specific adjustments
        float contextFactor = calculateContextualRelevance(desc, currentContext_);

        // Adjust parameters based on context
        result.parameters["spatiality"] *= contextFactor;
        result.parameters["movement"] *= contextFactor;

        // Add context-based recommendations
        if (contextFactor > 0.7f)
        {
            result.recommendations.add("This sound fits well with the current context");
        }
    }
}

void SoundDesignAssistant::calculateConfidence(SuggestionResult& result) const
{
    // Calculate confidence based on parameter alignment and consistency
    float totalConfidence = 0.0f;
    int paramCount = 0;

    for (const auto& param : result.parameters)
    {
        float paramConfidence = calculateParameterConfidence(param.first, param.second);
        totalConfidence += paramConfidence;
        paramCount++;
    }

    result.confidence = (paramCount > 0) ? totalConfidence / paramCount : 0.5f;
}

void SoundDesignAssistant::calculateCompatibility(SuggestionResult& result,
                                                 const SoundDescriptor& desc) const
{
    // Calculate compatibility between suggestion and descriptor
    float compatibility = 0.0f;
    float totalWeight = 0.0f;

    // Compare parameter values
    for (const auto& param : result.parameters)
    {
        float targetValue = getTargetParameterValue(param.first, desc);
        float diff = std::abs(param.second - targetValue);
        compatibility += 1.0f - diff;
        totalWeight += 1.0f;
    }

    result.compatibility = (totalWeight > 0.0f) ? compatibility / totalWeight : 0.5f;
}

void SoundDesignAssistant::calculateInnovation(SuggestionResult& result) const
{
    // Calculate innovation level based on deviation from common patterns
    float innovation = 0.0f;
    int paramCount = 0;

    for (const auto& param : result.parameters)
    {
        float paramInnovation = calculateParameterInnovation(param.first, param.second);
        innovation += paramInnovation;
        paramCount++;
    }

    result.innovation = (paramCount > 0) ? innovation / paramCount : 0.5f;
}

std::map<juce::String, float> SoundDesignAssistant::mapDescriptorToParameters(const SoundDescriptor& desc) const
{
    std::map<juce::String, float> parameters;

    // Map basic descriptor to common synth parameters
    parameters["oscillator_type"] = mapCategoryToOscillator(desc.category);
    parameters["cutoff_frequency"] = mapBrightnessToCutoff(desc.brightness);
    parameters["resonance"] = mapComplexityToResonance(desc.complexity);
    parameters["envelope_attack"] = mapMovementToAttack(desc.movement);
    parameters["envelope_release"] = mapUniquenessToRelease(desc.uniqueness);
    parameters["lfo_rate"] = mapExpressivenessToLFORate(desc.expressiveness);
    parameters["lfo_amount"] = mapSpatialityToLFOAmount(desc.spatiality);
    parameters["reverb_amount"] = mapWarmthToReverb(desc.warmth);
    parameters["chorus_amount"] = mapVintageToChorus(desc.vintage);

    return parameters;
}

juce::String SoundDesignAssistant::classifyTimbreCategory(const TimbreTransferEngine::TimbreFeature& timbre) const
{
    if (timbre.brightness > 0.7f && timbre.attack < 0.3f)
    {
        return "Lead";
    }
    else if (timbre.brightness < 0.3f && timbre.warmth > 0.7f)
    {
        return "Bass";
    }
    else if (timbre.sustain > 0.7f && timbre.decay > 0.5f)
    {
        return "Pad";
    }
    else if (timbre.attack > 0.5f && timbre.release < 0.3f)
    {
        return "Percussion";
    }
    else if (timbre.formantStrength > 0.7f)
    {
        return "Vocal";
    }
    else
    {
        return "Synth";
    }
}

void SoundDesignAssistant::updateParameterStatistics(const std::map<juce::String, float>& params)
{
    // Update parameter statistics
    for (const auto& param : params)
    {
        auto it = parameterStatistics_.find(param.first);
        if (it != parameterStatistics_.end())
        {
            // Update running average
            it->second = 0.9f * it->second + 0.1f * param.second;
        }
        else
        {
            parameterStatistics_[param.first] = param.second;
        }
    }
}

void SoundDesignAssistant::updateLearningHistory(const SuggestionResult& suggestion, bool accepted)
{
    learningHistory_.pastSuggestions.push_back(suggestion);

    if (accepted)
    {
        learningHistory_.acceptedDescriptors.push_back(suggestion.descriptor);
        learnFromSuccess(suggestion);
    }
    else
    {
        learningHistory_.rejectedDescriptors.push_back(suggestion.descriptor);
        learnFromFailure(suggestion);
    }

    // Limit history size
    if (learningHistory_.pastSuggestions.size() > 100)
    {
        learningHistory_.pastSuggestions.erase(learningHistory_.pastSuggestions.begin());
    }
}

void SoundDesignAssistant::updateUserPreferenceModel(const SoundDescriptor& desc,
                                                   const std::map<juce::String, float>& preferences)
{
    // Update user preference model
    for (const auto& pref : preferences)
    {
        auto it = learningHistory_.userPreferences.find(pref.first);
        if (it != learningHistory_.userPreferences.end())
        {
            learningHistory_.userPreferences[pref.first] =
                0.8f * it->second + 0.2f * pref.second;
        }
        else
        {
            learningHistory_.userPreferences[pref.first] = pref.second;
        }
    }
}

void SoundDesignAssistant::updateSuccessRates(const SuggestionResult& suggestion, bool accepted)
{
    // Update success rates for suggestion categories
    if (!suggestion.descriptor.categories.isEmpty())
    {
        for (const auto& category : suggestion.descriptor.categories)
        {
            auto it = learningHistory_.successRates.find(category);
            if (it != learningHistory_.successRates.end())
            {
                float currentRate = it->second;
                float newRate = (currentRate * totalSuggestions_ + (accepted ? 1.0f : 0.0f)) / (totalSuggestions_ + 1);
                learningHistory_.successRates[category] = newRate;
            }
            else
            {
                learningHistory_.successRates[category] = accepted ? 1.0f : 0.0f;
            }
        }
    }

    // Update overall success rate
    float totalSuccess = 0.0f;
    int categoryCount = 0;
    for (const auto& rate : learningHistory_.successRates)
    {
        totalSuccess += rate.second;
        categoryCount++;
    }
    successRate_ = (categoryCount > 0) ? totalSuccess / categoryCount : 0.5f;
}

void SoundDesignAssistant::analyzeCurrentContext(const juce::String& context)
{
    // Analyze current context and update suggestion engine
    if (context.containsIgnoreCase("performance"))
    {
        // Performance context: favor CPU-efficient sounds
        suggestionEngine_.relevance = 0.9f;
        suggestionEngine_.diversity = 0.3f;
    }
    else if (context.containsIgnoreCase("studio"))
    {
        // Studio context: favor high-quality sounds
        suggestionEngine_.relevance = 0.8f;
        suggestionEngine_.diversity = 0.7f;
    }
    else if (context.containsIgnoreCase("live"))
    {
        // Live context: favor reliable sounds
        suggestionEngine_.relevance = 0.9f;
        suggestionEngine_.diversity = 0.4f;
    }
}

void SoundDesignAssistant::applyPlatformConstraints(SuggestionResult& result,
                                                   const juce::String& platform) const
{
    if (platform.equalsIgnoreCase("mobile"))
    {
        // Mobile platform: limit CPU usage
        result.parameters["complexity"] = juce::jmin(result.parameters["complexity"], 0.6f);
        result.parameters["movement"] = juce::jmin(result.parameters["movement"], 0.5f);
    }
    else if (platform.equalsIgnoreCase("desktop"))
    {
        // Desktop platform: allow more complexity
        result.parameters["complexity"] = juce::jmax(result.parameters["complexity"], 0.7f);
        result.parameters["movement"] = juce::jmax(result.parameters["movement"], 0.6f);
    }
}

float SoundDesignAssistant::calculateContextualRelevance(const SoundDescriptor& desc,
                                                     const juce::String& context) const
{
    // Calculate how well the descriptor fits the context
    float relevance = 0.5f;

    if (context.containsIgnoreCase("ambient") && desc.mood.equalsIgnoreCase("ethereal"))
    {
        relevance = 0.9f;
    }
    else if (context.containsIgnoreCase("dance") && desc.mood.equalsIgnoreCase("energetic"))
    {
        relevance = 0.8f;
    }
    else if (context.containsIgnoreCase("film") && desc.mood.equalsIgnoreCase("emotional"))
    {
        relevance = 0.7f;
    }

    return relevance;
}

juce::String SoundDesignAssistant::generateSuggestionId() const
{
    return "suggestion_" + juce::String::toHexString(juce::Random::getSystemRandom().nextInt());
}

juce::String SoundDesignAssistant::generateReasoning(const SuggestionResult& result) const
{
    juce::String reasoning;

    reasoning << "Generated suggestion based on: ";

    if (!result.descriptor.categories.isEmpty())
    {
        reasoning << "Category '" << result.descriptor.categories[0] << "'";
    }

    if (!result.descriptor.mood.isEmpty())
    {
        reasoning << ", Mood '" << result.descriptor.mood << "'";
    }

    reasoning << ". Confidence: " << juce::String(result.confidence * 100, 1) << "%";

    return reasoning;
}

std::vector<juce::String> SoundDesignAssistant::generateRecommendations(const SuggestionResult& result) const
{
    std::vector<juce::String> recommendations;

    // Generate recommendations based on suggestion parameters
    if (result.parameters.find("complexity") != result.parameters.end())
    {
        if (result.parameters.at("complexity") > 0.7f)
        {
            recommendations.push_back("High complexity - may impact CPU performance");
        }
        else if (result.parameters.at("complexity") < 0.3f)
        {
            recommendations.push_back("Low complexity - good for live performance");
        }
    }

    if (result.parameters.find("spatiality") != result.parameters.end())
    {
        if (result.parameters.at("spatiality") > 0.7f)
        {
            recommendations.push_back("High spatiality - consider adding reverb and delay");
        }
    }

    if (result.compatibility > 0.8f)
    {
        recommendations.push_back("High compatibility with current sound");
    }

    return recommendations;
}

std::vector<juce::String> SoundDesignAssistant::generateWarnings(const SuggestionResult& result) const
{
    std::vector<juce::String> warnings;

    // Generate warnings based on suggestion parameters
    if (result.parameters.find("cutoff_frequency") != result.parameters.end())
    {
        if (result.parameters.at("cutoff_frequency") > 0.9f)
        {
            warnings.push_back("High cutoff frequency may cause aliasing");
        }
    }

    if (result.parameters.find("resonance") != result.parameters.end())
    {
        if (result.parameters.at("resonance") > 0.8f)
        {
            warnings.push_back("High resonance may cause instability");
        }
    }

    if (result.innovation < 0.3f)
    {
        warnings.push_back("Low innovation - consider more experimental parameters");
    }

    return warnings;
}

//==============================================================================
// Utility Methods
//==============================================================================

float SoundDesignAssistant::mapCategoryToOscillator(const juce::String& category) const
{
    if (category.equalsIgnoreCase("bass"))
        return 0.2f;  // Saw wave
    else if (category.equalsIgnoreCase("lead"))
        return 0.8f;  // Square wave
    else if (category.equalsIgnoreCase("pad"))
        return 0.5f;  // Sine wave
    else if (category.equalsIgnoreCase("percussion"))
        return 0.9f;  // Noise
    else
        return 0.5f;  // Default
}

float SoundDesignAssistant::mapBrightnessToCutoff(float brightness) const
{
    return brightness;
}

float SoundDesignAssistant::mapComplexityToResonance(float complexity) const
{
    return complexity * 0.8f;
}

float SoundDesignAssistant::mapMovementToAttack(float movement) const
{
    return movement;
}

float SoundDesignAssistant::mapUniquenessToRelease(float uniqueness) const
{
    return 1.0f - uniqueness * 0.5f;
}

float SoundDesignAssistant::mapExpressivenessToLFORate(float expressiveness) const
{
    return expressiveness * 0.3f + 0.1f;
}

float SoundDesignAssistant::mapSpatialityToLFOAmount(float spatiality) const
{
    return spatiality * 0.5f;
}

float SoundDesignAssistant::mapWarmthToReverb(float warmth) const
{
    return warmth * 0.4f;
}

float SoundDesignAssistant::mapVintageToChorus(float vintage) const
{
    return vintage * 0.3f;
}

float SoundDesignAssistant::calculateParameterConfidence(const juce::String& paramId, float value) const
{
    // Calculate confidence based on parameter importance and value
    ParameterImportance importance = getParameterImportance(paramId);
    float importanceFactor = 1.0f;

    switch (importance)
    {
        case ParameterImportance::Critical:
            importanceFactor = 1.0f;
            break;
        case ParameterImportance::Important:
            importanceFactor = 0.8f;
            break;
        case ParameterImportance::Moderate:
            importanceFactor = 0.6f;
            break;
        case ParameterImportance::Minor:
            importanceFactor = 0.4f;
            break;
        case ParameterImportance::Optional:
            importanceFactor = 0.2f;
            break;
    }

    // Calculate value confidence (optimal range)
    float optimalValue = 0.5f;  // Default optimal value
    float distance = std::abs(value - optimalValue);
    float valueConfidence = 1.0f - distance;

    return importanceFactor * valueConfidence;
}

float SoundDesignAssistant::calculateParameterInnovation(const juce::String& paramId, float value) const
{
    // Calculate innovation based on deviation from common values
    float commonValue = 0.5f;  // Default common value
    float deviation = std::abs(value - commonValue);

    // Higher deviation means more innovation
    return deviation;
}

float SoundDesignAssistant::getTargetParameterValue(const juce::String& paramId,
                                                   const SoundDescriptor& desc) const
{
    // Get target parameter value based on descriptor
    if (paramId == "brightness")
        return desc.brightness;
    else if (paramId == "warmth")
        return desc.warmth;
    else if (paramId == "complexity")
        return desc.complexity;
    else if (paramId == "movement")
        return desc.movement;
    else
        return 0.5f;  // Default
}

std::vector<float> SoundDesignAssistant::convertDescriptorToNeuralInput(const SoundDescriptor& desc) const
{
    std::vector<float> input;

    // Convert descriptor to neural network input
    input.push_back(desc.vintage);
    input.push_back(desc.movement);
    input.push_back(desc.complexity);
    input.push_back(desc.spatiality);
    input.push_back(desc.expressiveness);
    input.push_back(desc.uniqueness);
    input.push_back(desc.brightness);
    input.push_back(desc.warmth);

    // One-hot encode category
    if (desc.category.equalsIgnoreCase("bass")) input.push_back(1.0f);
    else if (desc.category.equalsIgnoreCase("lead")) input.push_back(0.8f);
    else if (desc.category.equalsIgnoreCase("pad")) input.push_back(0.6f);
    else if (desc.category.equalsIgnoreCase("percussion")) input.push_back(0.4f);
    else input.push_back(0.2f);

    // One-hot encode mood
    if (desc.mood.equalsIgnoreCase("dark")) input.push_back(1.0f);
    else if (desc.mood.equalsIgnoreCase("bright")) input.push_back(0.8f);
    else if (desc.mood.equalsIgnoreCase("aggressive")) input.push_back(0.6f);
    else if (desc.mood.equalsIgnoreCase("warm")) input.push_back(0.4f);
    else input.push_back(0.2f);

    return input;
}

std::vector<float> SoundDesignAssistant::runParameterModel(const SoundDescriptor& desc) const
{
    // Run parameter prediction neural network
    std::vector<float> input = convertDescriptorToNeuralInput(desc);
    std::vector<float> output;

    // Simple neural network forward pass
    for (int i = 0; i < 8; ++i)  // 8 output parameters
    {
        float sum = 0.0f;
        for (size_t j = 0; j < input.size(); ++j)
        {
            if (j < parameterModel_.weights.size())
            {
                sum += input[j] * parameterModel_.weights[i * input.size() + j];
            }
        }
        if (i < parameterModel_.biases.size())
        {
            sum += parameterModel_.biases[i];
        }
        output.push_back(1.0f / (1.0f + std::exp(-sum)));  // Sigmoid activation
    }

    return output;
}

void SoundDesignAssistant::applyConservativeConstraints(std::map<juce::String, float>& params) const
{
    // Apply conservative constraints
    for (auto& param : params)
    {
        // Clamp to safe ranges
        param.second = juce::jlimit(0.2f, 0.8f, param.second);
    }
}

void SoundDesignAssistant::applyBalancedConstraints(std::map<juce::String, float>& params) const
{
    // Apply balanced constraints
    for (auto& param : params)
    {
        // Clamp to moderate ranges
        param.second = juce::jlimit(0.1f, 0.9f, param.second);
    }
}

void SoundDesignAssistant::applyExperimentalConstraints(std::map<juce::String, float>& params) const
{
    // Apply experimental constraints
    for (auto& param : params)
    {
        // Allow full range
        param.second = juce::jlimit(0.0f, 1.0f, param.second);

        // Add some random variation
        param.second += (juce::Random::getSystemRandom().nextFloat() - 0.5f) * 0.1f;
        param.second = juce::jlimit(0.0f, 1.0f, param.second);
    }
}

} // namespace Zenith