#include "ModelManagerDialog.h"
#include "../controls/SkiaAlertWindow.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace zenith {

//==============================================================================
// ModelManagerDialog Implementation
//==============================================================================

juce::String ModelManagerDialog::getModelDescription(const juce::String& modelName) const {
    if (modelName == "htdemucs") {
        return "6 stems (vocals, drums, bass, other, piano, guitar) - Highest quality";
    } else if (modelName == "htdemucs_light") {
        return "6 stems - Faster processing, good quality";
    } else if (modelName == "demucs") {
        return "4 stems (vocals, drums, bass, other) - Baseline model";
    } else if (modelName == "demucs_extra") {
        return "4 stems - Trained on diverse data";
    } else if (modelName == "demucs_hybrid") {
        return "4 stems - Transformer-based";
    }
    return "AI model for stem separation";
}

ModelManagerDialog::ModelManagerDialog() {
    setSize(700, 500);  // Larger for more models

    // Title label
    titleLabel.setText("AI Models for Stem Separation", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(18.0f).withStyle("Bold"));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(titleLabel);

    // Refresh button
    refreshButton.addListener(this);
    addAndMakeVisible(refreshButton);

    // Close button
    closeButton.addListener(this);
    addAndMakeVisible(closeButton);

    // Viewport for scrollable model list
    viewport.setViewedComponent(&modelList, false);
    viewport.setScrollBarsShown(true, false);
    addAndMakeVisible(viewport);

    // Total size label
    totalSizeLabel.setText("Total: " + juce::File::descriptionOfSizeInBytes(modelManager.getTotalModelSize()),
                          juce::dontSendNotification);
    addAndMakeVisible(totalSizeLabel);

    // Load models
    refreshModels();

    // Start timer for progress updates
    startTimerHz(10);
}

ModelManagerDialog::~ModelManagerDialog() {
    stopTimer();
}

void ModelManagerDialog::paint(juce::Graphics& g) {
    g.fillAll(juce::Colours::darkgrey);

    g.setColour(juce::Colours::white);
    g.setFont(14.0f);
    g.drawText("Download AI models to separate audio into stems.",
              getLocalBounds().removeFromTop(70).withTrimmedTop(45).reduced(20, 5),
              juce::Justification::centredLeft);
}

void ModelManagerDialog::resized() {
    auto area = getLocalBounds().reduced(10);

    // Title at top
    titleLabel.setBounds(area.removeFromTop(30));

    // Button row
    auto buttonRow = area.removeFromTop(35);
    refreshButton.setBounds(buttonRow.removeFromLeft(100));
    buttonRow.removeFromLeft(10);
    closeButton.setBounds(buttonRow.removeFromRight(80));

    // Total size label
    totalSizeLabel.setBounds(area.removeFromBottom(25));

    // Viewport
    auto viewportArea = area.withTrimmedTop(10);
    viewport.setBounds(viewportArea);
    modelList.setBounds(0, 0, viewport.getViewWidth(),
                       std::max(viewport.getViewHeight(), modelList.getHeight()));
}

void ModelManagerDialog::buttonClicked(juce::Button* button) {
    if (button == &closeButton) {
        setVisible(false);
        if (onClose_) onClose_();
    } else if (button == &refreshButton) {
        // Refresh model status
        refreshModels();
    }
}

void ModelManagerDialog::timerCallback() {
    updateProgress();
}

void ModelManagerDialog::refreshModels() {
    modelManager.refreshModelStatus();
    updateModelDisplay();
}

void ModelManagerDialog::downloadModel(const juce::String& modelName) {
    if (isDownloading) {
        return;  // Already downloading
    }

    isDownloading = true;

    modelManager.downloadModel(modelName,
        [this](int64_t downloaded, int64_t total) {
            // Progress updates handled in timerCallback
        },
        [this](bool success, const juce::String& message) {
            isDownloading = false;
            refreshModels();

            // Show result
            SkiaAlertWindow::showMessageBoxAsync(
                success ? SkiaAlertWindow::IconType::InfoIcon : SkiaAlertWindow::IconType::WarningIcon,
                juce::String(success ? "Download " : "Download ") + (success ? "Complete" : "Failed"),
                message,
                "OK");
        }
    );
}

void ModelManagerDialog::deleteModel(const juce::String& modelName) {
    SkiaAlertWindow::showAsync(
        SkiaAlertWindow::IconType::WarningIcon,
        "Delete Model",
        "Are you sure you want to delete " + modelName + "?",
        "Delete",
        "Cancel",
        {},
        [this, modelName](int result) {
          if (result != static_cast<int>(SkiaAlertWindow::Result::Button1)) {
            return;
          }
          if (modelManager.deleteModel(modelName)) {
            refreshModels();
          } else {
            SkiaAlertWindow::showMessageBoxAsync(
                SkiaAlertWindow::IconType::WarningIcon,
                "Delete Failed",
                "Failed to delete model",
                "OK");
          }
        });
}

void ModelManagerDialog::createModelRow(const ModelManager::ModelInfo& info, int yPosition) {
    ModelRow row;
    row.name = info.name;
    row.description = getModelDescription(info.name);

    const int rowHeight = 95;  // Taller for description
    const int padding = 10;
    auto rowBounds = juce::Rectangle<int>(padding, yPosition, getWidth() - padding * 3, rowHeight);

    // Model name label (bold, larger)
    auto nameLabel = std::make_unique<juce::Label>();
    nameLabel->setText(info.name, juce::dontSendNotification);
    nameLabel->setFont(juce::FontOptions(15.0f).withStyle("Bold"));
    nameLabel->setColour(juce::Label::textColourId, juce::Colours::white);
    nameLabel->setBounds(rowBounds.removeFromTop(22));
    modelList.addAndMakeVisible(*nameLabel);
    nameLabel.release();  // modelList owns it now

    // Description label (smaller, gray)
    row.descriptionLabel = new juce::Label();
    row.descriptionLabel->setText(row.description, juce::dontSendNotification);
    row.descriptionLabel->setFont(juce::FontOptions(11.0f));
    row.descriptionLabel->setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    row.descriptionLabel->setBounds(rowBounds.removeFromTop(18));
    modelList.addAndMakeVisible(*row.descriptionLabel);

    // Size label
    row.sizeLabel = new juce::Label();
    juce::String sizeText = info.sizeBytes > 0 ?
                              juce::File::descriptionOfSizeInBytes(info.sizeBytes) :
                              "Size: Unknown";
    row.sizeLabel->setText(sizeText, juce::dontSendNotification);
    row.sizeLabel->setFont(juce::FontOptions(11.0f));
    row.sizeLabel->setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    row.sizeLabel->setBounds(rowBounds.removeFromLeft(90));
    modelList.addAndMakeVisible(*row.sizeLabel);

    // Status label
    row.statusLabel = new juce::Label();
    row.statusLabel->setText(info.isInstalled ? "✓ Installed" : "Not installed",
                            juce::dontSendNotification);
    row.statusLabel->setFont(juce::FontOptions(11.0f));
    row.statusLabel->setColour(juce::Label::textColourId,
                              info.isInstalled ? juce::Colours::green : juce::Colours::orange);
    row.statusLabel->setBounds(rowBounds.removeFromLeft(90));
    modelList.addAndMakeVisible(*row.statusLabel);

    // Progress bar (requires reference to progress value)
    row.progressBar = new juce::ProgressBar(row.progressValue);
    row.progressBar->setBounds(rowBounds.removeFromRight(150).reduced(0, 10));
    row.progressBar->setVisible(false);
    modelList.addAndMakeVisible(*row.progressBar);

    // Buttons
    if (info.isInstalled) {
        row.deleteButton = new juce::TextButton("Delete");
        row.deleteButton->onClick = [this, name = info.name]() { deleteModel(name); };
        row.deleteButton->setBounds(rowBounds.removeFromRight(80).reduced(0, 15));
        modelList.addAndMakeVisible(*row.deleteButton);
    } else {
        row.downloadButton = new juce::TextButton("Download");
        row.downloadButton->onClick = [this, name = info.name]() { downloadModel(name); };
        row.downloadButton->setEnabled(!isDownloading);
        row.downloadButton->setBounds(rowBounds.removeFromRight(100).reduced(0, 15));
        modelList.addAndMakeVisible(*row.downloadButton);
    }

    modelRows.push_back(std::move(row));
}

void ModelManagerDialog::updateModelDisplay() {
    // Clear existing
    modelList.removeAllChildren();
    modelRows.clear();

    // Add models
    auto models = modelManager.getAvailableModels();
    int yPosition = 10;

    for (const auto& model : models) {
        createModelRow(model, yPosition);
        yPosition += 105;  // Row height (95) + padding (10)
    }

    // Update model list size
    modelList.setSize(getWidth() - 20, yPosition + 10);

    // Update total size
    totalSizeLabel.setText("Total: " + juce::File::descriptionOfSizeInBytes(modelManager.getTotalModelSize()),
                          juce::dontSendNotification);
}

void ModelManagerDialog::updateProgress() {
    if (!isDownloading) {
        return;
    }

    double progress = modelManager.getDownloadProgress();

    for (auto& row : modelRows) {
        if (row.progressBar) {
            row.progressBar->setVisible(true);
            row.progressValue = progress;  // Update the value that progressBar references
        }

        if (row.downloadButton) {
            row.downloadButton->setEnabled(false);
            row.downloadButton->setButtonText("Downloading...");
        }

        if (row.statusLabel) {
            int percentage = static_cast<int>(progress * 100);
            row.statusLabel->setText("Downloading: " + juce::String(percentage) + "%",
                                    juce::dontSendNotification);
        }
    }
}

} // namespace zenith
