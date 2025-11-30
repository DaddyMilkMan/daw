/*
  ==============================================================================

    MainLayoutComponent.h
    Created: 2025-11-28
    Author:  Dr. Aris Vokos + Leo Rossi + Isabella Moretti

    Tri-pane layout manager for Browser, Session View, and Arranger View.
    The heart of the "Perfect DAW" layout.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../../include/ArrangementComponent.h"
#include "../../include/ProjectState.h"
#include "../../include/Engine.h"

namespace zenith {

/**
 * @brief Main layout component managing Browser, Session, and Arranger views
 *
 * Layout structure:
 * [Browser (collapsible)] [Session/Arranger (toggleable)]
 */
class MainLayoutComponent : public juce::Component {
public:
    MainLayoutComponent(ProjectState& state, Engine& engine);
    ~MainLayoutComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // View management
    void toggleView();  // Toggle between Session and Arranger
    void toggleBrowser();  // Show/hide browser panel
    
    bool isSessionView() const { return showSessionView_; }
    bool isBrowserVisible() const { return browserVisible_; }

private:
    ProjectState& projectState_;
    Engine& engine_;

    // View state
    bool showSessionView_ = false;  // false = Arranger, true = Session
    bool browserVisible_ = true;

    // Layout constants
    static constexpr int browserWidth_ = 300;
    static constexpr int minCenterWidth_ = 400;

    // Components
    std::unique_ptr<ArrangementComponent> arrangerComponent_;
    // SessionViewComponent would go here when implemented

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainLayoutComponent)
};

} // namespace zenith
