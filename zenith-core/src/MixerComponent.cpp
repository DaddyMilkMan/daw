/**
 * @file MixerComponent.cpp
 * @brief Mixer component implementation
 */

#include "../include/MixerComponent.h"
#include "../Source/rendering/SkiaContextManager.h"
#include "../Source/ui/ZenithLookAndFeel.h"

#ifdef ZENITH_USE_SKIA
#include <include/core/SkCanvas.h>
#include <include/core/SkFont.h>
#include <include/core/SkPaint.h>
#include <include/core/SkPath.h>
#include <include/core/SkRRect.h>
#include <include/effects/SkGradientShader.h>
#endif
#include "../Source/ui/skia/SkiaTheme.h"
#include "../Source/ui/skia/SkiaUtils.h"

//==============================================================================
MixerComponent::MixerComponent(ProjectState &ps) : projectState(ps) {
  // Listen to the entire state tree for changes
  projectState.getState().addListener(this);

  // Build initial track strips
  rebuildTrackStrips();
}

MixerComponent::~MixerComponent() {
  // Stop listening
  projectState.getState().removeListener(this);

  // Clear all strips (will destroy all child components)
  trackStrips.clear();
}

//==============================================================================
// Component interface
//==============================================================================

void MixerComponent::paint(juce::Graphics &g) {
#ifdef ZENITH_USE_SKIA
  auto &manager = zenith::SkiaContextManager::getInstance();
  // If Skia is ready, render via Skia and EXIT this function
  if (manager.isInitialized() && supportsSkiaRendering()) {
    manager.renderToComponent(*this, [this](SkCanvas *canvas) {
      // Convert component bounds to SkRect
      SkRect bounds = SkRect::MakeWH((float)getWidth(), (float)getHeight());
      paintToSkia(canvas, bounds);
    });
    return; // CRITICAL: Stop here so we don't double-render with JUCE
  }
#endif

  // Background
  g.fillAll(juce::Colour(zenith::ZenithLookAndFeel::Colors::backgroundPanel));

  // Draw border
  g.setColour(juce::Colour(zenith::ZenithLookAndFeel::Colors::border));
  g.drawRect(getLocalBounds(), 1);

  // If no tracks, show hint
  if (trackStrips.empty()) {
    g.setColour(juce::Colour(zenith::ZenithLookAndFeel::Colors::textSecondary));
    g.setFont(zenith::ZenithLookAndFeel::getFontBody());
    g.drawText("No tracks - Add a track to see mixer controls",
               getLocalBounds(), juce::Justification::centred, true);
  }
}

//==============================================================================

void MixerComponent::resized() {
  using namespace zenith;

  auto bounds = getLocalBounds().reduced(ZenithLookAndFeel::Spacing::m);

  int x = 0;
  const int stripWidth = 80; // Fixed strip width
  const int stripSpacing = ZenithLookAndFeel::Spacing::s;

  for (auto &strip : trackStrips) {
    if (!strip)
      continue;

    // Allocate space for this strip
    strip->bounds = juce::Rectangle<int>(bounds.getX() + x, bounds.getY(),
                                         stripWidth, bounds.getHeight());

    // Layout components within the strip
    auto area = strip->bounds;

    // Name label at top
    if (strip->nameLabel) {
      strip->nameLabel->setBounds(area.removeFromTop(24));
    }

    area.removeFromTop(ZenithLookAndFeel::Spacing::s);

    // Buttons at bottom (stacking up)
    auto buttonArea = area.removeFromBottom(80); // Reserve space for buttons

    if (strip->armButton)
      strip->armButton->setBounds(buttonArea.removeFromBottom(24));

    if (strip->soloButton)
      strip->soloButton->setBounds(buttonArea.removeFromBottom(24));

    if (strip->muteButton)
      strip->muteButton->setBounds(buttonArea.removeFromBottom(24));

    area.removeFromBottom(ZenithLookAndFeel::Spacing::s);

    // Pan knob (fixed height)
    if (strip->panSlider) {
      auto panArea = area.removeFromBottom(60);
      strip->panSlider->setBounds(panArea.reduced(4));
    }

    area.removeFromBottom(ZenithLookAndFeel::Spacing::s);

    // Volume slider (takes remaining space)
    if (strip->volumeSlider) {
      strip->volumeSlider->setBounds(area);
    }

    x += stripWidth + stripSpacing;
  }
}

//==============================================================================

void MixerComponent::valueTreePropertyChanged(
    juce::ValueTree &treeWhosePropertyHasChanged,
    const juce::Identifier &property) {
  // Check if this is a track node
  if (treeWhosePropertyHasChanged.getType() != ProjectState::ID_TRACK)
    return;

  // Get track ID
  auto trackId = treeWhosePropertyHasChanged[ProjectState::PROP_ID].toString();

  if (trackId.isEmpty())
    return;

  // Find the corresponding track strip
  auto *strip = findTrackStrip(trackId);

  if (!strip)
    return;

  // Update the strip from state (only if the property changed is relevant)
  if (property == ProjectState::PROP_NAME ||
      property == ProjectState::PROP_VOLUME ||
      property == ProjectState::PROP_PAN ||
      property == ProjectState::PROP_MUTE ||
      property == ProjectState::PROP_SOLO ||
      property == ProjectState::PROP_ARMED) {
    updatingFromState = true;
    updateTrackStripFromState(*strip, treeWhosePropertyHasChanged);
    updatingFromState = false;
  }
}

void MixerComponent::valueTreeChildAdded(
    juce::ValueTree &parentTree, juce::ValueTree &childWhichHasBeenAdded) {
  // Check if a track was added
  if (parentTree.getType() == ProjectState::ID_TRACKS &&
      childWhichHasBeenAdded.getType() == ProjectState::ID_TRACK) {
    // Rebuild all strips
    rebuildTrackStrips();
    resized();
    repaint();
  }
}

void MixerComponent::valueTreeChildRemoved(
    juce::ValueTree &parentTree, juce::ValueTree &childWhichHasBeenRemoved,
    int indexFromWhichChildWasRemoved) {
  // Check if a track was removed
  if (parentTree.getType() == ProjectState::ID_TRACKS &&
      childWhichHasBeenRemoved.getType() == ProjectState::ID_TRACK) {
    // Rebuild all strips
    rebuildTrackStrips();
    resized();
    repaint();
  }
}

void MixerComponent::valueTreeChildOrderChanged(
    juce::ValueTree &parentTreeWhoseChildrenHaveMoved, int oldIndex,
    int newIndex) {
  // If tracks were reordered, rebuild
  if (parentTreeWhoseChildrenHaveMoved.getType() == ProjectState::ID_TRACKS) {
    rebuildTrackStrips();
    resized();
    repaint();
  }
}

//==============================================================================
// Helper methods
//==============================================================================

void MixerComponent::rebuildTrackStrips() {
  // Clear existing strips
  trackStrips.clear();

  // Get tracks node
  auto tracksNode =
      projectState.getState().getChildWithName(ProjectState::ID_TRACKS);

  if (!tracksNode.isValid())
    return;

  // Create a strip for each track
  for (int i = 0; i < tracksNode.getNumChildren(); ++i) {
    auto trackNode = tracksNode.getChild(i);

    if (trackNode.getType() != ProjectState::ID_TRACK)
      continue;

    auto strip = createTrackStrip(trackNode);

    if (strip)
      trackStrips.push_back(std::move(strip));
  }

  DBG("MixerComponent: Rebuilt " + juce::String(trackStrips.size()) +
      " track strips");
}

std::unique_ptr<MixerComponent::TrackStrip>
MixerComponent::createTrackStrip(const juce::ValueTree &trackNode) {
  auto strip = std::make_unique<TrackStrip>();

  // Get track info
  strip->trackId = trackNode[ProjectState::PROP_ID].toString();
  strip->trackName = trackNode[ProjectState::PROP_NAME].toString();

  if (strip->trackId.isEmpty())
    return nullptr;

  // Create name label
  strip->nameLabel = std::make_unique<juce::Label>();
  strip->nameLabel->setText(strip->trackName, juce::dontSendNotification);
  strip->nameLabel->setJustificationType(juce::Justification::centred);
  strip->nameLabel->setFont(
      zenith::ZenithLookAndFeel::getFontSmall().withStyle(juce::Font::bold));
  strip->nameLabel->setColour(
      juce::Label::textColourId,
      juce::Colour(zenith::ZenithLookAndFeel::Colors::textPrimary));
  addAndMakeVisible(*strip->nameLabel);

  // Create volume slider (vertical)
  strip->volumeSlider = std::make_unique<juce::Slider>();
  strip->volumeSlider->setSliderStyle(juce::Slider::LinearVertical);
  strip->volumeSlider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50,
                                       20);
  strip->volumeSlider->setRange(0.0, 1.0, 0.01);
  strip->volumeSlider->setValue(trackNode[ProjectState::PROP_VOLUME],
                                juce::dontSendNotification);
  // Capture trackId by value for the lambda
  strip->volumeSlider->onValueChange = [this, trackId = strip->trackId,
                                        slider = strip->volumeSlider.get()]() {
    if (!updatingFromState)
      onVolumeChanged(trackId, (float)slider->getValue());
  };
  addAndMakeVisible(*strip->volumeSlider);

  // Create pan slider (rotary)
  strip->panSlider = std::make_unique<juce::Slider>();
  strip->panSlider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  strip->panSlider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
  strip->panSlider->setRange(-1.0, 1.0, 0.01);
  strip->panSlider->setValue(trackNode[ProjectState::PROP_PAN],
                             juce::dontSendNotification);
  strip->panSlider->onValueChange = [this, trackId = strip->trackId,
                                     slider = strip->panSlider.get()]() {
    if (!updatingFromState)
      onPanChanged(trackId, (float)slider->getValue());
  };
  addAndMakeVisible(*strip->panSlider);

  // Create mute button
  strip->muteButton = std::make_unique<juce::ToggleButton>("M");
  strip->muteButton->setClickingTogglesState(true);
  strip->muteButton->setToggleState(trackNode[ProjectState::PROP_MUTE],
                                    juce::dontSendNotification);
  strip->muteButton->onClick = [this, trackId = strip->trackId,
                                button = strip->muteButton.get()]() {
    if (!updatingFromState)
      onMuteClicked(trackId, button->getToggleState());
  };
  strip->muteButton->setColour(
      juce::ToggleButton::textColourId,
      juce::Colour(zenith::ZenithLookAndFeel::Colors::textPrimary));
  addAndMakeVisible(*strip->muteButton);

  // Create solo button
  strip->soloButton = std::make_unique<juce::ToggleButton>("S");
  strip->soloButton->setClickingTogglesState(true);
  strip->soloButton->setToggleState(trackNode[ProjectState::PROP_SOLO],
                                    juce::dontSendNotification);
  strip->soloButton->onClick = [this, trackId = strip->trackId,
                                button = strip->soloButton.get()]() {
    if (!updatingFromState)
      onSoloClicked(trackId, button->getToggleState());
  };
  strip->soloButton->setColour(
      juce::ToggleButton::textColourId,
      juce::Colour(zenith::ZenithLookAndFeel::Colors::textPrimary));
  addAndMakeVisible(*strip->soloButton);

  // Create arm button
  strip->armButton = std::make_unique<juce::ToggleButton>("R");
  strip->armButton->setClickingTogglesState(true);
  strip->armButton->setToggleState(trackNode[ProjectState::PROP_ARMED],
                                   juce::dontSendNotification);
  strip->armButton->onClick = [this, trackId = strip->trackId,
                               button = strip->armButton.get()]() {
    if (!updatingFromState)
      onArmClicked(trackId, button->getToggleState());
  };
  strip->armButton->setColour(
      juce::ToggleButton::textColourId,
      juce::Colour(zenith::ZenithLookAndFeel::Colors::textPrimary));
  addAndMakeVisible(*strip->armButton);

  return strip;
}

void MixerComponent::updateTrackStripFromState(
    TrackStrip &strip, const juce::ValueTree &trackNode) {
  // Update track name
  auto newName = trackNode[ProjectState::PROP_NAME].toString();
  if (newName != strip.trackName) {
    strip.trackName = newName;
    if (strip.nameLabel)
      strip.nameLabel->setText(newName, juce::dontSendNotification);
  }

  // Update volume slider
  if (strip.volumeSlider) {
    float volume = trackNode[ProjectState::PROP_VOLUME];
    if (std::abs(strip.volumeSlider->getValue() - volume) > 0.001)
      strip.volumeSlider->setValue(volume, juce::dontSendNotification);
  }

  // Update pan slider
  if (strip.panSlider) {
    float pan = trackNode[ProjectState::PROP_PAN];
    if (std::abs(strip.panSlider->getValue() - pan) > 0.001)
      strip.panSlider->setValue(pan, juce::dontSendNotification);
  }

  // Update mute button
  if (strip.muteButton) {
    bool mute = trackNode[ProjectState::PROP_MUTE];
    if (strip.muteButton->getToggleState() != mute)
      strip.muteButton->setToggleState(mute, juce::dontSendNotification);
  }

  // Update solo button
  if (strip.soloButton) {
    bool solo = trackNode[ProjectState::PROP_SOLO];
    if (strip.soloButton->getToggleState() != solo)
      strip.soloButton->setToggleState(solo, juce::dontSendNotification);
  }

  // Update arm button
  if (strip.armButton) {
    bool armed = trackNode[ProjectState::PROP_ARMED];
    if (strip.armButton->getToggleState() != armed)
      strip.armButton->setToggleState(armed, juce::dontSendNotification);
  }
}

MixerComponent::TrackStrip *
MixerComponent::findTrackStrip(const juce::String &trackId) {
  for (auto &strip : trackStrips) {
    if (strip && strip->trackId == trackId)
      return strip.get();
  }

  return nullptr;
}

//==============================================================================
// Control callbacks
//==============================================================================

void MixerComponent::onVolumeChanged(const juce::String &trackId, float value) {
  projectState.setTrackVolume(trackId, value, "Set track volume");
}

void MixerComponent::onPanChanged(const juce::String &trackId, float value) {
  projectState.setTrackPan(trackId, value, "Set track pan");
}

void MixerComponent::onMuteClicked(const juce::String &trackId, bool state) {
  projectState.setTrackMute(trackId, state, "Set track mute");
}

void MixerComponent::onSoloClicked(const juce::String &trackId, bool state) {
  projectState.setTrackSolo(trackId, state, "Set track solo");
}

void MixerComponent::onArmClicked(const juce::String &trackId, bool state) {
  projectState.setTrackArmed(trackId, state, "Set track armed");
}

//==============================================================================
// Skia Rendering Implementation
//==============================================================================

#ifdef ZENITH_USE_SKIA

void MixerComponent::paintToSkia(SkCanvas *canvas, SkRect bounds) {
  // 1. Draw Background with Gradient
  SkPoint gradientPoints[2] = {{bounds.x(), bounds.y()},
                               {bounds.x(), bounds.y() + bounds.height()}};

  SkColor gradientColors[2] = {
      SkColorSetARGB(255, 30, 30, 30), // Dark grey top
      SkColorSetARGB(255, 20, 20, 20)  // Darker bottom
  };

  SkScalar gradientPositions[2] = {0.0f, 1.0f};

  auto gradient = SkGradientShader::MakeLinear(
      gradientPoints, gradientColors, gradientPositions, 2, SkTileMode::kClamp);

  SkPaint bgPaint;
  bgPaint.setShader(gradient);
  canvas->drawRect(bounds, bgPaint);

  // 2. Draw Top Border
  SkPaint borderPaint;
  borderPaint.setColor(SkColorSetARGB(128, 80, 80, 80));
  borderPaint.setStrokeWidth(1.0f);
  borderPaint.setStyle(SkPaint::kStroke_Style);
  borderPaint.setAntiAlias(true);

  canvas->drawLine(bounds.x(), bounds.y(), bounds.right(), bounds.y(),
                   borderPaint);

  // 3. Draw Track Strips
  const float startX = bounds.x() + static_cast<float>(sideMargin);
  float currentX = startX;
  const float marginTop = static_cast<float>(topMargin);
  const float marginBottom = static_cast<float>(bottomMargin);

  const float sWidth = static_cast<float>(stripWidth);
  const float sSpacing = static_cast<float>(stripSpacing);

  for (const auto &strip : trackStrips) {
    if (!strip)
      continue;

    SkRect stripBounds =
        SkRect::MakeXYWH(currentX, bounds.y() + marginTop, sWidth,
                         bounds.height() - marginTop - marginBottom);

    drawTrackStripSkia(canvas, stripBounds, *strip);
    currentX += sWidth + sSpacing;
  }

  // 4. Draw Empty State Message
  if (trackStrips.empty()) {
    SkFont font;
    font.setSize(14);
    font.setEdging(SkFont::Edging::kAntiAlias);

    SkPaint textPaint;
    textPaint.setColor(SkColorSetARGB(128, 255, 255, 255));
    textPaint.setAntiAlias(true);

    const char *message = "No tracks - Add a track to see mixer controls";
    SkRect textBounds;
    font.measureText(message, strlen(message), SkTextEncoding::kUTF8,
                     &textBounds);

    canvas->drawString(message, bounds.centerX() - textBounds.width() / 2.0f,
                       bounds.centerY(), font, textPaint);
  }
}

void MixerComponent::drawTrackStripSkia(SkCanvas *canvas, SkRect stripBounds,
                                        const TrackStrip &strip) {
  canvas->save();

  // Strip Background
  SkRRect roundedStrip;
  roundedStrip.setRectXY(stripBounds, 4, 4);

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
                     stripBounds.y() + 20, nameFont, namePaint);

  // NOTE: Volume Fader, Pan Slider, and M/S/R Buttons are child components
  // that render themselves. They handle their own Skia or JUCE rendering.
  // Do NOT draw them here to avoid double-rendering or desync issues.
  // The component bounds are set in resized() to position them within the
  // strip.

  canvas->restore();
}

#endif
