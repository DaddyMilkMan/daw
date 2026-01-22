/*
  ==============================================================================

    MainLayoutComponent.cpp
    Created: 2025-11-28
    Author:  Dr. Aris Vokos + Leo Rossi + Isabella Moretti

  ==============================================================================
*/

#include "MainLayoutComponent.h"
#include "../../browser/BrowserModel.h"
#include "../../engine/Engine.h"
#include "../../engine/PluginHost.h"
#include "../../engine/ZenithLogger.h"
#include "../../instruments/InstrumentRegistry.h"
#include "../arranger/ArrangerComponent.h"
#include "../arranger/ArrangerClipManager.h"
#include "../browser/BrowserPanel.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../framework/GlassmorphicPanel.h"
#include "../framework/LayoutManager.h"
#include "../framework/SkiaMainWindowIntegration.h"
#include "../piano-roll/PianoRollComponent.h"
#include "../sample-editor/SampleEditorComponent.h"
#include "../views/SessionViewComponent.h"
#include "../ui/common/RemoteCursorOverlay.h"
#include <memory>
#include <utility>
#include <vector>

#include "../ui/piano-roll/PianoRollComponent.h"
#include "ResizablePanelContainer.h"
#include "HelpViewPanel.h"
#include "RightSidePanel.h"

namespace zenith {

// Helper class to switch between Arranger and Session views while keeping them
// alive
class ViewSwitcher : public SkiaComponent {
public:
  ViewSwitcher() { setOpaque(false); }

  void addView(std::unique_ptr<juce::Component> view) {
    views_.push_back(std::move(view));
    addChildComponent(views_.back().get());
    if (views_.size() == 1) {
      activeIndex_ = 0;
      views_[0]->setVisible(true);
    }
  }

  void setActiveView(int index) {
    if (index < 0 || index >= static_cast<int>(views_.size()))
      return;
    activeIndex_ = index;
    resized();
  }

  int getActiveViewIndex() const { return activeIndex_; }

  void resized() override {
    auto bounds = getLocalBounds();
    for (size_t i = 0; i < views_.size(); ++i) {
      if (static_cast<int>(i) == activeIndex_) {
        views_[i]->setBounds(bounds);
        views_[i]->setVisible(true);
      } else {
        views_[i]->setVisible(false);
      }
    }
  }

  Component *getView(int index) {
    if (index >= 0 && index < static_cast<int>(views_.size()))
      return views_[static_cast<size_t>(index)].get();
    return nullptr;
  }

  void drawSkia(SkCanvas *canvas) override {
    if (auto *view = getView(activeIndex_)) {
      if (auto *sc = dynamic_cast<SkiaComponent *>(view)) {
        sc->drawSkia(canvas);
      }
    }
  }

private:
  std::vector<std::unique_ptr<juce::Component>> views_;
  int activeIndex_ = -1;
};

//==============================================================================
// MainLayoutComponent
//==============================================================================

MainLayoutComponent::MainLayoutComponent(Engine &engine, CommandAPI &api, ProjectState &state)
    : engine_(engine), projectState_(state) {

  // 1. Initialize Browser Model
  browserModel_ = std::make_unique<BrowserModel>(
      engine_.getInstrumentRegistry(), engine_.getPluginHost());

  auto &layoutMgr = layout::LayoutManager::getInstance();

  // Register Factories for Layout Persistence
  layoutMgr.registerPanelType(
      "browser", "Browser", [this]() -> std::unique_ptr<juce::Component> {
        auto browser = std::make_unique<BrowserPanel>(*browserModel_);
        browser->onItemDoubleClicked =
            [this](std::shared_ptr<BrowserItem> item) {
              if (item && !item->isDirectory) {
                DBG("MainLayout: Browser item activated: " + item->name);
              }
            };
        return std::unique_ptr<juce::Component>(browser.release());
      });

  layoutMgr.registerPanelType(
      "main_views", "Main View", [this]() -> std::unique_ptr<juce::Component> {
        // NUKED - Return empty component since no views exist
        auto empty = std::make_unique<juce::Component>();
        empty->setName("NukedMainViews");
        return empty;
      });

  layoutMgr.registerPanelType("sample_editor", "Sample Editor",
                              [this]() -> std::unique_ptr<juce::Component> {
                                // NUKED - Return empty component since no sample editor exists
                                auto empty = std::make_unique<juce::Component>();
                                empty->setName("NukedSampleEditor");
                                return empty;
                              });

  // 2. Create Root Container (Horizontal: Browser | Center)
  panelContainer_ = std::make_unique<ResizablePanelContainer>();
  panelContainer_->setSplitDirection(
      ResizablePanelContainer::SplitDirection::Horizontal);
  addAndMakeVisible(panelContainer_.get());

  // 2a. Wingman Panel (LEFT SIDE - Added FIRST to appear on left)
  auto wingmanSidePanel = std::make_unique<RightSidePanel>(api, engine_, projectState_);
  rightSidePanel_ = wingmanSidePanel.get();
  
  layout::PanelConfig wingmanCfg;
  wingmanCfg.id = "left_sidebar";
  wingmanCfg.type = "left_sidebar";
  wingmanCfg.name = "Wingman";
  wingmanCfg.initialSize = 320;
  wingmanCfg.minSize = 280;
  wingmanCfg.flex = 0;
  wingmanCfg.isCollapsible = true;
  wingmanCfg.isCollapsed = true; // Default to closed
  
  panelContainer_->addPanel(std::move(wingmanSidePanel), wingmanCfg);

  // 3. (Browser & Info View Removed)
  // The user requested to remove the "clutter" (Browser + Info View)
  // and only keep Wingman togglable.
  // Previous code for Left Container (Browser | Info View) has been removed.

  // 4. NUKED - All center content removed (Arranger, Session, Editors)
  // Create a completely transparent center area below the transport bar
  ZENITH_LOG_INFO("MainLayoutComponent: Creating transparent center area");
  
  class TransparentComponent : public juce::Component {
  public:
    TransparentComponent() { 
      setOpaque(false); 
      setInterceptsMouseClicks(false, false);
    }
    void paint(juce::Graphics& g) override {
      // Paint nothing - completely transparent
    }
  };
  
  auto transparentCenter = std::make_unique<TransparentComponent>();
  transparentCenter->setName("TransparentCenter");
  
  layout::PanelConfig centerCfg;
  centerCfg.id = "center_container";
  centerCfg.type = "empty";
  centerCfg.name = "Center";
  centerCfg.flex = 1.0f;
  centerCfg.minSize = 200;
  centerCfg.showHeader = false;

  ZENITH_LOG_INFO("MainLayoutComponent: Adding transparent center to panelContainer_");
  panelContainer_->addPanel(std::move(transparentCenter), centerCfg);
  ZENITH_LOG_INFO("MainLayoutComponent: Transparent center added to panelContainer_");

  // RIGHT SIDEBAR REMOVED - Wingman is now on left side

  // 7. NUKED - Cursor Overlay removed since no arranger exists
  ZENITH_LOG_INFO("MainLayoutComponent: Constructor complete - UI nuked below transport bar");
}

MainLayoutComponent::~MainLayoutComponent() = default;

void MainLayoutComponent::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();
  SkRect skBounds = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());
  
  // NUKED - Don't draw background, keep it transparent
  // GlassmorphicPanel::fillBackground(canvas, skBounds);

  if (panelContainer_) {
    panelContainer_->drawSkia(canvas);
  }

  // NUKED - No cursor overlay since no arranger exists
}

void MainLayoutComponent::resized() {
  auto bounds = getLocalBounds();
  if (panelContainer_) {
    panelContainer_->setBounds(bounds);
  }
  // NUKED - No cursor overlay to resize
}

void MainLayoutComponent::toggleView() {
  // NUKED - No view switcher exists
}

void MainLayoutComponent::toggleBrowser() {
  if (auto *wrapper = panelContainer_->getPanel("browser")) {
    wrapper->toggleCollapse(true);
  }
}

void MainLayoutComponent::toggleWingman() {
  if (auto *wrapper = panelContainer_->getPanel("left_sidebar")) {
    wrapper->toggleCollapse(true);
  }
}

void MainLayoutComponent::toggleSampleEditor() {
  // NUKED - No sample editor exists
}

bool MainLayoutComponent::isSessionView() const {
  // NUKED - Always return false since no views exist
  return false;
}

bool MainLayoutComponent::isBrowserVisible() const {
  if (auto *wrapper = panelContainer_->getPanel("browser")) {
    return !wrapper->isCollapsed();
  }
  return false;
}

bool MainLayoutComponent::isSampleEditorVisible() const {
  // NUKED - Always return false since no sample editor exists
  return false;
}

SampleEditorComponent *MainLayoutComponent::getSampleEditor() {
  // NUKED - Return nullptr since no sample editor exists
  return nullptr;
}

MidiEditorContainer *MainLayoutComponent::getMidiEditor() {
  // NUKED - Return nullptr since no MIDI editor exists
  return nullptr;
}

bool MainLayoutComponent::isMidiEditorVisible() const {
  // NUKED - Always return false since no MIDI editor exists
  return false;
}

} // namespace zenith
