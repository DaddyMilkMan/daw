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

private:
    //==========================================================================
    // Setup methods
    //==========================================================================

    void setupCallbacks();

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

    // Layout constants
    static constexpr int topBarHeight = 48;
    static constexpr int transportBarHeight = 60;
    static constexpr int sidebarWidth = 250;
    static constexpr int mixerWidth = 0;  // Hidden by default, 300 when visible

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
