#pragma once
#include "../../include/ProjectState.h"
#include "ArrangerComponent.h"
#include "skia/BrowserPanel.h"
#include "views/SessionViewComponent.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace zenith {

/**
 * @brief Main layout orchestrator for the "Perfect DAW" tri-pane architecture
 *
 * Manages:
 * - Left: Collapsible Browser Panel
 * - Center: Session View (clip launcher) OR Arranger View (timeline)
 * - Right: Inspector Panel (future)
 * - Top: Transport Bar (future)
 * - Bottom: Editor Panel (future)
 */
class MainLayoutComponent : public juce::Component {
public:
  MainLayoutComponent(ProjectState &ps) : arranger(ps) {
    // 1. Setup Components
    addAndMakeVisible(browserPanel);
    addAndMakeVisible(sessionView);
    addAndMakeVisible(arranger);

    // Show Session View by default to prove it works
    sessionView.setVisible(true);
    arranger.setVisible(false);

    // Set opaque for better performance
    setOpaque(true);
  }

  void paint(juce::Graphics &g) override {
    // Failsafe: If Skia fails, you will see this dark grey background
    g.fillAll(juce::Colours::darkgrey);
  }

  void resized() override {
    auto area = getLocalBounds();

    // --- Tri-Pane Layout Logic ---

    // 1. Top Bar (Transport) - Fixed Height
    auto topBarArea = area.removeFromTop(56);
    // transportBar.setBounds(topBarArea); // TODO: Add transport bar

    // 2. Bottom Editor - Resizable (Placeholder for now)
    auto bottomEditorArea =
        area.removeFromBottom(250); // Fixed for now, make resizable later

    // 3. Left Browser - Collapsible
    if (!browserCollapsed) {
      browserPanel.setBounds(area.removeFromLeft(260));
    } else {
      browserPanel.setBounds(area.removeFromLeft(48)); // Icon width
    }

    // 4. Right Inspector - Collapsible
    auto inspectorArea = area.removeFromRight(280);
    // inspectorPanel.setBounds(inspectorArea); // TODO: Add inspector

    // 5. Central Workspace (Fills remaining)
    if (sessionView.isVisible())
      sessionView.setBounds(area);
    else
      arranger.setBounds(area);
  }

  void toggleBrowser() {
    browserCollapsed = !browserCollapsed;
    browserPanel.setCollapsed(browserCollapsed);
    resized();
  }

  void toggleView() {
    bool isSession = sessionView.isVisible();
    sessionView.setVisible(!isSession);
    arranger.setVisible(isSession);
    resized();
  }

private:
  BrowserPanel browserPanel;
  SessionViewComponent sessionView;
  ArrangerComponent arranger;

  bool browserCollapsed = false;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainLayoutComponent)
};
} // namespace zenith
