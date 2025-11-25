/**
 * @file ProjectSettingsComponent.cpp
 * @brief Project settings component implementation
 */

#include "ProjectSettingsComponent.h"
#include "../../include/ProjectState.h"
#include "ZenithLookAndFeel.h"

namespace zenith {

//==============================================================================
ProjectSettingsComponent::ProjectSettingsComponent(ProjectState &state)
    : projectState_(state) {
  setSize(450, 420);

  // Start 60Hz animation timer
  startTimerHz(60);

  // Setup project name editor
  setupTextEditor(projectNameEditor_);
  projectNameEditor_.setText(projectState_.getProjectName(), false);
  addAndMakeVisible(projectNameEditor_);

  // Setup tempo editor
  setupTextEditor(tempoEditor_, 6);
  currentTempo_ = projectState_.getTempo();
  tempoEditor_.setText(juce::String(currentTempo_, 1), false);
  addAndMakeVisible(tempoEditor_);

  // Tempo stepper buttons
  tempoUpButton_.setButtonText("+");
  tempoUpButton_.onClick = [this]() { adjustTempo(1.0f); };
  addAndMakeVisible(tempoUpButton_);

  tempoDownButton_.setButtonText("-");
  tempoDownButton_.onClick = [this]() { adjustTempo(-1.0f); };
  addAndMakeVisible(tempoDownButton_);

  // Tap tempo button
  tapTempoButton_.setButtonText("Tap");
  tapTempoButton_.onClick = [this]() { onTapTempo(); };
  addAndMakeVisible(tapTempoButton_);

  // Setup time signature editors
  setupTextEditor(timeSignatureNumEditor_, 2);
  setupTextEditor(timeSignatureDenEditor_, 2);
  timeSignatureNum_ = projectState_.getTimeSignatureNumerator();
  timeSignatureDen_ = projectState_.getTimeSignatureDenominator();
  timeSignatureNumEditor_.setText(juce::String(timeSignatureNum_), false);
  timeSignatureDenEditor_.setText(juce::String(timeSignatureDen_), false);
  addAndMakeVisible(timeSignatureNumEditor_);
  addAndMakeVisible(timeSignatureDenEditor_);

  // Time signature stepper buttons
  timeSignatureNumUpButton_.setButtonText("+");
  timeSignatureNumUpButton_.onClick = [this]() { adjustTimeSignatureNum(1); };
  addAndMakeVisible(timeSignatureNumUpButton_);

  timeSignatureNumDownButton_.setButtonText("-");
  timeSignatureNumDownButton_.onClick = [this]() {
    adjustTimeSignatureNum(-1);
  };
  addAndMakeVisible(timeSignatureNumDownButton_);

  timeSignatureDenUpButton_.setButtonText("+");
  timeSignatureDenUpButton_.onClick = [this]() { adjustTimeSignatureDen(1); };
  addAndMakeVisible(timeSignatureDenUpButton_);

  timeSignatureDenDownButton_.setButtonText("-");
  timeSignatureDenDownButton_.onClick = [this]() {
    adjustTimeSignatureDen(-1);
  };
  addAndMakeVisible(timeSignatureDenDownButton_);

  // Setup sample rate label (read-only)
  sampleRateLabel_.setText("Sample Rate", juce::dontSendNotification);
  sampleRateLabel_.setJustificationType(juce::Justification::centredLeft);
  addAndMakeVisible(sampleRateLabel_);

  sampleRateValue_.setText("44100 Hz", juce::dontSendNotification);
  sampleRateValue_.setJustificationType(juce::Justification::centredLeft);
  sampleRateValue_.setColour(
      juce::Label::textColourId,
      juce::Colour(ZenithLookAndFeel::Colors::textSecondary));
  addAndMakeVisible(sampleRateValue_);

  // Action buttons
  applyButton_.setButtonText("Apply");
  applyButton_.setColour(
      juce::TextButton::buttonColourId,
      juce::Colour(ZenithLookAndFeel::Colors::accentPrimary));
  applyButton_.setColour(juce::TextButton::textColourOffId,
                         juce::Colours::white);
  applyButton_.onClick = [this]() { applyChanges(); };
  addAndMakeVisible(applyButton_);

  cancelButton_.setButtonText("Cancel");
  cancelButton_.setColour(
      juce::TextButton::buttonColourId,
      juce::Colour(ZenithLookAndFeel::Colors::borderSubtle));
  cancelButton_.setColour(juce::TextButton::textColourOffId,
                          juce::Colours::white);
  cancelButton_.onClick = [this]() { cancelChanges(); };
  addAndMakeVisible(cancelButton_);

  // Style stepper buttons
  auto styleStepperButton = [](juce::TextButton &btn) {
    btn.setColour(juce::TextButton::buttonColourId,
                  juce::Colour(ZenithLookAndFeel::Elevation::dp4));
    btn.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
  };

  styleStepperButton(tempoUpButton_);
  styleStepperButton(tempoDownButton_);
  styleStepperButton(timeSignatureNumUpButton_);
  styleStepperButton(timeSignatureNumDownButton_);
  styleStepperButton(timeSignatureDenUpButton_);
  styleStepperButton(timeSignatureDenDownButton_);

  // Style tap tempo button
  tapTempoButton_.setColour(juce::TextButton::buttonColourId,
                            juce::Colour(ZenithLookAndFeel::Colors::success));
  tapTempoButton_.setColour(juce::TextButton::textColourOffId,
                            juce::Colours::white);
}

ProjectSettingsComponent::~ProjectSettingsComponent() { stopTimer(); }

//==============================================================================
void ProjectSettingsComponent::paint(juce::Graphics &g) {
  auto bounds = getLocalBounds();

  // Ultra-clean gradient
  juce::ColourGradient gradient(
      juce::Colour(ZenithLookAndFeel::Elevation::dp2), 0.0f, 0.0f,
      juce::Colour(ZenithLookAndFeel::Colors::backgroundRaised), 0.0f,
      static_cast<float>(bounds.getHeight()), false);
  g.setGradientFill(gradient);
  g.fillRoundedRectangle(bounds.toFloat(), 12.0f);

  // Soft shadow for elevation
  juce::Path shadowPath;
  shadowPath.addRoundedRectangle(bounds.toFloat(), 12.0f);
  juce::DropShadow shadow(juce::Colours::black.withAlpha(0.25f), 6,
                          juce::Point<int>(0, 1));
  shadow.drawForPath(g, shadowPath);

  // Title with shadow
  g.setColour(juce::Colours::black.withAlpha(0.3f));
  g.setFont(juce::FontOptions(18.0f, juce::Font::bold));
  g.drawText("Project Settings",
             bounds.removeFromTop(50).translated(1, 1).reduced(PADDING, 0),
             juce::Justification::centredLeft, false);

  g.setColour(juce::Colours::white.withAlpha(0.95f));
  g.setFont(juce::FontOptions(18.0f, juce::Font::bold));
  g.drawText("Project Settings",
             bounds.toFloat().translated(0, -50).reduced(PADDING, 0),
             juce::Justification::centredLeft, false);

  // Draw setting labels
  auto contentBounds = bounds.reduced(PADDING);
  int rowIndex = 0;

  // Project Name
  paintSettingRow(
      g, "Project Name",
      contentBounds.withHeight(ROW_HEIGHT)
          .withY(contentBounds.getY() + rowIndex * (ROW_HEIGHT + SPACING)));
  // Draw validation icon
  if (projectNameValid_ && !projectNameEditor_.isEmpty()) {
    g.setColour(juce::Colour(ZenithLookAndFeel::Colors::success));
    g.drawText("✓", contentBounds.getRight() - 30,
               contentBounds.getY() + rowIndex * (ROW_HEIGHT + SPACING), 24,
               ROW_HEIGHT, juce::Justification::centred, false);
  }
  rowIndex++;

  // Tempo
  paintSettingRow(
      g, "Tempo (BPM)",
      contentBounds.withHeight(ROW_HEIGHT)
          .withY(contentBounds.getY() + rowIndex * (ROW_HEIGHT + SPACING)));
  // Draw validation icon
  if (tempoValid_) {
    g.setColour(juce::Colour(ZenithLookAndFeel::Colors::success));
    g.drawText("✓", contentBounds.getRight() - 30,
               contentBounds.getY() + rowIndex * (ROW_HEIGHT + SPACING), 24,
               ROW_HEIGHT, juce::Justification::centred, false);
  }
  rowIndex++;

  // Time Signature
  paintSettingRow(
      g, "Time Signature",
      contentBounds.withHeight(ROW_HEIGHT)
          .withY(contentBounds.getY() + rowIndex * (ROW_HEIGHT + SPACING)));
  // Draw validation icon
  if (timeSignatureValid_) {
    g.setColour(juce::Colour(ZenithLookAndFeel::Colors::success));
    g.drawText("✓", contentBounds.getRight() - 30,
               contentBounds.getY() + rowIndex * (ROW_HEIGHT + SPACING), 24,
               ROW_HEIGHT, juce::Justification::centred, false);
  }
  rowIndex++;

  // Sample Rate
  paintSettingRow(
      g, "Sample Rate",
      contentBounds.withHeight(ROW_HEIGHT)
          .withY(contentBounds.getY() + rowIndex * (ROW_HEIGHT + SPACING)));

  // Draw focus glows
  if (nameEditorFocusAnim_ > 0.01f) {
    auto editorBounds = projectNameEditor_.getBounds().toFloat().expanded(2.0f);
    g.setColour(juce::Colour(ZenithLookAndFeel::Colors::accentPrimary)
                    .withAlpha(nameEditorFocusAnim_ * 0.3f));
    g.drawRoundedRectangle(editorBounds, 6.0f, 2.0f);
  }

  if (tempoEditorFocusAnim_ > 0.01f) {
    auto editorBounds = tempoEditor_.getBounds().toFloat().expanded(2.0f);
    g.setColour(juce::Colour(ZenithLookAndFeel::Colors::accentPrimary)
                    .withAlpha(tempoEditorFocusAnim_ * 0.3f));
    g.drawRoundedRectangle(editorBounds, 6.0f, 2.0f);
  }
}

void ProjectSettingsComponent::resized() {
  auto bounds = getLocalBounds().reduced(PADDING);

  // Skip title area
  bounds.removeFromTop(50);

  int buttonSize = 28;
  int buttonSpacing = 4;

  // Project Name row
  {
    auto row = bounds.removeFromTop(ROW_HEIGHT);
    row.removeFromLeft(LABEL_WIDTH + SPACING);
    projectNameEditor_.setBounds(row);
  }
  bounds.removeFromTop(SPACING);

  // Tempo row with stepper buttons and tap tempo
  {
    auto row = bounds.removeFromTop(ROW_HEIGHT);
    row.removeFromLeft(LABEL_WIDTH + SPACING);

    // Tap tempo button on the right
    tapTempoButton_.setBounds(row.removeFromRight(55));
    row.removeFromRight(buttonSpacing);

    // Stepper buttons
    tempoUpButton_.setBounds(row.removeFromRight(buttonSize));
    row.removeFromRight(buttonSpacing);
    tempoDownButton_.setBounds(row.removeFromRight(buttonSize));
    row.removeFromRight(buttonSpacing);

    // Tempo editor takes remaining space
    tempoEditor_.setBounds(row);
  }
  bounds.removeFromTop(SPACING);

  // Time Signature row with stepper buttons
  {
    auto row = bounds.removeFromTop(ROW_HEIGHT);
    row.removeFromLeft(LABEL_WIDTH + SPACING);

    int slashWidth = 20;
    int halfWidth =
        (row.getWidth() - slashWidth - (buttonSize * 4) - (buttonSpacing * 6)) /
        2;

    // Numerator section
    timeSignatureNumEditor_.setBounds(row.removeFromLeft(halfWidth));
    row.removeFromLeft(buttonSpacing);
    timeSignatureNumDownButton_.setBounds(row.removeFromLeft(buttonSize));
    row.removeFromLeft(buttonSpacing);
    timeSignatureNumUpButton_.setBounds(row.removeFromLeft(buttonSize));

    // Slash separator
    row.removeFromLeft(slashWidth);

    // Denominator section
    timeSignatureDenEditor_.setBounds(row.removeFromLeft(halfWidth));
    row.removeFromLeft(buttonSpacing);
    timeSignatureDenDownButton_.setBounds(row.removeFromLeft(buttonSize));
    row.removeFromLeft(buttonSpacing);
    timeSignatureDenUpButton_.setBounds(row.removeFromLeft(buttonSize));
  }
  bounds.removeFromTop(SPACING);

  // Sample Rate row
  {
    auto row = bounds.removeFromTop(ROW_HEIGHT);
    row.removeFromLeft(LABEL_WIDTH + SPACING);
    sampleRateValue_.setBounds(row);
  }
  bounds.removeFromTop(SPACING * 2);

  // Action buttons at bottom
  {
    auto buttonRow = bounds.removeFromBottom(36);
    int buttonWidth = 100;

    cancelButton_.setBounds(buttonRow.removeFromLeft(buttonWidth));
    buttonRow.removeFromLeft(SPACING);
    applyButton_.setBounds(buttonRow.removeFromLeft(buttonWidth));
  }
}

void ProjectSettingsComponent::paintSettingRow(
    juce::Graphics &g, const juce::String &label,
    const juce::Rectangle<int> &bounds) {
  // Label text with subtle color
  g.setColour(juce::Colours::white.withAlpha(0.7f));
  g.setFont(juce::FontOptions(13.0f, juce::Font::plain));
  g.drawText(label, bounds.withWidth(LABEL_WIDTH),
             juce::Justification::centredLeft, false);

  // Subtle bottom divider
  g.setColour(
      juce::Colour(ZenithLookAndFeel::Colors::borderSubtle).withAlpha(0.5f));
  g.drawHorizontalLine(bounds.getBottom(), static_cast<float>(bounds.getX()),
                       static_cast<float>(bounds.getRight()));
}

//==============================================================================
void ProjectSettingsComponent::setupTextEditor(juce::TextEditor &editor,
                                               int maxLength) {
  editor.setFont(juce::FontOptions(14.0f));
  editor.setColour(juce::TextEditor::backgroundColourId,
                   juce::Colour(ZenithLookAndFeel::Elevation::dp4));
  editor.setColour(juce::TextEditor::textColourId,
                   juce::Colours::white.withAlpha(0.95f));
  editor.setColour(juce::TextEditor::outlineColourId,
                   juce::Colour(ZenithLookAndFeel::Colors::borderSubtle));
  editor.setColour(juce::TextEditor::focusedOutlineColourId,
                   juce::Colour(ZenithLookAndFeel::Colors::accentPrimary));
  editor.setColour(juce::CaretComponent::caretColourId,
                   juce::Colour(ZenithLookAndFeel::Colors::accentPrimary));

  editor.setBorder(juce::BorderSize<int>(6));
  editor.setReturnKeyStartsNewLine(false);
  editor.addListener(this);

  if (maxLength > 0) {
    editor.setInputRestrictions(maxLength, "0123456789.");
  }
}

//==============================================================================
void ProjectSettingsComponent::textEditorTextChanged(juce::TextEditor &editor) {
  // Real-time validation while typing
  if (&editor == &tempoEditor_) {
    float newTempo = tempoEditor_.getText().getFloatValue();
    currentTempo_ = validateTempo(newTempo);
  } else if (&editor == &timeSignatureNumEditor_ ||
             &editor == &timeSignatureDenEditor_) {
    int num = timeSignatureNumEditor_.getText().getIntValue();
    int den = timeSignatureDenEditor_.getText().getIntValue();
    validateTimeSignature(num, den);
  }
}

void ProjectSettingsComponent::textEditorReturnKeyPressed(
    juce::TextEditor &editor) {
  if (&editor == &projectNameEditor_) {
    projectState_.setProjectName(projectNameEditor_.getText());
  } else if (&editor == &tempoEditor_) {
    updateTempoFromText();
  } else if (&editor == &timeSignatureNumEditor_ ||
             &editor == &timeSignatureDenEditor_) {
    updateTimeSignatureFromText();
  }
}

void ProjectSettingsComponent::textEditorEscapeKeyPressed(
    juce::TextEditor &editor) {
  // Revert on escape
  if (&editor == &projectNameEditor_) {
    projectNameEditor_.setText(projectState_.getProjectName(), false);
  } else if (&editor == &tempoEditor_) {
    tempoEditor_.setText(juce::String(projectState_.getTempo(), 1), false);
  } else if (&editor == &timeSignatureNumEditor_) {
    timeSignatureNumEditor_.setText(
        juce::String(projectState_.getTimeSignatureNumerator()), false);
  } else if (&editor == &timeSignatureDenEditor_) {
    timeSignatureDenEditor_.setText(
        juce::String(projectState_.getTimeSignatureDenominator()), false);
  }
}

void ProjectSettingsComponent::textEditorFocusLost(juce::TextEditor &editor) {
  // Apply changes when focus is lost
  if (&editor == &projectNameEditor_) {
    projectState_.setProjectName(projectNameEditor_.getText());
  } else if (&editor == &tempoEditor_) {
    updateTempoFromText();
  } else if (&editor == &timeSignatureNumEditor_ ||
             &editor == &timeSignatureDenEditor_) {
    updateTimeSignatureFromText();
  }
}

//==============================================================================
void ProjectSettingsComponent::refresh() {
  projectNameEditor_.setText(projectState_.getProjectName(), false);
  currentTempo_ = projectState_.getTempo();
  tempoEditor_.setText(juce::String(currentTempo_, 1), false);
  timeSignatureNum_ = projectState_.getTimeSignatureNumerator();
  timeSignatureDen_ = projectState_.getTimeSignatureDenominator();
  timeSignatureNumEditor_.setText(juce::String(timeSignatureNum_), false);
  timeSignatureDenEditor_.setText(juce::String(timeSignatureDen_), false);
}

//==============================================================================
void ProjectSettingsComponent::updateTempoFromText() {
  float newTempo = tempoEditor_.getText().getFloatValue();
  newTempo = validateTempo(newTempo);
  projectState_.setTempo(newTempo);
  currentTempo_ = newTempo;
  tempoEditor_.setText(juce::String(newTempo, 1), false);
}

void ProjectSettingsComponent::updateTimeSignatureFromText() {
  int num = timeSignatureNumEditor_.getText().getIntValue();
  int den = timeSignatureDenEditor_.getText().getIntValue();
  validateTimeSignature(num, den);
  projectState_.setTimeSignature(num, den);
  timeSignatureNum_ = num;
  timeSignatureDen_ = den;
}

//==============================================================================
float ProjectSettingsComponent::validateTempo(float value) const {
  // Valid range: 1 BPM to 960 BPM
  return juce::jlimit(1.0f, 960.0f, value);
}

void ProjectSettingsComponent::validateTimeSignature(int &num, int &den) const {
  // Valid numerator: 1-16
  num = juce::jlimit(1, 16, num);

  // Valid denominator: 1, 2, 4, 8, 16, 32
  if (den != 1 && den != 2 && den != 4 && den != 8 && den != 16 && den != 32) {
    den = 4; // Default to 4/4
  }
}

//==============================================================================
void ProjectSettingsComponent::timerCallback() {
  // Update focus animations (60Hz)
  const float animationSpeed = 0.15f;

  float nameTarget = projectNameEditor_.hasKeyboardFocus(true) ? 1.0f : 0.0f;
  nameEditorFocusAnim_ += (nameTarget - nameEditorFocusAnim_) * animationSpeed;

  float tempoTarget = tempoEditor_.hasKeyboardFocus(true) ? 1.0f : 0.0f;
  tempoEditorFocusAnim_ +=
      (tempoTarget - tempoEditorFocusAnim_) * animationSpeed;

  float numTarget =
      timeSignatureNumEditor_.hasKeyboardFocus(true) ? 1.0f : 0.0f;
  timeSignatureNumFocusAnim_ +=
      (numTarget - timeSignatureNumFocusAnim_) * animationSpeed;

  float denTarget =
      timeSignatureDenEditor_.hasKeyboardFocus(true) ? 1.0f : 0.0f;
  timeSignatureDenFocusAnim_ +=
      (denTarget - timeSignatureDenFocusAnim_) * animationSpeed;

  // Repaint if any animation is active
  if (std::abs(nameEditorFocusAnim_ - nameTarget) > 0.01f ||
      std::abs(tempoEditorFocusAnim_ - tempoTarget) > 0.01f ||
      std::abs(timeSignatureNumFocusAnim_ - numTarget) > 0.01f ||
      std::abs(timeSignatureDenFocusAnim_ - denTarget) > 0.01f) {
    repaint();
  }
}

void ProjectSettingsComponent::adjustTempo(float delta) {
  float newTempo = currentTempo_ + delta;
  newTempo = validateTempo(newTempo);
  currentTempo_ = newTempo;
  tempoEditor_.setText(juce::String(newTempo, 1), false);
  tempoValid_ = true;
  repaint();
}

void ProjectSettingsComponent::adjustTimeSignatureNum(int delta) {
  int newNum = timeSignatureNum_ + delta;
  int den = timeSignatureDen_;
  validateTimeSignature(newNum, den);
  timeSignatureNum_ = newNum;
  timeSignatureNumEditor_.setText(juce::String(newNum), false);
  timeSignatureValid_ = true;
  repaint();
}

void ProjectSettingsComponent::adjustTimeSignatureDen(int delta) {
  // For denominator, use power-of-2 stepping
  int currentPower = static_cast<int>(std::log2(timeSignatureDen_));
  int newPower =
      juce::jlimit(0, 5, currentPower + delta); // 2^0 to 2^5 (1 to 32)
  int newDen = static_cast<int>(std::pow(2, newPower));

  int num = timeSignatureNum_;
  validateTimeSignature(num, newDen);
  timeSignatureDen_ = newDen;
  timeSignatureDenEditor_.setText(juce::String(newDen), false);
  timeSignatureValid_ = true;
  repaint();
}

void ProjectSettingsComponent::onTapTempo() {
  juce::int64 currentTime = juce::Time::currentTimeMillis();

  // Reset if more than 2 seconds since last tap
  if (currentTime - lastTapTime_ > 2000) {
    tapTimes_.clear();
  }

  tapTimes_.push_back(currentTime);
  lastTapTime_ = currentTime;

  // Need at least 2 taps to calculate tempo
  if (tapTimes_.size() >= 2) {
    // Calculate average time between taps
    juce::int64 totalInterval = 0;
    for (size_t i = 1; i < tapTimes_.size(); ++i) {
      totalInterval += tapTimes_[i] - tapTimes_[i - 1];
    }

    double averageInterval =
        static_cast<double>(totalInterval) / (tapTimes_.size() - 1);
    double newTempo = 60000.0 / averageInterval; // Convert ms to BPM

    newTempo = validateTempo(static_cast<float>(newTempo));
    currentTempo_ = newTempo;
    tempoEditor_.setText(juce::String(newTempo, 1), false);
    tempoValid_ = true;
    repaint();
  }

  // Keep only last 8 taps
  if (tapTimes_.size() > 8) {
    tapTimes_.erase(tapTimes_.begin());
  }
}

void ProjectSettingsComponent::applyChanges() {
  // Apply all changes to project state
  projectState_.setProjectName(projectNameEditor_.getText());
  projectState_.setTempo(currentTempo_);
  projectState_.setTimeSignature(timeSignatureNum_, timeSignatureDen_);

  // Visual feedback (could add a confirmation toast)
  repaint();
}

void ProjectSettingsComponent::cancelChanges() {
  // Revert all editors to project state
  refresh();
  repaint();
}

} // namespace zenith
