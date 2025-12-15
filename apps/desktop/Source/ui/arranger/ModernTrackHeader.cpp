/**
 * @file ModernTrackHeader.cpp
 * @brief Professional track header implementation
 */

#include "ModernTrackHeader.h"
#include "ZenithDesignSystem.h"

namespace zenith {

// Helper to convert SkColor to juce::Colour
static juce::Colour skToJuce(SkColor sk) {
  return juce::Colour::fromRGBA(SkColorGetR(sk), SkColorGetG(sk),
                                SkColorGetB(sk), SkColorGetA(sk));
}

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
  nameLabel_.setFont(14.0f);
  nameLabel_.setColour(juce::Label::textColourId, skToJuce(design::colors::TEXT_PRIMARY));
  nameLabel_.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
  nameLabel_.setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);
  nameLabel_.setColour(juce::Label::backgroundWhenEditingColourId, skToJuce(design::colors::BG_LIGHT));
  nameLabel_.setColour(juce::TextEditor::backgroundColourId, skToJuce(design::colors::BG_LIGHT));
  nameLabel_.setColour(juce::TextEditor::textColourId, skToJuce(design::colors::TEXT_PRIMARY));
  nameLabel_.setColour(juce::TextEditor::focusedOutlineColourId, skToJuce(design::colors::CYAN));

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

  setSize(240, 64);
}

void ModernTrackHeader::paint(juce::Graphics &g) {
  auto bounds = getLocalBounds().toFloat();

  // Background with subtle depth
  auto bgColor = skToJuce(isSelected_ ? design::colors::BG_LIGHT : design::colors::BG_DARK);
  if (isHovered_) {
    bgColor = bgColor.brighter(0.05f);
  }

  g.setColour(bgColor);
  g.fillRoundedRectangle(bounds, 4.0f);

  // Track color stripe (left edge, 4px wide)
  auto stripeBounds = bounds.removeFromLeft(4.0f);
  g.setColour(trackColor_);
  g.fillRect(stripeBounds);

  // Add subtle glow to stripe when soloed or armed
  if (isSoloed_ || isArmed_) {
    g.setColour(trackColor_.withAlpha(0.3f));
    g.fillRect(stripeBounds.expanded(2.0f, 0.0f));
  }

  // Bottom border for separation
  g.setColour(skToJuce(design::colors::BORDER_SUBTLE));
  g.fillRect(bounds.withTop(bounds.getBottom() - 1.0f));

  // Selected state indicator (top accent line)
  if (isSelected_) {
    auto selectionLine = bounds.removeFromTop(2.0f);
    g.setColour(skToJuce(design::colors::CYAN));
    g.fillRect(selectionLine);
  }
}

void ModernTrackHeader::resized() {
  auto bounds = getLocalBounds();

  // Skip color stripe area
  bounds.removeFromLeft(4);

  // Add padding
  bounds.reduce(16, 8);

  // Buttons on the right (24x24 with 4px spacing)
  const int buttonSize = 24;
  auto buttonArea = bounds.removeFromRight(buttonSize * 3 + 4 * 2);

  armButton_.setBounds(buttonArea.removeFromRight(buttonSize));
  buttonArea.removeFromRight(4);
  soloButton_.setBounds(buttonArea.removeFromRight(buttonSize));
  buttonArea.removeFromRight(4);
  muteButton_.setBounds(buttonArea.removeFromRight(buttonSize));

  // Add spacing between buttons and name
  bounds.removeFromRight(16);

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
  float cornerSize = 4.0f;

  // Determine colors based on button type and state
  juce::Colour buttonColor;
  juce::Colour textColor;

  bool isOn = getToggleState();

  switch (type_) {
  case Type::Mute:
    buttonColor = isOn ? skToJuce(design::colors::AMBER) : skToJuce(design::colors::BG_MEDIUM);
    textColor = isOn ? juce::Colours::white : skToJuce(design::colors::TEXT_SECONDARY);
    break;
  case Type::Solo:
    buttonColor = isOn ? skToJuce(design::colors::GREEN) : skToJuce(design::colors::BG_MEDIUM);
    textColor = isOn ? juce::Colours::white : skToJuce(design::colors::TEXT_SECONDARY);
    break;
  case Type::Arm:
    buttonColor = isOn ? skToJuce(design::colors::RED) : skToJuce(design::colors::BG_MEDIUM);
    textColor = isOn ? juce::Colours::white : skToJuce(design::colors::TEXT_SECONDARY);
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
    g.setColour(skToJuce(design::colors::BORDER_DEFAULT));
    g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
  }

  // Draw icon/text
  g.setColour(textColor);
  g.setFont(12.0f);
  g.drawText(getButtonText(), bounds.toNearestInt(),
             juce::Justification::centred, false);

  // Focus ring
  if (hasKeyboardFocus(true)) {
    g.setColour(skToJuce(design::colors::CYAN).withAlpha(0.5f));
    g.drawRoundedRectangle(bounds.expanded(2.0f), cornerSize + 2.0f, 2.0f);
  }
}

} // namespace zenith
