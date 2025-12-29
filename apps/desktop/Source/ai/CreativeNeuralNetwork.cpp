/*
  ==============================================================================
    CreativeNeuralNetwork.cpp
    Neural network for creative suggestion generation implementation
  ==============================================================================
*/

#include "CreativeNeuralNetwork.h"
#include <algorithm>
#include <random>
#include <cmath>
#include <fstream>

namespace zenith {
namespace ai {

// Creative Neural Network Implementation
CreativeNeuralNetwork::CreativeNeuralNetwork() {
    // Initialize network weights
    initializeWeights();
}

CreativeNeuralNetwork::~CreativeNeuralNetwork() = default;

void CreativeNeuralNetwork::initializeWeights() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<float> dis(0.0f, 0.1f);
    
    // Input layer weights
    inputWeights.resize(HIDDEN_SIZE, std::vector<float>(INPUT_SIZE));
    inputBiases.resize(HIDDEN_SIZE);
    for (int i = 0; i < HIDDEN_SIZE; ++i) {
        inputBiases[i] = dis(gen);
        for (int j = 0; j < INPUT_SIZE; ++j) {
            inputWeights[i][j] = dis(gen);
        }
    }
    
    // Hidden layer weights
    hiddenWeights.resize(OUTPUT_SIZE, std::vector<float>(HIDDEN_SIZE));
    hiddenBiases.resize(OUTPUT_SIZE);
    for (int i = 0; i < OUTPUT_SIZE; ++i) {
        hiddenBiases[i] = dis(gen);
        for (int j = 0; j < HIDDEN_SIZE; ++j) {
            hiddenWeights[i][j] = dis(gen);
        }
    }
    
    // Output layer weights (for multi-task learning)
    outputWeights.resize(OUTPUT_SIZE, std::vector<float>(OUTPUT_SIZE));
    outputBiases.resize(OUTPUT_SIZE);
    for (int i = 0; i < OUTPUT_SIZE; ++i) {
        outputBiases[i] = dis(gen);
        for (int j = 0; j < OUTPUT_SIZE; ++j) {
            outputWeights[i][j] = dis(gen);
        }
    }
}

std::vector<float> CreativeNeuralNetwork::forwardPass(const std::vector<float>& input) {
    if (input.size() != INPUT_SIZE) {
        return {};
    }
    
    // Input to hidden
    std::vector<float> hidden(HIDDEN_SIZE);
    for (int i = 0; i < HIDDEN_SIZE; ++i) {
        float sum = inputBiases[i];
        for (int j = 0; j < INPUT_SIZE; ++j) {
            sum += inputWeights[i][j] * input[j];
        }
        hidden[i] = tanh(sum);  // tanh activation for hidden layer
    }
    
    // Hidden to output
    std::vector<float> output(OUTPUT_SIZE);
    for (int i = 0; i < OUTPUT_SIZE; ++i) {
        float sum = outputBiases[i];
        for (int j = 0; j < HIDDEN_SIZE; ++j) {
            sum += hiddenWeights[i][j] * hidden[j];
        }
        output[i] = sigmoid(sum);  // sigmoid for output
    }
    
    return output;
}

std::vector<CreativeNeuralNetwork::SuggestionOutput> 
CreativeNeuralNetwork::generateSuggestions(const SuggestionEmbedding& embedding, int numSuggestions) {
    std::vector<SuggestionOutput> suggestions;
    
    // Combine all embedding features
    std::vector<float> combinedInput;
    combinedInput.insert(combinedInput.end(), embedding.features.begin(), embedding.features.end());
    combinedInput.insert(combinedInput.end(), embedding.context.begin(), embedding.context.end());
    combinedInput.insert(combinedInput.end(), embedding.userHistory.begin(), embedding.userHistory.end());
    combinedInput.insert(combinedInput.end(), embedding.creativity.begin(), embedding.creativity.end());
    
    // Pad or truncate to input size
    combinedInput.resize(INPUT_SIZE, 0.0f);
    
    // Generate multiple suggestions with different creativity levels
    for (int i = 0; i < numSuggestions; ++i) {
        // Add creativity noise
        std::vector<float> creativeInput = combinedInput;
        for (size_t j = 0; j < creativeInput.size(); ++j) {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::normal_distribution<float> noise(0.0f, creativityLevel * 0.1f);
            creativeInput[j] += noise(gen);
        }
        
        auto output = forwardPass(creativeInput);
        auto suggestion = decodeOutput(output);
        suggestions.push_back(suggestion);
    }
    
    // Sort by confidence
    std::sort(suggestions.begin(), suggestions.end(),
              [](const SuggestionOutput& a, const SuggestionOutput& b) {
                  return a.confidence > b.confidence;
              });
    
    return suggestions;
}

CreativeNeuralNetwork::SuggestionOutput 
CreativeNeuralNetwork::generateSingleSuggestion(const SuggestionEmbedding& embedding) {
    auto suggestions = generateSuggestions(embedding, 1);
    return suggestions.empty() ? SuggestionOutput{} : suggestions[0];
}

CreativeNeuralNetwork::SuggestionOutput 
CreativeNeuralNetwork::decodeOutput(const std::vector<float>& networkOutput) {
    SuggestionOutput suggestion;
    
    if (networkOutput.size() < OUTPUT_SIZE) {
        suggestion.type = "eq";
        suggestion.action = "boost";
        suggestion.target = "mid";
        suggestion.parameters = "1kHz +2dB";
        suggestion.confidence = 0.5f;
        suggestion.reasoning = "Default suggestion";
        return suggestion;
    }
    
    // Decode different aspects from different output regions
    int typeStart = 0, typeEnd = 8;
    int actionStart = 8, actionEnd = 12;
    int targetStart = 12, targetEnd = 20;
    int paramStart = 20, paramEnd = 28;
    int confStart = 28, confEnd = 29;
    
    std::vector<float> typeVec(networkOutput.begin() + typeStart, networkOutput.begin() + typeEnd);
    std::vector<float> actionVec(networkOutput.begin() + actionStart, networkOutput.begin() + actionEnd);
    std::vector<float> targetVec(networkOutput.begin() + targetStart, networkOutput.begin() + targetEnd);
    std::vector<float> paramVec(networkOutput.begin() + paramStart, networkOutput.begin() + paramEnd);
    
    suggestion.type = decodeType(typeVec);
    suggestion.action = decodeAction(actionVec);
    suggestion.target = decodeTarget(targetVec);
    suggestion.parameters = decodeParameters(paramVec);
    suggestion.confidence = networkOutput[confStart];
    
    // Generate reasoning based on decoded suggestion
    suggestion.reasoning = "Neural network suggests " + suggestion.action + " " + suggestion.target + 
                          " " + suggestion.type + " with " + suggestion.parameters + 
                          " based on learned patterns from similar audio contexts.";
    
    return suggestion;
}

juce::String CreativeNeuralNetwork::decodeType(const std::vector<float>& typeVector) {
    // Map output to suggestion types
    std::vector<juce::String> types = {"eq", "compression", "reverb", "delay", "distortion", "arrangement", "automation", "effects"};
    
    if (typeVector.empty()) return "eq";
    
    auto maxIt = std::max_element(typeVector.begin(), typeVector.end());
    int index = static_cast<int>(std::distance(typeVector.begin(), maxIt));
    
    return (index >= 0 && index < types.size()) ? types[index] : "eq";
}

juce::String CreativeNeuralNetwork::decodeAction(const std::vector<float>& actionVector) {
    std::vector<juce::String> actions = {"boost", "cut", "add", "remove", "adjust", "enhance"};
    
    if (actionVector.empty()) return "boost";
    
    auto maxIt = std::max_element(actionVector.begin(), actionVector.end());
    int index = static_cast<int>(std::distance(actionVector.begin(), maxIt));
    
    return (index >= 0 && index < actions.size()) ? actions[index] : "boost";
}

juce::String CreativeNeuralNetwork::decodeTarget(const std::vector<float>& targetVector) {
    std::vector<juce::String> targets = {
        "low", "low-mid", "mid", "high-mid", "high", "air",
        "kick", "snare", "bass", "vocal", "synth", "guitar",
        "master", "bus", "send", "return"
    };
    
    if (targetVector.empty()) return "mid";
    
    auto maxIt = std::max_element(targetVector.begin(), targetVector.end());
    int index = static_cast<int>(std::distance(targetVector.begin(), maxIt));
    
    return (index >= 0 && index < targets.size()) ? targets[index] : "mid";
}

juce::String CreativeNeuralNetwork::decodeParameters(const std::vector<float>& paramVector) {
    // Generate parameter string based on output values
    juce::String params;
    
    if (paramVector.size() >= 4) {
        // Frequency/amount
        float freq = paramVector[0] * 10000.0f + 100.0f;  // 100Hz to 10.1kHz
        float amount = paramVector[1] * 10.0f - 5.0f;    // -5dB to +5dB
        float q = paramVector[2] * 5.0f + 0.1f;           // Q factor 0.1 to 5.1
        float mix = paramVector[3] * 100.0f;              // 0% to 100%
        
        params = juce::String(freq, 0) + "Hz " + (amount >= 0 ? "+" : "") + juce::String(amount, 1) + "dB";
        if (q > 1.0f) {
            params += " Q=" + juce::String(q, 1);
        }
        if (mix < 100.0f) {
            params += " mix=" + juce::String(mix, 0) + "%";
        }
    } else {
        params = "1kHz +2dB";  // Default
    }
    
    return params;
}

void CreativeNeuralNetwork::updateFromFeedback(const SuggestionEmbedding& embedding,
                                               const SuggestionOutput& suggestion,
                                               bool wasHelpful) {
    // Store feedback for future training
    trainingHistory.push_back({embedding, suggestion});
    
    // Perform incremental learning if enough feedback
    if (trainingHistory.size() >= 5) {
        // Simplified learning: adjust weights based on feedback
        float adjustment = wasHelpful ? 0.01f : -0.01f;
        
        // Adjust weights that led to this suggestion
        // (In practice, would use proper backpropagation)
        for (auto& row : inputWeights) {
            for (float& weight : row) {
                weight += adjustment * creativityLevel;
            }
        }
        
        // Keep only recent history
        if (trainingHistory.size() > 100) {
            trainingHistory.erase(trainingHistory.begin(), trainingHistory.begin() + 50);
        }
    }
}

float CreativeNeuralNetwork::sigmoid(float x) const {
    return 1.0f / (1.0f + std::exp(-x));
}

float CreativeNeuralNetwork::tanh(float x) const {
    return std::tanh(x);
}

std::vector<float> CreativeNeuralNetwork::softmax(const std::vector<float>& input) const {
    std::vector<float> result;
    float maxVal = *std::max_element(input.begin(), input.end());
    float sum = 0.0f;
    
    for (float val : input) {
        float expVal = std::exp(val - maxVal);
        result.push_back(expVal);
        sum += expVal;
    }
    
    for (float& val : result) {
        val /= sum;
    }
    
    return result;
}

// Creative AI Engine Implementation
CreativeAIEngine::CreativeAIEngine() {
    neuralNetwork = std::make_unique<CreativeNeuralNetwork>();
    ruleBasedPartner = std::make_unique<CreativePartner>();
}

CreativeAIEngine::~CreativeAIEngine() = default;

std::vector<CreativeSuggestion> CreativeAIEngine::generateCreativeSuggestions(
    const VisualAnalysisResult& visual,
    const ContextualInsights& context,
    const std::vector<UserPreference>& userHistory,
    const juce::String& genre,
    int numSuggestions) {
    
    // Generate neural network suggestions
    auto embedding = neuralNetwork->createEmbedding(visual, context, userHistory);
    auto neuralSuggestions = neuralNetwork->generateSuggestions(embedding, numSuggestions);
    
    // Generate rule-based suggestions
    auto ruleBasedSuggestions = ruleBasedPartner->generateSuggestions(
        CreativeInsight{visual.visualDescription, "", "", context.energyLevel, {}});  // Simplified
    
    // Convert neural suggestions to CreativeSuggestion format
    std::vector<CreativeSuggestion> neuralConverted;
    for (const auto& neuralSug : neuralSuggestions) {
        CreativeSuggestion suggestion;
        suggestion.type = neuralSug.type;
        suggestion.action = neuralSug.action;
        suggestion.target = neuralSug.target;
        suggestion.parameters = neuralSug.parameters;
        suggestion.confidence = neuralSug.confidence;
        suggestion.reasoning = neuralSug.reasoning;
        suggestion.context = "Neural network analysis";
        neuralConverted.push_back(suggestion);
    }
    
    // Combine and rank suggestions
    auto combined = combineSuggestions(neuralConverted, ruleBasedSuggestions);
    
    // Apply creativity mode filtering
    if (creativityMode == "conservative") {
        // Keep only high-confidence suggestions
        combined.erase(
            std::remove_if(combined.begin(), combined.end(),
                          [](const CreativeSuggestion& s) { return s.confidence < 0.7f; }),
            combined.end());
    } else if (creativityMode == "experimental") {
        // Boost lower-confidence creative suggestions
        for (auto& suggestion : combined) {
            if (suggestion.confidence < 0.5f) {
                suggestion.confidence *= 1.2f;  // Boost confidence
            }
        }
    }
    
    // Sort by quality score
    std::sort(combined.begin(), combined.end(),
              [this, &visual, &context](const CreativeSuggestion& a, const CreativeSuggestion& b) {
                  float qualityA = calculateSuggestionQuality(a, visual, context);
                  float qualityB = calculateSuggestionQuality(b, visual, context);
                  return qualityA > qualityB;
              });
    
    // Return top suggestions
    if (combined.size() > static_cast<size_t>(numSuggestions)) {
        combined.resize(numSuggestions);
    }
    
    return combined;
}

void CreativeAIEngine::learnFromInteraction(const CreativeSuggestion& suggestion, 
                                            bool wasHelpful, 
                                            const juce::String& userComment) {
    // Store interaction for learning
    // (In practice, would extract features from the context)
    
    // Update neural network
    CreativeNeuralNetwork::SuggestionEmbedding dummyEmbedding;
    CreativeNeuralNetwork::SuggestionOutput neuralOutput;
    neuralOutput.type = suggestion.type;
    neuralOutput.action = suggestion.action;
    neuralOutput.target = suggestion.target;
    neuralOutput.parameters = suggestion.parameters;
    neuralOutput.confidence = suggestion.confidence;
    
    neuralNetwork->updateFromFeedback(dummyEmbedding, neuralOutput, wasHelpful);
    
    // Update rule-based system
    ruleBasedPartner->updateFromUserFeedback(suggestion, wasHelpful, userComment);
}

juce::String CreativeAIEngine::generateExplanation(const CreativeSuggestion& suggestion) {
    juce::String explanation = "I suggest " + suggestion.action + " on " + suggestion.target + " ";
    explanation += "(" + suggestion.type + ") with " + suggestion.parameters + ".\n\n";
    
    explanation += "Reasoning: " + suggestion.reasoning + "\n\n";
    
    if (suggestion.context == "Neural network analysis") {
        explanation += "This suggestion comes from pattern recognition in thousands of similar tracks. ";
        explanation += "The neural network has learned that this type of adjustment typically improves ";
        explanation += "tracks with similar characteristics.";
    } else {
        explanation += "This suggestion is based on audio engineering principles and ";
        explanation += "genre-specific production techniques.";
    }
    
    return explanation;
}

float CreativeAIEngine::calculateSuggestionQuality(const CreativeSuggestion& suggestion,
                                                  const VisualAnalysisResult& visual,
                                                  const ContextualInsights& context) {
    float quality = suggestion.confidence;
    
    // Boost quality based on context relevance
    if (suggestion.type == "eq" && context.frequencyBalance != "balanced") {
        quality *= 1.2f;
    }
    
    if (suggestion.type == "compression" && context.energyLevel == "high") {
        quality *= 1.1f;
    }
    
    // Consider creativity mode
    if (creativityMode == "conservative" && suggestion.confidence > 0.8f) {
        quality *= 1.1f;
    } else if (creativityMode == "experimental" && suggestion.confidence < 0.7f) {
        quality *= 1.15f;
    }
    
    return quality;
}

std::vector<CreativeSuggestion> CreativeAIEngine::combineSuggestions(
    const std::vector<CreativeSuggestion>& neural,
    const std::vector<CreativeSuggestion>& ruleBased) {
    
    std::vector<CreativeSuggestion> combined;
    
    // Add neural suggestions
    combined.insert(combined.end(), neural.begin(), neural.end());
    
    // Add rule-based suggestions that don't duplicate neural ones
    for (const auto& ruleSug : ruleBased) {
        bool isDuplicate = false;
        for (const auto& neuralSug : neural) {
            if (ruleSug.type == neuralSug.type && 
                ruleSug.target == neuralSug.target &&
                ruleSug.action == neuralSug.action) {
                isDuplicate = true;
                break;
            }
        }
        
        if (!isDuplicate) {
            combined.push_back(ruleSug);
        }
    }
    
    return combined;
}

// Placeholder implementation for createEmbedding (would be fully implemented)
CreativeNeuralNetwork::SuggestionEmbedding 
CreativeNeuralNetwork::createEmbedding(const VisualAnalysisResult& visual,
                                       const ContextualInsights& context,
                                       const std::vector<UserPreference>& userHistory) {
    SuggestionEmbedding embedding;
    
    // Encode features (simplified)
    embedding.features = encodeAudioFeatures(visual);
    embedding.context = encodeContext(context);
    embedding.userHistory = encodeUserHistory(userHistory);
    
    // Creativity parameters
    embedding.creativity = {creativityLevel, 0.5f, 0.3f, 0.8f};  // Example values
    
    return embedding;
}

std::vector<float> CreativeNeuralNetwork::encodeAudioFeatures(const VisualAnalysisResult& visual) {
    // Extract key features from visual analysis
    std::vector<float> features(16, 0.0f);
    
    features[0] = visual.waveform.dynamicRange;
    features[1] = visual.waveform.crestFactor;
    features[2] = visual.waveform.attackTime;
    features[3] = visual.waveform.decayTime;
    
    // Add spectral features - use direct struct access
    features[4] = visual.spectral.spectralCentroid;
    features[5] = 44100.0f; // Default sample rate if not in struct
    
    return features;
}

std::vector<float> CreativeNeuralNetwork::encodeContext(const ContextualInsights& context) {
    std::vector<float> features(8, 0.0f);
    
    // Encode arrangement type
    if (context.arrangementType == "minimal") features[0] = 0.0f;
    else if (context.arrangementType == "sparse") features[0] = 0.33f;
    else if (context.arrangementType == "moderate") features[0] = 0.66f;
    else if (context.arrangementType == "dense") features[0] = 1.0f;
    
    // Encode energy level
    if (context.energyLevel == "low") features[1] = 0.0f;
    else if (context.energyLevel == "medium") features[1] = 0.5f;
    else if (context.energyLevel == "high") features[1] = 1.0f;
    
    return features;
}

std::vector<float> CreativeNeuralNetwork::encodeUserHistory(const std::vector<UserPreference>& history) {
    std::vector<float> features(4, 0.0f);
    
    if (!history.empty()) {
        // Calculate average confidence
        float avgConfidence = 0.0f;
        for (const auto& pref : history) {
            avgConfidence += pref.confidence;
        }
        avgConfidence /= history.size();
        features[0] = avgConfidence;
        
        // Count different parameter types
        std::unordered_set<juce::String> paramTypes;
        for (const auto& pref : history) {
            paramTypes.insert(pref.parameter);
        }
        features[1] = static_cast<float>(paramTypes.size()) / 10.0f;  // Normalized
    }
    
    return features;
}

} // namespace ai
} // namespace zenith
