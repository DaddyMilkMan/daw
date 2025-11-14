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

// Forward declarations
class ArrangerComponent;
class WingmanPanel;

namespace zenith {
    class CommandAPI;
    class AIBridgeClient;
}

//==============================================================================
/**
 * @class MainComponent
 * @brief Main content component that holds the UI
 *
 * This component is the main content area and will contain:
 * - Transport bar
 * - Browser panel
 * - Arrangement view (Phase 4)
 * - Mixer panel
 * - Wingman AI panel (Phase 5+)
 */
class MainComponent : public juce::Component,
                      private juce::Timer,
                      public juce::KeyListener
{
public:
    //==========================================================================
    MainComponent(Engine& engine, zenith::CommandAPI& api, zenith::AIBridgeClient& aiClient, ProjectState& state);
    ~MainComponent() override;

    //==========================================================================
    // Component interface
    //==========================================================================

    void paint(juce::Graphics& g) override;
    void resized() override;

    //==========================================================================
    // KeyListener interface (for undo/redo shortcuts)
    //==========================================================================

    bool keyPressed(const juce::KeyPress& key, Component* originatingComponent) override;

private:
    //==========================================================================
    // Timer interface (for status updates)
    //==========================================================================

    void timerCallback() override;

    //==========================================================================
    // C4: Track count monitoring (read-only, dirty-checked)
    //==========================================================================

    void refreshTrackCountLabel();

    //==========================================================================
    // Member variables
    //==========================================================================

    Engine& engine;
    ProjectState& projectState;

    // UI Components (will add more in Phase 1)
    juce::Label statusLabel;
    juce::Label cpuLabel;
    juce::TextButton playButton;
    juce::TextButton stopButton;
    juce::TextButton recordButton;

    // Audio device info
    juce::Label audioDeviceLabel;

    // C4: Track count label (read-only)
    juce::Label trackCountLabel;
    int lastTrackCount_ = -1;

    // Phase 4: Arranger/Timeline view
    std::unique_ptr<ArrangerComponent> arrangerComponent;

    // Phase 5: Wingman command console
    std::unique_ptr<WingmanPanel> wingmanPanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

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
    // Menu bar
    //==========================================================================

    /**
     * @brief Creates the menu bar
     */
    std::unique_ptr<juce::MenuBarModel> createMenuBar();

private:
    //==========================================================================
    // Member variables
    //==========================================================================

    // Audio engine (created first, destroyed last)
    std::unique_ptr<Engine> engine;

    // Project state
    std::unique_ptr<ProjectState> projectState;

    // Phase 5: Wingman command API
    std::unique_ptr<zenith::CommandAPI> commandAPI;

    // Phase 7: AI bridge client
    std::unique_ptr<zenith::AIBridgeClient> aiBridgeClient;

    // Main content
    std::unique_ptr<MainComponent> mainComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
};
