#pragma once
#include "../../include/ProjectState.h"
#include "ArrangerComponent.h"
#include "skia/BrowserPanel.h"
#include "skia/SkiaComponent.h"
#include "views/SessionViewComponent.h"
#include <juce_gui_basics/juce_gui_basics.h>

#ifdef ZENITH_USE_SKIA
#include <include/core/SkCanvas.h>
#include <include/core/SkPaint.h>
#include <include/core/SkRect.h>
#endif

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
class MainLayoutComponent : public SkiaComponent {
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
    setOpaque(false);
  }

#ifdef ZENITH_USE_SKIA
  void drawSkia(SkCanvas *canvas) override {
    if (!canvas) return;
    
    auto bounds = getLocalBounds();
    
    // Draw dark background
    SkPaint bgPaint;
    bgPaint.setColor(SkColorSetRGB(30, 30, 35));
    bgPaint.setAntiAlias(true);
    canvas->drawRect(SkRect::MakeWH(static_cast<float>(bounds.getWidth()), 
                                     static_cast<float>(bounds.getHeight())), bgPaint);
    
    // Render all visible children
    for (auto *child : getChildren()) {
      if (!child->isVisible())
        continue;

      auto childBounds = child->getBounds();

      canvas->save();
      canvas->translate(static_cast<float>(childBounds.getX()), 
                        static_cast<float>(childBounds.getY()));
      canvas->clipRect(SkRect::MakeWH(static_cast<float>(childBounds.getWidth()), 
                                       static_cast<float>(childBounds.getHeight())));

      if (auto *skiaChild = dynamic_cast<SkiaComponent *>(child)) {
        skiaChild->drawSkia(canvas);
      }

      canvas->restore();
    }
  }
#endif

#ifndef ZENITH_USE_SKIA
  void paint(juce::Graphics &g) override {
    // Failsafe: If Skia fails, you will see this dark grey background
    g.fillAll(juce::Colours::darkgrey);
  }
#endif

  void resized() override {
    auto area = getLocalBounds();

    // --- Tri-Pane Layout Logic ---

    // 1. Left Browser - Collapsible
    if (!browserCollapsed) {
      browserPanel.setBounds(area.removeFromLeft(260));
    } else {
      browserPanel.setBounds(area.removeFromLeft(48)); // Icon width
    }

    // 2. Central Workspace (Fills remaining)
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
