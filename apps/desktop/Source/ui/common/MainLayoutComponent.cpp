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
        auto switcher = std::make_unique<ViewSwitcher>();
        // Add Arranger
        auto arranger =
            std::make_unique<ArrangerComponent>(engine_, projectState_);
        arranger->onClipDoubleClicked = [this](const juce::String &trackId,
                                               const juce::String &clipId) {
          // Check clip type
          auto [track, clip] = projectState_.findClip(clipId);
          if (clip.isValid()) {
            bool isMidi = clip.getProperty("type").toString() == "midi";

            if (isMidi) {
              // Switch to MIDI Editor
              if (editorSwitcher_)
                editorSwitcher_->setActiveView(1);
              if (midiEditor_) {
                MidiClipContext ctx;
                ctx.clipId = clipId;
                ctx.trackId = trackId;
                ctx.clipName = clip.getProperty("name");
                ctx.clipStartBeats = clip.getProperty("start");
                ctx.clipLengthBeats = clip.getProperty("length");
                midiEditor_->setClipContext(ctx);
              }
            } else {
              // Switch to Audio Editor
              if (editorSwitcher_)
                editorSwitcher_->setActiveView(0);
              if (sampleEditor_) {
                sampleEditor_->setClipToEdit(trackId, clipId);
              }
            }

            // Ensure bottom panel is visible
            toggleSampleEditor(); // Renamed conceptually to toggleEditor, but
                                  // keeping method name for now
          }
        };
        switcher->addView(std::move(arranger));
        // Add Session
        switcher->addView(
            std::make_unique<SessionViewComponent>(engine_, projectState_));
        return std::unique_ptr<juce::Component>(switcher.release());
      });

  layoutMgr.registerPanelType("sample_editor", "Sample Editor",
                              [this]() -> std::unique_ptr<juce::Component> {
                                return std::make_unique<SampleEditorComponent>(
                                    engine_, projectState_);
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

  // 3. Create Left Container (Vertical: Browser | Info View)
  auto leftContainer = std::make_unique<ResizablePanelContainer>();
  leftContainer->setSplitDirection(ResizablePanelContainer::SplitDirection::Vertical);

  // 3a. Browser
  auto browser = std::make_unique<BrowserPanel>(*browserModel_);
  browser->onItemDoubleClicked = [this](std::shared_ptr<BrowserItem> item) {
    if (item && !item->isDirectory) {
      DBG("MainLayout: Browser item activated: " + item->name);
    }
  };

  layout::PanelConfig browserCfg;
  browserCfg.id = "browser";
  browserCfg.type = "browser"; 
  browserCfg.name = "Browser";
  browserCfg.flex = 1.0f;
  browserCfg.minSize = 200;
  
  leftContainer->addPanel(std::move(browser), browserCfg);

  // 3b. Info View
  auto helpView = std::make_unique<HelpViewPanel>();
  
  layout::PanelConfig helpCfg;
  helpCfg.id = "help_view";
  helpCfg.type = "help_view";
  helpCfg.name = "Info View";
  helpCfg.flex = 0.0f; 
  helpCfg.initialSize = 150.0f;
  helpCfg.minSize = 100.0f;
  helpCfg.isCollapsible = true;
  
  leftContainer->addPanel(std::move(helpView), helpCfg);
  ZENITH_LOG_INFO("MainLayoutComponent: help_view panel added to leftContainer");

  // Add Left Container to Root
  layout::PanelConfig leftCfg;
  leftCfg.id = "left_container";
  leftCfg.type = "container";
  leftCfg.name = "Sidebar";
  leftCfg.initialSize = 300;
  leftCfg.minSize = 200;
  leftCfg.flex = 0; 
  leftCfg.isCollapsible = true;
  leftCfg.showHeader = false; // Hide header for sidebar container

  ZENITH_LOG_INFO("MainLayoutComponent: Adding leftContainer to panelContainer_");
  panelContainer_->addPanel(std::move(leftContainer), leftCfg);
  ZENITH_LOG_INFO("MainLayoutComponent: leftContainer added successfully");

  // 4. Create Center Container (Vertical: Views | Sample Editor)
  ZENITH_LOG_INFO("MainLayoutComponent: Creating centerContainer");
  auto centerContainer = std::make_unique<ResizablePanelContainer>();
  centerContainer_ = centerContainer.get(); // Cache pointer
  centerContainer->setSplitDirection(
      ResizablePanelContainer::SplitDirection::Vertical);
  ZENITH_LOG_INFO("MainLayoutComponent: centerContainer created");

  // 4a. Views Panel (Switcher)
  ZENITH_LOG_INFO("MainLayoutComponent: Creating ViewSwitcher");
  auto switcher = std::make_unique<ViewSwitcher>();
  viewSwitcher_ = switcher.get();
  ZENITH_LOG_INFO("MainLayoutComponent: ViewSwitcher created");

  ZENITH_LOG_INFO("MainLayoutComponent: Creating ArrangerComponent");
  auto arranger = std::make_unique<ArrangerComponent>(engine_, projectState_);
  ZENITH_LOG_INFO("MainLayoutComponent: ArrangerComponent created");
  arranger->onClipDoubleClicked = [this](const juce::String &trackId,
                                         const juce::String &clipId) {
    ZENITH_LOG_INFO("MainLayoutComponent: onClipDoubleClicked callback");
    // Check clip type
    auto [track, clip] = projectState_.findClip(clipId);
    if (clip.isValid()) {
      bool isMidi = clip.getProperty("type").toString() == "midi";

      if (isMidi) {
        if (editorSwitcher_)
          editorSwitcher_->setActiveView(1);
        if (midiEditor_) {
          MidiClipContext ctx;
          ctx.clipId = clipId;
          ctx.trackId = trackId;
          ctx.clipName = clip.getProperty("name");
          ctx.clipStartBeats = clip.getProperty("start");
          ctx.clipLengthBeats = clip.getProperty("length");
          midiEditor_->setClipContext(ctx);
        }
      } else {
        if (editorSwitcher_)
          editorSwitcher_->setActiveView(0);
        if (sampleEditor_) {
          sampleEditor_->setClipToEdit(trackId, clipId);
        }
      }
      toggleSampleEditor();
    }
  };
  ZENITH_LOG_INFO("MainLayoutComponent: ArrangerComponent callback set");
  
  // Store raw pointer for collaboration features
  ArrangerComponent* arrangerPtr = arranger.get();
  ZENITH_LOG_INFO("MainLayoutComponent: Adding arranger to switcher");
  switcher->addView(std::move(arranger));
  ZENITH_LOG_INFO("MainLayoutComponent: Arranger added to switcher");
  
  ZENITH_LOG_INFO("MainLayoutComponent: Creating SessionViewComponent");
  switcher->addView(
      std::make_unique<SessionViewComponent>(engine_, projectState_));
  ZENITH_LOG_INFO("MainLayoutComponent: SessionViewComponent added");

  layout::PanelConfig viewsCfg;
  viewsCfg.id = "main_views";
  viewsCfg.type = "main_views"; // Important
  viewsCfg.name = "Main View";
  viewsCfg.flex = 1.0f;
  viewsCfg.minSize = 300;
  viewsCfg.showHeader = false; // Hide header for main content area

  ZENITH_LOG_INFO("MainLayoutComponent: Adding switcher to centerContainer");
  centerContainer->addPanel(std::move(switcher), viewsCfg);
  ZENITH_LOG_INFO("MainLayoutComponent: Switcher added to centerContainer");

  // 4b. Editors Panel (Switcher: Sample Editor | MIDI Editor)
  ZENITH_LOG_INFO("MainLayoutComponent: Creating editorSwitcher");
  auto editorSwitcher = std::make_unique<ViewSwitcher>();
  editorSwitcher_ = editorSwitcher.get();
  ZENITH_LOG_INFO("MainLayoutComponent: editorSwitcher created");

  // View 0: Sample Editor
  ZENITH_LOG_INFO("MainLayoutComponent: Creating SampleEditorComponent");
  auto sampleEditor =
      std::make_unique<SampleEditorComponent>(engine_, projectState_);
  ZENITH_LOG_INFO("MainLayoutComponent: SampleEditorComponent created");
  sampleEditor_ = sampleEditor.get();
  ZENITH_LOG_INFO("MainLayoutComponent: Adding sampleEditor to editorSwitcher");
  editorSwitcher->addView(std::move(sampleEditor));
  ZENITH_LOG_INFO("MainLayoutComponent: sampleEditor added");

  // View 1: MIDI Editor
  ZENITH_LOG_INFO("MainLayoutComponent: Creating MidiEditorContainer");
  auto midiEditor =
      std::make_unique<MidiEditorContainer>(projectState_, engine_);
  ZENITH_LOG_INFO("MainLayoutComponent: MidiEditorContainer created");
  midiEditor_ = midiEditor.get();
  ZENITH_LOG_INFO("MainLayoutComponent: Adding midiEditor to editorSwitcher");
  editorSwitcher->addView(std::move(midiEditor));
  ZENITH_LOG_INFO("MainLayoutComponent: midiEditor added");

  layout::PanelConfig editorCfg;
  editorCfg.id =
      "sample_editor"; // Keep ID for layout persistence compatibility
  editorCfg.type = "sample_editor";
  editorCfg.name = "Editor";
  editorCfg.initialSize = 300;
  editorCfg.minSize = 150;
  editorCfg.flex = 0; // Fixed height
  editorCfg.isCollapsible = true;
  editorCfg.isCollapsed = true;

  ZENITH_LOG_INFO("MainLayoutComponent: Adding editorSwitcher to centerContainer");
  centerContainer->addPanel(std::move(editorSwitcher), editorCfg);
  ZENITH_LOG_INFO("MainLayoutComponent: editorSwitcher added to centerContainer");

  // Add Center Container
  layout::PanelConfig centerCfg;
  centerCfg.id = "center_container";
  centerCfg.type =
      "container"; // We don't have a factory for this generic container,
                   // but ResizablePanelContainer handles recursion?
                   // Actually, LayoutManager doesn't handle nested containers
                   // automatically yet. We'll need to improve LayoutManager for
                   // nested containers later.
  centerCfg.name = "Center";
  centerCfg.flex = 1.0f;
  centerCfg.minSize = 400;
  centerCfg.showHeader = false; // Hide header for center container

  ZENITH_LOG_INFO("MainLayoutComponent: Adding centerContainer to panelContainer_");
  panelContainer_->addPanel(std::move(centerContainer), centerCfg);
  ZENITH_LOG_INFO("MainLayoutComponent: centerContainer added to panelContainer_");

  // RIGHT SIDEBAR REMOVED - Wingman is now on left side

  // 7. Cursor Overlay with ID-to-Rect mapping for collaboration
  ZENITH_LOG_INFO("MainLayoutComponent: Creating RemoteCursorOverlay");
  cursorOverlay_ = std::make_unique<RemoteCursorOverlay>();
  ZENITH_LOG_INFO("MainLayoutComponent: RemoteCursorOverlay created");
  
  // Set up the mapper to convert selection IDs to screen rectangles
  // This enables remote users' selections to be visualized
  cursorOverlay_->setIdToRectMapper([arrangerPtr](const juce::String& clipId) -> juce::Rectangle<float> {
    if (!arrangerPtr) return {};
    auto* clipMgr = arrangerPtr->getClipManager();
    if (!clipMgr) return {};
    return clipMgr->getClipBounds(clipId);
  });
  ZENITH_LOG_INFO("MainLayoutComponent: RemoteCursorOverlay mapper set");
  
  addAndMakeVisible(cursorOverlay_.get());
  ZENITH_LOG_INFO("MainLayoutComponent: Constructor complete");
}

MainLayoutComponent::~MainLayoutComponent() = default;

void MainLayoutComponent::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();
  SkRect skBounds = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());
  
  GlassmorphicPanel::fillBackground(canvas, skBounds);

  if (panelContainer_) {
    panelContainer_->drawSkia(canvas);
  }

  // Draw Remote Cursors on top of everything
  if (cursorOverlay_ && cursorOverlay_->isVisible()) {
    canvas->save();
    canvas->translate((float)cursorOverlay_->getX(), (float)cursorOverlay_->getY());
    cursorOverlay_->drawSkia(canvas);
    canvas->restore();
  }
}

void MainLayoutComponent::resized() {
  auto bounds = getLocalBounds();
  if (panelContainer_) {
    panelContainer_->setBounds(bounds);
  }
  if (cursorOverlay_) {
    cursorOverlay_->setBounds(bounds);
    cursorOverlay_->toFront(false);
  }
}

void MainLayoutComponent::toggleView() {
  if (viewSwitcher_) {
    int current = viewSwitcher_->getActiveViewIndex();
    viewSwitcher_->setActiveView(current == 0 ? 1 : 0);
  }
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
  if (centerContainer_) {
    if (auto *wrapper = centerContainer_->getPanel("sample_editor")) {
      wrapper->toggleCollapse(true);
    }
  }
}

bool MainLayoutComponent::isSessionView() const {
  if (viewSwitcher_)
    return viewSwitcher_->getActiveViewIndex() == 1;
  return false;
}

bool MainLayoutComponent::isBrowserVisible() const {
  if (auto *wrapper = panelContainer_->getPanel("browser")) {
    return !wrapper->isCollapsed();
  }
  return false;
}

bool MainLayoutComponent::isSampleEditorVisible() const {
  if (centerContainer_) {
    if (auto *wrapper = centerContainer_->getPanel("sample_editor")) {
      return !wrapper->isCollapsed();
    }
  }
  return false;
}

SampleEditorComponent *MainLayoutComponent::getSampleEditor() {
  return sampleEditor_;
}

MidiEditorContainer *MainLayoutComponent::getMidiEditor() {
  return midiEditor_;
}

bool MainLayoutComponent::isMidiEditorVisible() const {
  if (centerContainer_) {
    if (auto *wrapper = centerContainer_->getPanel("sample_editor")) {
      return !wrapper->isCollapsed() && editorSwitcher_ &&
             editorSwitcher_->getActiveViewIndex() == 1;
    }
  }
  return false;
}

} // namespace zenith
