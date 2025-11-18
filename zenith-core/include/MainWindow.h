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
#include "ArrangerView.h"
#include "ClipSynchronizer.h"

//==============================================================================
/**
 * @class MainComponent
 * @brief Main content component that holds the UI
 *
 * This component is the main content area and contains:
 * - Transport bar (play/stop/record)
 * - ArrangerView (timeline with clips and automation)
 * - Status displays (CPU, device info, track count)
 *
 * Integration points:
 * - Hosts ArrangerView which displays ProjectState clips
 * - Opens PianoRollEditor when user double-clicks MIDI clip
 * - Provides "Show Automation" buttons per track
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
    // Integration: Piano roll opener
    //==========================================================================

    /**
     * @brief Open piano roll editor for a MIDI clip
     * @param trackId Track ID
     * @param clipId Clip ID
     */
    void openPianoRoll(const juce::String& trackId, const juce::String& clipId);

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

    // Phase 1: Import Audio button
    juce::TextButton importButton;

    // Audio device info
    juce::Label audioDeviceLabel;

    // C4: Track count label (read-only)
    juce::Label trackCountLabel;
    int lastTrackCount_ = -1;

    // Integration: ArrangerView
    std::unique_ptr<ArrangerView> arrangerView;

    // Integration: Show automation buttons (per track)
    std::map<juce::String, std::unique_ptr<juce::TextButton>> automationButtons;
    juce::Component automationButtonsContainer;

    //==========================================================================
    // Phase 1: Audio import
    //==========================================================================

    void handleImportAudio();

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

    // Integration: Clip synchronizer
    std::unique_ptr<ClipSynchronizer> clipSynchronizer;

    // Main content
    std::unique_ptr<MainComponent> mainComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
};
