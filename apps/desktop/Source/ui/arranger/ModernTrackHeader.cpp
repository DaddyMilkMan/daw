/**
 * @file ModernTrackHeader.cpp
 * @brief Professional track header implementation
 * @author Fixed by Claude - December 2025
 */

#include "ModernTrackHeader.h"
#include "ZenithTheme.h"

namespace zenith {

//==============================================================================
// ModernTrackHeader Implementation
//==============================================================================

ModernTrackHeader::ModernTrackHeader(int trackIndex)
    : trackIndex_(trackIndex),
      trackColor_(ZenithTheme::Colors::getTrackColor(trackIndex)) {
  using namespace ZenithTheme;

  // Setup name label
  nameLabel_.setText("Track " + juce::String(trackIndex + 1),
                     juce::dontSendNotification);
  nameLabel_.setEditable(true, true);
  nameLabel_.setJustificationType(juce::Justification::centredLeft);
  nameLabel_.setFont(Typography::getBodyFont(Typography::Weight::Medium));
  nameLabel_.setColour(juce::Label::textColourId, Colors::text_primary);
  nameLabel_.setColour(juce::Label::backgroundColourId,
                       juce::Colours::transparentBlack);
  nameLabel_.setColour(juce::Label::outlineColourId,
                       juce::Colours::transparentBlack);
  nameLabel_.setColour(juce::Label::backgroundWhenEditingColourId,
                       Colors::bg_03);
  nameLabel_.setColour(juce::TextEditor::backgroundColourId, Colors::bg_03);
  nameLabel_.setColour(juce::TextEditor::textColourId, Colors::text_primary);
  nameLabel_.setColour(juce::TextEditor::highlightColourId,
                       Colors::accent_subtle);
  nameLabel_.setColour(juce::TextEditor::outlineColourId,
                       Colors::border_default);
  nameLabel_.setColour(juce::TextEditor::focusedOutlineColourId,
                       Colors::accent_primary);

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

  setSize(240, 64); // Using modern spacing system
}

void ModernTrackHeader::paint(juce::Graphics &g) {
  using namespace ZenithTheme;

  auto bounds = getLocalBounds().toFloat();

  // Background with subtle depth
  auto bgColor = isSelected_ ? Colors::bg_03 : Colors::bg_02;
  if (isHovered_) {
    bgColor = bgColor.brighter(0.05f);
  }

  g.setColour(bgColor);
  g.fillRoundedRectangle(bounds, Radius::sm);

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
  g.setColour(Colors::border_subtle);
  g.fillRect(bounds.withTop(bounds.getBottom() - 1.0f)
                 .withLeft(0.0f)
                 .withRight(bounds.getRight()));

  // Selected state indicator (top accent line)
  if (isSelected_) {
    auto selectionLine = bounds.removeFromTop(2.0f);
    g.setColour(Colors::accent_primary);
    g.fillRect(selectionLine);
  }
}

void ModernTrackHeader::resized() {
  using namespace ZenithTheme;

  auto bounds = getLocalBounds();

  // Skip color stripe area
  bounds.removeFromLeft(4);

  // Add padding
  bounds.reduce(Spacing::md, Spacing::sm);

  // Buttons on the right (24x24 with 4px spacing)
  const int buttonSize = 24;
  auto buttonArea = bounds.removeFromRight(buttonSize * 3 + Spacing::xs * 2);

  armButton_.setBounds(buttonArea.removeFromRight(buttonSize));
  buttonArea.removeFromRight(Spacing::xs);
  soloButton_.setBounds(buttonArea.removeFromRight(buttonSize));
  buttonArea.removeFromRight(Spacing::xs);
  muteButton_.setBounds(buttonArea.removeFromRight(buttonSize));

  // Add spacing between buttons and name
  bounds.removeFromRight(Spacing::md);

  // Name label gets remaining space
  nameLabel_.setBounds(bounds);
}

void ModernTrackHeader::mouseEnter(const juce::MouseEvent &e) {
  isHovered_ = true;
  repaint();
}

void ModernTrackHeader::mouseExit(const juce::MouseEvent &e) {
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
  using namespace ZenithTheme;

  auto bounds = getLocalBounds().toFloat();
  auto cornerSize = Radius::sm;

  // Determine colors based on button type and state
  juce::Colour buttonColor;
  juce::Colour textColor;

  bool isOn = getToggleState();

  switch (type_) {
  case Type::Mute:
    buttonColor = isOn ? Colors::warning : Colors::bg_03;
    textColor = isOn ? Colors::text_inverse : Colors::text_secondary;
    break;
  case Type::Solo:
    buttonColor = isOn ? Colors::success : Colors::bg_03;
    textColor = isOn ? Colors::text_inverse : Colors::text_secondary;
    break;
  case Type::Arm:
    buttonColor = isOn ? Colors::error : Colors::bg_03;
    textColor = isOn ? Colors::text_inverse : Colors::text_secondary;
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
    g.setColour(Colors::border_default);
    g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
  }

  // Draw icon/text
  g.setColour(textColor);
  g.setFont(Typography::getBodyFont(Typography::Weight::Bold));
  g.drawText(getButtonText(), bounds.toNearestInt(),
             juce::Justification::centred, false);

  // Focus ring
  if (hasKeyboardFocus(true)) {
    g.setColour(Colors::accent_primary.withAlpha(0.5f));
    g.drawRoundedRectangle(bounds.expanded(2.0f), cornerSize + 2.0f, 2.0f);
  }
}

} // namespace zenith
