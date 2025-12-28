/*
  ==============================================================================
    ProductionModelManager.h
    Production model management with versioning and deployment
    Phase 3: Pre-trained Models
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include "ModelTrainer.h"
#include <memory>
#include <unordered_map>
#include <vector>

namespace zenith {
namespace ai {

// Model version information
struct ModelVersion {
    juce::String version;
    juce::String description;
    juce::Time createdAt;
    TrainingResults trainingResults;
    ValidationMetrics performanceMetrics;
    juce::String filePath;
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

// Model deployment configuration
struct DeploymentConfig {
    juce::String environment;  // "development", "staging", "production"
    bool enableRollback = true;
    bool enableA_BTesting = false;
    float trafficSplit = 1.0f;  // For A/B testing
    juce::String canaryDeployment = "";  // Model to test
    int maxMemoryUsage = 512;  // MB
    int maxInferenceTime = 10;  // ms
};

// Model performance monitoring
struct ModelPerformance {
    juce::String modelName;
    juce::String version;
    float accuracy = 0.0f;
    float latency = 0.0f;      // ms
    float throughput = 0.0f;   // requests/second
    float errorRate = 0.0f;    // percentage
    float memoryUsage = 0.0f;   // MB
    int totalRequests = 0;
    juce::Time lastUpdated;
    
    bool isHealthy() const {
        return accuracy >= 0.7f && 
               latency <= 100.0f && 
               errorRate <= 5.0f &&
               memoryUsage <= 512.0f;
    }
};

// Production model manager
class ProductionModelManager {
public:
    ProductionModelManager();
    ~ProductionModelManager();
    
    // Model registration
    bool registerModel(const juce::String& modelName, const ModelVersion& version);
    bool updateModel(const juce::String& modelName, const ModelVersion& version);
    bool deprecateModel(const juce::String& modelName, const juce::String& version);
    
    // Model deployment
    bool deployModel(const juce::String& modelName, const juce::String& version, const DeploymentConfig& config);
    bool rollbackModel(const juce::String& modelName);
    bool promoteToProduction(const juce::String& modelName, const juce::String& version);
    
    // Model access
    std::unique_ptr<NeuralNetwork> getModel(const juce::String& modelName, const juce::String& version = "");
    std::unique_ptr<NeuralNetwork> getProductionModel(const juce::String& modelName);
    std::vector<ModelVersion> getModelVersions(const juce::String& modelName) const;
    
    // Model information
    std::vector<juce::String> getAvailableModels() const;
    std::vector<juce::String> getProductionModels() const;
    ModelVersion getModelInfo(const juce::String& modelName, const juce::String& version) const;
    juce::String getProductionVersion(const juce::String& modelName) const;
    
    // Model comparison
    std::vector<ModelVersion> compareModels(const std::vector<juce::String>& modelNames) const;
    ModelVersion getBestModel(const juce::String& modelType) const;
    std::vector<juce::String> getUpgradeCandidates(const juce::String& modelName) const;
    
    // Performance monitoring
    void recordPerformance(const ModelPerformance& performance);
    ModelPerformance getPerformanceMetrics(const juce::String& modelName, const juce::String& version) const;
    std::vector<ModelPerformance> getPerformanceHistory(const juce::String& modelName, int hours = 24) const;
    
    // Health checks
    bool isModelHealthy(const juce::String& modelName, const juce::String& version) const;
    std::vector<juce::String> getUnhealthyModels() const;
    juce::String getSystemHealthReport() const;
    
    // Model validation
    bool validateModel(const juce::String& modelName, const juce::String& version);
    std::vector<juce::String> getValidationErrors(const juce::String& modelName, const juce::String& version) const;
    
    // Model backup and restore
    bool backupModel(const juce::String& modelName, const juce::String& version);
    bool restoreModel(const juce::String& modelName, const juce::String& version);
    bool exportModel(const juce::String& modelName, const juce::String& version, const juce::File& exportPath);
    bool importModel(const juce::File& importPath);
    
    // Automation
    bool enableAutoDeployment(bool enabled);
    bool enableAutoScaling(bool enabled);
    void setPerformanceThresholds(float minAccuracy, float maxLatency, float maxErrorRate);
    
    // Persistence
    bool saveRegistry(const juce::File& filePath) const;
    bool loadRegistry(const juce::File& filePath);
    bool savePerformanceData(const juce::File& filePath) const;
    bool loadPerformanceData(const juce::File& filePath);
    
private:
    // Model registry
    std::unordered_map<juce::String, std::vector<ModelVersion>> modelRegistry;
    std::unordered_map<juce::String, juce::String> productionModels;  // name -> version
    std::unordered_map<juce::String, std::string> modelCache;  // name+version -> cached model
    
    // Performance monitoring
    std::vector<ModelPerformance> performanceHistory;
    std::unordered_map<juce::String, ModelPerformance> currentPerformance;
    
    // Configuration
    bool autoDeploymentEnabled = false;
    bool autoScalingEnabled = false;
    float minAccuracyThreshold = 0.7f;
    float maxLatencyThreshold = 100.0f;
    float maxErrorRateThreshold = 5.0f;
    
    // File paths
    juce::File registryFile;
    juce::File performanceFile;
    juce::File modelDirectory;
    juce::File backupDirectory;
    
    // Cache management
    void clearCache();
    void cacheModel(const juce::String& modelName, const juce::String& version, std::unique_ptr<NeuralNetwork> model);
    
    // Validation
    bool validateModelFile(const juce::File& filePath) const;
    bool validateModelPerformance(const ModelVersion& version) const;
    std::vector<juce::String> getValidationRules() const;
    
    // Deployment logic
    bool canDeployModel(const ModelVersion& version, const DeploymentConfig& config) const;
    bool performDeployment(const juce::String& modelName, const ModelVersion& version, const DeploymentConfig& config);
    bool performRollback(const juce::String& modelName);
    
    // Performance analysis
    void analyzePerformanceTrends();
    std::vector<juce::String> getPerformanceAnomalies() const;
    
    // Automation
    void checkAutoDeployment();
    void checkAutoScaling();
    void triggerAlert(const juce::String& alertType, const juce::String& message);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProductionModelManager)
};

// Model deployment pipeline
class ModelDeploymentPipeline {
public:
    enum class DeploymentStage {
        Validation,
        Testing,
        Staging,
        Production,
        Rollback,
        Complete
    };
    
    struct DeploymentRequest {
        juce::String modelName;
        juce::String version;
        DeploymentConfig config;
        juce::Time requestedAt;
        juce::String requestedBy;
        bool isEmergency = false;
    };
    
    struct DeploymentStatus {
        DeploymentStage stage;
        float progress;
        juce::String message;
        juce::Time lastUpdate;
        bool hasErrors;
        juce::String error;
    };
    
    ModelDeploymentPipeline(ProductionModelManager& manager);
    ~ModelDeploymentPipeline();
    
    // Deployment management
    juce::String submitDeployment(const DeploymentRequest& request);
    bool cancelDeployment(const juce::String& deploymentId);
    DeploymentStatus getDeploymentStatus(const juce::String& deploymentId) const;
    
    // Deployment monitoring
    std::vector<juce::String> getActiveDeployments() const;
    std::vector<juce::String> getDeploymentHistory(int limit = 100) const;
    
    // Automation
    void enableContinuousDeployment(bool enabled);
    void setDeploymentTriggers(const std::vector<juce::String>& triggers);
    
private:
    ProductionModelManager& modelManager;
    
    std::unordered_map<juce::String, DeploymentRequest> activeDeployments;
    std::unordered_map<juce::String, DeploymentStatus> deploymentStatuses;
    std::vector<std::tuple<juce::String, DeploymentRequest, DeploymentStatus>> deploymentHistory;
    
    bool continuousDeploymentEnabled = false;
    std::vector<juce::String> deploymentTriggers;
    
    // Deployment execution
    void executeDeployment(const juce::String& deploymentId);
    bool validateDeployment(const DeploymentRequest& request);
    bool testDeployment(const DeploymentRequest& request);
    bool stageDeployment(const DeploymentRequest& request);
    bool productionDeployment(const DeploymentRequest& request);
    
    // Status management
    void updateDeploymentStatus(const juce::String& deploymentId, DeploymentStage stage, float progress, const juce::String& message);
    void completeDeployment(const juce::String& deploymentId, bool success);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModelDeploymentPipeline)
};

// Model analytics dashboard
class ModelAnalytics {
public:
    struct AnalyticsData {
        juce::String modelName;
        juce::String version;
        juce::Time timestamp;
        float accuracy;
        float latency;
        float throughput;
        float errorRate;
        float memoryUsage;
        int requestCount;
        juce::String environment;
    };
    
    ModelAnalytics(ProductionModelManager& manager);
    ~ModelAnalytics();
    
    // Data collection
    void recordMetrics(const AnalyticsData& data);
    std::vector<AnalyticsData> getMetrics(const juce::String& modelName, int hours = 24) const;
    
    // Analytics queries
    std::vector<AnalyticsData> getTopPerformingModels(int limit = 10) const;
    std::vector<AnalyticsData> getSlowModels(float thresholdMs = 100.0f) const;
    std::vector<AnalyticsData> getHighErrorModels(float thresholdPercent = 5.0f) const;
    
    // Trend analysis
    std::vector<std::pair<juce::Time, float>> getAccuracyTrend(const juce::String& modelName, int hours = 24) const;
    std::vector<std::pair<juce::Time, float>> getLatencyTrend(const juce::String& modelName, int hours = 24) const;
    
    // Reporting
    juce::String generateReport(const juce::String& modelName) const;
    juce::String generateSystemReport() const;
    bool exportAnalytics(const juce::File& filePath) const;
    
    // Alerts
    void setAlertThresholds(float accuracyThreshold, float latencyThreshold, float errorThreshold);
    std::vector<juce::String> getActiveAlerts() const;
    
private:
    ProductionModelManager& modelManager;
    
    std::vector<AnalyticsData> analyticsData;
    std::unordered_map<juce::String, std::vector<AnalyticsData>> modelMetrics;
    
    // Alert thresholds
    float accuracyAlertThreshold = 0.7f;
    float latencyAlertThreshold = 100.0f;
    float errorAlertThreshold = 5.0f;
    
    // Data processing
    void processData();
    void aggregateMetrics();
    std::vector<AnalyticsData> filterByTimeRange(const std::vector<AnalyticsData>& data, int hours) const;
    
    // Trend calculation
    float calculateTrend(const std::vector<std::pair<juce::Time, float>>& data) const;
    std::vector<std::pair<juce::Time, float>> smoothData(const std::vector<std::pair<juce::Time, float>>& data) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModelAnalytics)
};

} // namespace ai
} // namespace zenith
