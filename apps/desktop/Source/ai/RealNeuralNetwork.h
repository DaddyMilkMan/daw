/*
  ==============================================================================
    RealNeuralNetwork.h
    Actual neural network implementation with backpropagation training
    Phase 3: Autonomous Intelligence (10/10) - Fixed Implementation
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <vector>
#include <memory>
#include <random>
#include <functional>

namespace zenith {
namespace ai {

// Activation functions
enum class ActivationType {
    Sigmoid,
    Tanh,
    ReLU,
    LeakyReLU,
    Softmax
};

// Real neural network layer with backpropagation
class NeuralLayer {
public:
    NeuralLayer(int inputSize, int outputSize, ActivationType activation = ActivationType::ReLU);
    
    // Forward pass
    std::vector<float> forward(const std::vector<float>& input);
    
    // Backward pass
    std::vector<float> backward(const std::vector<float>& gradient, float learningRate);
    
    // Weight management
    void initializeWeights();
    void loadWeights(const std::vector<std::vector<float>>& weights, const std::vector<float>& biases);
    std::pair<std::vector<std::vector<float>>, std::vector<float>> getWeights() const;
    
    // Getters
    const std::vector<float>& getOutput() const { return output; }
    const std::vector<float>& getActivation() const { return activation; }
    int getInputSize() const { return inputSize; }
    int getOutputSize() const { return outputSize; }
    
private:
    int inputSize, outputSize;
    ActivationType activationType;
    
    // Weights and biases
    std::vector<std::vector<float>> weights;
    std::vector<float> biases;
    
    // Forward pass data
    std::vector<float> input;
    std::vector<float> weightedSum;
    std::vector<float> output;
    std::vector<float> activation;
    
    // Backward pass data
    std::vector<std::vector<float>> weightGradient;
    std::vector<float> biasGradient;
    
    // Activation functions and derivatives
    float activate(float x) const;
    float activateDerivative(float x) const;
    std::vector<float> activateVector(const std::vector<float>& x) const;
    std::vector<float> activateDerivativeVector(const std::vector<float>& x) const;
    
    // Weight initialization
    float xavierInitialization(int fanIn, int fanOut) const;
    float heInitialization(int fanIn) const;
};

// Complete neural network with training capabilities
class NeuralNetwork {
public:
    NeuralNetwork(const std::vector<int>& layerSizes, 
                  const std::vector<ActivationType>& activations = {});
    
    // Forward pass
    std::vector<float> forward(const std::vector<float>& input);
    
    // Training
    void train(const std::vector<std::vector<float>>& inputs,
               const std::vector<std::vector<float>>& targets,
               int epochs = 100,
               float learningRate = 0.01f,
               float batchSize = 32.0f,
               std::function<void(int, float)> progressCallback = nullptr);
    
    // Single sample training
    float trainSingle(const std::vector<float>& input,
                     const std::vector<float>& target,
                     float learningRate = 0.01f);
    
    // Prediction
    std::vector<float> predict(const std::vector<float>& input);
    float calculateLoss(const std::vector<float>& output, const std::vector<float>& target);
    
    // Model management
    void saveModel(const juce::File& filePath) const;
    void saveWeights(const juce::File& filePath) const;
    bool loadModel(const juce::File& filePath);
    void loadWeights(const juce::File& filePath);
    
    // Utility
    int getLayerCount() const { return static_cast<int>(layers.size()); }
    std::vector<int> getLayerSizes() const;
    void setLearningRate(float rate) { learningRate = rate; }
    float getLearningRate() const { return learningRate; }
    
private:
    std::vector<std::unique_ptr<NeuralLayer>> layers;
    float learningRate = 0.01f;
    
    // Loss functions
    float meanSquaredError(const std::vector<float>& output, const std::vector<float>& target) const;
    std::vector<float> meanSquaredErrorGradient(const std::vector<float>& output, const std::vector<float>& target) const;
    
    float crossEntropy(const std::vector<float>& output, const std::vector<float>& target) const;
    std::vector<float> crossEntropyGradient(const std::vector<float>& output, const std::vector<float>& target) const;
    
    // Training utilities
    void shuffleDataset(std::vector<std::vector<float>>& inputs,
                       std::vector<std::vector<float>>& targets);
    std::vector<float> createMiniBatch(const std::vector<std::vector<float>>& data,
                                       int startIndex, int batchSize);
    
    // Model serialization
    void saveLayer(std::ostream& stream, const NeuralLayer& layer) const;
    bool loadLayer(std::istream& stream, NeuralLayer& layer);
};

// Specialized networks for different tasks
class GenreClassificationNetwork {
public:
    GenreClassificationNetwork();
    
    // Training with real data
    void trainOnDataset(const std::vector<std::pair<std::vector<float>, juce::String>>& dataset,
                       int epochs = 100);
    
    // Classification
    juce::String classify(const std::vector<float>& features);
    std::vector<std::pair<juce::String, float>> classifyWithProbabilities(const std::vector<float>& features);
    
    // Model management
    void saveModel(const juce::File& path) const;
    bool loadModel(const juce::File& path);
    
private:
    std::unique_ptr<NeuralNetwork> network;
    std::vector<juce::String> genreLabels;
    
    juce::String indexToGenre(int index) const;
    int genreToIndex(const juce::String& genre) const;
    std::vector<float> oneHotEncode(const juce::String& genre) const;
};

class CreativeSuggestionNetwork {
public:
    CreativeSuggestionNetwork();
    
    // Training
    void trainOnSuggestions(const std::vector<std::pair<std::vector<float>, std::vector<float>>>& trainingData,
                          int epochs = 100);
    
    // Generation
    std::vector<float> generateSuggestion(const std::vector<float>& context);
    
    // Learning from feedback
    void learnFromFeedback(const std::vector<float>& context,
                          const std::vector<float>& suggestion,
                          bool wasHelpful);
    
private:
    std::unique_ptr<NeuralNetwork> network;
    std::vector<std::tuple<std::vector<float>, std::vector<float>, float>> feedbackHistory;
    
    void updateFromFeedback();
};

class QualityPredictionNetwork {
public:
    QualityPredictionNetwork();
    
    // Training
    void trainOnQualityData(const std::vector<std::tuple<std::vector<float>, std::vector<float>, float>>& data,
                          int epochs = 100);
    
    // Prediction
    std::vector<float> predictQuality(const std::vector<float>& audioFeatures,
                                      const std::vector<float>& settings);
    
    // Continuous learning
    void updateFromResult(const std::vector<float>& audioFeatures,
                         const std::vector<float>& settings,
                         const std::vector<float>& actualQuality);
    
private:
    std::unique_ptr<NeuralNetwork> network;
    float learningRate = 0.001f;  // Lower for quality prediction
};

// Training data generator and manager
class TrainingDataManager {
public:
    struct TrainingSample {
        std::vector<float> input;
        std::vector<float> target;
        float weight = 1.0f;  // For weighted training
        juce::String metadata;
    };
    
    TrainingDataManager();
    
    // Data management
    void addSample(const TrainingSample& sample);
    void addDataset(const std::vector<TrainingSample>& dataset);
    void clearData();
    
    // Training preparation
    std::vector<TrainingSample> getTrainingBatch(int batchSize) const;
    std::vector<TrainingSample> getValidationSet() const;
    std::vector<TrainingSample> getTestSet() const;
    
    // Data splitting
    void splitData(float trainRatio = 0.7f, float validationRatio = 0.2f, float testRatio = 0.1f);
    
    // Statistics
    size_t getTotalSamples() const { return allSamples.size(); }
    size_t getTrainingSamples() const { return trainingSamples.size(); }
    size_t getValidationSamples() const { return validationSamples.size(); }
    size_t getTestSamples() const { return testSamples.size(); }
    
    // Export/Import
    void saveDataset(const juce::File& filePath) const;
    bool loadDataset(const juce::File& filePath);
    
private:
    std::vector<TrainingSample> allSamples;
    std::vector<TrainingSample> trainingSamples;
    std::vector<TrainingSample> validationSamples;
    std::vector<TrainingSample> testSamples;
    
    mutable std::random_device rd;
    mutable std::mt19937 gen{rd()};
    
    void shuffleSamples(std::vector<TrainingSample>& samples) const;
};

} // namespace ai
} // namespace zenith
