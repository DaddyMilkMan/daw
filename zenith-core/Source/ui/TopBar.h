/**
 * @file TopBar.h
 * @brief Top bar component for Zenith DAW
 *
 * Replaces React components:
 * - vexel-daw/src/renderer/components/TopBar.tsx (partially)
 * - vexel-daw/src/renderer/components/Settings.tsx (settings icon)
 *
 * Features:
 * - Application branding/logo
 * - Project title/name (editable)
 * - Settings button
 * - Wingman AI toggle (Phase 2)
 * - System menu button
 */

#pragma once

#include <JuceHeader.h>
#include "ZenithLookAndFeel.h"

//==============================================================================
/**
 * @class TopBar
 * @brief Top application bar with branding, project info, and controls
 *
 * Layout:
 * [Logo] [Project Name] ________________ [AI] [Settings] [Menu]
 */
class TopBar : public juce::Component
{
public:
    //==========================================================================
    /**
     * @brief Callback when settings button clicked
     */
    std::function<void()> onSettingsClicked;

    /**
     * @brief Callback when project name changed
     */
    std::function<void(const juce::String&)> onProjectNameChanged;

    /**
     * @brief Callback when AI toggle changed
     */
    std::function<void(bool)> onAIToggleChanged;

    //==========================================================================
    TopBar();
    ~TopBar() override = default;

    //==========================================================================
    // Component interface
    //==========================================================================

    void paint(juce::Graphics& g) override;
    void resized() override;

    //==========================================================================
    // Public API
    //==========================================================================

    /**
     * @brief Set the project name
     */
    void setProjectName(const juce::String& name);

    /**
     * @brief Get current project name
     */
    juce::String getProjectName() const;

    /**
     * @brief Set AI enabled state
     */
    void setAIEnabled(bool enabled);

private:
    //==========================================================================
    // Member variables
    //==========================================================================

    // Logo/branding
    juce::Label logoLabel;

    // Project name (editable)
    juce::Label projectNameLabel;

    // Controls
    juce::TextButton aiToggleButton;
    juce::TextButton settingsButton;
    juce::TextButton menuButton;

    // Constants
    static constexpr int height = 48;
    static constexpr int buttonSize = 32;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TopBar)
};
