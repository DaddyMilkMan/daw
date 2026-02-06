/*
  ==============================================================================
    ProductionModelManager.h
    Production model management with versioning and deployment
  ==============================================================================
 */

#pragma once

#include <juce_core/juce_core.h>
#include "AICommon.h"
#include <memory>
#include <unordered_map>
#include <vector>

namespace zenith {
namespace ai {

// Production model manager
class ProductionModelManager {
public:
    // Model info structure
    struct ModelInfo {
        juce::String name;
        juce::String version;
        juce::String description;
        juce::Time createdAt;
        TrainingResults trainingResults;
        ValidationMetrics performanceMetrics;
        juce::File filePath;
        bool isActive = false;
        bool isProduction = false;
        bool isDeprecated = false;
        juce::String changelog;
        
        // Model metadata
        std::vector<juce::String> tags;
        juce::String author;
        juce::String datasetInfo;
        float modelSize = 0.0f;  // MB
        int inferenceTime = 0;    // ms
    };

    ProductionModelManager();
    ~ProductionModelManager();
    
    // Model registration and activation
    bool registerModel(const ModelInfo& modelInfo);
    bool activateModel(const juce::String& modelName, const juce::String& version);
    
    // Model access and info
    std::vector<ModelInfo> getAvailableModels() const;
    ModelInfo getModelInfo(const juce::String& modelName, const juce::String& version) const;
    
    // Validation
    bool validateModel(const juce::String& modelName, const juce::String& version);
    bool isValidModel(const ModelInfo& modelInfo) const;
    bool isModelCompatible(const ModelInfo& modelInfo) const;
    
    // Persistence
    bool saveModelRegistry(const juce::File& filePath) const;
    bool loadModelRegistry(const juce::File& filePath);
    
private:
    void initializeRegistry();
    void updateRegistry();
    
    // Model registry data
    std::unordered_map<juce::String, std::vector<ModelInfo>> modelRegistry;
    std::unordered_map<juce::String, juce::String> activeModels;  // name -> version
    
    // Configuration and paths
    juce::File modelDirectory;
    juce::File registryFile;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProductionModelManager)
};

} // namespace ai
} // namespace zenith
