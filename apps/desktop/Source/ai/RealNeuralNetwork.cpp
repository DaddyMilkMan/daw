/*
  ==============================================================================
    RealNeuralNetwork.cpp
    Actual neural network implementation with backpropagation
  ==============================================================================
*/

#include "RealNeuralNetwork.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <iostream>

namespace zenith {
namespace ai {

// NeuralLayer Implementation
NeuralLayer::NeuralLayer(int inputSize, int outputSize, ActivationType activation)
    : inputSize(inputSize), outputSize(outputSize), activationType(activation) {
    
    weights.resize(outputSize, std::vector<float>(inputSize));
    biases.resize(outputSize);
    input.resize(inputSize);
    weightedSum.resize(outputSize);
    output.resize(outputSize);
    this->activation.resize(outputSize);
    weightGradient.assign(outputSize, std::vector<float>(inputSize));
    biasGradient.resize(outputSize);
    
    initializeWeights();
}

void NeuralLayer::initializeWeights() {
    std::random_device rd;
    std::mt19937 gen(rd());
    
    // Use He initialization for ReLU, Xavier for others
    float stdDev;
    if (activationType == ActivationType::ReLU || activationType == ActivationType::LeakyReLU) {
        stdDev = heInitialization(inputSize);
    } else {
        stdDev = xavierInitialization(inputSize, outputSize);
    }
    
    std::normal_distribution<float> dis(0.0f, stdDev);
    
    for (int i = 0; i < outputSize; ++i) {
        biases[i] = 0.0f;
        for (int j = 0; j < inputSize; ++j) {
            weights[i][j] = dis(gen);
        }
    }
}

void NeuralLayer::loadWeights(const std::vector<std::vector<float>>& weights, const std::vector<float>& biases) {
    if (weights.size() == static_cast<size_t>(outputSize) && biases.size() == static_cast<size_t>(outputSize)) {
        this->weights = weights;
        this->biases = biases;
    }
}

std::pair<std::vector<std::vector<float>>, std::vector<float>> NeuralLayer::getWeights() const {
    return {weights, biases};
}

std::vector<float> NeuralLayer::forward(const std::vector<float>& input) {
    this->input = input;
    
    // Calculate weighted sums
    for (int i = 0; i < outputSize; ++i) {
        weightedSum[i] = biases[i];
        for (int j = 0; j < inputSize; ++j) {
            weightedSum[i] += weights[i][j] * input[j];
        }
    }
    
    // Apply activation function
    output = activateVector(weightedSum);
    activation = output;
    
    return output;
}

std::vector<float> NeuralLayer::backward(const std::vector<float>& gradient, float learningRate) {
    if (gradient.size() != static_cast<size_t>(outputSize)) {
        return {};
    }
    
    // Calculate activation derivative
    auto activationDeriv = activateDerivativeVector(weightedSum);
    
    // Calculate delta for this layer
    std::vector<float> delta(outputSize);
    for (int i = 0; i < outputSize; ++i) {
        delta[i] = gradient[i] * activationDeriv[i];
    }
    
    // Calculate gradients
    for (int i = 0; i < outputSize; ++i) {
        biasGradient[i] = delta[i];
        for (int j = 0; j < inputSize; ++j) {
            weightGradient[i][j] = delta[i] * input[j];
        }
    }
    
    // Calculate gradient for previous layer
    std::vector<float> prevGradient(inputSize, 0.0f);
    for (int j = 0; j < inputSize; ++j) {
        for (int i = 0; i < outputSize; ++i) {
            prevGradient[j] += delta[i] * weights[i][j];
        }
    }
    
    // Update weights and biases
    for (int i = 0; i < outputSize; ++i) {
        biases[i] -= learningRate * biasGradient[i];
        for (int j = 0; j < inputSize; ++j) {
            weights[i][j] -= learningRate * weightGradient[i][j];
        }
    }
    
    return prevGradient;
}

float NeuralLayer::activate(float x) const {
    switch (activationType) {
        case ActivationType::Sigmoid:
            return 1.0f / (1.0f + std::exp(-x));
        case ActivationType::Tanh:
            return std::tanh(x);
        case ActivationType::ReLU:
            return std::max(0.0f, x);
        case ActivationType::LeakyReLU:
            return (x > 0.0f) ? x : 0.01f * x;
        default:
            return x;
    }
}

float NeuralLayer::activateDerivative(float x) const {
    switch (activationType) {
        case ActivationType::Sigmoid: {
            float sig = activate(x);
            return sig * (1.0f - sig);
        }
        case ActivationType::Tanh: {
            float tanh_val = std::tanh(x);
            return 1.0f - tanh_val * tanh_val;
        }
        case ActivationType::ReLU:
            return (x > 0.0f) ? 1.0f : 0.0f;
        case ActivationType::LeakyReLU:
            return (x > 0.0f) ? 1.0f : 0.01f;
        default:
            return 1.0f;
    }
}

std::vector<float> NeuralLayer::activateVector(const std::vector<float>& x) const {
    std::vector<float> result;
    result.reserve(x.size());
    
    for (float val : x) {
        result.push_back(activate(val));
    }
    
    // Apply softmax if specified
    if (activationType == ActivationType::Softmax) {
        float maxVal = *std::max_element(result.begin(), result.end());
        float sum = 0.0f;
        
        for (float& val : result) {
            val = std::exp(val - maxVal);
            sum += val;
        }
        
        for (float& val : result) {
            val /= sum;
        }
    }
    
    return result;
}

std::vector<float> NeuralLayer::activateDerivativeVector(const std::vector<float>& x) const {
    std::vector<float> result;
    result.reserve(x.size());
    
    for (float val : x) {
        result.push_back(activateDerivative(val));
    }
    
    return result;
}

float NeuralLayer::xavierInitialization(int fanIn, int fanOut) const {
    return std::sqrt(2.0f / (fanIn + fanOut));
}

float NeuralLayer::heInitialization(int fanIn) const {
    return std::sqrt(2.0f / fanIn);
}

// NeuralNetwork Implementation
NeuralNetwork::NeuralNetwork(const std::vector<int>& layerSizes, 
                           const std::vector<ActivationType>& activations) {
    
    for (size_t i = 0; i < layerSizes.size() - 1; ++i) {
        ActivationType activation = (i < activations.size()) ? activations[i] : ActivationType::ReLU;
        layers.push_back(std::make_unique<NeuralLayer>(layerSizes[i], layerSizes[i + 1], activation));
    }
}

std::vector<float> NeuralNetwork::forward(const std::vector<float>& input) {
    std::vector<float> current = input;
    
    for (auto& layer : layers) {
        current = layer->forward(current);
    }
    
    return current;
}

void NeuralNetwork::train(const std::vector<std::vector<float>>& inputs,
                         const std::vector<std::vector<float>>& targets,
                         int epochs,
                         float learningRate,
                         float batchSize,
                         std::function<void(int, float)> progressCallback) {
    
    if (inputs.size() != targets.size()) {
        return;
    }
    
    // Make copies to shuffle
    auto trainingInputs = inputs;
    auto trainingTargets = targets;
    
    for (int epoch = 0; epoch < epochs; ++epoch) {
        // Shuffle data
        shuffleDataset(trainingInputs, trainingTargets);
        
        float totalLoss = 0.0f;
        int numBatches = 0;
        
        // Mini-batch training
        for (size_t batchStart = 0; batchStart < trainingInputs.size(); batchStart += static_cast<size_t>(batchSize)) {
            size_t batchEnd = std::min(batchStart + static_cast<size_t>(batchSize), trainingInputs.size());
            
            float batchLoss = 0.0f;
            int batchSamples = 0;
            
            // Process mini-batch
            for (size_t i = batchStart; i < batchEnd; ++i) {
                float loss = trainSingle(trainingInputs[i], trainingTargets[i], learningRate);
                batchLoss += loss;
                batchSamples++;
            }
            
            if (batchSamples > 0) {
                totalLoss += batchLoss / batchSamples;
                numBatches++;
            }
        }
        
        float avgLoss = (numBatches > 0) ? totalLoss / numBatches : 0.0f;
        
        if (progressCallback) {
            progressCallback(epoch, avgLoss);
        }
        
        // Early stopping if loss is very low
        if (avgLoss < 0.001f) {
            break;
        }
    }
}

float NeuralNetwork::trainSingle(const std::vector<float>& input,
                                const std::vector<float>& target,
                                float learningRate) {
    // Forward pass
    auto output = forward(input);
    
    // Calculate loss
    float loss = meanSquaredError(output, target);
    
    // Backward pass
    auto gradient = meanSquaredErrorGradient(output, target);
    
    // Propagate gradient backward
    std::vector<float> currentGradient = gradient;
    for (auto it = layers.rbegin(); it != layers.rend(); ++it) {
        currentGradient = (*it)->backward(currentGradient, learningRate);
    }
    
    return loss;
}

std::vector<float> NeuralNetwork::predict(const std::vector<float>& input) {
    return forward(input);
}

float NeuralNetwork::calculateLoss(const std::vector<float>& output, const std::vector<float>& target) {
    return meanSquaredError(output, target);
}

void NeuralNetwork::saveModel(const juce::File& filePath) const {
    std::ofstream file(filePath.getFullPathName().toStdString(), std::ios::binary);
    
    if (!file.is_open()) {
        return;
    }
    
    // Save network architecture
    int numLayers = static_cast<int>(layers.size());
    file.write(reinterpret_cast<const char*>(&numLayers), sizeof(numLayers));
    
    for (const auto& layer : layers) {
        saveLayer(file, *layer);
    }
}

void NeuralNetwork::loadWeights(const juce::File& filePath) { loadModel(filePath); }
void NeuralNetwork::saveWeights(const juce::File& filePath) const { saveModel(filePath); }

bool NeuralNetwork::loadModel(const juce::File& filePath) {
    std::ifstream file(filePath.getFullPathName().toStdString(), std::ios::binary);
    
    if (!file.is_open()) {
        return false;
    }
    
    // Load network architecture
    int numLayers;
    file.read(reinterpret_cast<char*>(&numLayers), sizeof(numLayers));
    
    layers.clear();
    for (int i = 0; i < numLayers; ++i) {
        // Create layer (need to know sizes from saved data)
        int inputSize, outputSize;
        file.read(reinterpret_cast<char*>(&inputSize), sizeof(inputSize));
        file.read(reinterpret_cast<char*>(&outputSize), sizeof(outputSize));
        
        layers.push_back(std::make_unique<NeuralLayer>(inputSize, outputSize));
        
        if (!loadLayer(file, *layers.back())) {
            return false;
        }
    }
    
    return true;
}

float NeuralNetwork::meanSquaredError(const std::vector<float>& output, const std::vector<float>& target) const {
    if (output.size() != target.size()) {
        return 0.0f;
    }
    
    float sum = 0.0f;
    for (size_t i = 0; i < output.size(); ++i) {
        float diff = output[i] - target[i];
        sum += diff * diff;
    }
    
    return sum / output.size();
}

std::vector<float> NeuralNetwork::meanSquaredErrorGradient(const std::vector<float>& output, const std::vector<float>& target) const {
    if (output.size() != target.size()) {
        return {};
    }
    
    std::vector<float> gradient(output.size());
    for (size_t i = 0; i < output.size(); ++i) {
        gradient[i] = 2.0f * (output[i] - target[i]) / output.size();
    }
    
    return gradient;
}

void NeuralNetwork::shuffleDataset(std::vector<std::vector<float>>& inputs,
                                  std::vector<std::vector<float>>& targets) {
    std::vector<size_t> indices(inputs.size());
    for (size_t i = 0; i < indices.size(); ++i) {
        indices[i] = i;
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(indices.begin(), indices.end(), gen);
    
    std::vector<std::vector<float>> shuffledInputs(inputs.size());
    std::vector<std::vector<float>> shuffledTargets(targets.size());
    
    for (size_t i = 0; i < indices.size(); ++i) {
        shuffledInputs[i] = inputs[indices[i]];
        shuffledTargets[i] = targets[indices[i]];
    }
    
    inputs = shuffledInputs;
    targets = shuffledTargets;
}

void NeuralNetwork::saveLayer(std::ostream& stream, const NeuralLayer& layer) const {
    // Save layer dimensions
    int inputSize = layer.getInputSize();
    int outputSize = layer.getOutputSize();
    stream.write(reinterpret_cast<const char*>(&inputSize), sizeof(inputSize));
    stream.write(reinterpret_cast<const char*>(&outputSize), sizeof(outputSize));
    
    // Save weights
    auto weights = layer.getWeights().first;
    for (const auto& weightRow : weights) {
        stream.write(reinterpret_cast<const char*>(weightRow.data()), weightRow.size() * sizeof(float));
    }
    
    // Save biases
    auto biases = layer.getWeights().second;
    stream.write(reinterpret_cast<const char*>(biases.data()), biases.size() * sizeof(float));
}

bool NeuralNetwork::loadLayer(std::istream& stream, NeuralLayer& layer) {
    // Load layer dimensions
    int inputSize, outputSize;
    stream.read(reinterpret_cast<char*>(&inputSize), sizeof(inputSize));
    stream.read(reinterpret_cast<char*>(&outputSize), sizeof(outputSize));
    
    // Load weights
    std::vector<std::vector<float>> weights(outputSize, std::vector<float>(inputSize));
    for (auto& weightRow : weights) {
        stream.read(reinterpret_cast<char*>(weightRow.data()), weightRow.size() * sizeof(float));
    }
    
    // Load biases
    std::vector<float> biases(outputSize);
    stream.read(reinterpret_cast<char*>(biases.data()), biases.size() * sizeof(float));
    
    // Set weights
    layer.loadWeights(weights, biases);
    
    return true;
}

// GenreClassificationNetwork Implementation
GenreClassificationNetwork::GenreClassificationNetwork() {
    genreLabels = {"electronic", "rock", "hip-hop", "pop", "classical", "jazz", "blues", "country"};
    
    // Network architecture: features -> 64 -> 32 -> 16 -> num_genres
    std::vector<int> layerSizes = {
        20,  // Input features
        64,  // Hidden 1
        32,  // Hidden 2
        16,  // Hidden 3
        static_cast<int>(genreLabels.size())  // Output
    };
    
    std::vector<ActivationType> activations = {
        ActivationType::ReLU,
        ActivationType::ReLU,
        ActivationType::ReLU,
        ActivationType::Softmax
    };
    
    network = std::make_unique<NeuralNetwork>(layerSizes, activations);
}

void GenreClassificationNetwork::trainOnDataset(const std::vector<std::pair<std::vector<float>, juce::String>>& dataset,
                                               int epochs) {
    std::vector<std::vector<float>> inputs;
    std::vector<std::vector<float>> targets;
    
    for (const auto& sample : dataset) {
        inputs.push_back(sample.first);
        targets.push_back(oneHotEncode(sample.second));
    }
    
    network->train(inputs, targets, epochs, 0.01f, 32.0f);
}

juce::String GenreClassificationNetwork::classify(const std::vector<float>& features) {
    auto output = network->predict(features);
    
    if (output.empty()) {
        return "unknown";
    }
    
    auto maxIt = std::max_element(output.begin(), output.end());
    int index = static_cast<int>(std::distance(output.begin(), maxIt));
    
    return indexToGenre(index);
}

std::vector<std::pair<juce::String, float>> GenreClassificationNetwork::classifyWithProbabilities(const std::vector<float>& features) {
    auto output = network->predict(features);
    std::vector<std::pair<juce::String, float>> results;
    
    for (size_t i = 0; i < output.size() && i < genreLabels.size(); ++i) {
        results.push_back({genreLabels[i], output[i]});
    }
    
    // Sort by probability
    std::sort(results.begin(), results.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    return results;
}

juce::String GenreClassificationNetwork::indexToGenre(int index) const {
    if (index >= 0 && index < static_cast<int>(genreLabels.size())) {
        return genreLabels[index];
    }
    return "unknown";
}

int GenreClassificationNetwork::genreToIndex(const juce::String& genre) const {
    for (size_t i = 0; i < genreLabels.size(); ++i) {
        if (genreLabels[i] == genre) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

std::vector<float> GenreClassificationNetwork::oneHotEncode(const juce::String& genre) const {
    std::vector<float> encoded(genreLabels.size(), 0.0f);
    int index = genreToIndex(genre);
    
    if (index >= 0) {
        encoded[index] = 1.0f;
    }
    
    return encoded;
}

void GenreClassificationNetwork::saveModel(const juce::File& path) const {
    network->saveModel(path);
}

bool GenreClassificationNetwork::loadModel(const juce::File& path) {
    return network->loadModel(path);
}

// TrainingDataManager Implementation
TrainingDataManager::TrainingDataManager() : gen(rd()) {}

void TrainingDataManager::addSample(const TrainingSample& sample) {
    allSamples.push_back(sample);
}

void TrainingDataManager::addDataset(const std::vector<TrainingSample>& dataset) {
    allSamples.insert(allSamples.end(), dataset.begin(), dataset.end());
}

void TrainingDataManager::clearData() {
    allSamples.clear();
    trainingSamples.clear();
    validationSamples.clear();
    testSamples.clear();
}

void TrainingDataManager::splitData(float trainRatio, float validationRatio, float testRatio) {
    shuffleSamples(allSamples);
    
    size_t total = allSamples.size();
    size_t trainSize = static_cast<size_t>(total * trainRatio);
    size_t validationSize = static_cast<size_t>(total * validationRatio);
    size_t testSize = total - trainSize - validationSize;
    
    trainingSamples.assign(allSamples.begin(), allSamples.begin() + trainSize);
    validationSamples.assign(allSamples.begin() + trainSize, allSamples.begin() + trainSize + validationSize);
    testSamples.assign(allSamples.begin() + trainSize + validationSize, allSamples.end());
}

std::vector<TrainingDataManager::TrainingSample> TrainingDataManager::getTrainingBatch(int batchSize) const {
    std::vector<TrainingSample> batch;
    
    if (trainingSamples.empty()) {
        return batch;
    }
    
    std::uniform_int_distribution<int> dist(0, static_cast<int>(trainingSamples.size()) - 1);
    
    for (int i = 0; i < batchSize && i < static_cast<int>(trainingSamples.size()); ++i) {
        int index = dist(gen);
        batch.push_back(trainingSamples[index]);
    }
    
    return batch;
}

void TrainingDataManager::shuffleSamples(std::vector<TrainingSample>& samples) const {
    std::shuffle(samples.begin(), samples.end(), gen);
}

} // namespace ai
} // namespace zenith
