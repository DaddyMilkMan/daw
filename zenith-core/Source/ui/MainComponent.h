/**
 * @file MainComponent.h
 * @brief Main UI component that wires all UI panels together
 *
 * Replaces React component hierarchy:
 * - vexel-daw/src/renderer/App.tsx (root layout)
 *
 * This is the root component that manages the complete UI layout:
 * +----------------------------------------------------------+
 * |                        TopBar                            |
 * +----------+-------------------------------------------+---+
 * |          |                                           | M |
 * | Sidebar  |           TrackView                       | i |
 * |          |         (Arrangement)                     | x |
 * |          |                                           | e |
 * |          |                                           | r |
 * +----------+-------------------------------------------+---+
 * |                     TransportBar                         |
 * +----------------------------------------------------------+
 */

#pragma once

#include <JuceHeader.h>
#include "../../include/Engine.h"
#include "../../include/editor/ProjectEditorState.h"
#include "../render/ExportWavJob.h"
#include "ZenithLookAndFeel.h"
#include "TopBar.h"
#include "Sidebar.h"
#include "ArrangerComponent.h"
#include "TransportBar.h"

#ifdef _WIN32
    #include "AudioSettingsWindows.h"
#endif

#if JUCE_DEBUG
    #include "StatsOverlay.h"
    #if defined(ZENITH_ENABLE_PHASE1_AUDIO) && ZENITH_ENABLE_PHASE1_AUDIO
        #include "Phase1DebugOverlay.h"
    #endif
#endif

//==============================================================================
/**
 * @class MainComponent
 * @brief Root UI component managing entire DAW interface
 *
 * Responsibilities:
 * - Layout management for all panels
 * - Coordinating communication between panels
 * - Managing global UI state
 * - Applying custom LookAndFeel
 */
class MainComponent : public juce::Component
{
public:
    //==========================================================================
    explicit MainComponent(Engine& engine);
    ~MainComponent() override;

    //==========================================================================
    // Component interface
    //==========================================================================

    void paint(juce::Graphics& g) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress& key) override;

    #if JUCE_DEBUG
        /**
         * @brief W6.1: Get stats overlay for menu bar access
         */
        StatsOverlay* getStatsOverlay() { return statsOverlay.get(); }

        /**
         * @brief W6.1: Toggle stats overlay (public for menu bar)
         */
        void toggleStatsOverlay();
    #endif

private:
    //==========================================================================
    // Setup methods
    //==========================================================================

    void setupCallbacks();

    /**
     * @brief W5: Inject test session data (100 tracks × 50 clips)
     */
    void injectTestSessionData();

    //==========================================================================
    // Settings panel management
    //==========================================================================

    /**
     * @brief Show/hide audio settings panel
     */
    void toggleAudioSettings();

    #if JUCE_DEBUG
        /**
         * @brief W6: Update stats overlay with TrackView paint timing
         */
        void updateStatsOverlay();
    #endif

    //==========================================================================
    // Audio Import
    //==========================================================================

    /**
     * @brief Start audio file import (File → Import Audio...)
     */
    void startImportAudio();

    /**
     * @brief Handle audio file import after file chooser
     * @param file Audio file to import
     */
    void handleImportAudioFile(const juce::File& file);

    //==========================================================================
    // Export management
    //==========================================================================

    /**
     * @brief Start WAV export process (File → Export WAV)
     */
    void startExportWav();

    /**
     * @brief Handle export completion (success or failure)
     * @param ok True if export succeeded
     * @param message Error message (if failed)
     * @param file Output file path
     */
    void onExportFinished(bool ok, const juce::String& message, juce::File file);

    /**
     * @brief Poll export job for completion (called from timer)
     * @param file Output file being exported
     */
    void pollExportCompletion(juce::File file);

    //==========================================================================
    // Member variables
    //==========================================================================

    Engine& engine;
    zenith::ProjectEditorState editorState;

    // Custom LookAndFeel
    ZenithLookAndFeel zenithLookAndFeel;

    // UI Components (order matches visual hierarchy)
    TopBar topBar;
    Sidebar sidebar;
    ArrangerComponent arrangerComponent;
    TransportBar transportBar;

    #ifdef _WIN32
        // Audio settings panel (Windows-specific, shown as overlay)
        std::unique_ptr<AudioSettingsWindows> audioSettingsPanel;
    #endif

    #if JUCE_DEBUG
        // W6: Performance monitoring overlay (DEBUG-only)
        std::unique_ptr<StatsOverlay> statsOverlay;

        #if defined(ZENITH_ENABLE_PHASE1_AUDIO) && ZENITH_ENABLE_PHASE1_AUDIO
            // Phase 1: Real-time engine debug HUD (F12 toggle)
            std::unique_ptr<Phase1DebugOverlay> phase1DebugOverlay;
        #endif

        // W6.1: Persistent settings for Debug HUD
        juce::ApplicationProperties appProperties;
    #endif

    // Export management
    std::unique_ptr<ExportWavJob> exportJob_;
    bool isExporting_ = false;

    // Layout constants
    static constexpr int topBarHeight = 48;
    static constexpr int transportBarHeight = 60;
    static constexpr int sidebarWidth = 250;
    static constexpr int mixerWidth = 0;  // Hidden by default, 300 when visible

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
