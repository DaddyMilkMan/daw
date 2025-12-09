/*
  ==============================================================================
*/
#include "../../include/ui/MixerComponent.h"
<<<<<<< HEAD
#include "../ui/ZenithLookAndFeel.h"
#include "../../Source/engine/Track.h"

#include <include/core/SkCanvas.h>
#include <include/core/SkFont.h>
#include <include/core/SkPaint.h>
#include <include/core/SkPath.h>
#include <include/core/SkRRect.h>
#include <include/effects/SkGradientShader.h>
#include "../ui/skia/SkiaTheme.h"

//==============================================================================
MixerComponent::MixerComponent(zenith::Engine &engine, zenith::ProjectState &ps) : projectState(ps) {
=======
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
>>>>>>> origin/master
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

<<<<<<< HEAD
=======
void MixerComponent::paint(juce::Graphics &g) {
  // Basic background for JUCE fallback
  g.fillAll(juce::Colour(0xff1e1e1e));
}

>>>>>>> origin/master
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

<<<<<<< HEAD
void MixerComponent::drawTrackStripSkia(SkCanvas *canvas, SkRect stripBounds,
                                        const TrackStrip &strip) {
  canvas->save();

  // Strip Background
  SkRRect roundedStrip;
  roundedStrip.setRectXY(stripBounds, 4.0f, 4.0f);

  SkPaint stripBgPaint;
  stripBgPaint.setColor(SkColorSetARGB(255, 40, 40, 40));
  canvas->drawRRect(roundedStrip, stripBgPaint);

  SkPaint stripBorderPaint;
  stripBorderPaint.setColor(SkColorSetARGB(255, 60, 60, 60));
  stripBorderPaint.setStyle(SkPaint::kStroke_Style);
  stripBorderPaint.setStrokeWidth(1.0f);
  stripBorderPaint.setAntiAlias(true);
  canvas->drawRRect(roundedStrip, stripBorderPaint);

  // Track Name
  SkFont nameFont;
  nameFont.setSize(12);
  nameFont.setEdging(SkFont::Edging::kAntiAlias);

  SkPaint namePaint;
  namePaint.setColor(SK_ColorWHITE);
  namePaint.setAntiAlias(true);

  juce::String displayName = strip.trackName;
  if (displayName.length() > 10)
    displayName = displayName.substring(0, 9) + "...";
  const char *nameStr = displayName.toRawUTF8();

  SkRect nameBounds;
  nameFont.measureText(nameStr, strlen(nameStr), SkTextEncoding::kUTF8,
                       &nameBounds);
  canvas->drawString(nameStr, stripBounds.centerX() - nameBounds.width() / 2.0f,
                     stripBounds.y() + 20.0f, nameFont, namePaint);

  canvas->restore();
}

//==============================================================================

=======
>>>>>>> origin/master
void MixerComponent::resized() {
  auto bounds = getLocalBounds();
  int x = sideMargin;

<<<<<<< HEAD
  auto bounds = getLocalBounds().reduced(ZenithLookAndFeel::Spacing::m);
  int x = 0;
  const int localStripWidth = 80; // Fixed strip width
  const int localStripSpacing = ZenithLookAndFeel::Spacing::s;

  for (auto &strip : trackStrips) {
    if (!strip)
      continue;

    // Allocate space for this strip
    strip->bounds = juce::Rectangle<int>(bounds.getX() + x, bounds.getY(),
                                         localStripWidth, bounds.getHeight());

    // Layout components within the strip
    auto area = strip->bounds;

    // 1. Track Name (Top)
    auto nameArea = area.removeFromTop(20);
#ifndef ZENITH_USE_SKIA
    if (strip->nameLabel) strip->nameLabel->setBounds(nameArea);

    // 2. Arm Button (Below Name)
    auto armArea = area.removeFromTop(25);
    if (strip->armButton)
        strip->armButton->setBounds(armArea.reduced(10, 2));

    // 5. Mute/Solo Buttons (Bottom)
    auto buttonArea = area.removeFromBottom(30);
    auto muteArea = buttonArea.removeFromLeft(buttonArea.getWidth() / 2);
    if (strip->muteButton) strip->muteButton->setBounds(muteArea.reduced(2));
    if (strip->soloButton) strip->soloButton->setBounds(buttonArea.reduced(2));

    // 4. Pan Slider (Above Buttons)
    auto panArea = area.removeFromBottom(60);
    if (strip->panSlider)
        strip->panSlider->setBounds(panArea.reduced(ZenithLookAndFeel::Spacing::s));

    // 3. Volume Slider (Remaining Middle)
    if (strip->volumeSlider)
        strip->volumeSlider->setBounds(area.reduced(ZenithLookAndFeel::Spacing::s, 0));

    x += localStripWidth + localStripSpacing;
=======
  for (auto &channel : channels_) {
    channel->setBounds(x, topMargin, stripWidth,
                       bounds.getHeight() - topMargin - bottomMargin);
    x += stripWidth + stripSpacing;
>>>>>>> origin/master
  }
}

//==============================================================================
<<<<<<< HEAD

void MixerComponent::valueTreePropertyChanged(
    juce::ValueTree &treeWhosePropertyHasChanged,
    const juce::Identifier &property) {
  // Check if this is a track node
  if (treeWhosePropertyHasChanged.getType() != zenith::ProjectState::ID_TRACK)
    return;

  // Get track ID
  auto trackId = treeWhosePropertyHasChanged[zenith::ProjectState::PROP_ID].toString();

  if (trackId.isEmpty())
    return;

  // Find the corresponding track strip
  auto *strip = findTrackStrip(trackId);

  if (!strip)
    return;

  // Update the strip from state (only if the property changed is relevant)
  if (property == zenith::ProjectState::PROP_NAME ||
      property == zenith::ProjectState::PROP_VOLUME ||
      property == zenith::ProjectState::PROP_PAN ||
      property == zenith::ProjectState::PROP_MUTE ||
      property == zenith::ProjectState::PROP_SOLO ||
      property == zenith::ProjectState::PROP_ARMED) {
    updatingFromState = true;
    updateTrackStripFromState(*strip, treeWhosePropertyHasChanged);
    updatingFromState = false;
  }
}

void MixerComponent::valueTreeChildAdded(
    juce::ValueTree &parentTree, juce::ValueTree &childWhichHasBeenAdded) {
  // Check if a track was added
  if (parentTree.getType() == zenith::ProjectState::ID_TRACKS &&
      childWhichHasBeenAdded.getType() == zenith::ProjectState::ID_TRACK) {
    
    // Incremental update: insert new strip at correct position rather than rebuilding
    // This provides O(1) insertion instead of O(N) rebuild
    int index = parentTree.indexOf(childWhichHasBeenAdded);
    
    if (index >= 0) {
        auto strip = createTrackStrip(childWhichHasBeenAdded);
        if (strip) {
            // Ensure index is valid for vector
            index = juce::jmin(index, static_cast<int>(trackStrips.size()));
            trackStrips.insert(trackStrips.begin() + index, std::move(strip));
            
            DBG("MixerComponent: Incrementally added track strip at index " + juce::String(index));
            
            resized();
            repaint();
        }
    } else {
        // Fallback if index not found (shouldn't happen)
        rebuildTrackStrips();
        resized();
        repaint();
    }
  }
}

void MixerComponent::valueTreeChildRemoved(
    juce::ValueTree &parentTree, juce::ValueTree &childWhichHasBeenRemoved,
    int indexFromWhichChildWasRemoved) {
  // Check if a track was removed
  if (parentTree.getType() == zenith::ProjectState::ID_TRACKS &&
      childWhichHasBeenRemoved.getType() == zenith::ProjectState::ID_TRACK) {
    
    // Incremental update: remove strip directly instead of full rebuild
    if (indexFromWhichChildWasRemoved >= 0 && 
        indexFromWhichChildWasRemoved < static_cast<int>(trackStrips.size())) {
        
        trackStrips.erase(trackStrips.begin() + indexFromWhichChildWasRemoved);
        
        DBG("MixerComponent: Incrementally removed track strip at index " + juce::String(indexFromWhichChildWasRemoved));
        
        resized();
        repaint();
    } else {
        // Fallback if index invalid
        rebuildTrackStrips();
        resized();
        repaint();
    }
  }
}

void MixerComponent::valueTreeChildOrderChanged(
    juce::ValueTree &parentTreeWhoseChildrenHaveMoved, int oldIndex,
    int newIndex) {
  // If tracks were reordered, rebuild
  if (parentTreeWhoseChildrenHaveMoved.getType() == zenith::ProjectState::ID_TRACKS) {
    rebuildTrackStrips();
    resized();
    repaint();
  }
}

//==============================================================================
// Helper methods
=======
// Internal logic
>>>>>>> origin/master
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

<<<<<<< HEAD
std::unique_ptr<MixerComponent::TrackStrip>
MixerComponent::createTrackStrip(const juce::ValueTree &trackNode) {
  auto strip = std::make_unique<TrackStrip>();

  // Get track info
  strip->trackId = trackNode[zenith::ProjectState::PROP_ID].toString();
  strip->trackName = trackNode[zenith::ProjectState::PROP_NAME].toString();

  if (strip->trackId.isEmpty())
    return nullptr;

  // --- SKIA CONTROLS ---

  // Volume Slider (Vertical)
  strip->volumeSlider = std::make_unique<zenith::ZenithSlider>("Vol");
  strip->volumeSlider->setRange(0.0, 1.0);
  strip->volumeSlider->setValue(trackNode[zenith::ProjectState::PROP_VOLUME], false);
  
  // Capture raw pointer to slider safely since the slider owns the callback but we need its value
  auto* volSlider = strip->volumeSlider.get();
  strip->volumeSlider->onValueChange = [this, trackId = strip->trackId, volSlider]() {
    if (!updatingFromState) onVolumeChanged(trackId, volSlider->getValue());
  };
  addAndMakeVisible(*strip->volumeSlider);

  // Pan Knob
  strip->panSlider = std::make_unique<zenith::ZenithKnob>("Pan");
  strip->panSlider->setRange(-1.0, 1.0);
  // Display range not supported on ZenithKnob
  strip->panSlider->setValue(trackNode[zenith::ProjectState::PROP_PAN], false);
  
  auto* panKnob = strip->panSlider.get();
  strip->panSlider->onValueChange = [this, trackId = strip->trackId, panKnob]() {
    if (!updatingFromState) onPanChanged(trackId, panKnob->getValue());
  };
  addAndMakeVisible(*strip->panSlider);

  // Mute Button
  strip->muteButton = std::make_unique<zenith::ZenithButton>("M");
  strip->muteButton->setToggleable(true);
  strip->muteButton->setToggleState(trackNode[zenith::ProjectState::PROP_MUTE], false);
  strip->muteButton->setButtonStyle(zenith::ZenithButton::Danger); // Red when muted? Or standard?
  strip->muteButton->onClick = [this, trackId = strip->trackId, btn = strip->muteButton.get()]() {
    if (!updatingFromState) onMuteClicked(trackId, btn->getToggleState());
  };
  addAndMakeVisible(*strip->muteButton);

  // Solo Button
  strip->soloButton = std::make_unique<zenith::ZenithButton>("S");
  strip->soloButton->setToggleable(true);
  strip->soloButton->setToggleState(trackNode[zenith::ProjectState::PROP_SOLO], false);
  strip->soloButton->setButtonStyle(zenith::ZenithButton::Warning); // Yellow/Amber
  strip->soloButton->onClick = [this, trackId = strip->trackId, btn = strip->soloButton.get()]() {
    if (!updatingFromState) onSoloClicked(trackId, btn->getToggleState());
  };
  addAndMakeVisible(*strip->soloButton);

  // Arm Button
  strip->armButton = std::make_unique<zenith::ZenithButton>("R");
  strip->armButton->setToggleable(true);
  strip->armButton->setToggleState(trackNode[zenith::ProjectState::PROP_ARMED], false);
  strip->armButton->setButtonStyle(zenith::ZenithButton::Danger); // Red
  strip->armButton->onClick = [this, trackId = strip->trackId, btn = strip->armButton.get()]() {
    if (!updatingFromState) onArmClicked(trackId, btn->getToggleState());
  };
  addAndMakeVisible(*strip->armButton);

  return strip;
}

void MixerComponent::updateTrackStripFromState(
    TrackStrip &strip, const juce::ValueTree &trackNode) {
  // Update track name
  auto newName = trackNode[zenith::ProjectState::PROP_NAME].toString();
  if (newName != strip.trackName) {
    strip.trackName = newName;
    // In Skia mode, we just redraw
    repaint(); 
  }

  // Update volume slider
  if (strip.volumeSlider) {
    float volume = trackNode[zenith::ProjectState::PROP_VOLUME];
    if (std::abs(strip.volumeSlider->getValue() - volume) > 0.001)
      strip.volumeSlider->setValue(volume, false);
  }

  // Update pan slider
  if (strip.panSlider) {
    float pan = trackNode[zenith::ProjectState::PROP_PAN];
    if (std::abs(strip.panSlider->getValue() - pan) > 0.001)
      strip.panSlider->setValue(pan, false);
  }

  // Update mute button
  if (strip.muteButton) {
    bool mute = trackNode[zenith::ProjectState::PROP_MUTE];
    if (strip.muteButton->getToggleState() != mute)
      strip.muteButton->setToggleState(mute, false);
  }

  // Update solo button
  if (strip.soloButton) {
    bool solo = trackNode[zenith::ProjectState::PROP_SOLO];
    if (strip.soloButton->getToggleState() != solo)
      strip.soloButton->setToggleState(solo, false);
  }

  // Update arm button
  if (strip.armButton) {
    bool armed = trackNode[zenith::ProjectState::PROP_ARMED];
    if (strip.armButton->getToggleState() != armed)
      strip.armButton->setToggleState(armed, false);
  }
}

MixerComponent::TrackStrip *
MixerComponent::findTrackStrip(const juce::String &trackId) {
  for (auto &strip : trackStrips) {
    if (strip && strip->trackId == trackId)
      return strip.get();
  }

=======
Track *MixerComponent::findTrackById(const juce::String &trackId) {
  // Safe message-thread iteration of Engine tracks
  // Engine::tracks() returns const ref to vector<shared_ptr<Track>>
  const auto &tracks = engine_.tracks();
  for (const auto &track : tracks) {
    if (track->getTrackId() == trackId) {
      return track.get();
    }
  }
>>>>>>> origin/master
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

<<<<<<< HEAD
void MixerComponent::onArmClicked(const juce::String &trackId, bool state) {
  projectState.setTrackArmed(trackId, state, "Set track armed");
}

//==============================================================================
// Skia Rendering Implementation
//==============================================================================
=======
void MixerComponent::valueTreeParentChanged(juce::ValueTree &tree) {}

} // namespace zenith
>>>>>>> origin/master
