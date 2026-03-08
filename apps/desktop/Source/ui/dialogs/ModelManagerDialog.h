#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include "dsp/ModelManager.h"

namespace zenith {

/**
 * @brief Dialog for managing AI model downloads and installation
 */
class ModelManagerDialog : public juce::Component,
                          public juce::Button::Listener,
                          public juce::Timer {
public:
    ModelManagerDialog();
    ~ModelManagerDialog() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void buttonClicked(juce::Button* button) override;
    void timerCallback() override;

    void refreshModels();
    void downloadModel(const juce::String& modelName);
    void deleteModel(const juce::String& modelName);
    void setOnClose(std::function<void()> cb) { onClose_ = std::move(cb); }

private:
    struct ModelRow {
        juce::String name;
        juce::String description;  // Human-readable description
        juce::TextButton* downloadButton;
        juce::TextButton* deleteButton;
        juce::ProgressBar* progressBar;
        juce::Label* statusLabel;
        juce::Label* sizeLabel;
        juce::Label* descriptionLabel;  // Shows description text
        double progressValue = 0.0;  // Must outlive the progressBar
    };

    juce::Viewport viewport;
    juce::Component modelList;
    std::vector<ModelRow> modelRows;

    juce::TextButton closeButton{ "Close" };
    juce::TextButton refreshButton{ "Refresh" };  // Add refresh button
    juce::Label totalSizeLabel;
    juce::Label titleLabel;

    ModelManager modelManager;
    bool isDownloading = false;
    std::function<void()> onClose_;

    // Helper to get model description
    juce::String getModelDescription(const juce::String& modelName) const;

    void createModelRow(const ModelManager::ModelInfo& info, int yPosition);
    void updateModelDisplay();
    void updateProgress();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModelManagerDialog)
};

} // namespace zenith
