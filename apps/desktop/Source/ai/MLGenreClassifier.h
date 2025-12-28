/*
  ==============================================================================
    MLGenreClassifier.h
    Real machine learning genre classification using neural networks
    Phase 2: Context-Aware AI (10/10)
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "GenreDetector.h"
#include <vector>
#include <memory>

namespace zenith {
namespace ai {

// Simple neural network implementation for genre classification
class MLGenreNetwork {
public:
    struct Layer {
        std::vector<std::vector<float>> weights;
        std::vector<float> biases;
        std::vector<float> activations;
        
        Layer(int inputSize, int outputSize);
        void forward(const std::vector<float>& input);
        std::vector<float> getOutput() const { return activations; }
    };
    
    MLGenreNetwork(const std::vector<int>& layerSizes);
    std::vector<float> forward(const std::vector<float>& input);
    void loadWeights(const juce::File& weightsFile);
    void saveWeights(const juce::File& weightsFile) const;
    
private:
    std::vector<Layer> layers;
    static float sigmoid(float x);
    static float relu(float x);
};

class MLGenreClassifier {
public:
    MLGenreClassifier();
    ~MLGenreClassifier();
    
    // Training methods
    bool loadTrainingData(const juce::File& datasetPath);
    void train(int epochs = 100, float learningRate = 0.01f);
    void saveModel(const juce::File& modelPath) const;
    bool loadModel(const juce::File& modelPath);
    
    // Classification methods
     GenrePrediction classifyGenre(const GenreFeatures& features);
    GenrePrediction classifyGenreFromProcessed(const std::vector<float>& processedFeatures, const GenreFeatures* originalFeatures = nullptr);
    std::vector<GenrePrediction> classifyMultiple(const GenreFeatures& features, int topK = 3);
    
    // Feature preprocessing
    std::vector<float> preprocessFeatures(const GenreFeatures& features);
    void normalizeFeatures(std::vector<float>& features);
    
    // Model information
    std::vector<juce::String> getSupportedGenres() const;
    float getModelAccuracy() const { return accuracy; }
    
private:
    std::unique_ptr<MLGenreNetwork> network;
    std::vector<juce::String> genreLabels;
    std::vector<std::vector<float>> trainingData;
    std::vector<int> trainingLabels;
    
    float accuracy = 0.0f;
    bool isTrained = false;
    
    // Training helpers
    void initializeNetwork();
    void splitTrainingData(float trainRatio = 0.8f);
    std::vector<float> oneHotEncode(int label, int numClasses) const;
    int predictLabel(const std::vector<float>& output) const;
    
    // Feature scaling parameters
    std::vector<float> featureMeans;
    std::vector<float> featureStds;
    void calculateFeatureStatistics();
};

// Real-time genre adaptation based on user feedback
class AdaptiveGenreClassifier {
public:
    AdaptiveGenreClassifier();
    ~AdaptiveGenreClassifier();
    
    // Classification with learning
    GenrePrediction classifyWithAdaptation(const GenreFeatures& features,
                                           const juce::String& userHint = "");
    
    // User feedback integration
    void provideFeedback(const GenreFeatures& features,
                        const juce::String& correctGenre,
                        float confidence = 1.0f);
    
    // Adaptation parameters
    void setLearningRate(float rate) { learningRate = rate; }
    void setAdaptationStrength(float strength) { adaptationStrength = strength; }
    
private:
    std::unique_ptr<MLGenreClassifier> baseClassifier;
    std::vector<std::pair<GenreFeatures, juce::String>> feedbackHistory;
    
    float learningRate = 0.01f;
    float adaptationStrength = 0.1f;
    
    // Adaptation logic
    void updateModelFromFeedback();
    std::vector<float> adaptFeatures(const GenreFeatures& features,
                                     const juce::String& userHint) const;
};

} // namespace ai
} // namespace zenith
