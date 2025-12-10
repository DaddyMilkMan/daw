/**
 * @file MixerComponent.cpp
 * @brief Mixer component implementation
 */

#include "../../include/ui/MixerComponent.h"
#include "../../Source/ui/skia/ZenithDesignSystem.h"
#include "../../include/Engine.h"
#include "../../include/ui/MixerChannelComponent.h"
#include "../engine/Track.h"

// Check for Skia availability
#ifdef ZENITH_USE_SKIA
#include <core/SkCanvas.h>
#include <core/SkPaint.h>
#include <core/SkRRect.h>
#include <effects/SkGradientShader.h>
#endif

namespace zenith {

//==============================================================================
MixerComponent::MixerComponent(Engine &engine, ProjectState &state)
    : engine_(engine), projectState_(state) {
  // Listen to the entire state tree for changes
  projectState_.getState().addListener(this);

  // Build initial track strips
  rebuildChannels();
}

MixerComponent::~MixerComponent() {
  // Stop listening
  projectState_.getState().removeListener(this);
  channels_.clear();
}

//==============================================================================
// Component interface
//==============================================================================

void MixerComponent::paint(juce::Graphics &g) {
  // Basic background for JUCE fallback
  g.fillAll(juce::Colour(0xff1e1e1e));
}

void MixerComponent::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();

  // Use Zenith Design System for background
  // ZenithDesignSystem::drawPanel(g, ...) logic via Skia directly

  // 1. Draw Background with Gradient
  SkRect skBounds = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());

  SkPoint gradientPoints[2] = {
      {skBounds.x(), skBounds.y()},
      {skBounds.x(), skBounds.y() + skBounds.height()}};

  SkColor gradientColors[2] = {
      SkColorSetARGB(255, 30, 30, 35), // Dark grey/blue top
      SkColorSetARGB(255, 20, 20, 25)  // Darker bottom
  };

  auto gradient = SkGradientShader::MakeLinear(gradientPoints, gradientColors,
                                               nullptr, 2, SkTileMode::kClamp);

  SkPaint bgPaint;
  bgPaint.setShader(gradient);
  canvas->drawRect(skBounds, bgPaint);

  // 2. Draw Top Border/Glow
  SkPaint borderPaint;
  borderPaint.setColor(SkColorSetARGB(100, 255, 255, 255));
  borderPaint.setStrokeWidth(1.0f);
  borderPaint.setStyle(SkPaint::kStroke_Style);
  borderPaint.setAntiAlias(true);

  canvas->drawLine(skBounds.x(), skBounds.y(), skBounds.right(), skBounds.y(),
                   borderPaint);

  // 3. Children are drawn automatically by SkiaComponent
  drawChildren(canvas);

  // 4. Empty state
  if (channels_.empty()) {
    SkPaint textPaint;
    textPaint.setColor(SkColorSetARGB(100, 255, 255, 255));
    textPaint.setAntiAlias(true);
    SkFont textFont(nullptr, 24.0f);
    canvas->drawString("No Tracks", skBounds.centerX() - 50, skBounds.centerY(),
                       textFont, textPaint);
  }
}

void MixerComponent::resized() {
  auto bounds = getLocalBounds();
  int x = sideMargin;

  for (auto &channel : channels_) {
    channel->setBounds(x, topMargin, stripWidth,
                       bounds.getHeight() - topMargin - bottomMargin);
    x += stripWidth + stripSpacing;
  }
}

//==============================================================================
// Internal logic
//==============================================================================

void MixerComponent::rebuildChannels() {
  channels_.clear();

  // Iterate tracks from ProjectState to maintain order
  auto tracksNode =
      projectState_.getState().getChildWithName(ProjectState::ID_TRACKS);
  if (!tracksNode.isValid())
    return;

  for (const auto &trackNode : tracksNode) {
    juce::String trackId = trackNode[ProjectState::PROP_ID].toString();

    Track *track = findTrackById(trackId);
    if (track) {
      auto channel = std::make_unique<MixerChannelComponent>(track);
      addAndMakeVisible(channel.get());
      channels_.push_back(std::move(channel));
    }
  }

  resized();
  repaint();
}

Track *MixerComponent::findTrackById(const juce::String &trackId) {
  // Safe message-thread iteration of Engine tracks
  // Engine::tracks() returns const ref to vector<shared_ptr<Track>>
  const auto &tracks = engine_.tracks();
  for (const auto &track : tracks) {
    if (track->getTrackId() == trackId) {
      return track.get();
    }
  }
  return nullptr;
}

//==============================================================================
// ValueTree::Listener
//==============================================================================

void MixerComponent::valueTreePropertyChanged(
    juce::ValueTree &tree, const juce::Identifier &property) {
  // Check if name changed? MixerChannelComponent mostly handles its own updates
  // via Track listeners, but if the structure changes (e.g. tracks reordered?)
  // Actually MixerChannel handle name changes via Track listener.
  // So we might not need much here, unless it affects layout (e.g. new track).
}

void MixerComponent::valueTreeChildAdded(juce::ValueTree &parent,
                                         juce::ValueTree &child) {
  if (parent.getType() == ProjectState::ID_TRACKS) {
    rebuildChannels();
  }
}

void MixerComponent::valueTreeChildRemoved(juce::ValueTree &parent,
                                           juce::ValueTree &child, int index) {
  if (parent.getType() == ProjectState::ID_TRACKS) {
    rebuildChannels();
  }
}

void MixerComponent::valueTreeChildOrderChanged(juce::ValueTree &parent,
                                                int oldIndex, int newIndex) {
  if (parent.getType() == ProjectState::ID_TRACKS) {
    rebuildChannels();
  }
}

void MixerComponent::valueTreeParentChanged(juce::ValueTree &tree) {}

} // namespace zenith
