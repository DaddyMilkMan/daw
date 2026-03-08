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
#include "../../engine/ProjectState.h"
#include "../../engine/ZenithLogger.h"
#include "../../instruments/InstrumentRegistry.h"
#include "../arranger/ArrangerComponent.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../framework/LayoutManager.h"
#include "../mixer/MixerComponent.h"
#include "../sample-editor/SampleEditorComponent.h"
#include "../views/SessionViewComponent.h"
#include "ResizablePanelContainer.h"
#include "RightSidePanel.h"
#include "../panels/BrowserPanel.h"

#include <array>

namespace {

constexpr int kWorkspaceOuterPadding = 14;
constexpr int kSectionGap = 12;
constexpr int kWorkspaceHeaderHeight = 52;
constexpr int kDockHeaderHeight = 44;
constexpr int kDockContentGap = 8;
constexpr float kDockHeightRatio = 0.34f;

SkRect toSkRect(const juce::Rectangle<int> &bounds) {
  return SkRect::MakeXYWH(static_cast<float>(bounds.getX()),
                          static_cast<float>(bounds.getY()),
                          static_cast<float>(bounds.getWidth()),
                          static_cast<float>(bounds.getHeight()));
}

void drawRoundedSurface(SkCanvas *canvas, const juce::Rectangle<int> &bounds,
                        SkColor fillColor, SkColor strokeColor,
                        float radius = 16.0f) {
  if (bounds.isEmpty())
    return;

  SkPaint fillPaint;
  fillPaint.setColor(fillColor);
  fillPaint.setAntiAlias(true);
  canvas->drawRoundRect(toSkRect(bounds), radius, radius, fillPaint);

  SkPaint strokePaint;
  strokePaint.setColor(strokeColor);
  strokePaint.setStyle(SkPaint::kStroke_Style);
  strokePaint.setStrokeWidth(1.0f);
  strokePaint.setAntiAlias(true);
  canvas->drawRoundRect(toSkRect(bounds), radius, radius, strokePaint);
}

int estimatePillWidth(const juce::String &label, int minWidth) {
  return juce::jmax(minWidth, 26 + (label.length() * 8));
}

} // namespace

namespace zenith {

class WorkspaceCenterComponent final : public SkiaComponent {
public:
  WorkspaceCenterComponent(Engine &engine, ProjectState &projectState)
      : engine_(engine), projectState_(projectState),
        arrangerView_(std::make_unique<ArrangerComponent>(engine_, projectState_)),
        sessionView_(
            std::make_unique<SessionViewComponent>(engine_, projectState_)),
        mixerView_(std::make_unique<MixerComponent>(engine_, projectState_)),
        sampleEditor_(
            std::make_unique<SampleEditorComponent>(engine_, projectState_)) {
    addAndMakeVisible(arrangerView_.get());
    addChildComponent(sessionView_.get());
    addAndMakeVisible(mixerView_.get());
    addChildComponent(sampleEditor_.get());

    arrangerView_->onClipDoubleClicked =
        [this](const juce::String &trackId, const juce::String &clipId) {
          openClipFromArranger(trackId, clipId);
        };

    refreshVisibleViews();
  }

  void drawSkia(SkCanvas *canvas) override {
    SkPaint backgroundPaint;
    backgroundPaint.setColor(design::colors::BG_DARKEST);
    canvas->drawRect(
        SkRect::MakeWH(static_cast<float>(getWidth()),
                       static_cast<float>(getHeight())),
        backgroundPaint);

    drawRoundedSurface(canvas, workspaceHeaderBounds_,
                       design::withAlpha(design::colors::BG_DARK, 0.96f),
                       design::withAlpha(design::colors::ACCENT_PRIMARY, 0.16f));
    drawRoundedSurface(canvas, mainSurfaceBounds_,
                       design::withAlpha(design::colors::BG_DARKER, 0.98f),
                       design::withAlpha(design::colors::ACCENT_PRIMARY, 0.10f),
                       18.0f);
    drawRoundedSurface(canvas, dockHeaderBounds_,
                       design::withAlpha(design::colors::BG_DARK, 0.96f),
                       design::colors::BORDER_SUBTLE, 14.0f);

    if (editorDockVisible_) {
      drawRoundedSurface(canvas, dockSurfaceBounds_,
                         design::withAlpha(design::colors::BG_DARKER, 0.98f),
                         design::colors::BORDER_SUBTLE, 18.0f);
    }

    drawWorkspaceHeader(canvas);
    drawDockHeader(canvas);
    drawChildren(canvas);
  }

  void resized() override {
    auto bounds = getLocalBounds().reduced(kWorkspaceOuterPadding);
    workspaceHeaderBounds_ = bounds.removeFromTop(kWorkspaceHeaderHeight);
    bounds.removeFromTop(kSectionGap);

    const int dockHeight =
        juce::jlimit(200, 340,
                     static_cast<int>(bounds.getHeight() * kDockHeightRatio));
    const int reservedBottom =
        kDockHeaderHeight +
        (editorDockVisible_ ? kSectionGap + dockHeight : 0);
    const int mainHeight =
        juce::jmax(120, bounds.getHeight() - reservedBottom);

    mainSurfaceBounds_ = bounds.removeFromTop(mainHeight);

    if (editorDockVisible_)
      bounds.removeFromTop(kSectionGap);

    dockHeaderBounds_ = bounds.removeFromTop(kDockHeaderHeight);
    dockSurfaceBounds_ = {};

    if (editorDockVisible_) {
      bounds.removeFromTop(kDockContentGap);
      dockSurfaceBounds_ = bounds;
    }

    layoutHeaderTabs();
    layoutDockTabs();

    const auto primaryBounds = mainSurfaceBounds_.reduced(1);
    arrangerView_->setBounds(primaryBounds);
    sessionView_->setBounds(primaryBounds);

    if (editorDockVisible_) {
      const auto dockContentBounds = dockSurfaceBounds_.reduced(1);
      mixerView_->setBounds(dockContentBounds);
      sampleEditor_->setBounds(dockContentBounds);
    }

    refreshVisibleViews();
  }

  void mouseMove(const juce::MouseEvent &e) override {
    updateHoverState(e.getPosition());
  }

  void mouseDown(const juce::MouseEvent &e) override {
    const auto position = e.getPosition();

    if (primaryTabBounds_[0].contains(position)) {
      setPrimaryView(PrimaryView::Arranger);
      return;
    }

    if (primaryTabBounds_[1].contains(position)) {
      setPrimaryView(PrimaryView::Session);
      return;
    }

    if (dockTabBounds_[0].contains(position)) {
      showDockView(DockView::Mixer);
      return;
    }

    if (dockTabBounds_[1].contains(position)) {
      showDockView(DockView::SampleEditor);
      return;
    }

    if (dockToggleBounds_.contains(position)) {
      editorDockVisible_ = !editorDockVisible_;
      resized();
      repaint();
    }
  }

  void mouseExit(const juce::MouseEvent &e) override {
    hoveredPrimaryTab_ = -1;
    hoveredDockTab_ = -1;
    isDockToggleHovered_ = false;
    setMouseCursor(juce::MouseCursor::NormalCursor);
    SkiaComponent::mouseExit(e);
  }

  void togglePrimaryView() {
    setPrimaryView(activePrimaryView_ == PrimaryView::Arranger
                       ? PrimaryView::Session
                       : PrimaryView::Arranger);
  }

  void toggleSampleEditor() {
    if (!editorDockVisible_) {
      showDockView(DockView::SampleEditor);
      return;
    }

    if (activeDockView_ != DockView::SampleEditor) {
      showDockView(DockView::SampleEditor);
      return;
    }

    editorDockVisible_ = false;
    refreshVisibleViews();
    resized();
    repaint();
  }

  bool isSessionView() const {
    return activePrimaryView_ == PrimaryView::Session;
  }

  bool isSampleEditorVisible() const {
    return editorDockVisible_ && activeDockView_ == DockView::SampleEditor;
  }

  SampleEditorComponent *getSampleEditor() const { return sampleEditor_.get(); }

private:
  enum class PrimaryView { Arranger, Session };
  enum class DockView { Mixer, SampleEditor };

  void setPrimaryView(PrimaryView view) {
    if (activePrimaryView_ == view)
      return;

    activePrimaryView_ = view;
    refreshVisibleViews();
    resized();
    repaint();
  }

  void showDockView(DockView view) {
    activeDockView_ = view;
    editorDockVisible_ = true;
    refreshVisibleViews();
    resized();
    repaint();
  }

  void refreshVisibleViews() {
    arrangerView_->setVisible(activePrimaryView_ == PrimaryView::Arranger);
    sessionView_->setVisible(activePrimaryView_ == PrimaryView::Session);

    const bool showMixer =
        editorDockVisible_ && activeDockView_ == DockView::Mixer;
    const bool showSample =
        editorDockVisible_ && activeDockView_ == DockView::SampleEditor;

    mixerView_->setVisible(showMixer);
    sampleEditor_->setVisible(showSample);
  }

  void openClipFromArranger(const juce::String &trackId,
                            const juce::String &clipId) {
    auto [trackNode, clipNode] = projectState_.findClip(clipId);
    juce::ignoreUnused(trackNode);

    if (!clipNode.isValid()) {
      ZENITH_LOG_WARNING(
          "WorkspaceCenterComponent: Failed to open clip " + clipId);
      return;
    }

    const auto clipType = clipNode.getProperty(ProjectState::PROP_TYPE).toString();
    if (clipType.equalsIgnoreCase("audio")) {
      sampleEditor_->setClipToEdit(trackId, clipId);
      showDockView(DockView::SampleEditor);
      return;
    }

    ZENITH_LOG_INFO(
        "WorkspaceCenterComponent: MIDI clip activation is not routed to a "
        "Skia editor yet.");
  }

  void updateHoverState(juce::Point<int> position) {
    int hoveredPrimary = -1;
    int hoveredDock = -1;

    for (int index = 0; index < static_cast<int>(primaryTabBounds_.size());
         ++index) {
      if (primaryTabBounds_[static_cast<size_t>(index)].contains(position)) {
        hoveredPrimary = index;
        break;
      }
    }

    for (int index = 0; index < static_cast<int>(dockTabBounds_.size());
         ++index) {
      if (dockTabBounds_[static_cast<size_t>(index)].contains(position)) {
        hoveredDock = index;
        break;
      }
    }

    const bool hoveredToggle = dockToggleBounds_.contains(position);
    const bool hasInteractiveTarget =
        hoveredPrimary != -1 || hoveredDock != -1 || hoveredToggle;

    if (hoveredPrimary == hoveredPrimaryTab_ && hoveredDock == hoveredDockTab_ &&
        hoveredToggle == isDockToggleHovered_) {
      setMouseCursor(hasInteractiveTarget ? juce::MouseCursor::PointingHandCursor
                                          : juce::MouseCursor::NormalCursor);
      return;
    }

    hoveredPrimaryTab_ = hoveredPrimary;
    hoveredDockTab_ = hoveredDock;
    isDockToggleHovered_ = hoveredToggle;
    setMouseCursor(hasInteractiveTarget ? juce::MouseCursor::PointingHandCursor
                                        : juce::MouseCursor::NormalCursor);
    repaint();
  }

  void layoutHeaderTabs() {
    const bool compact = workspaceHeaderBounds_.getWidth() < 560;
    const int tabHeight = workspaceHeaderBounds_.getHeight() - 16;
    int x = workspaceHeaderBounds_.getX() + (compact ? 14 : 126);
    const int y = workspaceHeaderBounds_.getY() + 8;

    primaryTabBounds_[0] =
        {x, y, estimatePillWidth("Arrange", compact ? 82 : 100), tabHeight};
    x = primaryTabBounds_[0].getRight() + 10;
    primaryTabBounds_[1] =
        {x, y, estimatePillWidth("Session", compact ? 82 : 100), tabHeight};
  }

  void layoutDockTabs() {
    const bool compact = dockHeaderBounds_.getWidth() < 520;
    const int tabHeight = dockHeaderBounds_.getHeight() - 14;
    const int toggleWidth = compact ? 64 : 78;
    const int toggleInset = compact ? 78 : 92;
    const int tabStartX = dockHeaderBounds_.getX() + (compact ? 14 : 80);
    int x = tabStartX;
    const int y = dockHeaderBounds_.getY() + 7;

    dockTabBounds_[0] =
        {x, y, estimatePillWidth("Mixer", compact ? 74 : 92), tabHeight};
    x = dockTabBounds_[0].getRight() + 10;
    dockTabBounds_[1] =
        {x, y, estimatePillWidth("Sample", compact ? 78 : 96), tabHeight};

    dockToggleBounds_ = {dockHeaderBounds_.getRight() - toggleInset, y,
                         toggleWidth, tabHeight};

    if (dockTabBounds_[1].getRight() > dockToggleBounds_.getX() - 8) {
      const int availableWidth = dockToggleBounds_.getX() - 8 - tabStartX;
      const int uniformTabWidth = juce::jmax(68, (availableWidth - 10) / 2);
      dockTabBounds_[0] = {tabStartX, y, uniformTabWidth, tabHeight};
      dockTabBounds_[1] = {dockTabBounds_[0].getRight() + 10, y,
                           uniformTabWidth, tabHeight};
    }
  }

  void drawWorkspaceHeader(SkCanvas *canvas) const {
    drawSectionLabel(canvas, workspaceHeaderBounds_, "WORKSPACE",
                     currentPrimaryCaption());
    drawPill(canvas, primaryTabBounds_[0], "Arrange",
             activePrimaryView_ == PrimaryView::Arranger, hoveredPrimaryTab_ == 0);
    drawPill(canvas, primaryTabBounds_[1], "Session",
             activePrimaryView_ == PrimaryView::Session, hoveredPrimaryTab_ == 1);
  }

  void drawDockHeader(SkCanvas *canvas) const {
    drawSectionLabel(canvas, dockHeaderBounds_, "DOCK", currentDockCaption());
    drawPill(canvas, dockTabBounds_[0], "Mixer",
             editorDockVisible_ && activeDockView_ == DockView::Mixer,
             hoveredDockTab_ == 0);
    drawPill(canvas, dockTabBounds_[1], "Sample",
             editorDockVisible_ && activeDockView_ == DockView::SampleEditor,
             hoveredDockTab_ == 1);

    drawActionButton(canvas, dockToggleBounds_,
                     editorDockVisible_ ? "Hide" : "Open",
                     isDockToggleHovered_);
  }

  void drawSectionLabel(SkCanvas *canvas,
                        const juce::Rectangle<int> &bounds,
                        const juce::String &section,
                        const juce::String &caption) const {
    if (bounds.getWidth() < 560)
      return;

    SkPaint sectionPaint;
    sectionPaint.setColor(design::withAlpha(design::colors::TEXT_SECONDARY, 0.9f));
    sectionPaint.setAntiAlias(true);

    auto sectionFont = design::getSkFont(10.0f, design::FontWeight::SemiBold);
    auto captionFont = design::getSkFont(11.0f, design::FontWeight::Regular);

    canvas->drawString(section.toRawUTF8(),
                       static_cast<float>(bounds.getX() + 16),
                       static_cast<float>(bounds.getY() + 19), sectionFont,
                       sectionPaint);

    SkPaint captionPaint;
    captionPaint.setColor(
        design::withAlpha(design::colors::TEXT_SECONDARY, 0.72f));
    captionPaint.setAntiAlias(true);

    if (bounds.getWidth() >= 760) {
      canvas->drawString(caption.toRawUTF8(),
                         static_cast<float>(bounds.getRight() - 150),
                         static_cast<float>(bounds.getY() + 29), captionFont,
                         captionPaint);
    }
  }

  void drawPill(SkCanvas *canvas, const juce::Rectangle<int> &bounds,
                const juce::String &label, bool isActive,
                bool isHovered) const {
    SkPaint fillPaint;
    fillPaint.setAntiAlias(true);
    fillPaint.setColor(
        isActive
            ? design::withAlpha(design::colors::ACCENT_PRIMARY, 0.16f)
            : isHovered ? design::withAlpha(design::colors::BG_LIGHT, 0.48f)
                        : design::withAlpha(design::colors::BG_LIGHT, 0.18f));
    canvas->drawRoundRect(toSkRect(bounds), 10.0f, 10.0f, fillPaint);

    SkPaint borderPaint;
    borderPaint.setAntiAlias(true);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.0f);
    borderPaint.setColor(
        isActive ? design::withAlpha(design::colors::ACCENT_PRIMARY, 0.42f)
                 : design::withAlpha(design::colors::BORDER_SUBTLE, 0.95f));
    canvas->drawRoundRect(toSkRect(bounds), 10.0f, 10.0f, borderPaint);

    if (isActive) {
      SkPaint accentPaint;
      accentPaint.setColor(
          design::withAlpha(design::colors::ACCENT_PRIMARY, 0.85f));
      accentPaint.setAntiAlias(true);
      canvas->drawRect(
          SkRect::MakeXYWH(static_cast<float>(bounds.getX() + 10),
                           static_cast<float>(bounds.getBottom() - 3),
                           static_cast<float>(bounds.getWidth() - 20), 2.0f),
          accentPaint);
    }

    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(isActive ? design::colors::TEXT_PRIMARY
                                : design::colors::TEXT_SECONDARY);
    auto textFont = design::getSkFont(
        11.0f, isActive ? design::FontWeight::SemiBold
                        : design::FontWeight::Medium);
    canvas->drawString(label.toRawUTF8(),
                       static_cast<float>(bounds.getX() + 14),
                       static_cast<float>(bounds.getCentreY() + 4), textFont,
                       textPaint);
  }

  void drawActionButton(SkCanvas *canvas, const juce::Rectangle<int> &bounds,
                        const juce::String &label, bool isHovered) const {
    SkPaint fillPaint;
    fillPaint.setAntiAlias(true);
    fillPaint.setColor(isHovered
                           ? design::withAlpha(design::colors::ACCENT_PRIMARY,
                                               0.16f)
                           : design::withAlpha(design::colors::BG_LIGHT, 0.20f));
    canvas->drawRoundRect(toSkRect(bounds), 10.0f, 10.0f, fillPaint);

    SkPaint borderPaint;
    borderPaint.setAntiAlias(true);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.0f);
    borderPaint.setColor(isHovered
                             ? design::withAlpha(design::colors::ACCENT_PRIMARY,
                                                 0.30f)
                             : design::withAlpha(design::colors::BORDER_SUBTLE,
                                                 0.95f));
    canvas->drawRoundRect(toSkRect(bounds), 10.0f, 10.0f, borderPaint);

    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(design::colors::TEXT_PRIMARY);
    auto textFont = design::getSkFont(11.0f, design::FontWeight::Medium);
    canvas->drawString(label.toRawUTF8(),
                       static_cast<float>(bounds.getX() + 18),
                       static_cast<float>(bounds.getCentreY() + 4), textFont,
                       textPaint);
  }

  juce::String currentPrimaryCaption() const {
    return activePrimaryView_ == PrimaryView::Arranger ? "Timeline focus"
                                                       : "Clip launch focus";
  }

  juce::String currentDockCaption() const {
    if (!editorDockVisible_)
      return "Dock hidden";

    return activeDockView_ == DockView::Mixer ? "Mix balance"
                                              : "Sample detail";
  }

  Engine &engine_;
  ProjectState &projectState_;
  std::unique_ptr<ArrangerComponent> arrangerView_;
  std::unique_ptr<SessionViewComponent> sessionView_;
  std::unique_ptr<MixerComponent> mixerView_;
  std::unique_ptr<SampleEditorComponent> sampleEditor_;

  PrimaryView activePrimaryView_ = PrimaryView::Arranger;
  DockView activeDockView_ = DockView::Mixer;
  bool editorDockVisible_ = true;

  int hoveredPrimaryTab_ = -1;
  int hoveredDockTab_ = -1;
  bool isDockToggleHovered_ = false;

  juce::Rectangle<int> workspaceHeaderBounds_;
  juce::Rectangle<int> mainSurfaceBounds_;
  juce::Rectangle<int> dockHeaderBounds_;
  juce::Rectangle<int> dockSurfaceBounds_;
  juce::Rectangle<int> dockToggleBounds_;
  std::array<juce::Rectangle<int>, 2> primaryTabBounds_;
  std::array<juce::Rectangle<int>, 2> dockTabBounds_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WorkspaceCenterComponent)
};

MainLayoutComponent::MainLayoutComponent(Engine &engine, ProjectState &state,
                                         CommandAPI &api)
    : engine_(engine), projectState_(state), api_(api) {
  browserModel_ = std::make_unique<BrowserModel>(
      engine_.getInstrumentRegistry(), engine_.getPluginHost());

  auto& layoutMgr = layout::LayoutManager::getInstance();
  layoutMgr.registerPanelType(
      "main_views", "Main View",
      [this]() -> std::unique_ptr<juce::Component> {
        return std::make_unique<WorkspaceCenterComponent>(engine_,
                                                          projectState_);
      });

  layoutMgr.registerPanelType(
      "sample_editor", "Sample Editor",
      [this]() -> std::unique_ptr<juce::Component> {
        return std::make_unique<SampleEditorComponent>(engine_, projectState_);
      });

  panelContainer_ = std::make_unique<ResizablePanelContainer>();
  panelContainer_->setSplitDirection(ResizablePanelContainer::SplitDirection::Horizontal);
  addAndMakeVisible(panelContainer_.get());

  // Browser on the left (primary content navigation).
  auto browserPanel = std::make_unique<BrowserPanel>(*browserModel_, api_);
  browserPanel_ = browserPanel.get();

  layout::PanelConfig browserCfg;
  browserCfg.id = "left_browser";
  browserCfg.type = "left_browser";
  browserCfg.name = "Browser";
  browserCfg.initialSize = 360;
  browserCfg.minSize = 280;
  browserCfg.flex = 0;
  browserCfg.isCollapsible = true;
  browserCfg.isCollapsed = false;
  panelContainer_->addPanel(std::move(browserPanel), browserCfg);

  // Wingman is right-docked (contextual assistant zone).
  auto wingmanSidePanel = std::make_unique<RightSidePanel>(api_, engine_, projectState_);
  rightSidePanel_ = wingmanSidePanel.get();

  layout::PanelConfig wingmanCfg;
  wingmanCfg.id = "right_sidebar";
  wingmanCfg.type = "right_sidebar";
  wingmanCfg.name = "Wingman";
  wingmanCfg.initialSize = 320;
  wingmanCfg.minSize = 280;
  wingmanCfg.flex = 0;
  wingmanCfg.isCollapsible = true;
  wingmanCfg.isCollapsed = true;

  auto workspaceShell = std::make_unique<WorkspaceCenterComponent>(engine_, projectState_);
  workspaceShell_ = workspaceShell.get();

  layout::PanelConfig centerCfg;
  centerCfg.id = "center_container";
  centerCfg.type = "main_views";
  centerCfg.name = "Workspace";
  centerCfg.flex = 1.0f;
  centerCfg.minSize = 200;
  centerCfg.showHeader = false;
  panelContainer_->addPanel(std::move(workspaceShell), centerCfg);
  panelContainer_->addPanel(std::move(wingmanSidePanel), wingmanCfg);

  ZENITH_LOG_INFO(
      "MainLayoutComponent: Premium workspace shell initialized.");
}

MainLayoutComponent::~MainLayoutComponent() = default;

void MainLayoutComponent::drawSkia(SkCanvas *canvas) {
  if (panelContainer_) {
    panelContainer_->drawSkia(canvas);
  }
}

void MainLayoutComponent::resized() {
  if (panelContainer_) {
    panelContainer_->setBounds(getLocalBounds());
  }
}

void MainLayoutComponent::toggleView() {
  if (workspaceShell_ != nullptr)
    workspaceShell_->togglePrimaryView();
}

void MainLayoutComponent::toggleBrowser() {
  if (panelContainer_) {
    if (auto* wrapper = panelContainer_->getPanel("left_browser")) {
      wrapper->toggleCollapse(true);
    }
  }
}

void MainLayoutComponent::toggleSampleEditor() {
  if (workspaceShell_ != nullptr)
    workspaceShell_->toggleSampleEditor();
}

void MainLayoutComponent::toggleWingman() {
  if (panelContainer_) {
    if (auto* wrapper = panelContainer_->getPanel("right_sidebar")) {
      wrapper->toggleCollapse(true);
    }
  }
}

bool MainLayoutComponent::isSessionView() const {
  return workspaceShell_ != nullptr && workspaceShell_->isSessionView();
}

bool MainLayoutComponent::isBrowserVisible() const {
  if (!panelContainer_) return false;
  if (auto* wrapper = panelContainer_->getPanel("left_browser")) {
    return !wrapper->isCollapsed();
  }
  return false;
}

bool MainLayoutComponent::isSampleEditorVisible() const {
  return workspaceShell_ != nullptr && workspaceShell_->isSampleEditorVisible();
}

SampleEditorComponent *MainLayoutComponent::getSampleEditor() {
  return workspaceShell_ != nullptr ? workspaceShell_->getSampleEditor()
                                    : nullptr;
}

MidiEditorContainer *MainLayoutComponent::getMidiEditor() { return nullptr; }

bool MainLayoutComponent::isMidiEditorVisible() const { return false; }

} // namespace zenith
