#include "ArrangerComponent.h"
#include "ArrangerTrackComponent.h"

namespace zenith {

ArrangerComponent::ArrangerComponent(Engine &engine, ProjectState &ps)
    : engine_(engine), projectState(ps) {
  setWantsKeyboardFocus(true);

  projectState.getState().addListener(this);

  macroToolbar = std::make_unique<MacroToolbar>(engine_, projectState);
  addAndMakeVisible(*macroToolbar);

  freezeOverlay = std::make_unique<FreezeProgressOverlay>();
  addAndMakeVisible(*freezeOverlay);

  rebuildTrackComponents();
  startTimerHz(30);
}

ArrangerComponent::~ArrangerComponent() {
  stopTimer();
  projectState.getState().removeListener(this);
}

void ArrangerComponent::rebuildTrackComponents() {
  // Disabled for now: ArrangerTrackComponent depends on a stale grid API.
  // Keep component list empty until the arranger module is unified.
  trackComponents.clear();
}

void ArrangerComponent::resized() {
  auto bounds = getLocalBounds();

  if (macroToolbar) {
    macroToolbar->setBounds(bounds.removeFromTop(40));
  }

  if (freezeOverlay) {
    freezeOverlay->setBounds(getLocalBounds());
  }

  const int trackHeight = 64;
  for (size_t i = 0; i < trackComponents.size(); ++i) {
    auto y = 40 + static_cast<int>(i) * trackHeight;
    trackComponents[i]->setBounds(0, y, getWidth(), trackHeight);
  }
}

void ArrangerComponent::mouseDown(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
}

void ArrangerComponent::mouseDrag(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
}

void ArrangerComponent::mouseUp(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
}

void ArrangerComponent::mouseMove(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
}

void ArrangerComponent::mouseDoubleClick(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
}

void ArrangerComponent::mouseWheelMove(const juce::MouseEvent &e,
                                       const juce::MouseWheelDetails &wheel) {
  juce::ignoreUnused(e, wheel);
}

void ArrangerComponent::valueTreePropertyChanged(
    juce::ValueTree &tree, const juce::Identifier &property) {
  juce::ignoreUnused(tree, property);
  rebuildTrackComponents();
  repaint();
}

void ArrangerComponent::valueTreeChildAdded(juce::ValueTree &parent,
                                            juce::ValueTree &child) {
  juce::ignoreUnused(parent, child);
  rebuildTrackComponents();
  resized();
  repaint();
}

void ArrangerComponent::valueTreeChildRemoved(juce::ValueTree &parent,
                                              juce::ValueTree &child,
                                              int index) {
  juce::ignoreUnused(parent, child, index);
  rebuildTrackComponents();
  resized();
  repaint();
}

void ArrangerComponent::valueTreeChildOrderChanged(juce::ValueTree &parent,
                                                   int oldIndex,
                                                   int newIndex) {
  juce::ignoreUnused(parent, oldIndex, newIndex);
  rebuildTrackComponents();
  resized();
  repaint();
}

void ArrangerComponent::drawSkia(SkCanvas *canvas) {
  if (canvas == nullptr)
    return;

  SkPaint bg;
  bg.setColor(SkColorSetARGB(255, 16, 20, 28));
  canvas->drawRect(SkRect::MakeWH(static_cast<float>(getWidth()),
                                  static_cast<float>(getHeight())),
                   bg);
}

juce::String ArrangerComponent::getTooltip() {
  return "Arranger";
}

bool ArrangerComponent::isInterestedInDragSource(
    const juce::DragAndDropTarget::SourceDetails &details) {
  juce::ignoreUnused(details);
  return false;
}

void ArrangerComponent::itemDropped(
    const juce::DragAndDropTarget::SourceDetails &details) {
  juce::ignoreUnused(details);
}

void ArrangerComponent::itemDragEnter(
    const juce::DragAndDropTarget::SourceDetails &details) {
  juce::ignoreUnused(details);
}

void ArrangerComponent::itemDragExit(
    const juce::DragAndDropTarget::SourceDetails &details) {
  juce::ignoreUnused(details);
}

void ArrangerComponent::itemDragMove(
    const juce::DragAndDropTarget::SourceDetails &details) {
  juce::ignoreUnused(details);
}

void ArrangerComponent::setGridResolution(GridResolution res) {
  gridResolution_ = res;
  repaint();
}

void ArrangerComponent::timerCallback() {
  repaint();
}

} // namespace zenith
