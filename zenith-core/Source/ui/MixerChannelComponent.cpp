/**
 * @file MixerChannelComponent.cpp
 * @brief Mixer channel strip implementation
 */

#include "../../include/ui/MixerChannelComponent.h"
#include "../../Source/engine/Track.h"

//==============================================================================
MixerChannelComponent::MixerChannelComponent(zenith::Track* track)
    : track_(track)
#ifdef ZENITH_USE_SKIA
    , faderSlider_(zenith::SkiaSliderComponent::Orientation::Vertical, zenith::SkiaSliderComponent::Style::Linear)
    , panKnob_(zenith::SkiaKnobComponent::Style::Continuous)
    , muteButton_("M", zenith::SkiaButtonComponent::Style::Secondary)
    , soloButton_("S", zenith::SkiaButtonComponent::Style::Secondary)
#endif
{
    jassert(track_ != nullptr);

    // Track name label
    nameLabel_.setText(track_->getName(), juce::dontSendNotification);
    nameLabel_.setJustificationType(juce::Justification::centred);
    nameLabel_.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    addAndMakeVisible(nameLabel_);

#ifdef ZENITH_USE_SKIA
    // GPU-accelerated volume fader with spring physics
    faderSlider_.setRange(0.0, 1.0);
    faderSlider_.setValue(track_->getVolume(), false);
    faderSlider_.setTextSuffix(" dB");
    faderSlider_.onValueChange = [this](double value) { onFaderChanged(); };
    addAndMakeVisible(faderSlider_);

    // GPU-accelerated pan knob with spring physics
    panKnob_.setRange(-1.0, 1.0);
    panKnob_.setValue(track_->getPan(), false);
    panKnob_.setDefaultValue(0.0);  // Center is default
    panKnob_.setLabel("Pan");
    panKnob_.onValueChange = [this](double value) { onPanChanged(); };
    addAndMakeVisible(panKnob_);

    // GPU-accelerated buttons with spring physics
    muteButton_.setToggleable(true);
    muteButton_.setToggleState(track_->isMuted());
    muteButton_.onClick = [this]() { onMuteClicked(); };
    addAndMakeVisible(muteButton_);

    soloButton_.setToggleable(true);
    soloButton_.setToggleState(track_->isSolo());
    soloButton_.onClick = [this]() { onSoloClicked(); };
    addAndMakeVisible(soloButton_);
#else
    // Fallback: Traditional JUCE custom components
    faderSlider_.setRange(0.0f, 1.0f, 0.7f);  // Min, Max, Default
    faderSlider_.setValue(track_->getVolume(), false);
    faderSlider_.setLabel("Vol");
    faderSlider_.setSuffix("dB");
    faderSlider_.onValueChange = [this](float value) { onFaderChanged(); };
    addAndMakeVisible(faderSlider_);

    panKnob_.setRange(-1.0f, 1.0f, 0.0f);  // Min, Max, Default (center)
    panKnob_.setValue(track_->getPan(), false);
    panKnob_.setLabel("Pan");
    panKnob_.onValueChange = [this](float value) { onPanChanged(); };
    addAndMakeVisible(panKnob_);

    muteButton_.setButtonText("M");
    muteButton_.setToggleable(true);
    muteButton_.setButtonStyle(zenith::ZenithButton::Secondary);
    muteButton_.setToggleState(track_->isMuted(), false);
    muteButton_.onClick = [this]() { onMuteClicked(); };
    addAndMakeVisible(muteButton_);

    soloButton_.setButtonText("S");
    soloButton_.setToggleable(true);
    soloButton_.setButtonStyle(zenith::ZenithButton::Secondary);
    soloButton_.setToggleState(track_->isSolo(), false);
    soloButton_.onClick = [this]() { onSoloClicked(); };
    addAndMakeVisible(soloButton_);
#endif

    // Level meter
    addAndMakeVisible(meter_);

    // Start timer for meter updates (30 Hz)
    startTimer(33);

    setSize(80, 400);
}

MixerChannelComponent::~MixerChannelComponent()
{
    stopTimer();
}

//==============================================================================
void MixerChannelComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Background with Ableton-style gradient
    juce::ColourGradient backgroundGradient(
        juce::Colour(0xff323232), bounds.getCentreX(), bounds.getY(),
        juce::Colour(0xff252525), bounds.getCentreX(), bounds.getBottom(),
        false);
    g.setGradientFill(backgroundGradient);
    g.fillRoundedRectangle(bounds, 6.0f);

    // Inner highlight at top (subtle)
    g.setColour(juce::Colour(0xffffffff).withAlpha(0.03f));
    auto highlightBounds = bounds.withHeight(bounds.getHeight() * 0.2f);
    g.fillRoundedRectangle(highlightBounds, 6.0f);

    // Subtle border
    g.setColour(juce::Colour(0xff3a3a3a).withAlpha(0.5f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 6.0f, 1.0f);
}

void MixerChannelComponent::resized()
{
    auto bounds = getLocalBounds().reduced(8);

    // Track name at top
    nameLabel_.setBounds(bounds.removeFromTop(30));
    bounds.removeFromTop(4);  // Spacing

    // Mute/Solo buttons at bottom
    auto buttonArea = bounds.removeFromBottom(64);
    muteButton_.setBounds(buttonArea.removeFromTop(30).reduced(2));
    buttonArea.removeFromTop(4);  // Spacing
    soloButton_.setBounds(buttonArea.removeFromTop(30).reduced(2));

    // Pan knob above buttons
    auto panArea = bounds.removeFromBottom(80);
    panKnob_.setBounds(panArea.withSizeKeepingCentre(70, 70));

    // Small spacing
    bounds.removeFromBottom(8);

    // Split remaining space between meter and fader
    auto meterBounds = bounds.removeFromLeft(18);
    meter_.setBounds(meterBounds.reduced(0, 5));

    // Small spacing between meter and fader
    bounds.removeFromLeft(4);

    // Fader takes remaining space
    faderSlider_.setBounds(bounds.reduced(2, 5));
}

//==============================================================================
void MixerChannelComponent::timerCallback()
{
    // Update meter from track level (thread-safe read via atomic)
    if (track_ != nullptr)
    {
        float level = track_->getCurrentLevel();
        meter_.setLevel(level);
        meter_.repaint();
    }
}

void MixerChannelComponent::updateFromTrack()
{
    if (track_ == nullptr)
        return;

    updatingControls_ = true;

    // Update fader
    faderSlider_.setValue(track_->getVolume(), false);

    // Update pan knob
    panKnob_.setValue(track_->getPan(), false);

#ifdef ZENITH_USE_SKIA
    // Skia buttons: toggle state provides visual feedback
    muteButton_.setToggleState(track_->isMuted());
    soloButton_.setToggleState(track_->isSolo());
#else
    // JUCE buttons: toggle state + style changes
    muteButton_.setToggleState(track_->isMuted(), false);
    if (track_->isMuted())
        muteButton_.setButtonStyle(zenith::ZenithButton::Danger);
    else
        muteButton_.setButtonStyle(zenith::ZenithButton::Secondary);

    soloButton_.setToggleState(track_->isSolo(), false);
    if (track_->isSolo())
        soloButton_.setButtonStyle(zenith::ZenithButton::Warning);
    else
        soloButton_.setButtonStyle(zenith::ZenithButton::Secondary);
#endif

    // Update name
    nameLabel_.setText(track_->getName(), juce::dontSendNotification);

    updatingControls_ = false;
}

//==============================================================================
void MixerChannelComponent::onFaderChanged()
{
    if (updatingControls_ || track_ == nullptr)
        return;

    // Update track volume (thread-safe via atomic)
    float newVolume = faderSlider_.getValue();
    track_->setVolume(newVolume);
}

void MixerChannelComponent::onPanChanged()
{
    if (updatingControls_ || track_ == nullptr)
        return;

    // Update track pan (thread-safe via atomic)
    float newPan = panKnob_.getValue();
    track_->setPan(newPan);
}

void MixerChannelComponent::onMuteClicked()
{
    if (updatingControls_ || track_ == nullptr)
        return;

    // Toggle mute (thread-safe via atomic)
    bool newMuted = muteButton_.getToggleState();
    track_->setMuted(newMuted);

#ifndef ZENITH_USE_SKIA
    // Update button style (JUCE buttons only)
    if (newMuted)
        muteButton_.setButtonStyle(zenith::ZenithButton::Danger);
    else
        muteButton_.setButtonStyle(zenith::ZenithButton::Secondary);
#endif
}

void MixerChannelComponent::onSoloClicked()
{
    if (updatingControls_ || track_ == nullptr)
        return;

    // Toggle solo (thread-safe via atomic)
    bool newSolo = soloButton_.getToggleState();
    track_->setSolo(newSolo);

#ifndef ZENITH_USE_SKIA
    // Update button style (JUCE buttons only)
    if (newSolo)
        soloButton_.setButtonStyle(zenith::ZenithButton::Warning);
    else
        soloButton_.setButtonStyle(zenith::ZenithButton::Secondary);
#endif
}

//==============================================================================
// LevelMeter Implementation
//==============================================================================

MixerChannelComponent::LevelMeter::LevelMeter()
{
    // Start animation timer at 60 Hz for smooth meter ballistics
    startTimerHz(60);
}

MixerChannelComponent::LevelMeter::~LevelMeter()
{
    stopTimer();
}

void MixerChannelComponent::LevelMeter::timerCallback()
{
    // Smooth meter ballistics (attack fast, decay slower)
    float target = targetLevel_.load();
    const float attackSpeed = 0.8f;   // Fast attack
    const float decaySpeed = 0.05f;   // Slow decay

    if (target > currentLevel_)
    {
        // Attack: rise quickly
        currentLevel_ += (target - currentLevel_) * attackSpeed;
    }
    else
    {
        // Decay: fall slowly
        currentLevel_ += (target - currentLevel_) * decaySpeed;
    }

    // Peak hold logic
    if (currentLevel_ > peakLevel_)
    {
        peakLevel_ = currentLevel_;
        peakHoldCounter_ = 120;  // Hold peak for 2 seconds (at 60 Hz)
    }
    else if (peakHoldCounter_ > 0)
    {
        peakHoldCounter_--;
    }
    else
    {
        // Peak slowly decays after hold time
        peakLevel_ *= 0.95f;
    }

    // Repaint only if level changed significantly
    if (std::abs(currentLevel_ - target) > 0.001f || peakLevel_ > 0.001f)
    {
        repaint();
    }
}

void MixerChannelComponent::LevelMeter::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Background with gradient (darker at top, lighter at bottom)
    juce::ColourGradient backgroundGradient(
        juce::Colour(0xff1a1a1a), bounds.getCentreX(), bounds.getY(),
        juce::Colour(0xff222222), bounds.getCentreX(), bounds.getBottom(),
        false);
    g.setGradientFill(backgroundGradient);
    g.fillRoundedRectangle(bounds, 3.0f);

    // Inner shadow at top
    g.setColour(juce::Colour(0x00000000).withAlpha(0.3f));
    g.fillRoundedRectangle(bounds.withHeight(bounds.getHeight() * 0.1f), 3.0f);

    // Level bar (using smoothed animated level)
    if (currentLevel_ > 0.001f)
    {
        // Convert to dB for better visualization
        float levelDb = juce::Decibels::gainToDecibels(currentLevel_);

        // Map -60dB to 0dB -> 0.0 to 1.0
        float normalizedLevel = juce::jmap(levelDb, -60.0f, 0.0f, 0.0f, 1.0f);
        normalizedLevel = juce::jlimit(0.0f, 1.0f, normalizedLevel);

        float barHeight = bounds.getHeight() * normalizedLevel;
        auto meterBounds = bounds.withTop(bounds.getBottom() - barHeight).reduced(2.0f);

        // Beautiful gradient based on level
        juce::Colour topColor, bottomColor;
        if (normalizedLevel > 0.9f)
        {
            // Clipping warning - red gradient
            topColor = juce::Colour(0xffff6666);
            bottomColor = juce::Colour(0xffff453a);  // Apple red
        }
        else if (normalizedLevel > 0.7f)
        {
            // Hot - orange/yellow gradient
            topColor = juce::Colour(0xffffcc00);
            bottomColor = juce::Colour(0xffff9500);  // Apple orange
        }
        else if (normalizedLevel > 0.4f)
        {
            // Moderate - green/yellow gradient
            topColor = juce::Colour(0xff66ff66);
            bottomColor = juce::Colour(0xff34c759);  // Apple green
        }
        else
        {
            // Normal - blue/green gradient
            topColor = juce::Colour(0xff5ab4ff);
            bottomColor = juce::Colour(0xff4a9eff);  // Apple blue
        }

        // Apply gradient to meter bar
        juce::ColourGradient meterGradient(
            topColor, meterBounds.getCentreX(), meterBounds.getY(),
            bottomColor, meterBounds.getCentreX(), meterBounds.getBottom(),
            false);
        g.setGradientFill(meterGradient);
        g.fillRoundedRectangle(meterBounds, 2.0f);

        // Highlight at top of bar for 3D effect
        g.setColour(juce::Colour(0xffffffff).withAlpha(0.2f));
        auto highlightBounds = meterBounds.withHeight(meterBounds.getHeight() * 0.3f);
        g.fillRoundedRectangle(highlightBounds, 2.0f);
    }

    // Peak indicator (thin line at peak level)
    if (peakLevel_ > 0.001f)
    {
        float peakDb = juce::Decibels::gainToDecibels(peakLevel_);
        float normalizedPeak = juce::jmap(peakDb, -60.0f, 0.0f, 0.0f, 1.0f);
        normalizedPeak = juce::jlimit(0.0f, 1.0f, normalizedPeak);

        float peakY = bounds.getBottom() - (bounds.getHeight() * normalizedPeak);
        auto peakBounds = juce::Rectangle<float>(
            bounds.getX() + 2.0f, peakY - 1.0f,
            bounds.getWidth() - 4.0f, 2.0f);

        // Peak color (red if clipping, otherwise white)
        juce::Colour peakColor = normalizedPeak > 0.95f
            ? juce::Colour(0xffff453a)  // Red for clipping
            : juce::Colour(0xffffffff);  // White for normal

        g.setColour(peakColor.withAlpha(0.9f));
        g.fillRoundedRectangle(peakBounds, 1.0f);
    }

    // Subtle border
    g.setColour(juce::Colour(0xff3a3a3a).withAlpha(0.5f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 3.0f, 1.0f);
}

void MixerChannelComponent::LevelMeter::setLevel(float level)
{
    targetLevel_.store(juce::jlimit(0.0f, 1.0f, level));
}

