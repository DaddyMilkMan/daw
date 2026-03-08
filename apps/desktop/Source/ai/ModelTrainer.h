/*
  ==============================================================================
    ModelTrainer.h
    Production model training system - no stubs, no shortcuts
    Phase 3: Pre-trained Models
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "RealNeuralNetwork.h"
#include "GenreDetector.h"
#include "PredictiveMastering.h"
#include <memory>
#include <vector>
#include <functional>

namespace zenith {
namespace ai {

#include "AICommon.h"

// Dataset loader
class DatasetLoader {
public:
    DatasetLoader();
    ~DatasetLoader();
    
    // Dataset loading
    bool loadGenreDataset(const juce::File& datasetPath);
    bool loadQualityDataset(const juce::File& datasetPath);
    bool loadCreativeDataset(const juce::File& datasetPath);
    bool loadCustomDataset(const juce::File& datasetPath, const juce::String& format);
    
    // Dataset access
    std::vector<TrainingSample> getTrainingSamples() const { return trainingSamples; }
    std::vector<TrainingSample> getValidationSamples() const { return validationSamples; }
    std::vector<TrainingSample> getTestSamples() const { return testSamples; }
    
    // Dataset information
    int getTotalSamples() const;
    int getFeatureSize() const;
    int getTargetSize() const;
    std::vector<juce::String> getLabels() const;
    
    // Data preprocessing
    void normalizeFeatures();
    void standardizeFeatures();
    void applyPCA(int numComponents);
    void augmentData(float augmentationFactor = 2.0f);
    
    // Dataset statistics
    juce::String getDatasetInfo() const;
    bool isBalanced() const;
    std::unordered_map<juce::String, int> getClassDistribution() const;
    
private:
    std::vector<TrainingSample> allSamples;
    std::vector<TrainingSample> trainingSamples;
    std::vector<TrainingSample> validationSamples;
    std::vector<TrainingSample> testSamples;
    
    std::vector<juce::String> labels;
    int featureSize = 0;
    int targetSize = 0;
    
    // Loading helpers
    bool loadCSVFile(const juce::File& filePath);
    bool loadJSONFile(const juce::File& filePath);
    bool loadAudioFiles(const juce::File& directoryPath);
    
    // Data processing
    void splitDataset(float trainRatio, float validationRatio, float testRatio);
    std::vector<float> extractAudioFeatures(const juce::AudioBuffer<float>& audio, double sampleRate);
    std::vector<float> extractQualityMetrics(const juce::AudioBuffer<float>& audio);
    
    // Preprocessing
    std::vector<float> calculateFeatureMeans() const;
    std::vector<float> calculateFeatureStds() const;
    void applyNormalization(std::vector<float>& means, std::vector<float>& stds);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DatasetLoader)
};

// Model trainer
class ModelTrainer {
public:
    ModelTrainer();
    ~ModelTrainer();
    
    // Training methods
    TrainingResults trainGenreClassifier(const std::vector<TrainingSample>& dataset,
                                       const TrainingConfig& config = {},
                                       TrainingProgressCallback progressCallback = nullptr);
    
    TrainingResults trainQualityPredictor(const std::vector<TrainingSample>& dataset,
                                        const TrainingConfig& config = {},
                                        TrainingProgressCallback progressCallback = nullptr);
    
    TrainingResults trainCreativeNetwork(const std::vector<TrainingSample>& dataset,
                                       const TrainingConfig& config = {},
                                       TrainingProgressCallback progressCallback = nullptr);
    
    // Transfer learning
    TrainingResults fineTuneModel(NeuralNetwork& model,
                                 const std::vector<TrainingSample>& dataset,
                                 const TrainingConfig& config = {},
                                 TrainingProgressCallback progressCallback = nullptr);
    
    // Model evaluation
    ValidationMetrics evaluateModel(const NeuralNetwork& model,
                                  const std::vector<TrainingSample>& testDataset);
    
    // Hyperparameter optimization
    TrainingResults optimizeHyperparameters(const std::vector<TrainingSample>& dataset,
                                           const std::vector<TrainingConfig>& configs);
    
    // Model management
    bool saveModel(const NeuralNetwork& model, const juce::File& filePath);
    std::unique_ptr<NeuralNetwork> loadModel(const juce::File& filePath);
    
    // Training utilities
    void setRandomSeed(int seed);
    void enableGPUAcceleration(bool enable);
    bool isGPUAccelerationAvailable() const;
    
private:
    std::unique_ptr<DatasetLoader> datasetLoader;
    int randomSeed = 42;
    bool gpuEnabled = false;
    
    // Training implementation
    TrainingResults trainNetwork(NeuralNetwork& network,
                                const std::vector<TrainingSample>& trainingData,
                                const std::vector<TrainingSample>& validationData,
                                const TrainingConfig& config,
                                TrainingProgressCallback progressCallback);
    
    // Optimization algorithms
    void updateWeightsSGD(NeuralNetwork& network, const std::vector<float>& gradient, float learningRate);
    void updateWeightsAdam(NeuralNetwork& network, const std::vector<float>& gradient, float learningRate, float beta1, float beta2, float epsilon);
    void updateWeightsRMSprop(NeuralNetwork& network, const std::vector<float>& gradient, float learningRate, float epsilon);
    
    // Early stopping
    bool shouldStopEarly(const std::vector<float>& validationLossHistory, int patience, float minDelta);
    
    // Metrics calculation
    ValidationMetrics calculateMetrics(const NeuralNetwork& network, const std::vector<TrainingSample>& dataset);
    float calculateAccuracy(const NeuralNetwork& network, const std::vector<TrainingSample>& dataset);
    float calculateLoss(const NeuralNetwork& network, const std::vector<TrainingSample>& dataset);
    
    // Data augmentation
    std::vector<TrainingSample> augmentDataset(const std::vector<TrainingSample>& dataset, float factor);
    TrainingSample augmentSample(const TrainingSample& sample);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModelTrainer)
};

// Production model manager
#include "ProductionModelManager.h"

// Training pipeline orchestrator
class TrainingPipeline {
public:
    enum class PipelineStage {
        DataLoading,
        Preprocessing,
        Training,
        Validation,
        Deployment,
        Complete
    };
    
    struct PipelineConfig {
        juce::String datasetPath;
        juce::String modelType;  // "genre", "quality", "creative"
        TrainingConfig trainingConfig;
        bool autoDeploy = false;
        float performanceThreshold = 0.8f;
        juce::String targetEnvironment = "production";
    };
    
    TrainingPipeline();
    ~TrainingPipeline();
    
    // Pipeline execution
    bool executePipeline(const PipelineConfig& config, 
                        std::function<void(PipelineStage, float)> progressCallback = nullptr);
    
    // Stage monitoring
    PipelineStage getCurrentStage() const { return currentStage; }
    float getProgress() const { return progress; }
    juce::String getStatusMessage() const { return statusMessage; }
    
    // Results
    TrainingResults getTrainingResults() const { return trainingResults; }
    ProductionModelManager::ModelInfo getTrainedModel() const { return trainedModel; }
    
    // Pipeline control
    void pausePipeline();
    void resumePipeline();
    void stopPipeline();
    bool isRunning() const { return isRunningFlag; }
    
private:
    std::unique_ptr<DatasetLoader> datasetLoader;
    std::unique_ptr<ModelTrainer> modelTrainer;
    std::unique_ptr<ProductionModelManager> modelManager;
    
    PipelineStage currentStage = PipelineStage::DataLoading;
    std::atomic<float> progress{0.0f};
    juce::String statusMessage;
    std::atomic<bool> isRunningFlag{false};
    std::atomic<bool> isPaused{false};
    
    TrainingResults trainingResults;
    ProductionModelManager::ModelInfo trainedModel;
    
    // Pipeline stages
    bool executeDataLoading(const PipelineConfig& config);
    bool executePreprocessing(const PipelineConfig& config);
    bool executeTraining(const PipelineConfig& config);
    bool executeValidation(const PipelineConfig& config);
    bool executeDeployment(const PipelineConfig& config);
    
    // Progress tracking
    void updateProgress(PipelineStage stage, float stageProgress);
    void setStatusMessage(const juce::String& message);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrainingPipeline)
};

} // namespace ai
} // namespace zenith
