# Export System UI Integration Guide

This document describes how to integrate the asynchronous export system with your UI.

## Quick Start

### Using AudioExporter (Recommended)

```cpp
#include "engine/AudioExporter.h"


// Setup options
zenith::ExportOptions options;
options.outputFile = juce::File("/path/to/output.wav");
options.sampleRate = 44100.0;
options.bitDepth = 24;
options.format = zenith::ExportFormat::WAV;

// Progress callback for UI updates
options.progressCallback = [this](float progress, const juce::String& status) {
    // Called on message thread - safe for UI updates
    myProgressBar.setValue(progress);
    myStatusLabel.setText(status, juce::dontSendNotification);
};

// Start async export (non-blocking)
engine.getAudioExporter().exportProjectAsync(
    options,
    [this](juce::Result result) {
        // Called on message thread when complete
        if (result.wasOk()) {
            juce::AlertWindow::showMessageBoxAsync(
                juce::MessageBoxIconType::InfoIcon,
                "Export Complete",
                "Your audio has been exported successfully.");
        } else {
            juce::AlertWindow::showMessageBoxAsync(
                juce::MessageBoxIconType::WarningIcon,
                "Export Failed",
                result.getErrorMessage());
        }
    }
);
```

---

## Integration with ThreadWithProgressWindow

For a modal progress dialog, use `juce::ThreadWithProgressWindow`:

```cpp
class ExportProgressWindow : public juce::ThreadWithProgressWindow {
public:
    ExportProgressWindow(zenith::Engine& engine, const zenith::ExportOptions& options)
        : juce::ThreadWithProgressWindow("Exporting Audio...", true, true),
          engine_(engine), options_(options) {}

    void run() override {
        // Create options with progress callback
        zenith::ExportOptions opts = options_;
        opts.progressCallback = [this](float progress, const juce::String& status) {
            setProgress(progress);
            setStatusMessage(status);
        };

        // Run blocking export (we're already on background thread)
        success_ = engine_.getAudioExporter().exportProject(opts);
    }

    void threadComplete(bool userPressedCancel) override {
        if (userPressedCancel) {
            engine_.getAudioExporter().cancelExport();
        } else if (success_) {
            // Handle success
        }
    }

private:
    zenith::Engine& engine_;
    zenith::ExportOptions options_;
    bool success_ = false;
};

// Usage:
ExportProgressWindow exportWindow(engine, options);
exportWindow.runThread(); // Modal, blocks until complete
```

---

## Cancellation

To cancel an export in progress:

```cpp
// Via AudioExporter
engine.getAudioExporter().cancelExport();

// Or check if exporting before starting new one
if (engine.getAudioExporter().isExporting()) {
    showMessage("An export is already in progress");
    return;
}
```

**Cancellation is responsive:** Export stops within **~100ms** (one block iteration).

---

## Error Handling

The completion callback receives a `juce::Result` with specific error messages:

| Error | Cause |
|-------|-------|
| "Invalid sample rate" | Sample rate ≤ 0 |
| "Invalid bit depth" | Not 8/16/24/32 |
| "Permission denied or disk full" | Cannot write to output |
| "Output directory does not exist" | Parent folder missing |
| "Export cancelled by user" | `cancelExport()` called |
| "Write error - disk may be full" | Write failed mid-export |
| "Export already in progress" | Called while exporting |

---

## Signal/Slot Pattern (Custom UI)

For reactive UI frameworks, use the callbacks as signals:

```cpp
class ExportController : public juce::ChangeBroadcaster {
public:
    void startExport(const zenith::ExportOptions& options) {
        zenith::ExportOptions opts = options;
        opts.progressCallback = [this](float progress, const juce::String& status) {
            currentProgress_ = progress;
            currentStatus_ = status;
            sendChangeMessage(); // Notify listeners
        };

        exporter_.exportProjectAsync(opts, [this](juce::Result result) {
            exportResult_ = result;
            isComplete_ = true;
            sendChangeMessage();
        });
    }

    float getProgress() const { return currentProgress_; }
    juce::String getStatus() const { return currentStatus_; }
    bool isComplete() const { return isComplete_; }
    juce::Result getResult() const { return exportResult_; }

private:
    zenith::AudioExporter& exporter_;
    std::atomic<float> currentProgress_{0.0f};
    juce::String currentStatus_;
    bool isComplete_ = false;
    juce::Result exportResult_;
};

// In your UI component:
class ExportPanel : public juce::Component, 
                    private juce::ChangeListener {
    void changeListenerCallback(juce::ChangeBroadcaster*) override {
        progressBar_.setValue(controller_.getProgress());
        if (controller_.isComplete()) {
            handleCompletion(controller_.getResult());
        }
    }
};
```
