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

#include "LayoutManager.h"
#include "ResizablePanelContainer.h"

// ... (keep existing includes if needed, or rely on factories)
#include "../../Source/ui/skia/BrowserPanel.h"
#include "../../Source/ui/skia/SkiaComponent.h"
#include "../../include/ProjectState.h"
#include "../../include/ui/ArrangerComponent.h"
#include "../browser/BrowserModel.h"
#include "RemoteCursorOverlay.h"
#include "SampleEditorComponent.h"
#include "SessionViewComponent.h"


class Engine; // Forward declaration

namespace zenith {

/**
 * @brief Main layout component managing flexible panes via
 * ResizablePanelContainer
 */
class MainLayoutComponent : public SkiaComponent {
public:
  explicit MainLayoutComponent(Engine &engine, ProjectState &state);
  ~MainLayoutComponent() override;

  void resized() override;
  void drawSkia(SkCanvas *canvas) override;

  // View management (Mapped to Layout Presets)
  void
  toggleView(); // Switch between Production (Arranger) and Mixing (Session)
  void toggleBrowser();      // Toggle Browser panel visibility
  void toggleSampleEditor(); // Toggle Sample Editor panel visibility

  bool isSessionView() const;
  bool isBrowserVisible() const;
  bool isSampleEditorVisible() const;

  // Accessors (finding panels dynamically)
  SampleEditorComponent *getSampleEditor();

private:
  Engine &engine_;
  ProjectState &projectState_;

  std::unique_ptr<ResizablePanelContainer> panelContainer_;
  std::unique_ptr<RemoteCursorOverlay> cursorOverlay_;

  // Persistent models (shared across panel re-creation)
  std::unique_ptr<BrowserModel> browserModel_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainLayoutComponent)
};

} // namespace zenith
