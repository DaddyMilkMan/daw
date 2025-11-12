/**
 * @file MainWindow.h
 * @brief Main application window for Zenith DAW
 *
 * Contains the main UI layout and hosts the audio engine.
 *
 * Phase 0: Foundation
 * - Basic window management
 * - Menu bar
 * - Status bar
 * - Audio engine integration
 */

#pragma once

#include <JuceHeader.h>
#include "Engine.h"
#include "ProjectState.h"

// Forward declare the new UI MainComponent (defined in Source/ui/MainComponent.h)
class MainComponent;

//==============================================================================
/**
 * @class MainWindow
 * @brief Top-level application window
 *
 * Manages:
 * - Window lifecycle
 * - Menu bar
 * - Main content component
 * - Audio engine
 * - Project state
 */
class MainWindow : public juce::DocumentWindow
{
public:
    //==========================================================================
    explicit MainWindow(const juce::String& name);
    ~MainWindow() override;

    //==========================================================================
    // DocumentWindow interface
    //==========================================================================

    void closeButtonPressed() override;

    //==========================================================================
    // Accessors (for menu bar access to main component)
    //==========================================================================

    ::MainComponent* getMainComponent() { return mainComponent.get(); }

    //==========================================================================
    // Menu bar
    //==========================================================================

    class MenuBar; // Forward declare inner class

private:
    //==========================================================================
    // Member variables
    //==========================================================================

    // Audio engine (created first, destroyed last)
    std::unique_ptr<Engine> engine;

    // Project state
    std::unique_ptr<ProjectState> projectState;

    // Main content (custom JUCE UI defined in Source/ui/MainComponent.h)
    std::unique_ptr<::MainComponent> mainComponent;

    // Menu bar model
    std::unique_ptr<MenuBar> menuBar;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
};
