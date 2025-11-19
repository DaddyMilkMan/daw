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
#include "MixerComponent.h"
#include "ArrangerView.h"
#include "ClipSynchronizer.h"

// Forward declarations
class ArrangerComponent;
class WingmanPanel;

namespace zenith {
    class InstrumentBrowserPanel;
}

namespace zenith {
    class CommandAPI;
    class AIBridgeClient;
}

//==============================================================================
/**
 * @class MainComponent
 * @brief Main content component that holds the UI
 *
 * This component is the main content area and contains:
 * - Transport bar (play/stop/record)
 * - ArrangerView (timeline with clips and automation)
 * - Status displays (CPU, device info, track count)
 * - Wingman AI panel (Phase 5+)
 *
 * Integration points:
 * - Hosts ArrangerView which displays ProjectState clips
 * - Opens PianoRollEditor when user double-clicks MIDI clip
 * - Provides "Show Automation" buttons per track
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

    // Phase 10: Mixer panel
    MixerComponent mixerComponent;

    // Phase 4: Arranger/Timeline view
    std::unique_ptr<ArrangerComponent> arrangerComponent;

    // Phase 5: Wingman command console
    std::unique_ptr<WingmanPanel> wingmanPanel;

    // Instrument & Preset Browser
    std::unique_ptr<zenith::InstrumentBrowserPanel> instrumentBrowserPanel;

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

private:
    //==========================================================================
    // Menu bar model
    //==========================================================================

    /**
     * @class ZenithMenuBar
     * @brief Menu bar model for the application
     */
    class ZenithMenuBar : public juce::MenuBarModel
    {
    public:
        explicit ZenithMenuBar(MainWindow& owner);

        juce::StringArray getMenuBarNames() override;
        juce::PopupMenu getMenuForIndex(int topLevelMenuIndex, const juce::String& menuName) override;
        void menuItemSelected(int menuItemID, int topLevelMenuIndex) override;

    private:
        MainWindow& owner;

        enum MenuItems
        {
            aboutZenith = 1,
            quit = 2
        };
    };

    //==========================================================================
    // Menu handlers
    //==========================================================================

    void showAboutDialog();

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

    // Integration: Clip synchronizer
    std::unique_ptr<ClipSynchronizer> clipSynchronizer;

    // Main content
    std::unique_ptr<MainComponent> mainComponent;

    // Menu bar
    std::unique_ptr<ZenithMenuBar> menuBar;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
};
