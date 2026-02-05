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

/*
    ==============================================================================
    Original file header:
*/

  ==============================================================================
    LearningDashboard.cpp
    Learning progress dashboard implementation
  ==============================================================================
*/


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
    // Use primary cyan for accuracy (positive metric)
    if (auto* chart = dynamic_cast<MetricsChart*>(accuracyChart.get())) {
        chart->setLineColor(zenith::design::colors::CYAN);
        chart->setYRange(0.0f, 1.0f);
        chart->setYLabel("Accuracy");
        chart->setXLabel("Epoch");
        chart->setShowGlow(true);
        chart->setShowGradientFill(true);
    }
    addAndMakeVisible(*accuracyChart);
}

void LearningDashboard::createLossChart() {
    lossChart = std::make_unique<MetricsChart>(MetricsChart::ChartType::Line);
    // Use neon pink/magenta for loss (metric to minimize)
    if (auto* chart = dynamic_cast<MetricsChart*>(lossChart.get())) {
        chart->setLineColor(zenith::design::colors::NEON_PINK);
        chart->setYRange(0.0f, 2.0f);
        chart->setYLabel("Loss");
        chart->setXLabel("Epoch");
        chart->setShowGlow(true);
        chart->setShowGradientFill(true);
    }
    addAndMakeVisible(*lossChart);
}

void LearningDashboard::createPerformanceChart() {
    performanceChart = std::make_unique<MetricsChart>(MetricsChart::ChartType::Bar);
    // Use violet for performance metrics
    if (auto* chart = dynamic_cast<MetricsChart*>(performanceChart.get())) {
        chart->setLineColor(zenith::design::colors::VIOLET);
        chart->setYLabel("Performance");
        chart->setShowGlow(true);
        chart->setShowGradientFill(true);
    }
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

// TrainingSessionListModel Implementation
TrainingSessionListModel::TrainingSessionListModel(const std::vector<TrainingSession>& sessions)
    : sessions(sessions) {
}

int TrainingSessionListModel::getNumRows() {
    return static_cast<int>(sessions.size());
}

void TrainingSessionListModel::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) {
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

void ModelListModel::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) {
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
