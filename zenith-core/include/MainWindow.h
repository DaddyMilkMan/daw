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

//==============================================================================
// Forward declarations
class ArrangerView;
class PianoRollEditor;

/**
 * @class MainComponent
 * @brief Main content component that holds the UI
 *
 * This component is the main content area and will contain:
 * - Transport bar
 * - Browser panel
 * - Arrangement view
 * - Mixer panel
 * - Wingman AI panel (Phase 2)
 */
class MainComponent : public juce::Component,
                      private juce::Timer
{
public:
    //==========================================================================
    MainComponent(Engine& engine, ProjectState& projectState);
    ~MainComponent() override;

    //==========================================================================
    // Component interface
    //==========================================================================

    void paint(juce::Graphics& g) override;
    void resized() override;

    //==========================================================================
    // UI actions
    //==========================================================================

    /**
     * @brief Open piano roll editor for a MIDI clip
     */
    void openPianoRoll(const juce::String& trackId, const juce::String& clipId);

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

    // UI Components
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

    // Arranger and Piano Roll
    std::unique_ptr<ArrangerView> arrangerView;
    std::unique_ptr<juce::DocumentWindow> pianoRollWindow;

    // Test buttons
    juce::TextButton addTrackButton;
    juce::TextButton testPianoRollButton;

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

    // Main content
    std::unique_ptr<MainComponent> mainComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
};
