/**
 * @file MixerComponent.cpp
 * @brief Full-featured Mixer component implementation
 *
 * Implements a professional-grade mixer view with:
 * - Horizontal scrolling for many tracks
 * - Master channel strip
 * - GPU-accelerated Skia rendering
 * - Glassmorphic panel design
 * - Selection glow on active channel
 */

#include "../../include/ui/MixerComponent.h"
#include "../../Source/ui/skia/GlassmorphicPanel.h"
#include "../../Source/ui/skia/ZenithDesignSystem.h"
#include "../../include/Engine.h"
#include "../../include/ui/MixerChannelComponent.h"
#include "../engine/Track.h"

#include <core/SkCanvas.h>
#include <core/SkPaint.h>
#include <core/SkRRect.h>
#include <effects/SkGradientShader.h>
#include <include/core/SkColor.h>

namespace zenith {

//==============================================================================
// ChannelContainer Implementation
//==============================================================================

MixerComponent::ChannelContainer::ChannelContainer() { setOpaque(false); }

void MixerComponent::ChannelContainer::drawSkia(SkCanvas *canvas) {
  // Transparent container - just draws children
  drawChildren(canvas);
}

void MixerComponent::ChannelContainer::resized() {
  // Layout is handled by layoutChannels()
}

void MixerComponent::ChannelContainer::addChannel(
    std::unique_ptr<MixerChannelComponent> channel) {
  addAndMakeVisible(channel.get());
  channels_.push_back(std::move(channel));
}

void MixerComponent::ChannelContainer::clearChannels() { channels_.clear(); }

MixerChannelComponent *MixerComponent::ChannelContainer::getChannel(int index) {
  if (index >= 0 && index < static_cast<int>(channels_.size())) {
    return channels_[static_cast<size_t>(index)].get();
  }
  return nullptr;
}

void MixerComponent::ChannelContainer::layoutChannels(int stripWidth,
                                                      int stripSpacing,
                                                      int topMargin,
                                                      int bottomMargin) {
  int x = 0;
  for (auto &channel : channels_) {
    channel->setBounds(x, topMargin, stripWidth,
                       getHeight() - topMargin - bottomMargin);
    x += stripWidth + stripSpacing;
  }
}

int MixerComponent::ChannelContainer::getTotalWidth(int stripWidth,
                                                    int stripSpacing,
                                                    int sideMargin) const {
  if (channels_.empty())
    return sideMargin * 2;
  return sideMargin +
         static_cast<int>(channels_.size()) * (stripWidth + stripSpacing) -
         stripSpacing + sideMargin;
}

//==============================================================================
// MixerComponent Implementation
//==============================================================================

MixerComponent::MixerComponent(Engine &engine, ProjectState &state)
    : engine_(engine), projectState_(state) {
  // Listen to the entire state tree for changes
  projectState_.getState().addListener(this);

  // Create channel container for viewport
  trackContainer_ = std::make_unique<ChannelContainer>();

  // Setup viewport for horizontal scrolling
  trackViewport_.setViewedComponent(trackContainer_.get(),
                                    false);       // Don't own it
  trackViewport_.setScrollBarsShown(false, true); // Horizontal scrollbar only
  trackViewport_.getHorizontalScrollBar().setColour(
      juce::ScrollBar::thumbColourId, juce::Colour(design::colors::CYAN));
  trackViewport_.setScrollBarThickness(8);
  addAndMakeVisible(trackViewport_);

  // Create master channel strip (find master track from engine's track list)
  Track *masterTrack = nullptr;
  for (const auto &track : engine_.tracks()) {
    if (track->getType() == Track::Type::Master) {
      masterTrack = track.get();
      break;
    }
  }
  if (masterTrack) {
    masterChannel_ = std::make_unique<MixerChannelComponent>(masterTrack, true);
    addAndMakeVisible(masterChannel_.get());
  }

  // Build initial track strips
  rebuildChannels();
}

MixerComponent::~MixerComponent() {
  projectState_.getState().removeListener(this);
  trackContainer_->clearChannels();
  masterChannel_.reset();
}

//==============================================================================
// Rendering
//==============================================================================

void MixerComponent::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();
  SkRect skBounds = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());

  // 1. Draw Background with Gradient
  GlassmorphicPanel::fillBackground(canvas, skBounds);

  // 2. Draw Top Border/Glow
  SkPaint borderPaint;
  borderPaint.setColor(SkColorSetARGB(100, 255, 255, 255));
  borderPaint.setStrokeWidth(1.0f);
  borderPaint.setStyle(SkPaint::kStroke_Style);
  borderPaint.setAntiAlias(true);
  canvas->drawLine(skBounds.x(), skBounds.y(), skBounds.right(), skBounds.y(),
                   borderPaint);

  // 3. Draw divider between tracks and master
  if (masterChannel_) {
    float dividerX =
        bounds.getWidth() - masterStripWidth - dividerWidth - sideMargin;

    // Gradient divider line
    SkPoint dividerPts[2] = {
        {dividerX, static_cast<float>(topMargin)},
        {dividerX, bounds.getHeight() - static_cast<float>(bottomMargin)}};
    SkColor dividerColors[3] = {
        SkColorSetARGB(0, 255, 255, 255), // Transparent at top
        design::colors::BORDER_STRONG,    // Visible in middle
        SkColorSetARGB(0, 255, 255, 255)  // Transparent at bottom
    };
    SkScalar positions[3] = {0.0f, 0.5f, 1.0f};

    SkPaint dividerPaint;
    dividerPaint.setShader(SkGradientShader::MakeLinear(
        dividerPts, dividerColors, positions, 3, SkTileMode::kClamp));
    dividerPaint.setStrokeWidth(static_cast<float>(dividerWidth));
    dividerPaint.setAntiAlias(true);
    canvas->drawLine(dividerX, static_cast<float>(topMargin), dividerX,
                     bounds.getHeight() - static_cast<float>(bottomMargin),
                     dividerPaint);

    // "MASTER" label above divider
    SkPaint labelPaint;
    labelPaint.setColor(design::colors::TEXT_TERTIARY);
    labelPaint.setAntiAlias(true);
    SkFont labelFont =
        design::typography::getSkFont(10.0f, design::FontWeight::Medium);
    canvas->drawString("MASTER", dividerX + 8, 20, labelFont, labelPaint);
  }

  // 4. Draw "TRACKS" label
  {
    SkPaint labelPaint;
    labelPaint.setColor(design::colors::TEXT_TERTIARY);
    labelPaint.setAntiAlias(true);
    SkFont labelFont =
        design::typography::getSkFont(10.0f, design::FontWeight::Medium);
    canvas->drawString("TRACKS", static_cast<float>(sideMargin), 20, labelFont,
                       labelPaint);
  }

  // 5. Children are drawn automatically by JUCE/Skia
  // (Viewport and master channel are JUCE components, drawn separately)

  // 6. Empty state message
  if (trackContainer_->getChannelCount() == 0) {
    SkPaint textPaint;
    textPaint.setColor(SkColorSetARGB(100, 255, 255, 255));
    textPaint.setAntiAlias(true);
    SkFont textFont =
        design::typography::getSkFont(18.0f, design::FontWeight::Regular);

    float textX = bounds.getWidth() / 2 - 80;
    float textY = bounds.getHeight() / 2;
    canvas->drawString("No Tracks - Add a track to begin", textX, textY,
                       textFont, textPaint);
  }
}

void MixerComponent::resized() {
  auto bounds = getLocalBounds();

  // Reserve space for master channel on the right
  int masterArea = 0;
  if (masterChannel_) {
    masterArea = masterStripWidth + dividerWidth + sideMargin * 2;
    auto masterBounds = bounds.removeFromRight(masterArea);
    masterChannel_->setBounds(masterBounds.reduced(sideMargin, topMargin));
  }

  // Viewport takes remaining space
  bounds.removeFromTop(topMargin);
  bounds.removeFromBottom(bottomMargin);
  bounds.removeFromLeft(sideMargin);
  bounds.removeFromRight(sideMargin);

  trackViewport_.setBounds(bounds);

  // Size the container to fit all channels
  int containerWidth =
      trackContainer_->getTotalWidth(stripWidth, stripSpacing, 0);
  int containerHeight = bounds.getHeight();
  trackContainer_->setSize(juce::jmax(containerWidth, bounds.getWidth()),
                           containerHeight);

  // Layout the channels within the container
  trackContainer_->layoutChannels(stripWidth, stripSpacing, 0, 0);
}

//==============================================================================
// Selection Management
//==============================================================================

void MixerComponent::selectChannel(const juce::String &trackId) {
  // Update Project State
  auto &state = projectState_.getState();
  if (state[ProjectState::PROP_SELECTED_TRACK_ID].toString() != trackId) {
    state.setProperty(ProjectState::PROP_SELECTED_TRACK_ID, trackId,
                      &projectState_.getUndoManager());
  }

  // Local update will happen via listener callback
}

void MixerComponent::updateSelection() {
  // Read from Project State
  auto selectedId =
      projectState_.getState()[ProjectState::PROP_SELECTED_TRACK_ID].toString();

  // Update selection state on all channels
  for (int i = 0; i < trackContainer_->getChannelCount(); ++i) {
    auto *channel = trackContainer_->getChannel(i);
    if (channel && channel->getTrack()) {
      channel->setSelected(channel->getTrack()->getTrackId() == selectedId);
    }
  }

  // Master channel is never "selected" in the same sense
  if (masterChannel_) {
    masterChannel_->setSelected(false);
  }
}

//==============================================================================
// Channel Rebuilding
//==============================================================================

void MixerComponent::rebuildChannels() {
  trackContainer_->clearChannels();

  // Iterate tracks from ProjectState to maintain order
  auto tracksNode =
      projectState_.getState().getChildWithName(ProjectState::ID_TRACKS);
  if (!tracksNode.isValid())
    return;

  for (const auto &trackNode : tracksNode) {
    juce::String trackId = trackNode[ProjectState::PROP_ID].toString();

    Track *track = findTrackById(trackId);
    if (track) {
      auto channel = std::make_unique<MixerChannelComponent>(track, false);

      // Setup click handler for selection
      channel->onClick = [this, trackId]() { selectChannel(trackId); };

      trackContainer_->addChannel(std::move(channel));
    }
  }

  resized();
  updateSelection();
  repaint();
}

Track *MixerComponent::findTrackById(const juce::String &trackId) {
  const auto &tracks = engine_.tracks();
  for (const auto &track : tracks) {
    if (track->getTrackId() == trackId) {
      return track.get();
    }
  }
  return nullptr;
}

//==============================================================================
// ValueTree::Listener Implementation
//==============================================================================

void MixerComponent::valueTreePropertyChanged(
    juce::ValueTree &tree, const juce::Identifier &property) {

  if (tree.hasType(ProjectState::ID_PROJECT) &&
      property == ProjectState::PROP_SELECTED_TRACK_ID) {
    updateSelection();
    repaint();
    return;
  }

  // Property changes are handled by individual MixerChannelComponents
  // via their Track listeners. No action needed here.
  juce::ignoreUnused(tree, property);
}

void MixerComponent::valueTreeChildAdded(juce::ValueTree &parent,
                                         juce::ValueTree &child) {
  if (parent.getType() == ProjectState::ID_TRACKS) {
    rebuildChannels();
  }
  juce::ignoreUnused(child);
}

void MixerComponent::valueTreeChildRemoved(juce::ValueTree &parent,
                                           juce::ValueTree &child, int index) {
  if (parent.getType() == ProjectState::ID_TRACKS) {
    rebuildChannels();
  }
  juce::ignoreUnused(child, index);
}

void MixerComponent::valueTreeChildOrderChanged(juce::ValueTree &parent,
                                                int oldIndex, int newIndex) {
  if (parent.getType() == ProjectState::ID_TRACKS) {
    rebuildChannels();
  }
  juce::ignoreUnused(oldIndex, newIndex);
}

void MixerComponent::valueTreeParentChanged(juce::ValueTree &tree) {
  juce::ignoreUnused(tree);
}

} // namespace zenith
