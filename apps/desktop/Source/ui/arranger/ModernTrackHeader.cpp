/**
 * @file ModernTrackHeader.cpp
 * @brief Professional track header implementation
 */

#include "ModernTrackHeader.h"
#include "ZenithDesignSystem.h"

#include <core/SkCanvas.h>
#include <core/SkPaint.h>
#include <core/SkRect.h>
#include <core/SkRRect.h>

namespace zenith {

//==============================================================================
// ModernTrackHeader Implementation
//==============================================================================

ModernTrackHeader::ModernTrackHeader(int trackIndex)
    : trackIndex_(trackIndex) {
  
  // Generate track color based on index
  float hue = (trackIndex * 0.618033988749895f); // Golden ratio
  hue = hue - std::floor(hue); // Keep fractional part
  trackColor_ = juce::Colour::fromHSV(hue, 0.7f, 0.8f, 1.0f);

  // Setup name label
  nameLabel_.setText("Track " + juce::String(trackIndex + 1),
                     juce::dontSendNotification);
  nameLabel_.setEditable(true, true);
  nameLabel_.setJustificationType(juce::Justification::centredLeft);
  nameLabel_.setFont(design::typography::FONT_MD);
  nameLabel_.setColour(juce::Label::textColourId, juce::Colour::fromRGBA(SkColorGetR(design::colors::TEXT_PRIMARY), SkColorGetG(design::colors::TEXT_PRIMARY), SkColorGetB(design::colors::TEXT_PRIMARY), SkColorGetA(design::colors::TEXT_PRIMARY)));
  nameLabel_.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
  nameLabel_.setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);
  nameLabel_.setColour(juce::Label::backgroundWhenEditingColourId, juce::Colour::fromRGBA(SkColorGetR(design::colors::BG_LIGHT), SkColorGetG(design::colors::BG_LIGHT), SkColorGetB(design::colors::BG_LIGHT), SkColorGetA(design::colors::BG_LIGHT)));
  nameLabel_.setColour(juce::TextEditor::backgroundColourId, juce::Colour::fromRGBA(SkColorGetR(design::colors::BG_LIGHT), SkColorGetG(design::colors::BG_LIGHT), SkColorGetB(design::colors::BG_LIGHT), SkColorGetA(design::colors::BG_LIGHT)));
  nameLabel_.setColour(juce::TextEditor::textColourId, juce::Colour::fromRGBA(SkColorGetR(design::colors::TEXT_PRIMARY), SkColorGetG(design::colors::TEXT_PRIMARY), SkColorGetB(design::colors::TEXT_PRIMARY), SkColorGetA(design::colors::TEXT_PRIMARY)));
  nameLabel_.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour::fromRGBA(SkColorGetR(design::colors::CYAN), SkColorGetG(design::colors::CYAN), SkColorGetB(design::colors::CYAN), SkColorGetA(design::colors::CYAN)));

  nameLabel_.onTextChange = [this]() {
    if (onNameChanged)
      onNameChanged();
  };

  addAndMakeVisible(nameLabel_);

  // Setup buttons
  muteButton_.setButtonType(TrackButton::Type::Mute);
  muteButton_.setClickingTogglesState(true);
  muteButton_.setTooltip("Mute Track");
  muteButton_.onClick = [this]() {
    isMuted_ = muteButton_.getToggleState();
    if (onMuteToggled)
      onMuteToggled(isMuted_);
  };
  addAndMakeVisible(muteButton_);

  soloButton_.setButtonType(TrackButton::Type::Solo);
  soloButton_.setClickingTogglesState(true);
  soloButton_.setTooltip("Solo Track");
  soloButton_.onClick = [this]() {
    isSoloed_ = soloButton_.getToggleState();
    if (onSoloToggled)
      onSoloToggled(isSoloed_);
  };
  addAndMakeVisible(soloButton_);

  armButton_.setButtonType(TrackButton::Type::Arm);
  armButton_.setClickingTogglesState(true);
  armButton_.setTooltip("Arm for Recording");
  armButton_.onClick = [this]() {
    isArmed_ = armButton_.getToggleState();
    if (onArmToggled)
      onArmToggled(isArmed_);
  };
  addAndMakeVisible(armButton_);

  setSize(static_cast<int>(design::dimensions::ARRANGER_HEADER_WIDTH),
          static_cast<int>(design::dimensions::ARRANGER_TRACK_HEIGHT));
}

void ModernTrackHeader::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds();
  float width = (float)bounds.getWidth();
  float height = (float)bounds.getHeight();
  
  using namespace design;

  // Background with subtle depth
  SkColor bgColor = isSelected_ ? colors::BG_LIGHT : colors::BG_DARK;
  if (isHovered_) {
      // Manual brightness adjustment for SkColor if needed, or just use another color
      // Since we don't have a direct brighten(0.05f) for SkColor here, let's just use BG_LIGHTER if it exists
      // or just keep it simple.
  }

  SkPaint bgPaint;
  bgPaint.setColor(bgColor);
  bgPaint.setAntiAlias(true);
  canvas->drawRRect(SkRRect::MakeRectXY(SkRect::MakeWH(width, height), 4.0f, 4.0f), bgPaint);

  // Track color stripe
  SkPaint stripePaint;
  stripePaint.setColor(SkColorSetARGB(trackColor_.getAlpha(), trackColor_.getRed(), trackColor_.getGreen(), trackColor_.getBlue()));
  canvas->drawRect(SkRect::MakeXYWH(0, 0, 4.0f, height), stripePaint);

  // Add subtle glow to stripe when soloed or armed
  if (isSoloed_ || isArmed_) {
      stripePaint.setAlphaf(0.3f);
      canvas->drawRect(SkRect::MakeXYWH(0, 0, 6.0f, height), stripePaint);
  }

  // Bottom border
  SkPaint borderPaint;
  borderPaint.setColor(colors::BORDER_SUBTLE);
  canvas->drawRect(SkRect::MakeXYWH(4.0f, height - 1.0f, width - 4.0f, 1.0f), borderPaint);

  // Selected state indicator
  if (isSelected_) {
      SkPaint selectionPaint;
      selectionPaint.setColor(colors::CYAN);
      canvas->drawRect(SkRect::MakeXYWH(4.0f, 0, width - 4.0f, 2.0f), selectionPaint);
  }
}

void ModernTrackHeader::resized() {
  auto bounds = getLocalBounds();

  // Skip color stripe area
  bounds.removeFromLeft(4);

  // Add padding
  bounds.reduce(design::spacing::MD, design::spacing::SM);

  // Buttons on the right (24x24 with 4px spacing)
  const int buttonSize = 24;
  auto buttonArea = bounds.removeFromRight(buttonSize * 3 + 4 * 2);

  armButton_.setBounds(buttonArea.removeFromRight(buttonSize));
  buttonArea.removeFromRight(design::spacing::XS);
  soloButton_.setBounds(buttonArea.removeFromRight(buttonSize));
  buttonArea.removeFromRight(design::spacing::XS);
  muteButton_.setBounds(buttonArea.removeFromRight(buttonSize));

  // Add spacing between buttons and name
  bounds.removeFromRight(design::spacing::MD);

  // Name label gets remaining space
  nameLabel_.setBounds(bounds);
}

void ModernTrackHeader::mouseEnter(const juce::MouseEvent &) {
  isHovered_ = true;
  repaint();
}

void ModernTrackHeader::mouseExit(const juce::MouseEvent &) {
  isHovered_ = false;
  repaint();
}

//==============================================================================
// Public Interface
//==============================================================================

void ModernTrackHeader::setTrackName(const juce::String &name) {
  nameLabel_.setText(name, juce::dontSendNotification);
}

juce::String ModernTrackHeader::getTrackName() const {
  return nameLabel_.getText();
}

void ModernTrackHeader::setTrackColor(const juce::Colour &color) {
  trackColor_ = color;
  repaint();
}

void ModernTrackHeader::setMuted(bool shouldBeMuted) {
  isMuted_ = shouldBeMuted;
  muteButton_.setToggleState(shouldBeMuted, juce::dontSendNotification);
  repaint();
}

void ModernTrackHeader::setSoloed(bool shouldBeSoloed) {
  isSoloed_ = shouldBeSoloed;
  soloButton_.setToggleState(shouldBeSoloed, juce::dontSendNotification);
  repaint();
}

void ModernTrackHeader::setArmed(bool shouldBeArmed) {
  isArmed_ = shouldBeArmed;
  armButton_.setToggleState(shouldBeArmed, juce::dontSendNotification);
  repaint();
}

void ModernTrackHeader::setSelected(bool shouldBeSelected) {
  isSelected_ = shouldBeSelected;
  repaint();
}

//==============================================================================
// TrackButton Implementation
//==============================================================================

ModernTrackHeader::TrackButton::TrackButton(const juce::String &buttonText)
    : juce::Button(buttonText) {}

void ModernTrackHeader::TrackButton::setButtonType(Type type) { type_ = type; }

void ModernTrackHeader::TrackButton::paintButton(juce::Graphics &g,
                                                 bool isHighlighted,
                                                 bool isDown) {
  auto bounds = getLocalBounds().toFloat();
  float cornerSize = design::dimensions::RADIUS_SM;

  // Determine colors based on button type and state
  juce::Colour buttonColor;
  juce::Colour textColor;

  bool isOn = getToggleState();

  // Inline skToJuce for now to avoid dependency on the removed static helper
  auto toJuce = [](SkColor sk) {
    return juce::Colour::fromRGBA(SkColorGetR(sk), SkColorGetG(sk), SkColorGetB(sk), SkColorGetA(sk));
  };

  switch (type_) {
  case Type::Mute:
    buttonColor = isOn ? toJuce(design::colors::AMBER) : toJuce(design::colors::BG_MEDIUM);
    textColor = isOn ? juce::Colours::white : toJuce(design::colors::TEXT_SECONDARY);
    break;
  case Type::Solo:
    buttonColor = isOn ? toJuce(design::colors::GREEN) : toJuce(design::colors::BG_MEDIUM);
    textColor = isOn ? juce::Colours::white : toJuce(design::colors::TEXT_SECONDARY);
    break;
  case Type::Arm:
    buttonColor = isOn ? toJuce(design::colors::RED) : toJuce(design::colors::BG_MEDIUM);
    textColor = isOn ? juce::Colours::white : toJuce(design::colors::TEXT_SECONDARY);
    break;
  }

  // Hover/press states
  if (isDown) {
    buttonColor = buttonColor.darker(0.1f);
  } else if (isHighlighted) {
    buttonColor = buttonColor.brighter(0.1f);
  }

  // Draw button background
  g.setColour(buttonColor);
  g.fillRoundedRectangle(bounds, cornerSize);

  // Draw border if not toggled
  if (!isOn) {
    g.setColour(toJuce(design::colors::BORDER_DEFAULT));
    g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
  }

  // Draw icon/text
  g.setColour(textColor);
  g.setFont(design::typography::FONT_SM);
  g.drawText(getButtonText(), bounds.toNearestInt(),
             juce::Justification::centred, false);

  // Focus ring
  if (hasKeyboardFocus(true)) {
    g.setColour(toJuce(design::colors::CYAN).withAlpha(0.5f));
    g.drawRoundedRectangle(bounds.expanded(2.0f), cornerSize + 2.0f, 2.0f);
  }
}

} // namespace zenith
