/*
  ==============================================================================
    ModelTrainer.cpp
    Production model training implementation
  ==============================================================================
*/

#include "ModelTrainer.h"
#include <juce_cryptography/juce_cryptography.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <algorithm>
#include <random>
#include <chrono>
#include <fstream>
#include <sstream>

namespace zenith {
namespace ai {

// DatasetLoader Implementation
DatasetLoader::DatasetLoader() = default;

DatasetLoader::~DatasetLoader() = default;

bool DatasetLoader::loadGenreDataset(const juce::File& datasetPath) {
    allSamples.clear();
    
    if (datasetPath.existsAsFile()) {
        if (datasetPath.getFileExtension() == ".csv") {
            return loadCSVFile(datasetPath);
        } else if (datasetPath.getFileExtension() == ".json") {
            return loadJSONFile(datasetPath);
        }
    } else if (datasetPath.isDirectory()) {
        return loadAudioFiles(datasetPath);
    }
    
    return false;
}

bool DatasetLoader::loadCSVFile(const juce::File& filePath) {
    juce::StringArray lines;
    filePath.readLines(lines);
    if (lines.isEmpty()) {
        return false;
    }
    
    // Skip header if present
    int startLine = lines[0].contains("label") ? 1 : 0;
    
    for (int i = startLine; i < lines.size(); ++i) {
        juce::StringArray tokens;
        tokens.addTokens(lines[i], ",", "\"");
        
        if (tokens.size() < 2) continue;
        
        TrainingSample sample;
        sample.label = tokens[tokens.size() - 1];
        sample.timestamp = juce::Time::getCurrentTime();
        
        // Parse features (all but last column)
        for (int j = 0; j < tokens.size() - 1; ++j) {
            sample.features.push_back(tokens[j].getFloatValue());
        }
        
        // One-hot encode targets
        if (labels.empty()) {
            // Initialize labels list
            labels.push_back(sample.label);
        } else if (std::find(labels.begin(), labels.end(), sample.label) == labels.end()) {
            labels.push_back(sample.label);
        }
        
        // Create one-hot target
        sample.targets.resize(labels.size(), 0.0f);
        auto labelIt = std::find(labels.begin(), labels.end(), sample.label);
        if (labelIt != labels.end()) {
            sample.targets[std::distance(labels.begin(), labelIt)] = 1.0f;
        }
        
        allSamples.push_back(sample);
    }
    
    // Set dimensions
    if (!allSamples.empty()) {
        featureSize = static_cast<int>(allSamples[0].features.size());
        targetSize = static_cast<int>(labels.size());
    }
    
    return !allSamples.empty();
}

bool DatasetLoader::loadJSONFile(const juce::File& filePath) {
    auto json = juce::JSON::parse(filePath.loadFileAsString());
    
    if (auto* obj = json.getDynamicObject()) {
        auto samplesArray = obj->hasProperty("samples") ? obj->getProperty("samples") : juce::var();
        
        if (auto* samples = samplesArray.getArray()) {
            for (const auto& sampleVar : *samples) {
                if (auto* sampleObj = sampleVar.getDynamicObject()) {
                    TrainingSample sample;
                    
                    // Extract features
                    auto featuresVar = sampleObj->hasProperty("features") ? sampleObj->getProperty("features") : juce::var();
                    if (auto* features = featuresVar.getArray()) {
                        for (const auto& feature : *features) {
                            sample.features.push_back(static_cast<float>(feature));
                        }
                    }
                    
                    // Extract targets
                    auto targetsVar = sampleObj->hasProperty("targets") ? sampleObj->getProperty("targets") : juce::var();
                    if (auto* targets = targetsVar.getArray()) {
                        for (const auto& target : *targets) {
                            sample.targets.push_back(static_cast<float>(target));
                        }
                    }
                    
                    // Extract metadata
                    sample.label = sampleObj->hasProperty("label") ? sampleObj->getProperty("label").toString() : "";
                    sample.weight = sampleObj->hasProperty("weight") ? static_cast<float>(sampleObj->getProperty("weight")) : 1.0f;
                    sample.metadata = sampleObj->hasProperty("metadata") ? sampleObj->getProperty("metadata").toString() : "";
                    sample.timestamp = juce::Time::getCurrentTime();
                    
                    allSamples.push_back(sample);
                }
            }
        }
    }
    
    return !allSamples.empty();
}

bool DatasetLoader::loadAudioFiles(const juce::File& directoryPath) {
    juce::Array<juce::File> audioFiles;
    directoryPath.findChildFiles(audioFiles, juce::File::findFiles, true, "*.wav;*.mp3;*.flac;*.aiff");
    
    for (const auto& audioFile : audioFiles) {
        // Extract genre from filename or directory structure
        juce::String genre = audioFile.getParentDirectory().getFileName();
        if (genre.isEmpty()) {
            genre = "unknown";
        }
        
        // Load audio file
        juce::AudioFormatManager formatManager;
        formatManager.registerBasicFormats();
        
        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(audioFile));
        
        if (reader != nullptr) {
            juce::AudioBuffer<float> buffer(reader->numChannels, static_cast<int>(reader->lengthInSamples));
            reader->read(&buffer, 0, static_cast<int>(reader->lengthInSamples), 0, true, true);
            
            // Extract features
            auto features = extractAudioFeatures(buffer, reader->sampleRate);
            
            TrainingSample sample;
            sample.features = features;
            sample.label = genre;
            sample.timestamp = juce::Time::getCurrentTime();
            
            // One-hot encode target
            if (labels.empty()) {
                labels.push_back(genre);
            } else if (std::find(labels.begin(), labels.end(), genre) == labels.end()) {
                labels.push_back(genre);
            }
            
            sample.targets.resize(labels.size(), 0.0f);
            auto labelIt = std::find(labels.begin(), labels.end(), genre);
            if (labelIt != labels.end()) {
                sample.targets[std::distance(labels.begin(), labelIt)] = 1.0f;
            }
            
            allSamples.push_back(sample);
        }
    }
    
    if (!allSamples.empty()) {
        featureSize = static_cast<int>(allSamples[0].features.size());
        targetSize = static_cast<int>(labels.size());
    }
    
    return !allSamples.empty();
}

std::vector<float> DatasetLoader::extractAudioFeatures(const juce::AudioBuffer<float>& audio, double sampleRate) {
    std::vector<float> features;
    
    // Basic audio statistics
    float rms = 0.0f;
    float peak = 0.0f;
    int numSamples = audio.getNumSamples();
    int numChannels = audio.getNumChannels();
    
    for (int ch = 0; ch < numChannels; ++ch) {
        for (int i = 0; i < numSamples; ++i) {
            float sample = audio.getSample(ch, i);
            rms += sample * sample;
            peak = std::max(peak, std::abs(sample));
        }
    }
    
    rms = std::sqrt(rms / (numSamples * numChannels));
    float crestFactor = 20.0f * std::log10(peak / rms);
    
    features.push_back(rms);
    features.push_back(peak);
    features.push_back(crestFactor);
    
    // Spectral features (simplified FFT)
    int fftSize = 1024;
    std::vector<float> fftData(fftSize * 2, 0.0f);
    
    // Copy first channel data
    int samplesToProcess = std::min(numSamples, fftSize);
    for (int i = 0; i < samplesToProcess; ++i) {
        fftData[i * 2] = audio.getSample(0, i);
    }
    
    // Apply window
    for (int i = 0; i < samplesToProcess; ++i) {
        float window = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * i / (samplesToProcess - 1)));
        fftData[i * 2] *= window;
    }
    
    // Simple spectral features (would use proper FFT in production)
    float spectralCentroid = 2000.0f;  // Placeholder
    float spectralRolloff = 8000.0f;   // Placeholder
    float spectralFlux = 0.5f;         // Placeholder
    
    features.push_back(spectralCentroid);
    features.push_back(spectralRolloff);
    features.push_back(spectralFlux);
    
    // Temporal features
    float attackTime = 0.01f;    // Placeholder
    float decayTime = 0.1f;      // Placeholder
    float tempo = 120.0f;        // Placeholder
    
    features.push_back(attackTime);
    features.push_back(decayTime);
    features.push_back(tempo);
    
    // Stereo features
    float stereoWidth = 1.0f;    // Placeholder
    if (numChannels >= 2) {
        float correlation = 0.0f;
        float leftPower = 0.0f;
        float rightPower = 0.0f;
        
        for (int i = 0; i < samplesToProcess; ++i) {
            float left = audio.getSample(0, i);
            float right = audio.getSample(1, i);
            
            correlation += left * right;
            leftPower += left * left;
            rightPower += right * right;
        }
        
        if (leftPower > 0.0f && rightPower > 0.0f) {
            correlation /= std::sqrt(leftPower * rightPower);
            stereoWidth = std::sqrt(2.0f * (1.0f - correlation));
        }
    }
    
    features.push_back(stereoWidth);
    
    // Add MFCCs (simplified)
    for (int i = 0; i < 13; ++i) {
        features.push_back(0.1f * std::sin(i * 0.5f));  // Placeholder MFCCs
    }
    
    return features;
}

void DatasetLoader::splitDataset(float trainRatio, float validationRatio, float testRatio) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(allSamples.begin(), allSamples.end(), gen);
    
    size_t totalSamples = allSamples.size();
    size_t trainSize = static_cast<size_t>(totalSamples * trainRatio);
    size_t validationSize = static_cast<size_t>(totalSamples * validationRatio);
    size_t testSize = totalSamples - trainSize - validationSize;
    
    trainingSamples.assign(allSamples.begin(), allSamples.begin() + trainSize);
    validationSamples.assign(allSamples.begin() + trainSize, allSamples.begin() + trainSize + validationSize);
    testSamples.assign(allSamples.begin() + trainSize + validationSize, allSamples.end());
}

void DatasetLoader::normalizeFeatures() {
    if (allSamples.empty()) return;
    
    // Calculate means and standard deviations
    std::vector<float> means(featureSize, 0.0f);
    std::vector<float> stds(featureSize, 0.0f);
    
    // Calculate means
    for (const auto& sample : allSamples) {
        for (int i = 0; i < featureSize; ++i) {
            means[i] += sample.features[i];
        }
    }
    
    for (float& mean : means) {
        mean /= allSamples.size();
    }
    
    // Calculate standard deviations
    for (const auto& sample : allSamples) {
        for (int i = 0; i < featureSize; ++i) {
            float diff = sample.features[i] - means[i];
            stds[i] += diff * diff;
        }
    }
    
    for (float& std : stds) {
        std = std::sqrt(std / allSamples.size());
    }
    
    // Apply normalization
    applyNormalization(means, stds);
}

void DatasetLoader::applyNormalization(std::vector<float>& means, std::vector<float>& stds) {
    for (auto& sample : allSamples) {
        for (int i = 0; i < featureSize; ++i) {
            if (stds[i] > 0.001f) {
                sample.features[i] = (sample.features[i] - means[i]) / stds[i];
            }
        }
    }
    
    for (auto& sample : trainingSamples) {
        for (int i = 0; i < featureSize; ++i) {
            if (stds[i] > 0.001f) {
                sample.features[i] = (sample.features[i] - means[i]) / stds[i];
            }
        }
    }
    
    for (auto& sample : validationSamples) {
        for (int i = 0; i < featureSize; ++i) {
            if (stds[i] > 0.001f) {
                sample.features[i] = (sample.features[i] - means[i]) / stds[i];
            }
        }
    }
    
    for (auto& sample : testSamples) {
        for (int i = 0; i < featureSize; ++i) {
            if (stds[i] > 0.001f) {
                sample.features[i] = (sample.features[i] - means[i]) / stds[i];
            }
        }
    }
}

int DatasetLoader::getTotalSamples() const {
    return static_cast<int>(allSamples.size());
}

int DatasetLoader::getFeatureSize() const {
    return featureSize;
}

int DatasetLoader::getTargetSize() const {
    return targetSize;
}

std::vector<juce::String> DatasetLoader::getLabels() const {
    return labels;
}

// ModelTrainer Implementation
ModelTrainer::ModelTrainer() {
    datasetLoader = std::make_unique<DatasetLoader>();
}

ModelTrainer::~ModelTrainer() = default;

TrainingResults ModelTrainer::trainGenreClassifier(const std::vector<TrainingSample>& dataset,
                                                   const TrainingConfig& config,
                                                   TrainingProgressCallback progressCallback) {
    TrainingResults results;
    
    try {
        // Create network architecture for genre classification
        std::vector<int> layerSizes = {
            dataset[0].features.size(),  // Input size
            128,  // Hidden layer 1
            64,   // Hidden layer 2
            32,   // Hidden layer 3
            dataset[0].targets.size()   // Output size (number of genres)
        };
        
        auto network = std::make_unique<NeuralNetwork>(layerSizes);
        
        // Split dataset
        std::vector<TrainingSample> trainingData, validationData;
        size_t trainSize = static_cast<size_t>(dataset.size() * (1.0f - config.validationSplit - config.testSplit));
        size_t validationSize = static_cast<size_t>(dataset.size() * config.validationSplit);
        
        trainingData.assign(dataset.begin(), dataset.begin() + trainSize);
        validationData.assign(dataset.begin() + trainSize, dataset.begin() + trainSize + validationSize);
        
        // Train network
        results = trainNetwork(*network, trainingData, validationData, config, progressCallback);
        
        if (results.success) {
            // Save model
            auto modelPath = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                              .getChildFile("ZenithDAW")
                              .getChildFile("models")
                              .getChildFile("genre_classifier.json");
            
            modelPath.createDirectory();
            saveModel(*network, modelPath);
        }
        
    } catch (const std::exception& e) {
        results.error = e.what();
        results.success = false;
    }
    
    return results;
}

TrainingResults ModelTrainer::trainNetwork(NeuralNetwork& network,
                                           const std::vector<TrainingSample>& trainingData,
                                           const std::vector<TrainingSample>& validationData,
                                           const TrainingConfig& config,
                                           TrainingProgressCallback progressCallback) {
    TrainingResults results;
    auto startTime = std::chrono::steady_clock::now();
    
    try {
        // Prepare training data
        std::vector<std::vector<float>> inputs;
        std::vector<std::vector<float>> targets;
        
        for (const auto& sample : trainingData) {
            inputs.push_back(sample.features);
            targets.push_back(sample.targets);
        }
        
        // Training loop
        results.totalEpochs = config.epochs;
        results.bestEpoch = 0;
        results.bestValidationLoss = std::numeric_limits<float>::max();
        
        for (int epoch = 0; epoch < config.epochs; ++epoch) {
            // Shuffle data if requested
            if (config.shuffleData) {
                std::random_device rd;
                std::mt19937 gen(config.randomSeed + epoch);
                std::shuffle(inputs.begin(), inputs.end(), gen);
                std::shuffle(targets.begin(), targets.end(), gen);
            }
            
            // Train one epoch
            float epochLoss = 0.0f;
            int numBatches = 0;
            
            for (size_t i = 0; i < inputs.size(); i += static_cast<size_t>(config.batchSize)) {
                size_t end = std::min(i + static_cast<size_t>(config.batchSize), inputs.size());
                
                for (size_t j = i; j < end; ++j) {
                    float loss = network.trainSingle(inputs[j], targets[j], config.learningRate);
                    epochLoss += loss;
                    numBatches++;
                }
            }
            
            epochLoss /= numBatches;
            
            // Calculate validation loss
            float validationLoss = calculateLoss(network, validationData);
            float validationAccuracy = calculateAccuracy(network, validationData);
            
            // Update results
            results.lossHistory.push_back(epochLoss);
            results.validationLossHistory.push_back(validationLoss);
            results.validationAccuracyHistory.push_back(validationAccuracy);
            
            // Check for best model
            if (validationLoss < results.bestValidationLoss - config.minDelta) {
                results.bestValidationLoss = validationLoss;
                results.bestEpoch = epoch;
            }
            
            // Early stopping
            if (config.earlyStopping && shouldStopEarly(results.validationLossHistory, config.patience, config.minDelta)) {
                results.warnings = "Early stopping triggered at epoch " + juce::String(epoch);
                break;
            }
            
            // Progress callback
            if (progressCallback) {
                progressCallback(epoch, epochLoss, validationAccuracy, validationLoss);
            }
            
            results.finalLoss = epochLoss;
            results.finalAccuracy = validationAccuracy;
        }
        
        // Final evaluation
        results.validationMetrics = calculateMetrics(network, validationData);
        results.success = true;
        
    } catch (const std::exception& e) {
        results.error = e.what();
        results.success = false;
    }
    
    auto endTime = std::chrono::steady_clock::now();
    results.trainingTime = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime).count();
    
    return results;
}

float ModelTrainer::calculateLoss(NeuralNetwork& network, const std::vector<TrainingSample>& dataset) {
    float totalLoss = 0.0f;
    
    for (const auto& sample : dataset) {
        auto output = network.forward(sample.features);
        float loss = network.calculateLoss(output, sample.targets);
        totalLoss += loss;
    }
    
    return totalLoss / dataset.size();
}

float ModelTrainer::calculateAccuracy(NeuralNetwork& network, const std::vector<TrainingSample>& dataset) {
    int correct = 0;
    
    for (const auto& sample : dataset) {
        auto output = network.forward(sample.features);
        
        // Find predicted class
        auto maxIt = std::max_element(output.begin(), output.end());
        int predictedClass = static_cast<int>(std::distance(output.begin(), maxIt));
        
        // Find actual class
        auto targetMaxIt = std::max_element(sample.targets.begin(), sample.targets.end());
        int actualClass = static_cast<int>(std::distance(sample.targets.begin(), targetMaxIt));
        
        if (predictedClass == actualClass) {
            correct++;
        }
    }
    
    return static_cast<float>(correct) / dataset.size();
}

ValidationMetrics ModelTrainer::calculateMetrics(NeuralNetwork& network, const std::vector<TrainingSample>& dataset) {
    ValidationMetrics metrics;
    metrics.totalSamples = static_cast<int>(dataset.size());
    
    std::vector<std::vector<int>> confusionMatrix;
    // Initialize confusion matrix (would need to know number of classes)
    
    for (const auto& sample : dataset) {
        auto output = network.forward(sample.features);
        
        auto maxIt = std::max_element(output.begin(), output.end());
        int predictedClass = static_cast<int>(std::distance(output.begin(), maxIt));
        
        auto targetMaxIt = std::max_element(sample.targets.begin(), sample.targets.end());
        int actualClass = static_cast<int>(std::distance(sample.targets.begin(), targetMaxIt));
        
        if (predictedClass == actualClass) {
            metrics.correctPredictions++;
        }
    }
    
    metrics.accuracy = static_cast<float>(metrics.correctPredictions) / metrics.totalSamples;
    metrics.loss = calculateLoss(network, dataset);
    
    // Calculate precision, recall, F1 (simplified)
    metrics.precision = metrics.accuracy;
    metrics.recall = metrics.accuracy;
    metrics.f1Score = metrics.accuracy;
    
    return metrics;
}

bool ModelTrainer::shouldStopEarly(const std::vector<float>& validationLossHistory, int patience, float minDelta) {
    if (validationLossHistory.size() < static_cast<size_t>(patience + 1)) {
        return false;
    }
    
    // Check if validation loss hasn't improved for 'patience' epochs
    float bestLoss = *std::min_element(validationLossHistory.end() - patience - 1, validationLossHistory.end() - 1);
    float currentLoss = validationLossHistory.back();
    
    return (currentLoss - bestLoss) > minDelta;
}

bool ModelTrainer::saveModel(const NeuralNetwork& network, const juce::File& filePath) {
    network.saveModel(filePath);
    return true;
}

std::unique_ptr<NeuralNetwork> ModelTrainer::loadModel(const juce::File& filePath) {
    auto network = std::make_unique<NeuralNetwork>(std::vector<int>{10, 10});  // Placeholder
    if (network->loadModel(filePath)) {
        return network;
    }
    return nullptr;
}

// ProductionModelManager Implementation
ProductionModelManager::ProductionModelManager() {
    modelDirectory = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                          .getChildFile("ZenithDAW")
                          .getChildFile("models");
    registryFile = modelDirectory.getChildFile("model_registry.json");
    
    initializeRegistry();
}

ProductionModelManager::~ProductionModelManager() = default;

void ProductionModelManager::initializeRegistry() {
    if (registryFile.exists()) {
        loadModelRegistry(registryFile);
    }
}

bool ProductionModelManager::registerModel(const ModelInfo& modelInfo) {
    if (!isValidModel(modelInfo)) {
        return false;
    }
    
    modelRegistry[modelInfo.name].push_back(modelInfo);
    updateRegistry();
    
    return true;
}

bool ProductionModelManager::activateModel(const juce::String& modelName, const juce::String& version) {
    auto it = modelRegistry.find(modelName);
    if (it == modelRegistry.end()) {
        return false;
    }
    
    // Find model with specified version
    for (const auto& model : it->second) {
        if (model.version == version) {
            activeModels[modelName] = version;
            return true;
        }
    }
    
    return false;
}

std::vector<ProductionModelManager::ModelInfo> ProductionModelManager::getAvailableModels() const {
    std::vector<ModelInfo> allModels;
    
    for (const auto& [name, models] : modelRegistry) {
        allModels.insert(allModels.end(), models.begin(), models.end());
    }
    
    return allModels;
}

bool ProductionModelManager::validateModel(const juce::String& modelName, const juce::String& version) {
    auto modelInfo = getModelInfo(modelName, version);
    if (modelInfo.name.isEmpty()) {
        return false;
    }
    
    return isValidModel(modelInfo) && isModelCompatible(modelInfo);
}

bool ProductionModelManager::isValidModel(const ModelInfo& modelInfo) const {
    return !modelInfo.name.isEmpty() && 
           !modelInfo.version.isEmpty() && 
           modelInfo.filePath.exists() &&
           modelInfo.trainingResults.success;
}

bool ProductionModelManager::isModelCompatible(const ModelInfo& modelInfo) const {
    // Check if model meets minimum performance requirements
    return modelInfo.performanceMetrics.accuracy >= 0.7f &&
           modelInfo.trainingResults.finalLoss <= 0.5f;
}

void ProductionModelManager::updateRegistry() {
    saveModelRegistry(registryFile);
}

bool ProductionModelManager::saveModelRegistry(const juce::File& filePath) const {
    auto registry = new juce::DynamicObject();
    
    for (const auto& [name, models] : modelRegistry) {
        juce::Array<juce::var> modelArray;
        
        for (const auto& model : models) {
            auto modelObj = new juce::DynamicObject();
            modelObj->setProperty("name", model.name);
            modelObj->setProperty("version", model.version);
            modelObj->setProperty("description", model.description);
            modelObj->setProperty("filePath", model.filePath.getFullPathName());
            modelObj->setProperty("isActive", model.isActive);
            
            modelArray.add(juce::var(modelObj));
        }
        
        registry->setProperty(name, modelArray);
    }
    
    auto activeObj = new juce::DynamicObject();
    for (const auto& [name, version] : activeModels) {
        activeObj->setProperty(name, version);
    }
    registry->setProperty("activeModels", juce::var(activeObj));
    
    return filePath.replaceWithText(juce::JSON::toString(juce::var(registry)));
}

bool ProductionModelManager::loadModelRegistry(const juce::File& filePath) {
    auto json = juce::JSON::parse(filePath.loadFileAsString());
    
    if (auto* obj = json.getDynamicObject()) {
        modelRegistry.clear();
        activeModels.clear();
        
        for (const auto& [key, value] : obj->getProperties()) {
            if (auto* modelArray = value.getArray()) {
                std::vector<ModelInfo> models;
                
                for (const auto& modelVar : *modelArray) {
                    if (auto* modelObj = modelVar.getDynamicObject()) {
                        ModelInfo model;
                        model.name = modelObj->hasProperty("name") ? modelObj->getProperty("name").toString() : "";
                        model.version = modelObj->hasProperty("version") ? modelObj->getProperty("version").toString() : "";
                        model.description = modelObj->hasProperty("description") ? modelObj->getProperty("description").toString() : "";
                        model.filePath = juce::File(modelObj->hasProperty("filePath") ? modelObj->getProperty("filePath").toString() : "");
                        model.isActive = modelObj->hasProperty("isActive") ? static_cast<bool>(modelObj->getProperty("isActive")) : false;
                        
                        models.push_back(model);
                    }
                }
                
                modelRegistry[key.toString()] = models;
            }
        }
        
        // Load active models
        auto activeVar = obj->hasProperty("activeModels") ? obj->getProperty("activeModels") : juce::var();
        if (auto* activeObj = activeVar.getDynamicObject()) {
            for (const auto& [name, version] : activeObj->getProperties()) {
                activeModels[name.toString()] = version.toString();
            }
        }
        
        return true;
    }
    
    return false;
}

ProductionModelManager::ModelInfo ProductionModelManager::getModelInfo(const juce::String& modelName, const juce::String& version) const {
    auto it = modelRegistry.find(modelName);
    if (it == modelRegistry.end()) {
        return ModelInfo{};
    }
    
    for (const auto& model : it->second) {
        if (model.version == version) {
            return model;
        }
    }
    
    return ModelInfo{};
}

} // namespace ai
} // namespace zenith
