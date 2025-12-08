/**
 * @file TrackHeaderComponent.cpp
 * @brief Track header implementation
 */

// POLISH: spacing normalized to 8px grid (stripe 8px, padding 4/8, buttons 24x24, radius 4px)
// POLISH: typography now uses SkiaTheme::Typography (body)
// POLISH: flattened background (removed gradient), unified hover/active using theme

#include "../../include/ui/TrackHeaderComponent.h"

#include <core/SkFont.h>
#include <core/SkPaint.h>
#include <core/SkPath.h>
#include <core/SkRRect.h>
#include <effects/SkGradientShader.h>

namespace zenith {

//==============================================================================
TrackHeaderComponent::TrackHeaderComponent(ProjectState& projectState, const juce::String& trackId)
    : projectState_(projectState), trackId_(trackId)
{
    // Start 60Hz animation timer
    startTimerHz(60);

    // Get track node from ProjectState
    trackNode_ = projectState_.getTrack(trackId_);
    jassert(trackNode_.isValid());

    // Listen to track node changes
    if (trackNode_.isValid())
        trackNode_.addListener(this);

    // Setup name label (editable) with Apple styling
    auto& typo = zenith::SkiaTheme::getInstance().getTypography();
    nameLabel_.setEditable(true);
    nameLabel_.setJustificationType(juce::Justification::centredLeft);
    nameLabel_.setFont(juce::FontOptions(typo.header.size));
    nameLabel_.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.95f));
    nameLabel_.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    nameLabel_.setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);
    nameLabel_.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff2C2C2E));
    nameLabel_.setColour(juce::TextEditor::textColourId, juce::Colours::white.withAlpha(0.95f));
    nameLabel_.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff3A3A3C));
    nameLabel_.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(0xff0A84FF));
    nameLabel_.onTextChange = [this]() { onNameChanged(); };
    nameLabel_.onEditorShow = [this]() { nameHasFocus_ = true; };
    nameLabel_.onEditorHide = [this]() { nameHasFocus_ = false; };
    addAndMakeVisible(nameLabel_);

    // Init buttons
    muteButton_.setText("M");
    muteButton_.setToggleable(true);
    muteButton_.setStyle(zenith::SkiaButton::Style::Secondary);
    muteButton_.setToggleState(isMuted_);
    muteButton_.onClick = [this]() { onMuteClicked(); };
    addAndMakeVisible(muteButton_);

    soloButton_.setText("S");
    soloButton_.setToggleable(true);
    soloButton_.setStyle(zenith::SkiaButton::Style::Secondary);
    soloButton_.setToggleState(isSoloed_);
    soloButton_.onClick = [this]() { onSoloClicked(); };
    addAndMakeVisible(soloButton_);

    armButton_.setText("R");
    armButton_.setToggleable(true);
    armButton_.setStyle(zenith::SkiaButton::Style::Secondary);
    armButton_.setToggleState(isArmed_);
    armButton_.onClick = [this]() { onArmClicked(); };
    addAndMakeVisible(armButton_);

    // Load initial state
    updateFromState();
}

TrackHeaderComponent::~TrackHeaderComponent()
{
    if (trackNode_.isValid())
        trackNode_.removeListener(this);
}

//==============================================================================
void TrackHeaderComponent::drawSkia(SkCanvas* canvas)
{
    auto bounds = getLocalBounds();
    using namespace zenith;
    auto& theme = SkiaTheme::getInstance();
    auto colors = theme.getColors();

    float width = (float)bounds.getWidth();
    float height = (float)bounds.getHeight();

    // Background - flat for modern look
    SkPaint bgPaint;
    bgPaint.setColor(colors.bg2);
    bgPaint.setAntiAlias(true);

    SkRRect bgRRect = SkRRect::MakeRectXY(SkRect::MakeWH(width, height), 4.0f, 4.0f);
    canvas->drawRRect(bgRRect, bgPaint);

    // Delicate hover glow
    if (isHovered_)
    {
        SkPaint hoverPaint;
        hoverPaint.setColor(SkColorSetARGB(13, 255, 255, 255));  // Slightly stronger (5% alpha)
        hoverPaint.setAntiAlias(true);
        canvas->drawRRect(bgRRect, hoverPaint);
    }

    // Color stripe (left edge, 8px wide) - flat for clarity
    SkRect stripeRect = SkRect::MakeXYWH(0, 0, 8.0f, height);

    SkPaint stripePaint;
    stripePaint.setColor(SkColorSetARGB(trackColour_.getAlpha(), trackColour_.getRed(),
                                         trackColour_.getGreen(), trackColour_.getBlue()));
    stripePaint.setAntiAlias(true);

    // Rounded stripe (left side only)
    SkRRect stripeRRect = SkRRect::MakeRectXY(stripeRect, 4.0f, 4.0f);
    canvas->drawRRect(stripeRRect, stripePaint);

    // Subtle bottom border
    SkPaint borderPaint;
    borderPaint.setColor(SkColorSetARGB(128, 58, 58, 60));  // #3A3A3C.withAlpha(0.5)
    borderPaint.setStrokeWidth(0.5f);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setAntiAlias(true);
    canvas->drawLine(2.0f, height - 0.5f, width - 2.0f, height - 0.5f, borderPaint);

    // Name editor focus glow
    if (nameFocusAnim_ > 0.01f)
    {
        auto nameBounds = nameLabel_.getBounds().toFloat().expanded(2.0f);
        SkPaint focusPaint;
        focusPaint.setColor(SkColorSetARGB((int)(nameFocusAnim_ * 0.3f * 255), 10, 132, 255));  // #0A84FF
        focusPaint.setStrokeWidth(2.0f);
        focusPaint.setStyle(SkPaint::kStroke_Style);
        focusPaint.setAntiAlias(true);
        SkRRect focusRRect = SkRRect::MakeRectXY(
            SkRect::MakeXYWH(nameBounds.getX(), nameBounds.getY(), nameBounds.getWidth(), nameBounds.getHeight()),
            4.0f, 4.0f);
        canvas->drawRRect(focusRRect, focusPaint);
    }
}

void TrackHeaderComponent::resized()
{
    auto bounds = getLocalBounds();

    // Remove color stripe area
    bounds.removeFromLeft(8);

    // Add padding
    bounds.reduce(4, 4);

    // Buttons on the right (24x24 each)
    const int buttonWidth = 24;
    const int buttonHeight = 24;
    const int spacing = 4;

    auto buttonArea = bounds.removeFromRight(buttonWidth * 3 + spacing * 2);
    buttonArea = buttonArea.withSizeKeepingCentre(buttonWidth * 3 + spacing * 2, buttonHeight);

    muteButton_.setBounds(buttonArea.removeFromLeft(buttonWidth));
    buttonArea.removeFromLeft(spacing);
    soloButton_.setBounds(buttonArea.removeFromLeft(buttonWidth));
    buttonArea.removeFromLeft(spacing);
    armButton_.setBounds(buttonArea.removeFromLeft(buttonWidth));

    // Name label takes remaining space
    bounds.removeFromRight(spacing);  // Spacing between name and buttons
    nameLabel_.setBounds(bounds);
}

void TrackHeaderComponent::timerCallback()
{
    // Smooth focus animation (60Hz, 0.15 speed)
    bool needsRepaint = false;

    // Name editor focus animation
    float nameTarget = nameHasFocus_ ? 1.0f : 0.0f;
    if (std::abs(nameFocusAnim_ - nameTarget) > 0.01f)
    {
        nameFocusAnim_ += (nameTarget - nameFocusAnim_) * 0.15f;
        needsRepaint = true;
    }

    if (needsRepaint)
        repaint();
}

//==============================================================================
// ValueTree::Listener (MESSAGE THREAD)
//==============================================================================

void TrackHeaderComponent::valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property)
{
    if (tree != trackNode_)
        return;

    // Update UI when properties change
    updateFromState();
}

//==============================================================================
// UI Callbacks
//==============================================================================

void TrackHeaderComponent::onNameChanged()
{
    auto newName = nameLabel_.getText();
    projectState_.renameTrack(trackId_, newName, "Change Track Name");
}

void TrackHeaderComponent::onMuteClicked()
{
    bool newMuted = !isMuted_;  // Toggle current state
    projectState_.setTrackMute(trackId_, newMuted, "Toggle Mute");
}

void TrackHeaderComponent::onSoloClicked()
{
    bool newSoloed = !isSoloed_;  // Toggle current state
    projectState_.setTrackSolo(trackId_, newSoloed, "Toggle Solo");
}

void TrackHeaderComponent::onArmClicked()
{
    bool newArmed = !isArmed_;  // Toggle current state
    projectState_.setTrackArmed(trackId_, newArmed, "Toggle Record Arm");
}

//==============================================================================
// Helper Methods
//==============================================================================

void TrackHeaderComponent::updateFromState()
{
    if (!trackNode_.isValid())
        return;

    // Update name
    juce::String name = trackNode_[ProjectState::PROP_NAME].toString();
    if (nameLabel_.getText() != name)
       nameLabel_.setText(name, juce::dontSendNotification);

    // Update color
    if (trackNode_.hasProperty(ProjectState::PROP_COLOR)) {
        juce::String colorStr = trackNode_[ProjectState::PROP_COLOR].toString();
        trackColour_ = juce::Colour::fromString(colorStr);
    } else {
        trackColour_ = juce::Colours::grey;
    }

    // Update button states
    isMuted_ = trackNode_[ProjectState::PROP_MUTE];
    isSoloed_ = trackNode_[ProjectState::PROP_SOLO];
    isArmed_ = trackNode_[ProjectState::PROP_ARMED];

    // Update button toggle states
    muteButton_.setToggleState(isMuted_);
    soloButton_.setToggleState(isSoloed_);
    armButton_.setToggleState(isArmed_);

    // Update button styles based on state
    // Muted: grey out name, set button to Danger style
    if (isMuted_)
    {
        nameLabel_.setColour(juce::Label::textColourId, juce::Colours::grey);
        muteButton_.setStyle(zenith::SkiaButton::Style::Danger);
    }
    else
    {
        nameLabel_.setColour(juce::Label::textColourId, juce::Colours::white);
        muteButton_.setStyle(zenith::SkiaButton::Style::Secondary);
    }

    // Soloed: highlight S button with Warning style (orange)
    if (isSoloed_)
        soloButton_.setStyle(zenith::SkiaButton::Style::Warn);
    else
        soloButton_.setStyle(zenith::SkiaButton::Style::Secondary);

    // Armed: highlight R button with Danger style (red)
    if (isArmed_)
        armButton_.setStyle(zenith::SkiaButton::Style::Danger);
    else
        armButton_.setStyle(zenith::SkiaButton::Style::Secondary);

    repaint();
}

} // namespace zenith
