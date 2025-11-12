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
#include "ZenithLookAndFeel.h"
#include "TopBar.h"
#include "Sidebar.h"
#include "TrackView.h"
#include "TransportBar.h"

#ifdef _WIN32
    #include "AudioSettingsWindows.h"
#endif

#if JUCE_DEBUG
    #include "StatsOverlay.h"
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

    /**
     * @brief W8: Get application properties for menu bar access (crash reporting settings)
     */
    juce::ApplicationProperties& getAppProperties() { return appProperties; }

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
    // Member variables
    //==========================================================================

    Engine& engine;

    // Custom LookAndFeel
    ZenithLookAndFeel zenithLookAndFeel;

    // UI Components (order matches visual hierarchy)
    TopBar topBar;
    Sidebar sidebar;
    TrackView trackView;
    TransportBar transportBar;

    #ifdef _WIN32
        // Audio settings panel (Windows-specific, shown as overlay)
        std::unique_ptr<AudioSettingsWindows> audioSettingsPanel;
    #endif

    #if JUCE_DEBUG
        // W6: Performance monitoring overlay (DEBUG-only)
        std::unique_ptr<StatsOverlay> statsOverlay;
    #endif

    // W6.1 & W8: Persistent settings (HUD visibility, crash reporting, etc.)
    juce::ApplicationProperties appProperties;

    // Layout constants
    static constexpr int topBarHeight = 48;
    static constexpr int transportBarHeight = 60;
    static constexpr int sidebarWidth = 250;
    static constexpr int mixerWidth = 0;  // Hidden by default, 300 when visible

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
