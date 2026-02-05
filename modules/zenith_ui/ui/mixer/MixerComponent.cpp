/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/*
    ==============================================================================
    Original file header:
*/

 * @file MixerComponent.cpp
 * @brief Main mixer interface with horizontal scrolling and master strip
 */


#include "MixerChannelComponent.h"
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <core/SkCanvas.h>
#include <core/SkPaint.h>
#include <core/SkRRect.h>
#include <effects/SkGradientShader.h>
#include <core/SkColor.h>

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
  // Thread Safety: Constructor must be called from message thread
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

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
    masterChannel_ = std::make_unique<MixerChannelComponent>(masterTrack, projectState_, engine_, true);
    addAndMakeVisible(masterChannel_.get());
  }

  // Build initial track strips
  rebuildChannels();
  
  // Accessibility: Allow keyboard focus for navigation
  setWantsKeyboardFocus(true);
}

void MixerComponent::rebuildChannels() {
  trackContainer_->clearChannels();
  
  auto tracks = engine_.tracks();
  for (auto& t : tracks) {
      if (t->getType() != Track::Type::Master) {
        auto channel = std::make_unique<MixerChannelComponent>(t.get(), projectState_, engine_);
        trackContainer_->addChannel(std::move(channel));
      }
  }
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

  // 1. Draw Background with Glassmorphism
  GlassmorphicPanel::draw(canvas, skBounds, GlassmorphicPanel::Style::Subtle);

  // 2. Draw Top Border/Glow (Enhanced)
  SkPaint borderPaint;
  borderPaint.setColor(design::colors::BORDER_SUBTLE);
  borderPaint.setStrokeWidth(1.0f);
  borderPaint.setStyle(SkPaint::kStroke_Style);
  borderPaint.setAntiAlias(true);
  canvas->drawLine(skBounds.x(), skBounds.y(), skBounds.right(), skBounds.y(),
                   borderPaint);

  // 3. Draw divider between tracks and master
  if (masterChannel_) {
    float dividerX =
        bounds.getWidth() - (masterStripWidth + dividerWidth + sideMargin * 2);

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
    masterChannel_->setBounds(masterBounds.getX() + sideMargin + dividerWidth,
                              masterBounds.getY() + topMargin, masterStripWidth,
                              masterBounds.getHeight() - 2 * topMargin);
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
  // Thread Safety: Selection changes must happen on message thread
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  if (selectedTrackId_ == trackId)
    return;

  selectedTrackId_ = trackId;
  updateSelection();

  if (onSelectionChanged) {
    onSelectionChanged(trackId);
  }
}

void MixerComponent::updateSelection() {
  // Update selection state on all channels
  for (int i = 0; i < trackContainer_->getChannelCount(); ++i) {
    auto *channel = trackContainer_->getChannel(i);
    if (channel && channel->getTrack()) {
      channel->setSelected(channel->getTrack()->getTrackId() ==
                           selectedTrackId_);
    }
  }

  // Master channel is never "selected" in the same sense
  if (masterChannel_) {
    masterChannel_->setSelected(false);
  }
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
// Keyboard Navigation
//==============================================================================

bool MixerComponent::keyPressed(const juce::KeyPress& key, juce::Component* origin) {
  juce::ignoreUnused(origin);

  if (key == juce::KeyPress::leftKey || key == juce::KeyPress::upKey) {
    selectPreviousChannel();
    return true;
  }
  
  if (key == juce::KeyPress::rightKey || key == juce::KeyPress::downKey) {
    selectNextChannel();
    return true;
  }
  
  if (key == juce::KeyPress::homeKey) {
    if (trackContainer_->getChannelCount() > 0) {
      if (auto* first = trackContainer_->getChannel(0)) {
        if (auto* t = first->getTrack()) {
          selectChannel(t->getTrackId());
          trackViewport_.setViewPosition(0, 0);
        }
      }
    }
    return true;
  }
  
  if (key == juce::KeyPress::endKey) {
    if (masterChannel_) {
      if (auto* t = masterChannel_->getTrack()) {
        selectChannel(t->getTrackId());
      }
    }
    return true;
  }
  
  return false;
}

int MixerComponent::getSelectedChannelIndex() const {
  if (masterChannel_ && masterChannel_->isSelected()) {
    return trackContainer_->getChannelCount(); // Index = N means master
  }
  
  for (int i = 0; i < trackContainer_->getChannelCount(); ++i) {
    auto* ch = trackContainer_->getChannel(i);
    if (ch && ch->isSelected()) return i;
  }
  
  return -1;
}

void MixerComponent::selectNextChannel() {
  int current = getSelectedChannelIndex();
  int numTracks = trackContainer_->getChannelCount();
  
  // If nothing selected, select first
  if (current == -1) {
    if (numTracks > 0) {
      if (auto* ch = trackContainer_->getChannel(0)) {
        if (auto* t = ch->getTrack()) selectChannel(t->getTrackId());
      }
    } else if (masterChannel_) {
      if (auto* t = masterChannel_->getTrack()) selectChannel(t->getTrackId());
    }
    return;
  }
  
  // If master selected, do nothing (it's the end)
  if (current >= numTracks) return;
  
  // If last track selected, go to master
  if (current == numTracks - 1) {
    if (masterChannel_) {
      if (auto* t = masterChannel_->getTrack()) selectChannel(t->getTrackId());
    }
    return;
  }
  
  // Otherwise select next track
  if (current < numTracks - 1) {
    if (auto* ch = trackContainer_->getChannel(current + 1)) {
      if (auto* t = ch->getTrack()) {
        selectChannel(t->getTrackId());
        
        // Auto-scroll
        if (ch) {
          int x = ch->getX();
          int w = ch->getWidth();
          int vx = trackViewport_.getViewPositionX();
          int vw = trackViewport_.getViewWidth();
          
          if (x + w > vx + vw) {
            trackViewport_.setViewPosition(x + w - vw + sideMargin, 0);
          } else if (x < vx) {
            trackViewport_.setViewPosition(x - sideMargin, 0);
          }
        }
      }
    }
  }
}

void MixerComponent::selectPreviousChannel() {
  int current = getSelectedChannelIndex();
  int numTracks = trackContainer_->getChannelCount();
  
  // If nothing selected, select last (or master)
  if (current == -1) {
    if (masterChannel_) {
      if (auto* t = masterChannel_->getTrack()) selectChannel(t->getTrackId());
    } else if (numTracks > 0) {
      if (auto* ch = trackContainer_->getChannel(numTracks - 1)) {
        if (auto* t = ch->getTrack()) selectChannel(t->getTrackId());
      }
    }
    return;
  }
  
  // If master selected, go to last track
  if (current >= numTracks) {
    if (numTracks > 0) {
      if (auto* ch = trackContainer_->getChannel(numTracks - 1)) {
        if (auto* t = ch->getTrack()) {
          selectChannel(t->getTrackId());
          int targetX = ch->getRight() - trackViewport_.getViewWidth() + sideMargin;
          trackViewport_.setViewPosition(std::max(0, targetX), 0);
        }
      }
    }
    return;
  }
  
  // If first track, do nothing
  if (current <= 0) return;
  
  // Select previous track
  if (auto* ch = trackContainer_->getChannel(current - 1)) {
    if (auto* t = ch->getTrack()) {
      selectChannel(t->getTrackId());
      
      // Auto-scroll
      if (ch) {
        int x = ch->getX();
        int vx = trackViewport_.getViewPositionX();
        
        if (x < vx) {
            trackViewport_.setViewPosition(x - sideMargin, 0);
        } else if (x + ch->getWidth() > vx + trackViewport_.getViewWidth()) {
            trackViewport_.setViewPosition(x + ch->getWidth() - trackViewport_.getViewWidth() + sideMargin, 0);
        }
      }
    }
  }
}

//==============================================================================
// ValueTree::Listener Implementation
//==============================================================================

void MixerComponent::valueTreePropertyChanged(
    juce::ValueTree &tree, const juce::Identifier &property) {
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
