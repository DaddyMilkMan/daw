/**
 * @file ArrangerComponent.cpp
 * @brief Timeline/Arranger view implementation - Core component and event
 * dispatch
 */

#include "ArrangerComponent.h"
#include "../../engine/ProjectState.h"
#include "ArrangerClipManager.h"
#include "ArrangerGridUtils.h"
#include "ArrangerInputHandler.h"
#include "ArrangerTrackComponent.h"
#include <memory>
#include <vector>

#ifdef ZENITH_USE_SKIA
#include "ArrangerRenderer.h"
#endif

// Zenith Includes
#include "../../browser/BrowserDragSource.h"
#include "ZenithDesignSystem.h"

// JUCE Includes
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

namespace zenith {

//==============================================================================
// Layout Constants
//==============================================================================
static constexpr float HEADER_WIDTH = 220.0f;
static constexpr float SECTION_HEIGHT = 24.0f;
static constexpr float RULER_HEIGHT = 30.0f;
static constexpr float TRACK_HEIGHT = 80.0f;
static constexpr float TOP_MARGIN = SECTION_HEIGHT + RULER_HEIGHT;

//==============================================================================
// Constructor & Destructor
//==============================================================================

ArrangerComponent::ArrangerComponent(Engine &eng, ProjectState &ps)
    : engine_(eng), projectState(ps) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  setWantsKeyboardFocus(true);

  // Create helper modules
  gridUtils_ =
      std::make_unique<ArrangerGridUtils>(*this, engine_, projectState);
  clipManager_ =
      std::make_unique<ArrangerClipManager>(*this, projectState, *gridUtils_);
  inputHandler_ = std::make_unique<ArrangerInputHandler>(
      *this, projectState, *clipManager_, *gridUtils_);

#ifdef ZENITH_USE_SKIA
  renderer_ = std::make_unique<ArrangerRenderer>(*this, engine_, projectState,
                                                 *clipManager_, *gridUtils_);
#endif

  // Listen to ProjectState changes
  projectState.addListener(this);

  // Initial clip view build
  clipManager_->rebuildClipViews();

  // Smooth playhead animation driven by VBlank
  vBlankAttachment_ = std::make_unique<juce::VBlankAttachment>(
      this, [this] { updatePlayheadFromEngine(); });

  // Initialize Macro Toolbar
  macroToolbar = std::make_unique<MacroToolbar>(engine_, projectState);
  addChildComponent(macroToolbar.get());

  // Initialize MiniMap
  addAndMakeVisible(&miniMap);
  miniMap.setAlwaysOnTop(true);

  macroToolbar->getSelectedClipIds = [this]() {
    return clipManager_->getSelectedClipIds();
  };
  macroToolbar->getSelectedTrackId = [this]() {
    const auto &selectedIds = clipManager_->getSelectedClipIds();
    if (selectedIds.isEmpty())
      return juce::String();
    auto *view = clipManager_->findClipView(selectedIds[0]);
    return view ? view->trackId : juce::String();
  };

  // Initialize Section Track
  sectionTrack = std::make_unique<ArrangerTrackComponent>(
      projectState, ArrangerTrackComponent::TrackType::Section);
  addChildComponent(sectionTrack.get());

  rebuildTrackComponents();
}

ArrangerComponent::~ArrangerComponent() { projectState.removeListener(this); }

//==============================================================================
// Layout & Components
//==============================================================================

void ArrangerComponent::resized() {
  auto bounds = getLocalBounds();

  // Layout Macro Toolbar at the top
  if (macroToolbar != nullptr) {
    macroToolbar->setBounds(0, 0, bounds.getWidth(), 32);
  }

  // Layout MiniMap
  miniMap.setBounds(bounds.getWidth() - 300, 4, 290, 24);

  // Layout Section Track
  if (sectionTrack != nullptr) {
    sectionTrack->setVisible(true);
    sectionTrack->setBounds(0, static_cast<int>(RULER_HEIGHT),
                            bounds.getWidth(),
                            static_cast<int>(SECTION_HEIGHT));
    sectionTrack->setViewContext(pixelsPerBeat, viewStartBeats);
  }

  // Layout Tracks
  for (size_t i = 0; i < trackComponents.size(); ++i) {
    float y = gridUtils_->trackIndexToY(static_cast<int>(i));

    // Simple culling
    if (y + TRACK_HEIGHT < TOP_MARGIN || y > bounds.getHeight()) {
      trackComponents[i]->setVisible(false);
    } else {
      trackComponents[i]->setVisible(true);
      trackComponents[i]->setBounds(0, static_cast<int>(y), bounds.getWidth(),
                                    static_cast<int>(TRACK_HEIGHT));
      trackComponents[i]->setViewContext(pixelsPerBeat, viewStartBeats);
    }
  }

  if (renderer_ != nullptr) {
    markDirty();
  }
}

void ArrangerComponent::rebuildTrackComponents() {
  trackComponents.clear();

  auto tracksNode =
      projectState.getState().getChildWithName(zenith::ProjectState::ID_TRACKS);
  if (!tracksNode.isValid())
    return;

  for (int i = 0; i < tracksNode.getNumChildren(); ++i) {
    auto trackNode = tracksNode.getChild(i);
    auto trackComp = std::make_unique<ArrangerTrackComponent>(projectState);
    trackComp->setTrackId(
        trackNode.getProperty(ProjectState::PROP_ID).toString());
    trackComp->setTrackIndex(i);
    trackComp->syncWithState();

    addAndMakeVisible(trackComp.get());
    trackComponents.push_back(std::move(trackComp));
  }

  resized();
}

void ArrangerComponent::syncTrackComponents() {
  for (auto &trackComp : trackComponents) {
    trackComp->syncWithState();
  }
  markDirty();
}

//==============================================================================
// Mouse Events
//==============================================================================

void ArrangerComponent::mouseDown(const juce::MouseEvent &e) {
  if (inputHandler_)
    inputHandler_->mouseDown(e);
}

void ArrangerComponent::mouseDrag(const juce::MouseEvent &e) {
  if (inputHandler_)
    inputHandler_->mouseDrag(e);
}

void ArrangerComponent::mouseUp(const juce::MouseEvent &e) {
  if (inputHandler_)
    inputHandler_->mouseUp(e);
}

void ArrangerComponent::mouseMove(const juce::MouseEvent &e) {
  if (inputHandler_)
    inputHandler_->mouseMove(e);
}

void ArrangerComponent::mouseDoubleClick(const juce::MouseEvent &e) {
  if (inputHandler_)
    inputHandler_->mouseDoubleClick(e);
}

void ArrangerComponent::mouseWheelMove(const juce::MouseEvent &e,
                                       const juce::MouseWheelDetails &wheel) {
  // Zooming
  if (e.mods.isCommandDown() || e.mods.isAltDown()) {
    double oldPixelsPerBeat = pixelsPerBeat;
    if (wheel.deltaY > 0)
      pixelsPerBeat *= 1.1;
    else if (wheel.deltaY < 0)
      pixelsPerBeat /= 1.1;

    // Clamp zoom
    pixelsPerBeat = juce::jlimit(1.0, 500.0, pixelsPerBeat);

    // Zoom relative to mouse position
    double mouseBeats =
        (static_cast<double>(e.position.x) / oldPixelsPerBeat) + viewStartBeats;
    viewStartBeats =
        mouseBeats - (static_cast<double>(e.position.x) / pixelsPerBeat);

    resized();
    markDirty();
  } else {
    // Scrolling
    viewStartBeats -=
        (static_cast<double>(wheel.deltaX) * 10.0) / pixelsPerBeat;
    viewStartBeats = juce::jmax(0.0, viewStartBeats);

    markDirty();
    resized();
  }
}

//==============================================================================
// Rendering
//==============================================================================

void ArrangerComponent::drawSkia(SkCanvas *canvas) {
#ifdef ZENITH_USE_SKIA
  if (renderer_) {
    renderer_->drawSkia(canvas);
  }
#endif
}

//==============================================================================
// State Callbacks
//==============================================================================

void ArrangerComponent::valueTreePropertyChanged(
    juce::ValueTree &tree, const juce::Identifier &property) {
  if (tree.getType() == ProjectState::ID_TRACK) {
    syncTrackComponents();
  }
  markDirty();
}

void ArrangerComponent::valueTreeChildAdded(juce::ValueTree &parent,
                                            juce::ValueTree &child) {
  if (parent.getType() == ProjectState::ID_TRACKS) {
    rebuildTrackComponents();
  }
}

void ArrangerComponent::valueTreeChildRemoved(juce::ValueTree &parent,
                                              juce::ValueTree &child,
                                              int index) {
  if (parent.getType() == ProjectState::ID_TRACKS) {
    rebuildTrackComponents();
  }
}

void ArrangerComponent::valueTreeChildOrderChanged(juce::ValueTree &parent,
                                                   int oldIndex, int newIndex) {
  if (parent.getType() == ProjectState::ID_TRACKS) {
    rebuildTrackComponents();
  }
}

//==============================================================================
// Drag & Drop
//==============================================================================

bool ArrangerComponent::isInterestedInDragSource(
    const juce::DragAndDropTarget::SourceDetails &details) {
  return details.description == "BrowserFile";
}

void ArrangerComponent::itemDropped(
    const juce::DragAndDropTarget::SourceDetails &details) {
  isDropTargetActive_ = false;
  markDirty();
}

void ArrangerComponent::itemDragEnter(
    const juce::DragAndDropTarget::SourceDetails &details) {
  isDropTargetActive_ = true;
  markDirty();
}

void ArrangerComponent::itemDragExit(
    const juce::DragAndDropTarget::SourceDetails &details) {
  isDropTargetActive_ = false;
  markDirty();
}

void ArrangerComponent::itemDragMove(
    const juce::DragAndDropTarget::SourceDetails &details) {
  double currentBeats =
      (static_cast<double>(details.localPosition.x) / pixelsPerBeat) +
      viewStartBeats;
  int trackIndex =
      gridUtils_->yToTrackIndex(static_cast<float>(details.localPosition.y));

  dropTargetTrackIndex_ = trackIndex;
  dropTargetBeats_ = currentBeats;
  markDirty();
}

//==============================================================================
// Timer
//==============================================================================

void ArrangerComponent::timerCallback() { /* Legacy timer - replaced by VBlank
                                           */
}

void ArrangerComponent::updatePlayheadFromEngine() {
  double engineBeats = engine_.getPlaybackPositionBeats();
  double currentTime = juce::Time::getMillisecondCounterHiRes() * 0.001;

  if (std::abs(engineBeats - lastEngineBeats_) > 0.0001) {
    lastEngineBeats_ = engineBeats;
    lastEngineTime_ = currentTime;
  }

  double interpolatedBeats = lastEngineBeats_;
  if (engine_.isPlaying()) {
    double bpm = projectState.getTempo();
    double elapsedSeconds = currentTime - lastEngineTime_;
    double elapsedBeats = elapsedSeconds * (bpm / 60.0);
    interpolatedBeats += elapsedBeats;
  }

  if (std::abs(interpolatedBeats - playheadBeats_) > 0.0001) {
    playheadBeats_ = interpolatedBeats;

    if (followPlayhead_ && engine_.isPlaying()) {
      // Logic for keeping playhead in view could go here
    }

    markDirty();
  }
}

void ArrangerComponent::setGridResolution(GridResolution res) {
  gridResolution_ = res;
  gridSnapBeats = gridResolutionToBeats(res);
  markDirty();
}

juce::String ArrangerComponent::getTooltip() {
  return "Arranger View - Edit your timeline";
}

bool ArrangerComponent::keyPressed(const juce::KeyPress &key) { return false; }

} // namespace zenith
