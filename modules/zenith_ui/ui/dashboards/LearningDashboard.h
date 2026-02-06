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
*/

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "ai_client/ModelTrainer.h"
#include "ai_client/ProductionModelManager.h"
#include <memory>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <limits>

namespace zenith {
namespace ui {

// Learning metrics
struct LearningMetrics {
    float accuracy = 0.0f;
    float loss = 0.0f;
    float validationAccuracy = 0.0f;
    float validationLoss = 0.0f;
    int epoch = 0;
    int totalEpochs = 0;
    float learningRate = 0.0f;
    float timeElapsed = 0.0f;  // seconds
    float eta = 0.0f;  // estimated time remaining
    juce::Time timestamp;
    juce::String modelType;
    juce::String status;  // "training", "validating", "completed", "error"
};

// Model performance data
struct ModelPerformance {
    juce::String modelName;
    juce::String version;
    float accuracy = 0.0f;
    float precision = 0.0f;
    float recall = 0.0f;
    float f1Score = 0.0f;
    float inferenceTime = 0.0f;  // ms
    float memoryUsage = 0.0f;   // MB
    int totalRequests = 0;
    int successfulRequests = 0;
    juce::Time lastUpdated;
};

// Training session information
struct TrainingSession {
    juce::String sessionId;
    juce::String modelType;
    juce::String datasetName;
    juce::Time startTime;
    juce::Time endTime;
    int totalEpochs = 0;
    int completedEpochs = 0;
    float finalAccuracy = 0.0f;
    float finalLoss = 0.0f;
    bool isCompleted = false;
    bool hasError = false;
    juce::String errorMessage;
    std::vector<LearningMetrics> metricsHistory;
};

// Learning dashboard component
class LearningDashboard : public juce::Component,
                         public juce::Timer,
                         public juce::Button::Listener,
                         public juce::ComboBox::Listener {
public:
    LearningDashboard();
    ~LearningDashboard() override;
    
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Training control
    void startTraining(const juce::String& modelType, const ai::TrainingConfig& config);
    void pauseTraining();
    void resumeTraining();
    void stopTraining();
    bool isTrainingActive() const;
    
    // Model management
    void addModel(const ai::ProductionModelManager::ModelInfo& modelInfo);
    void removeModel(const juce::String& modelName);
    void activateModel(const juce::String& modelName, const juce::String& version);
    std::vector<ai::ProductionModelManager::ModelInfo> getAvailableModels() const;
    
    // Metrics and monitoring
    void updateMetrics(const LearningMetrics& metrics);
    void updatePerformance(const ModelPerformance& performance);
    LearningMetrics getCurrentMetrics() const;
    std::vector<ModelPerformance> getPerformanceHistory() const;
    
    // Visualization
    void setShowRealTimeUpdates(bool show);
    void setUpdateInterval(int intervalMs);
    void exportMetrics(const juce::File& filePath) const;
    
    // Session management
    std::vector<TrainingSession> getTrainingHistory() const;
    void clearTrainingHistory();
    void saveSession(const TrainingSession& session);
    
    // Timer callback for real-time updates
    void timerCallback() override;
    
    // Listeners
    struct Listener {
        virtual ~Listener() = default;
        virtual void trainingStarted(const juce::String& sessionId) {}
        virtual void trainingCompleted(const juce::String& sessionId, const LearningMetrics& finalMetrics) {}
        virtual void trainingError(const juce::String& sessionId, const juce::String& error) {}
        virtual void modelActivated(const juce::String& modelName, const juce::String& version) {}
        virtual void metricsUpdated(const LearningMetrics& metrics) {}
    };
    
    void addListener(Listener* listener);
    void removeListener(Listener* listener);
    
private:
    // Training management
    std::unique_ptr<ai::ModelTrainer> modelTrainer;
    std::unique_ptr<ai::TrainingPipeline> trainingPipeline;
    std::atomic<bool> isTraining{false};
    std::atomic<bool> isPaused{false};
    
    // Current training session
    std::unique_ptr<TrainingSession> currentSession;
    std::vector<TrainingSession> trainingHistory;
    
    // Model management
    std::unique_ptr<ai::ProductionModelManager> modelManager;
    std::vector<ModelPerformance> performanceHistory;
    
    // UI components
    std::unique_ptr<juce::Viewport> mainViewport;
    std::unique_ptr<juce::Component> mainComponent;
    
    // Training controls
    std::unique_ptr<juce::ComboBox> modelTypeComboBox;
    std::unique_ptr<juce::TextButton> startTrainingButton;
    std::unique_ptr<juce::TextButton> pauseTrainingButton;
    std::unique_ptr<juce::TextButton> stopTrainingButton;
    
    // Training configuration
    std::unique_ptr<juce::Slider> epochsSlider;
    std::unique_ptr<juce::Slider> learningRateSlider;
    std::unique_ptr<juce::Slider> batchSizeSlider;
    std::unique_ptr<juce::ToggleButton> earlyStoppingToggle;
    std::unique_ptr<juce::ToggleButton> gpuAccelerationToggle;
    
    // Progress display
    std::unique_ptr<juce::ProgressBar> trainingProgressBar;
    std::unique_ptr<juce::Label> progressLabel;
    std::unique_ptr<juce::Label> etaLabel;
    std::unique_ptr<juce::Label> statusLabel;
    
    // Metrics display
    std::unique_ptr<juce::Label> accuracyLabel;
    std::unique_ptr<juce::Label> lossLabel;
    std::unique_ptr<juce::Label> validationAccuracyLabel;
    std::unique_ptr<juce::Label> validationLossLabel;
    
    // Visualization components
    std::unique_ptr<juce::Component> accuracyChart;
    std::unique_ptr<juce::Component> lossChart;
    std::unique_ptr<juce::Component> performanceChart;
    
    // Model management
    std::unique_ptr<juce::ListBox> modelListBox;
    std::unique_ptr<juce::TextButton> activateModelButton;
    std::unique_ptr<juce::TextButton> deployModelButton;
    std::unique_ptr<juce::TextButton> deleteModelButton;
    
    // Session history
    std::unique_ptr<juce::ListBox> sessionListBox;
    std::unique_ptr<juce::TextButton> clearHistoryButton;
    std::unique_ptr<juce::TextButton> exportHistoryButton;
    
    // Real-time settings
    bool showRealTimeUpdates = true;
    int updateIntervalMs = 1000;  // 1 second
    
    // Data
    LearningMetrics currentMetrics;
    std::mutex metricsMutex;
    
    // Listeners
    std::vector<Listener*> listeners;
    
    // UI creation
    void createTrainingControls();
    void createConfigurationControls();
    void createProgressDisplay();
    void createMetricsDisplay();
    void createVisualizationComponents();
    void createModelManagement();
    void createSessionHistory();
    
    // Chart creation
    void createAccuracyChart();
    void createLossChart();
    void createPerformanceChart();
    
    // Event handlers
    void onTrainingStarted();
    void onTrainingCompleted(const LearningMetrics& finalMetrics);
    void onTrainingError(const juce::String& error);
    void onModelActivated(const juce::String& modelName, const juce::String& version);
    void onMetricsUpdated(const LearningMetrics& metrics);
    
    // UI updates
    void updateTrainingControls();
    void updateProgressDisplay();
    void updateMetricsDisplay();
    void updateCharts();
    void updateModelList();
    void updateSessionList();
    
    // Chart drawing
    void drawAccuracyChart(juce::Graphics& g);
    void drawLossChart(juce::Graphics& g);
    void drawPerformanceChart(juce::Graphics& g);
    
    // Data processing
    void calculateETA();
    void updatePerformanceMetrics();
    void aggregateMetrics();
    
    // Notification
    void notifyTrainingStarted(const juce::String& sessionId);
    void notifyTrainingCompleted(const juce::String& sessionId, const LearningMetrics& finalMetrics);
    void notifyTrainingError(const juce::String& sessionId, const juce::String& error);
    void notifyModelActivated(const juce::String& modelName, const juce::String& version);
    void notifyMetricsUpdated(const LearningMetrics& metrics);
    
    // Utility
    juce::String formatTime(float seconds) const;
    juce::String formatPercentage(float percentage) const;
    juce::Colour getAccuracyColor(float accuracy) const;
    juce::Colour getLossColor(float loss) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LearningDashboard)
};

#include "MetricsChart.h"

// Training session list model
class TrainingSessionListModel : public juce::ListBoxModel {
public:
    TrainingSessionListModel(const std::vector<TrainingSession>& sessions);
    ~TrainingSessionListModel() = default;
    
    // ListBoxModel overrides
    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    juce::Component* refreshComponentForRow(int rowNumber, bool isRowSelected, juce::Component* existingComponentToUpdate) override;
    
    // Data management
    void setSessions(const std::vector<TrainingSession>& sessions);
    void addSession(const TrainingSession& session);
    void removeSession(int index);
    
private:
    const std::vector<TrainingSession>& sessions;
    
    juce::String formatSessionInfo(const TrainingSession& session) const;
    juce::Colour getStatusColor(const TrainingSession& session) const;
};

// Model list model
class ModelListModel : public juce::ListBoxModel {
public:
    ModelListModel(const std::vector<ai::ProductionModelManager::ModelInfo>& models);
    ~ModelListModel() = default;
    
    // ListBoxModel overrides
    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    juce::Component* refreshComponentForRow(int rowNumber, bool isRowSelected, juce::Component* existingComponentToUpdate) override;
    
    // Data management
    void setModels(const std::vector<ai::ProductionModelManager::ModelInfo>& models);
    void addModel(const ai::ProductionModelManager::ModelInfo& model);
    void removeModel(int index);
    
private:
    std::vector<ai::ProductionModelManager::ModelInfo> models;
    
    juce::String formatModelInfo(const ai::ProductionModelManager::ModelInfo& model) const;
    juce::Colour getStatusColor(const ai::ProductionModelManager::ModelInfo& model) const;
};

// Learning dashboard factory
class LearningDashboardFactory {
public:
    static std::unique_ptr<LearningDashboard> createDefaultDashboard();
    static std::unique_ptr<LearningDashboard> createMinimalDashboard();
    static std::unique_ptr<LearningDashboard> createAdvancedDashboard();
    
    // Default configurations
    static ai::TrainingConfig getDefaultTrainingConfig();
    static std::vector<juce::String> getDefaultModelTypes();
    
private:
    static void setupDefaultCharts(LearningDashboard& dashboard);
    static void setupDefaultMetrics(LearningDashboard& dashboard);
};

} // namespace ui
} // namespace zenith
