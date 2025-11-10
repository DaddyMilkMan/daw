/*
  ==============================================================================

    Main.cpp
    Created: 2025-11-10

    Entry point for Zenith DAW application.
    Initializes the JUCE application and creates the main window.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "MainWindow.h"
#include "Engine.h"

//==============================================================================
class ZenithDAWApplication : public juce::JUCEApplication
{
public:
    //==========================================================================
    ZenithDAWApplication() = default;

    const juce::String getApplicationName() override       { return ProjectInfo::projectName; }
    const juce::String getApplicationVersion() override    { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override             { return false; }

    //==========================================================================
    void initialise(const juce::String& commandLine) override
    {
        juce::ignoreUnused(commandLine);

        // Create the audio engine (owns all audio state)
        engine = std::make_unique<ZenithEngine>();

        // Initialize audio device manager
        // Default: stereo in/out, 44.1kHz, 512 samples (adjust for lower latency)
        auto error = engine->getDeviceManager().initialiseWithDefaultDevices(2, 2);

        if (error.isNotEmpty())
        {
            // Log error or show alert
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon,
                "Audio Device Error",
                "Could not initialize audio device: " + error
            );
        }

        // Create the main window
        mainWindow = std::make_unique<MainWindow>(getApplicationName(), *engine);
    }

    void shutdown() override
    {
        // Shutdown order is important:
        // 1. Close UI first (stops listening to state)
        mainWindow = nullptr;

        // 2. Stop audio engine (ensures audio callbacks stop)
        engine = nullptr;
    }

    //==========================================================================
    void systemRequestedQuit() override
    {
        // Save project if needed, then quit
        if (engine && engine->hasUnsavedChanges())
        {
            auto result = juce::AlertWindow::showYesNoCancelBox(
                juce::AlertWindow::QuestionIcon,
                "Unsaved Changes",
                "Do you want to save your project before quitting?",
                "Save", "Don't Save", "Cancel"
            );

            if (result == 0) // Cancel
                return;

            if (result == 1) // Save
            {
                // TODO: Implement save dialog
                // engine->saveProject(file);
            }
        }

        quit();
    }

    void anotherInstanceStarted(const juce::String& commandLine) override
    {
        juce::ignoreUnused(commandLine);
        // Handle opening files from command line if needed
    }

private:
    std::unique_ptr<ZenithEngine> engine;
    std::unique_ptr<MainWindow> mainWindow;
};

//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION(ZenithDAWApplication)
