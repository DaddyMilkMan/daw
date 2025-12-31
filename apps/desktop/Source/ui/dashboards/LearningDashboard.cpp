/*
  ==============================================================================
    LearningDashboard.cpp
    Learning progress dashboard implementation
  ==============================================================================
*/

#include "LearningDashboard.h"
#include <algorithm>
#include <numeric>

namespace zenith {
namespace ui {

// LearningDashboard Implementation
LearningDashboard::LearningDashboard() 
    : modelTrainer(std::make_unique<ai::ModelTrainer>()),
      trainingPipeline(std::make_unique<ai::TrainingPipeline>()),
      modelManager(std::make_unique<ai::ProductionModelManager>()),
      undoRedoManager(std::make_unique<UndoRedoManager>()) {
    
    // Initialize UI
    createTrainingControls();
    createConfigurationControls();
    createProgressDisplay();
    createMetricsDisplay();
    createVisualizationComponents();
    createModelManagement();
    createSessionHistory();
    
    // Start timer for updates
    startTimerHz(1);
}

LearningDashboard::~LearningDashboard() {
    stopTimer();
}

void LearningDashboard::paint(juce::Graphics& g) {
    g.fillAll(juce::Colours::darkgrey);
}

void LearningDashboard::resized() {
    auto bounds = getLocalBounds();
    int margin = 10;
    
    // Layout controls (top)
    int controlHeight = 30;
    int topY = margin;
    
    if (modelTypeComboBox) {
        modelTypeComboBox->setBounds(margin, topY, 150, controlHeight);
    }
    if (startTrainingButton) {
        startTrainingButton->setBounds(170, topY, 80, controlHeight);
    }
    if (pauseTrainingButton) {
        pauseTrainingButton->setBounds(260, topY, 80, controlHeight);
    }
    if (stopTrainingButton) {
        stopTrainingButton->setBounds(350, topY, 80, controlHeight);
    }
    
    // Configuration (below controls)
    topY += controlHeight + margin;
    if (epochsSlider) {
        epochsSlider->setBounds(margin, topY, 200, controlHeight);
    }
    if (learningRateSlider) {
        learningRateSlider->setBounds(220, topY, 200, controlHeight);
    }
    if (batchSizeSlider) {
        batchSizeSlider->setBounds(430, topY, 200, controlHeight);
    }
    
    // Progress display
    topY += controlHeight + margin;
    if (trainingProgressBar) {
        trainingProgressBar->setBounds(margin, topY, bounds.getWidth() - 2 * margin, 20);
    }
    topY += 25;
    if (progressLabel) {
        progressLabel->setBounds(margin, topY, bounds.getWidth() - 2 * margin, 20);
    }
    
    // Metrics display
    topY += 25;
    int metricWidth = (bounds.getWidth() - 3 * margin) / 4;
    if (accuracyLabel) {
        accuracyLabel->setBounds(margin, topY, metricWidth, 20);
    }
    if (lossLabel) {
        lossLabel->setBounds(margin + metricWidth + margin, topY, metricWidth, 20);
    }
    if (validationAccuracyLabel) {
        validationAccuracyLabel->setBounds(margin + 2 * (metricWidth + margin), topY, metricWidth, 20);
    }
    if (validationLossLabel) {
        validationLossLabel->setBounds(margin + 3 * (metricWidth + margin), topY, metricWidth, 20);
    }
    
    // Charts (below metrics)
    topY += 25;
    int chartHeight = (bounds.getHeight() - topY - margin) / 2;
    int chartWidth = (bounds.getWidth() - 3 * margin) / 2;
    
    if (accuracyChart) {
        accuracyChart->setBounds(margin, topY, chartWidth, chartHeight);
    }
    if (lossChart) {
        lossChart->setBounds(margin + chartWidth + margin, topY, chartWidth, chartHeight);
    }
    
    topY += chartHeight + margin;
    if (performanceChart) {
        performanceChart->setBounds(margin, topY, chartWidth, chartHeight);
    }
    if (modelListBox) {
        modelListBox->setBounds(margin + chartWidth + margin, topY, chartWidth, chartHeight);
    }
}

void LearningDashboard::startTraining(const juce::String& modelType, const ai::TrainingConfig& config) {
    if (isTraining.load()) {
        return; // Already training
    }
    
    // Create new session
    currentSession = std::make_unique<TrainingSession>();
    currentSession->sessionId = juce::Uuid().toString();
    currentSession->modelType = modelType;
    currentSession->startTime = juce::Time::getCurrentTime();
    currentSession->totalEpochs = config.epochs;
    currentSession->completedEpochs = 0;
    
    // Start training in background
    isTraining.store(true);
    isPaused.store(false);
    
    // Start training thread
    juce::Thread::launch([this, config]() {
        try {
            // Initialize trainer
            modelTrainer->initialize(config);
            
            // Training loop
            for (int epoch = 0; epoch < config.epochs && isTraining.load(); ++epoch) {
                if (isPaused.load()) {
                    juce::Thread::sleep(100);
                    --epoch;
                    continue;
                }
                
                // Train one epoch
                auto metrics = modelTrainer->trainEpoch();
                
                // Update metrics
                {
                    std::lock_guard<std::mutex> lock(metricsMutex);
                    currentMetrics = metrics;
                    currentMetrics.epoch = epoch + 1;
                    currentMetrics.totalEpochs = config.epochs;
                    currentMetrics.modelType = currentSession->modelType;
                    currentMetrics.status = "training";
                }
                
                // Store in session
                currentSession->metricsHistory.push_back(metrics);
                currentSession->completedEpochs = epoch + 1;
                
                // Trigger async update
                triggerAsyncUpdate();
                
                // Check early stopping
                if (config.earlyStopping && shouldStopEarly(metrics)) {
                    break;
                }
            }
            
            // Training completed
            if (isTraining.load()) {
                currentSession->isCompleted = true;
                currentSession->endTime = juce::Time::getCurrentTime();
                
                // Get final metrics
                std::lock_guard<std::mutex> lock(metricsMutex);
                currentSession->finalAccuracy = currentMetrics.accuracy;
                currentSession->finalLoss = currentMetrics.loss;
                currentMetrics.status = "completed";
                
                // Save model
                auto modelPath = modelTrainer->saveModel("model_" + currentSession->sessionId);
                
                // Register with production manager
                ai::ProductionModelManager::ModelInfo modelInfo;
                modelInfo.modelId = currentSession->sessionId;
                modelInfo.name = "Trained Model " + currentSession->sessionId.substring(0, 8);
                modelInfo.version = "1.0.0";
                modelInfo.modelPath = modelPath.getFullPathName();
                modelInfo.accuracy = currentSession->finalAccuracy;
                modelInfo.isActive = false;
                modelInfo.isHealthy = true;
                
                modelManager->registerModel(modelInfo);
                
                notifyTrainingCompleted(*currentSession, currentMetrics);
            }
        } catch (const std::exception& e) {
            // Training failed
            currentSession->hasError = true;
            currentSession->errorMessage = e.what();
            currentMetrics.status = "error";
            
            notifyTrainingError(currentSession->sessionId, e.what());
        }
        
        isTraining.store(false);
    });
    
    notifyTrainingStarted(currentSession->sessionId);
}

void LearningDashboard::pauseTraining() {
    isPaused.store(true);
}

void LearningDashboard::resumeTraining() {
    isPaused.store(false);
}

void LearningDashboard::stopTraining() {
    isTraining.store(false);
    isPaused.store(false);
    
    if (currentSession) {
        currentSession->endTime = juce::Time::getCurrentTime();
        if (!currentSession->isCompleted) {
            currentSession->hasError = true;
            currentSession->errorMessage = "Training stopped by user";
        }
    }
}

bool LearningDashboard::isTrainingActive() const {
    return isTraining.load();
}

void LearningDashboard::addModel(const ai::ProductionModelManager::ModelInfo& modelInfo) {
    modelManager->registerModel(modelInfo);
    updateModelList();
}

void LearningDashboard::removeModel(const juce::String& modelName) {
    modelManager->deactivateModel(modelName);
    modelManager->removeModel(modelName);
    updateModelList();
}

void LearningDashboard::activateModel(const juce::String& modelName, const juce::String& version) {
    modelManager->activateModel(modelName, version);
    updateModelList();
    notifyModelActivated(modelName, version);
}

std::vector<ai::ProductionModelManager::ModelInfo> LearningDashboard::getAvailableModels() const {
    return modelManager->getAllModels();
}

void LearningDashboard::updateMetrics(const LearningMetrics& metrics) {
    std::lock_guard<std::mutex> lock(metricsMutex);
    currentMetrics = metrics;
    triggerAsyncUpdate();
}

void LearningDashboard::updatePerformance(const ModelPerformance& performance) {
    performanceHistory.push_back(performance);
    
    // Keep only last 100 entries
    if (performanceHistory.size() > 100) {
        performanceHistory.erase(performanceHistory.begin());
    }
    
    triggerAsyncUpdate();
}

LearningMetrics LearningDashboard::getCurrentMetrics() const {
    std::lock_guard<std::mutex> lock(metricsMutex);
    return currentMetrics;
}

std::vector<ModelPerformance> LearningDashboard::getPerformanceHistory() const {
    return performanceHistory;
}

void LearningDashboard::setShowRealTimeUpdates(bool show) {
    showRealTimeUpdates = show;
    if (show) {
        startTimerHz(1);
    } else {
        stopTimer();
    }
}

void LearningDashboard::setUpdateInterval(int intervalMs) {
    updateIntervalMs = intervalMs;
    if (isTimerRunning()) {
        startTimer(intervalMs);
    }
}

void LearningDashboard::exportMetrics(const juce::File& filePath) const {
    juce::DynamicObject::Ptr exportData = new juce::DynamicObject();
    
    // Export current metrics
    juce::DynamicObject::Ptr currentMetricsObj = new juce::DynamicObject();
    currentMetricsObj->setProperty("accuracy", currentMetrics.accuracy);
    currentMetricsObj->setProperty("loss", currentMetrics.loss);
    currentMetricsObj->setProperty("validationAccuracy", currentMetrics.validationAccuracy);
    currentMetricsObj->setProperty("validationLoss", currentMetrics.validationLoss);
    currentMetricsObj->setProperty("epoch", currentMetrics.epoch);
    currentMetricsObj->setProperty("totalEpochs", currentMetrics.totalEpochs);
    currentMetricsObj->setProperty("status", currentMetrics.status);
    exportData->setProperty("currentMetrics", currentMetricsObj);
    
    // Export training history
    juce::Array<juce::var> historyArray;
    for (const auto& session : trainingHistory) {
        juce::DynamicObject::Ptr sessionObj = new juce::DynamicObject();
        sessionObj->setProperty("sessionId", session.sessionId);
        sessionObj->setProperty("modelType", session.modelType);
        sessionObj->setProperty("finalAccuracy", session.finalAccuracy);
        sessionObj->setProperty("finalLoss", session.finalLoss);
        sessionObj->setProperty("isCompleted", session.isCompleted);
        sessionObj->setProperty("startTime", session.startTime.toMilliseconds());
        sessionObj->setProperty("endTime", session.endTime.toMilliseconds());
        historyArray.add(sessionObj);
    }
    exportData->setProperty("trainingHistory", historyArray);
    
    // Write to file
    filePath.replaceWithText(juce::JSON::toString(exportData));
}

std::vector<TrainingSession> LearningDashboard::getTrainingHistory() const {
    return trainingHistory;
}

void LearningDashboard::clearTrainingHistory() {
    trainingHistory.clear();
}

void LearningDashboard::saveSession(const TrainingSession& session) {
    trainingHistory.push_back(session);
    
    // Keep only last 50 sessions
    if (trainingHistory.size() > 50) {
        trainingHistory.erase(trainingHistory.begin());
    }
}

void LearningDashboard::timerCallback() {
    if (showRealTimeUpdates && isTraining.load()) {
        updateProgressDisplay();
        updateMetricsDisplay();
        updateCharts();
    }
}

void LearningDashboard::handleAsyncUpdate() {
    updateProgressDisplay();
    updateMetricsDisplay();
    updateCharts();
}

void LearningDashboard::addListener(Listener* listener) {
    listeners.push_back(listener);
}

void LearningDashboard::removeListener(Listener* listener) {
    listeners.erase(std::remove(listeners.begin(), listeners.end(), listener), listeners.end());
}

void LearningDashboard::buttonClicked(juce::Button* button) {
    if (button == startTrainingButton.get()) {
        ai::TrainingConfig config;
        config.epochs = static_cast<int>(epochsSlider->getValue());
        config.learningRate = learningRateSlider->getValue();
        config.batchSize = static_cast<int>(batchSizeSlider->getValue());
        config.earlyStopping = earlyStoppingToggle->getToggleState();
        
        juce::String modelType = modelTypeComboBox->getText();
        startTraining(modelType, config);
    } else if (button == pauseTrainingButton.get()) {
        if (isPaused.load()) {
            resumeTraining();
            pauseTrainingButton->setButtonText("Pause");
        } else {
            pauseTraining();
            pauseTrainingButton->setButtonText("Resume");
        }
    } else if (button == stopTrainingButton.get()) {
        stopTraining();
    } else if (button == activateModelButton.get()) {
        int selectedRow = modelListBox->getSelectedRow();
        if (selectedRow >= 0) {
            auto models = modelManager->getAllModels();
            if (selectedRow < models.size()) {
                activateModel(models[selectedRow].modelId, models[selectedRow].version);
            }
        }
    }
}

void LearningDashboard::comboBoxChanged(juce::ComboBox* comboBox) {
    // Handle combo box changes if needed
}

void LearningDashboard::createTrainingControls() {
    // Model type selection
    modelTypeComboBox = std::make_unique<juce::ComboBox>("Model Type");
    modelTypeComboBox->addItem("Genre Classifier", 1);
    modelTypeComboBox->addItem("Quality Predictor", 2);
    modelTypeComboBox->addItem("Creative Suggestion", 3);
    modelTypeComboBox->setSelectedId(1);
    addAndMakeVisible(*modelTypeComboBox);
    
    // Control buttons
    startTrainingButton = std::make_unique<juce::TextButton>("Start");
    startTrainingButton->addListener(this);
    addAndMakeVisible(*startTrainingButton);
    
    pauseTrainingButton = std::make_unique<juce::TextButton>("Pause");
    pauseTrainingButton->addListener(this);
    addAndMakeVisible(*pauseTrainingButton);
    
    stopTrainingButton = std::make_unique<juce::TextButton>("Stop");
    stopTrainingButton->addListener(this);
    addAndMakeVisible(*stopTrainingButton);
}

void LearningDashboard::createConfigurationControls() {
    // Epochs slider
    epochsSlider = std::make_unique<juce::Slider>("Epochs");
    epochsSlider->setRange(1, 1000, 1);
    epochsSlider->setValue(100);
    epochsSlider->setSliderStyle(juce::Slider::LinearHorizontal);
    epochsSlider->setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    addAndMakeVisible(*epochsSlider);
    
    // Learning rate slider
    learningRateSlider = std::make_unique<juce::Slider>("Learning Rate");
    learningRateSlider->setRange(0.0001, 1.0, 0.0001);
    learningRateSlider->setValue(0.01);
    learningRateSlider->setSliderStyle(juce::Slider::LinearHorizontal);
    learningRateSlider->setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    addAndMakeVisible(*learningRateSlider);
    
    // Batch size slider
    batchSizeSlider = std::make_unique<juce::Slider>("Batch Size");
    batchSizeSlider->setRange(1, 256, 1);
    batchSizeSlider->setValue(32);
    batchSizeSlider->setSliderStyle(juce::Slider::LinearHorizontal);
    batchSizeSlider->setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    addAndMakeVisible(*batchSizeSlider);
    
    // Early stopping toggle
    earlyStoppingToggle = std::make_unique<juce::ToggleButton>("Early Stopping");
    earlyStoppingToggle->setToggleState(true, juce::dontSendNotification);
    addAndMakeVisible(*earlyStoppingToggle);
}

void LearningDashboard::createProgressDisplay() {
    trainingProgressBar = std::make_unique<juce::ProgressBar>();
    addAndMakeVisible(*trainingProgressBar);
    
    progressLabel = std::make_unique<juce::Label>("Progress", "Ready to train");
    progressLabel->setFont(14.0f);
    progressLabel->setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(*progressLabel);
    
    etaLabel = std::make_unique<juce::Label>("ETA", "--");
    etaLabel->setFont(12.0f);
    etaLabel->setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(*etaLabel);
    
    statusLabel = std::make_unique<juce::Label>("Status", "Idle");
    statusLabel->setFont(12.0f);
    statusLabel->setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(*statusLabel);
}

void LearningDashboard::createMetricsDisplay() {
    accuracyLabel = std::make_unique<juce::Label>("Accuracy", "Accuracy: 0.00%");
    accuracyLabel->setFont(14.0f);
    accuracyLabel->setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(*accuracyLabel);
    
    lossLabel = std::make_unique<juce::Label>("Loss", "Loss: 0.000");
    lossLabel->setFont(14.0f);
    lossLabel->setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(*lossLabel);
    
    validationAccuracyLabel = std::make_unique<juce::Label>("Val Accuracy", "Val Acc: 0.00%");
    validationAccuracyLabel->setFont(14.0f);
    validationAccuracyLabel->setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(*validationAccuracyLabel);
    
    validationLossLabel = std::make_unique<juce::Label>("Val Loss", "Val Loss: 0.000");
    validationLossLabel->setFont(14.0f);
    validationLossLabel->setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(*validationLossLabel);
}

void LearningDashboard::createVisualizationComponents() {
    createAccuracyChart();
    createLossChart();
    createPerformanceChart();
}

void LearningDashboard::createAccuracyChart() {
    accuracyChart = std::make_unique<MetricsChart>(MetricsChart::ChartType::Line);
    accuracyChart->setLineColor(juce::Colours::green);
    accuracyChart->setYRange(0.0f, 1.0f);
    accuracyChart->setYLabel("Accuracy");
    accuracyChart->setXLabel("Epoch");
    addAndMakeVisible(*accuracyChart);
}

void LearningDashboard::createLossChart() {
    lossChart = std::make_unique<MetricsChart>(MetricsChart::ChartType::Line);
    lossChart->setLineColor(juce::Colours::red);
    lossChart->setYRange(0.0f, 2.0f);
    lossChart->setYLabel("Loss");
    lossChart->setXLabel("Epoch");
    addAndMakeVisible(*lossChart);
}

void LearningDashboard::createPerformanceChart() {
    performanceChart = std::make_unique<MetricsChart>(MetricsChart::ChartType::Bar);
    performanceChart->setLineColor(juce::Colours::blue);
    performanceChart->setYLabel("Performance");
    addAndMakeVisible(*performanceChart);
}

void LearningDashboard::createModelManagement() {
    // Model list
    modelListModel = std::make_unique<ModelListModel>(modelManager->getAllModels());
    modelListBox = std::make_unique<juce::ListBox>("Models");
    modelListBox->setModel(modelListModel.get());
    modelListBox->setRowHeight(25);
    addAndMakeVisible(*modelListBox);
    
    // Control buttons
    activateModelButton = std::make_unique<juce::TextButton>("Activate");
    activateModelButton->addListener(this);
    addAndMakeVisible(*activateModelButton);
    
    deployModelButton = std::make_unique<juce::TextButton>("Deploy");
    deployModelButton->addListener(this);
    addAndMakeVisible(*deployModelButton);
    
    deleteModelButton = std::make_unique<juce::TextButton>("Delete");
    deleteModelButton->addListener(this);
    addAndMakeVisible(*deleteModelButton);
}

void LearningDashboard::createSessionHistory() {
    sessionListModel = std::make_unique<TrainingSessionListModel>(trainingHistory);
    sessionListBox = std::make_unique<juce::ListBox>("Sessions");
    sessionListBox->setModel(sessionListModel.get());
    sessionListBox->setRowHeight(25);
    addAndMakeVisible(*sessionListBox);
    
    clearHistoryButton = std::make_unique<juce::TextButton>("Clear History");
    clearHistoryButton->addListener(this);
    addAndMakeVisible(*clearHistoryButton);
    
    exportHistoryButton = std::make_unique<juce::TextButton>("Export");
    exportHistoryButton->addListener(this);
    addAndMakeVisible(*exportHistoryButton);
}

void LearningDashboard::updateTrainingControls() {
    bool training = isTraining.load();
    startTrainingButton->setEnabled(!training);
    pauseTrainingButton->setEnabled(training);
    stopTrainingButton->setEnabled(training);
}

void LearningDashboard::updateProgressDisplay() {
    if (currentSession && isTraining.load()) {
        float progress = static_cast<float>(currentSession->completedEpochs) / 
                        static_cast<float>(currentSession->totalEpochs);
        
        trainingProgressBar->setProgress(progress);
        
        progressLabel->setText(juce::String::formatted(
            "Epoch %d/%d - %s",
            currentSession->completedEpochs,
            currentSession->totalEpochs,
            isPaused.load() ? "Paused" : "Training"
        ));
        
        if (!isPaused.load() && currentSession->completedEpochs > 0) {
            auto elapsed = juce::Time::getCurrentTime() - currentSession->startTime;
            auto estimated = (elapsed.inSeconds() * currentSession->totalEpochs) / 
                            currentSession->completedEpochs;
            auto remaining = estimated - elapsed.inSeconds();
            
            etaLabel->setText("ETA: " + formatTime(static_cast<float>(remaining)));
        }
        
        statusLabel->setText("Status: " + currentMetrics.status);
    } else {
        trainingProgressBar->setProgress(0.0);
        progressLabel->setText("Ready to train");
        etaLabel->setText("ETA: --");
        statusLabel->setText("Status: Idle");
    }
    
    updateTrainingControls();
}

void LearningDashboard::updateMetricsDisplay() {
    accuracyLabel->setText("Accuracy: " + formatPercentage(currentMetrics.accuracy));
    lossLabel->setText("Loss: " + juce::String(currentMetrics.loss, 3));
    validationAccuracyLabel->setText("Val Acc: " + formatPercentage(currentMetrics.validationAccuracy));
    validationLossLabel->setText("Val Loss: " + juce::String(currentMetrics.validationLoss, 3));
}

void LearningDashboard::updateCharts() {
    if (currentSession && !currentSession->metricsHistory.empty()) {
        // Update accuracy chart
        accuracyChart->clearData();
        for (size_t i = 0; i < currentSession->metricsHistory.size(); ++i) {
            accuracyChart->addDataPoint(static_cast<float>(i), 
                                       currentSession->metricsHistory[i].accuracy);
        }
        
        // Update loss chart
        lossChart->clearData();
        for (size_t i = 0; i < currentSession->metricsHistory.size(); ++i) {
            lossChart->addDataPoint(static_cast<float>(i), 
                                   currentSession->metricsHistory[i].loss);
        }
    }
    
    // Update performance chart
    performanceChart->clearData();
    for (size_t i = 0; i < performanceHistory.size(); ++i) {
        performanceChart->addDataPoint(static_cast<float>(i), 
                                      performanceHistory[i].accuracy);
    }
}

void LearningDashboard::updateModelList() {
    if (modelListModel) {
        modelListModel->setModels(modelManager->getAllModels());
        modelListBox->updateContent();
    }
}

void LearningDashboard::updateSessionList() {
    if (sessionListModel) {
        sessionListModel->setSessions(trainingHistory);
        sessionListBox->updateContent();
    }
}

bool LearningDashboard::shouldStopEarly(const LearningMetrics& metrics) {
    // Simple early stopping: if validation loss hasn't improved for 10 epochs
    static juce::Array<float> validationLossHistory;
    
    validationLossHistory.add(metrics.validationLoss);
    
    if (validationLossHistory.size() > 10) {
        validationLossHistory.remove(0);
        
        // Check if minimum was more than 5 epochs ago
        int minIndex = 0;
        float minLoss = validationLossHistory[0];
        for (int i = 1; i < validationLossHistory.size(); ++i) {
            if (validationLossHistory[i] < minLoss) {
                minLoss = validationLossHistory[i];
                minIndex = i;
            }
        }
        
        return (validationLossHistory.size() - minIndex - 1) >= 5;
    }
    
    return false;
}

juce::String LearningDashboard::formatTime(float seconds) const {
    int hours = static_cast<int>(seconds) / 3600;
    int minutes = (static_cast<int>(seconds) % 3600) / 60;
    int secs = static_cast<int>(seconds) % 60;
    
    if (hours > 0) {
        return juce::String::formatted("%dh %dm %ds", hours, minutes, secs);
    } else if (minutes > 0) {
        return juce::String::formatted("%dm %ds", minutes, secs);
    } else {
        return juce::String::formatted("%ds", secs);
    }
}

juce::String LearningDashboard::formatPercentage(float percentage) const {
    return juce::String::formatted("%.2f%%", percentage * 100.0f);
}

juce::Colour LearningDashboard::getAccuracyColor(float accuracy) const {
    if (accuracy >= 0.9f) return juce::Colours::green;
    if (accuracy >= 0.7f) return juce::Colours::yellow;
    return juce::Colours::red;
}

juce::Colour LearningDashboard::getLossColor(float loss) const {
    if (loss <= 0.1f) return juce::Colours::green;
    if (loss <= 0.5f) return juce::Colours::yellow;
    return juce::Colours::red;
}

void LearningDashboard::notifyTrainingStarted(const juce::String& sessionId) {
    for (auto* listener : listeners) {
        listener->trainingStarted(sessionId);
    }
}

void LearningDashboard::notifyTrainingCompleted(const juce::String& sessionId, const LearningMetrics& finalMetrics) {
    for (auto* listener : listeners) {
        listener->trainingCompleted(sessionId, finalMetrics);
    }
}

void LearningDashboard::notifyTrainingError(const juce::String& sessionId, const juce::String& error) {
    for (auto* listener : listeners) {
        listener->trainingError(sessionId, error);
    }
}

void LearningDashboard::notifyModelActivated(const juce::String& modelName, const juce::String& version) {
    for (auto* listener : listeners) {
        listener->modelActivated(modelName, version);
    }
}

void LearningDashboard::notifyMetricsUpdated(const LearningMetrics& metrics) {
    for (auto* listener : listeners) {
        listener->metricsUpdated(metrics);
    }
}

// MetricsChart Implementation
MetricsChart::MetricsChart(ChartType type) : chartType(type) {
    setOpaque(true);
}

MetricsChart::~MetricsChart() = default;

void MetricsChart::addDataPoint(float x, float y) {
    dataPoints.push_back({x, y});
    
    // Keep only last 1000 points
    if (dataPoints.size() > 1000) {
        dataPoints.erase(dataPoints.begin());
    }
    
    repaint();
}

void MetricsChart::setDataPoints(const std::vector<std::pair<float, float>>& points) {
    dataPoints = points;
    repaint();
}

void MetricsChart::clearData() {
    dataPoints.clear();
    repaint();
}

void MetricsChart::setChartType(ChartType type) {
    chartType = type;
    repaint();
}

void MetricsChart::setLineColor(const juce::Colour& color) {
    lineColor = color;
    repaint();
}

void MetricsChart::setBackgroundColor(const juce::Colour& color) {
    backgroundColor = color;
    repaint();
}

void MetricsChart::setShowGrid(bool show) {
    showGrid = show;
    repaint();
}

void MetricsChart::setShowLabels(bool show) {
    showLabels = show;
    repaint();
}

void MetricsChart::setXRange(float min, float max) {
    xMin = min;
    xMax = max;
    repaint();
}

void MetricsChart::setYRange(float min, float max) {
    yMin = min;
    yMax = max;
    repaint();
}

void MetricsChart::setXLabel(const juce::String& label) {
    xLabel = label;
    repaint();
}

void MetricsChart::setYLabel(const juce::String& label) {
    yLabel = label;
    repaint();
}

void MetricsChart::paint(juce::Graphics& g) {
    g.fillAll(backgroundColor);
    
    if (showGrid) {
        drawGrid(g);
    }
    
    drawAxes(g);
    drawData(g);
    
    if (showLabels) {
        drawLabels(g);
    }
    
    // Draw tooltip on top of everything
    if (isHovering && hoveredPointIndex >= 0) {
        drawTooltip(g);
    }
}

void MetricsChart::resized() {
    // Chart will be redrawn on paint
}

void MetricsChart::drawGrid(juce::Graphics& g) {
    g.setColour(juce::Colours::grey.withAlpha(0.3f));
    
    auto bounds = getLocalBounds().reduced(40, 20);
    
    // Vertical grid lines
    for (int i = 0; i <= 10; ++i) {
        float x = bounds.getX() + (bounds.getWidth() * i) / 10.0f;
        g.drawVerticalLine(static_cast<int>(x), bounds.getY(), bounds.getBottom());
    }
    
    // Horizontal grid lines
    for (int i = 0; i <= 10; ++i) {
        float y = bounds.getY() + (bounds.getHeight() * i) / 10.0f;
        g.drawHorizontalLine(static_cast<int>(y), bounds.getX(), bounds.getRight());
    }
}

void MetricsChart::drawAxes(juce::Graphics& g) {
    g.setColour(juce::Colours::white);
    
    auto bounds = getLocalBounds().reduced(40, 20);
    
    // X axis
    g.drawHorizontalLine(bounds.getBottom(), bounds.getX(), bounds.getRight());
    
    // Y axis
    g.drawVerticalLine(bounds.getX(), bounds.getY(), bounds.getBottom());
}

void MetricsChart::drawData(juce::Graphics& g) {
    if (dataPoints.empty()) return;
    
    g.setColour(lineColor);
    
    auto bounds = getLocalBounds().reduced(40, 20);
    
    switch (chartType) {
        case ChartType::Line:
            {
                juce::Path path;
                bool started = false;
                
                for (const auto& point : dataPoints) {
                    float x = xToScreen(point.first);
                    float y = yToScreen(point.second);
                    
                    if (!started) {
                        path.startNewSubPath(x, y);
                        started = true;
                    } else {
                        path.lineTo(x, y);
                    }
                }
                
                g.strokePath(path, juce::PathStrokeType(2.0f));
            }
            break;
            
        case ChartType::Bar:
            {
                float barWidth = bounds.getWidth() / static_cast<float>(dataPoints.size());
                
                for (size_t i = 0; i < dataPoints.size(); ++i) {
                    float x = bounds.getX() + i * barWidth;
                    float y = yToScreen(dataPoints[i].second);
                    float height = bounds.getBottom() - y;
                    
                    g.fillRect(x, y, barWidth * 0.8f, height);
                }
            }
            break;
            
        case ChartType::Scatter:
            {
                for (const auto& point : dataPoints) {
                    float x = xToScreen(point.first);
                    float y = yToScreen(point.second);
                    
                    g.fillEllipse(x - 3, y - 3, 6, 6);
                }
            }
            break;
    }
}

void MetricsChart::drawLabels(juce::Graphics& g) {
    g.setColour(juce::Colours::white);
    g.setFont(12.0f);
    
    auto bounds = getLocalBounds().reduced(40, 20);
    
    // X label
    if (!xLabel.isEmpty()) {
        g.drawText(xLabel, bounds.getX(), bounds.getBottom() + 5, 
                   bounds.getWidth(), 15, juce::Justification::centred);
    }
    
    // Y label
    if (!yLabel.isEmpty()) {
        g.drawText(yLabel, 5, bounds.getY(), 30, bounds.getHeight(), 
                   juce::Justification::centred);
    }
}

float MetricsChart::xToScreen(float x) const {
    auto bounds = getLocalBounds().reduced(40, 20);
    float normalized = (x - xMin) / (xMax - xMin);
    return bounds.getX() + normalized * bounds.getWidth();
}

float MetricsChart::yToScreen(float y) const {
    auto bounds = getLocalBounds().reduced(40, 20);
    float normalized = (y - yMin) / (yMax - yMin);
    return bounds.getBottom() - normalized * bounds.getHeight();
}

void MetricsChart::mouseMove(const juce::MouseEvent& event) {
    mousePosition = event.position;
    int nearestPoint = findNearestPoint(mousePosition);
    
    if (nearestPoint != hoveredPointIndex) {
        hoveredPointIndex = nearestPoint;
        isHovering = (nearestPoint >= 0);
        repaint();
    } else if (isHovering) {
        // Still hovering, update position for tooltip
        repaint();
    }
}

void MetricsChart::mouseExit(const juce::MouseEvent& event) {
    isHovering = false;
    hoveredPointIndex = -1;
    repaint();
}

int MetricsChart::findNearestPoint(juce::Point<float> pos, float maxDistance) const {
    if (dataPoints.empty()) return -1;
    
    int nearestIndex = -1;
    float nearestDistanceSq = maxDistance * maxDistance;
    
    for (size_t i = 0; i < dataPoints.size(); ++i) {
        float screenX = xToScreen(dataPoints[i].first);
        float screenY = yToScreen(dataPoints[i].second);
        
        float dx = pos.x - screenX;
        float dy = pos.y - screenY;
        float distSq = dx * dx + dy * dy;
        
        if (distSq < nearestDistanceSq) {
            nearestDistanceSq = distSq;
            nearestIndex = static_cast<int>(i);
        }
    }
    
    return nearestIndex;
}

void MetricsChart::drawTooltip(juce::Graphics& g) {
    if (hoveredPointIndex < 0 || hoveredPointIndex >= static_cast<int>(dataPoints.size())) 
        return;
    
    const auto& point = dataPoints[hoveredPointIndex];
    float screenX = xToScreen(point.first);
    float screenY = yToScreen(point.second);
    
    // Draw highlight circle on the hovered point
    g.setColour(lineColor.brighter(0.5f));
    g.fillEllipse(screenX - 5, screenY - 5, 10, 10);
    g.setColour(juce::Colours::white);
    g.drawEllipse(screenX - 5, screenY - 5, 10, 10, 1.5f);
    
    // Format tooltip text
    juce::String tooltipText;
    if (!xLabel.isEmpty()) {
        tooltipText += xLabel + ": ";
    } else {
        tooltipText += "X: ";
    }
    tooltipText += juce::String(point.first, 2);
    tooltipText += "\n";
    if (!yLabel.isEmpty()) {
        tooltipText += yLabel + ": ";
    } else {
        tooltipText += "Y: ";
    }
    tooltipText += juce::String(point.second, 4);
    
    // Calculate tooltip dimensions
    juce::Font tooltipFont(12.0f);
    g.setFont(tooltipFont);
    
    float textWidth = tooltipFont.getStringWidthFloat(tooltipText.upToFirstOccurrenceOf("\n", false, false));
    float secondLineWidth = tooltipFont.getStringWidthFloat(tooltipText.fromLastOccurrenceOf("\n", false, false));
    textWidth = juce::jmax(textWidth, secondLineWidth);
    
    float tooltipWidth = textWidth + 16.0f;
    float tooltipHeight = 36.0f;
    
    // Position tooltip near cursor but avoid going off-screen
    float tooltipX = mousePosition.x + 12.0f;
    float tooltipY = mousePosition.y - tooltipHeight - 5.0f;
    
    auto bounds = getLocalBounds();
    if (tooltipX + tooltipWidth > bounds.getRight()) {
        tooltipX = mousePosition.x - tooltipWidth - 12.0f;
    }
    if (tooltipY < bounds.getY()) {
        tooltipY = mousePosition.y + 20.0f;
    }
    
    juce::Rectangle<float> tooltipBounds(tooltipX, tooltipY, tooltipWidth, tooltipHeight);
    
    // Draw tooltip background with glassmorphic style
    g.setColour(juce::Colour(0xE0202020));  // Semi-transparent dark background
    g.fillRoundedRectangle(tooltipBounds, 6.0f);
    
    // Draw subtle border
    g.setColour(lineColor.withAlpha(0.6f));
    g.drawRoundedRectangle(tooltipBounds, 6.0f, 1.0f);
    
    // Draw text
    g.setColour(juce::Colours::white);
    g.drawFittedText(tooltipText, tooltipBounds.reduced(8.0f, 4.0f).toNearestInt(),
                     juce::Justification::centredLeft, 2);
}

// ==================== Multi-Series API Implementation ====================

void MetricsChart::addSeries(const juce::String& name, const juce::Colour& color) {
    DataSeries newSeries;
    newSeries.name = name;
    newSeries.color = color;
    series_[name] = newSeries;
    repaint();
}

void MetricsChart::addDataPointToSeries(const juce::String& name, float x, float y) {
    auto it = series_.find(name);
    if (it != series_.end()) {
        it->second.points.push_back({x, y});
        
        // Keep only last 1000 points per series
        if (it->second.points.size() > 1000) {
            it->second.points.erase(it->second.points.begin());
        }
        
        // Auto-scale if enabled
        if (autoScale_) {
            calculateAutoScale();
        }
        
        repaint();
    }
}

void MetricsChart::setSeriesData(const juce::String& name, const std::vector<std::pair<float, float>>& points) {
    auto it = series_.find(name);
    if (it != series_.end()) {
        it->second.points = points;
        
        // Update animated points for smooth transition
        if (animated_) {
            it->second.animatedPoints = it->second.points;
        }
        
        if (autoScale_) {
            calculateAutoScale();
        }
        
        repaint();
    }
}

void MetricsChart::clearSeries(const juce::String& name) {
    auto it = series_.find(name);
    if (it != series_.end()) {
        it->second.points.clear();
        it->second.animatedPoints.clear();
        repaint();
    }
}

void MetricsChart::clearAllSeries() {
    series_.clear();
    repaint();
}

// ==================== Additional Appearance Methods ====================

void MetricsChart::setShowGlow(bool show) {
    showGlow_ = show;
    repaint();
}

void MetricsChart::setShowGradientFill(bool show) {
    showGradientFill_ = show;
    repaint();
}

void MetricsChart::setAnimated(bool animated) {
    animated_ = animated;
    if (animated) {
        startTimer(1000 / animationFPS_);
    } else {
        stopTimer();
    }
    repaint();
}

void MetricsChart::setAutoScale(bool autoScale) {
    autoScale_ = autoScale;
    if (autoScale) {
        calculateAutoScale();
    }
    repaint();
}

void MetricsChart::timerCallback() {
    updateAnimation();
    repaint();
}

// ==================== Legend Drawing ====================

void MetricsChart::drawLegend(juce::Graphics& g) {
    if (series_.empty()) return;
    
    auto chartArea = getChartArea();
    
    // Calculate legend dimensions
    const float legendPadding = 8.0f;
    const float swatchSize = 12.0f;
    const float rowHeight = 18.0f;
    const float textPadding = 6.0f;
    
    float maxTextWidth = 0.0f;
    juce::Font legendFont(11.0f);
    g.setFont(legendFont);
    
    for (const auto& [name, series] : series_) {
        float width = legendFont.getStringWidthFloat(name);
        maxTextWidth = juce::jmax(maxTextWidth, width);
    }
    
    float legendWidth = legendPadding * 2 + swatchSize + textPadding + maxTextWidth;
    float legendHeight = legendPadding * 2 + series_.size() * rowHeight;
    
    // Position legend in top-right corner of chart area
    float legendX = chartArea.getRight() - legendWidth - 10.0f;
    float legendY = chartArea.getY() + 10.0f;
    
    juce::Rectangle<float> legendBounds(legendX, legendY, legendWidth, legendHeight);
    
    // Draw glassmorphic legend background
    g.setColour(juce::Colour(0xD0151520));  // Semi-transparent dark
    g.fillRoundedRectangle(legendBounds, 6.0f);
    
    // Draw subtle border
    g.setColour(juce::Colour(0x30FFFFFF));
    g.drawRoundedRectangle(legendBounds, 6.0f, 1.0f);
    
    // Draw legend entries
    float currentY = legendY + legendPadding;
    for (const auto& [name, series] : series_) {
        // Draw color swatch
        juce::Rectangle<float> swatchBounds(legendX + legendPadding, 
                                            currentY + (rowHeight - swatchSize) / 2.0f,
                                            swatchSize, swatchSize);
        g.setColour(series.color);
        g.fillRoundedRectangle(swatchBounds, 2.0f);
        
        // Draw series name
        g.setColour(labelColor);
        g.drawText(name, 
                   swatchBounds.getRight() + textPadding,
                   currentY,
                   maxTextWidth,
                   rowHeight,
                   juce::Justification::centredLeft);
        
        currentY += rowHeight;
    }
}

// ==================== Animation Helpers ====================

void MetricsChart::startAnimation() {
    animationProgress_ = 0.0f;
    if (animated_) {
        startTimer(1000 / animationFPS_);
    }
}

void MetricsChart::updateAnimation() {
    if (animationProgress_ < 1.0f) {
        animationProgress_ += animationSpeed_;
        if (animationProgress_ > 1.0f) {
            animationProgress_ = 1.0f;
        }
        
        // Interpolate animated points
        float t = easeOutCubic(animationProgress_);
        
        // Interpolate primary data points
        if (previousPoints.size() == dataPoints.size()) {
            animatedPoints.resize(dataPoints.size());
            for (size_t i = 0; i < dataPoints.size(); ++i) {
                animatedPoints[i] = interpolatePoint(previousPoints[i], dataPoints[i], t);
            }
        } else {
            animatedPoints = dataPoints;
        }
        
        // Interpolate series points
        for (auto& [name, series] : series_) {
            if (series.animatedPoints.size() != series.points.size()) {
                series.animatedPoints = series.points;
            }
        }
    }
}

std::pair<float, float> MetricsChart::interpolatePoint(const std::pair<float, float>& from,
                                                        const std::pair<float, float>& to,
                                                        float t) const {
    return {
        from.first + (to.first - from.first) * t,
        from.second + (to.second - from.second) * t
    };
}

// ==================== Chart Area Helper ====================

juce::Rectangle<int> MetricsChart::getChartArea() const {
    return getLocalBounds().reduced(marginLeft_, marginTop_)
                          .withTrimmedRight(marginRight_ - marginLeft_)
                          .withTrimmedBottom(marginBottom_ - marginTop_);
}

// ==================== Auto-Scale Calculation ====================

void MetricsChart::calculateAutoScale() {
    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();
    
    bool hasData = false;
    
    // Check primary data points
    for (const auto& point : dataPoints) {
        minX = juce::jmin(minX, point.first);
        maxX = juce::jmax(maxX, point.first);
        minY = juce::jmin(minY, point.second);
        maxY = juce::jmax(maxY, point.second);
        hasData = true;
    }
    
    // Check all series
    for (const auto& [name, series] : series_) {
        for (const auto& point : series.points) {
            minX = juce::jmin(minX, point.first);
            maxX = juce::jmax(maxX, point.first);
            minY = juce::jmin(minY, point.second);
            maxY = juce::jmax(maxY, point.second);
            hasData = true;
        }
    }
    
    if (hasData) {
        // Add some padding to the range
        float xPadding = (maxX - minX) * 0.05f;
        float yPadding = (maxY - minY) * 0.1f;
        
        if (xPadding < 0.001f) xPadding = 1.0f;
        if (yPadding < 0.001f) yPadding = 0.1f;
        
        xMin = minX - xPadding;
        xMax = maxX + xPadding;
        yMin = juce::jmax(0.0f, minY - yPadding);  // Don't go below 0 for most metrics
        yMax = maxY + yPadding;
    }
}


// TrainingSessionListModel Implementation
TrainingSessionListModel::TrainingSessionListModel(const std::vector<TrainingSession>& sessions)
    : sessions(sessions) {
}

int TrainingSessionListModel::getNumRows() {
    return static_cast<int>(sessions.size());
}

void TrainingSessionListModel::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height) {
    if (rowNumber >= sessions.size()) return;
    
    const auto& session = sessions[rowNumber];
    juce::String text = formatSessionInfo(session);
    
    g.setColour(getStatusColor(session));
    g.setFont(12.0f);
    g.drawText(text, 5, 0, width - 10, height, juce::Justification::centredLeft, false);
}

juce::Component* TrainingSessionListModel::refreshComponentForRow(int rowNumber, bool isRowSelected, 
                                                                juce::Component* existingComponentToUpdate) {
    return existingComponentToUpdate;
}

void TrainingSessionListModel::setSessions(const std::vector<TrainingSession>& newSessions) {
    sessions = newSessions;
}

void TrainingSessionListModel::addSession(const TrainingSession& session) {
    sessions.push_back(session);
}

void TrainingSessionListModel::removeSession(int index) {
    if (index >= 0 && index < sessions.size()) {
        sessions.erase(sessions.begin() + index);
    }
}

juce::String TrainingSessionListModel::formatSessionInfo(const TrainingSession& session) const {
    return session.modelType + " - " + 
           juce::String::formatted("%.2f%%", session.finalAccuracy * 100.0f) + " - " +
           session.startTime.toString(true, true);
}

juce::Colour TrainingSessionListModel::getStatusColor(const TrainingSession& session) const {
    if (session.hasError) return juce::Colours::red;
    if (session.isCompleted) return juce::Colours::green;
    return juce::Colours::white;
}

// ModelListModel Implementation
ModelListModel::ModelListModel(const std::vector<ai::ProductionModelManager::ModelInfo>& models)
    : models(models) {
}

int ModelListModel::getNumRows() {
    return static_cast<int>(models.size());
}

void ModelListModel::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height) {
    if (rowNumber >= models.size()) return;
    
    const auto& model = models[rowNumber];
    juce::String text = formatModelInfo(model);
    
    g.setColour(getStatusColor(model));
    g.setFont(12.0f);
    g.drawText(text, 5, 0, width - 10, height, juce::Justification::centredLeft, false);
}

juce::Component* ModelListModel::refreshComponentForRow(int rowNumber, bool isRowSelected, 
                                                      juce::Component* existingComponentToUpdate) {
    return existingComponentToUpdate;
}

void ModelListModel::setModels(const std::vector<ai::ProductionModelManager::ModelInfo>& newModels) {
    models = newModels;
}

void ModelListModel::addModel(const ai::ProductionModelManager::ModelInfo& model) {
    models.push_back(model);
}

void ModelListModel::removeModel(int index) {
    if (index >= 0 && index < models.size()) {
        models.erase(models.begin() + index);
    }
}

juce::String ModelListModel::formatModelInfo(const ai::ProductionModelManager::ModelInfo& model) const {
    return model.name + " v" + model.version + " - " +
           juce::String::formatted("%.2f%%", model.accuracy * 100.0f) +
           (model.isActive ? " [Active]" : "");
}

juce::Colour ModelListModel::getStatusColor(const ai::ProductionModelManager::ModelInfo& model) const {
    if (!model.isHealthy) return juce::Colours::red;
    if (model.isActive) return juce::Colours::green;
    return juce::Colours::white;
}

// LearningDashboardFactory Implementation
std::unique_ptr<LearningDashboard> LearningDashboardFactory::createDefaultDashboard() {
    auto dashboard = std::make_unique<LearningDashboard>();
    setupDefaultCharts(*dashboard);
    setupDefaultMetrics(*dashboard);
    return dashboard;
}

std::unique_ptr<LearningDashboard> LearningDashboardFactory::createMinimalDashboard() {
    auto dashboard = std::make_unique<LearningDashboard>();
    // Minimal configuration
    return dashboard;
}

std::unique_ptr<LearningDashboard> LearningDashboardFactory::createAdvancedDashboard() {
    auto dashboard = std::make_unique<LearningDashboard>();
    setupDefaultCharts(*dashboard);
    setupDefaultMetrics(*dashboard);
    // Add advanced features
    return dashboard;
}

ai::TrainingConfig LearningDashboardFactory::getDefaultTrainingConfig() {
    ai::TrainingConfig config;
    config.epochs = 100;
    config.learningRate = 0.01f;
    config.batchSize = 32;
    config.earlyStopping = true;
    config.validationSplit = 0.2f;
    return config;
}

std::vector<juce::String> LearningDashboardFactory::getDefaultModelTypes() {
    return {"Genre Classifier", "Quality Predictor", "Creative Suggestion"};
}

void LearningDashboardFactory::setupDefaultCharts(LearningDashboard& dashboard) {
    // Configure default chart settings
}

void LearningDashboardFactory::setupDefaultMetrics(LearningDashboard& dashboard) {
    // Configure default metrics tracking
}

} // namespace ui
} // namespace zenith
