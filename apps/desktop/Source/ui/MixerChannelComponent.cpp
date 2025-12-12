/**
 * @file MixerChannelComponent.cpp
 * @brief Mixer channel strip implementation
 */

#include "../../include/ui/MixerChannelComponent.h"
#include "../../Source/engine/Track.h"
#include "../../Source/ui/skia/SkiaTheme.h" // For SkiaTheme
#include "../../Source/ui/skia/ZenithDesignSystem.h" // For ThemeManager
#include "../../Source/ui/skia/GlassmorphicPanel.h" // Added
#include "../../Source/ui/skia/NeonGlow.h"          // Added

#include <core/SkCanvas.h>
#include <core/SkRRect.h>

//==============================================================================
namespace zenith {

MixerChannelComponent::MixerChannelComponent(Track *track)
    : track_(track)
#ifdef ZENITH_USE_SKIA
      ,
      faderSlider_("Vol"), panKnob_("Pan"), muteButton_("M"), soloButton_("S")
#endif
{
  jassert(track_ != nullptr);
  track_->addChangeListener(this);
  zenith::design::ThemeManager::getInstance().addChangeListener(this);

  // Initialize UI from track
  updateFromTrack();

  // Track name label
  nameLabel_.setText(track_->getName(), juce::dontSendNotification);
  nameLabel_.setJustificationType(juce::Justification::centred);
  nameLabel_.setFont(juce::FontOptions(14.0f, juce::Font::bold));
  addAndMakeVisible(nameLabel_);

#ifdef ZENITH_USE_SKIA
  // GPU-accelerated volume fader with spring physics
  faderSlider_.setOrientation(zenith::SkiaSlider::Orientation::Vertical);
  faderSlider_.setStyle(zenith::SkiaSlider::Style::Fader);
  faderSlider_.setDisplayRange(0.0, 1.0);
  faderSlider_.setValue(track_->getVolume());

  faderSlider_.onValueChange = [this](float value) { onFaderChanged(); };
  addAndMakeVisible(faderSlider_);

  // GPU-accelerated pan knob with spring physics
  panKnob_.setDisplayRange(-1.0, 1.0);
  panKnob_.setValue(track_->getPan());
  panKnob_.setDefaultValue(0.0); // Center is default
  // panKnob_.setLabel("Pan"); // Not in API?
  panKnob_.onValueChange = [this](float value) { onPanChanged(); };
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
  faderSlider_.setRange(0.0f, 1.0f, 0.7f); // Min, Max, Default
  faderSlider_.setValue(track_->getVolume(), false);
  faderSlider_.setLabel("Vol");
  faderSlider_.setSuffix("dB");
  faderSlider_.onValueChange = [this](float value) { onFaderChanged(); };
  addAndMakeVisible(faderSlider_);

  panKnob_.setRange(-1.0f, 1.0f, 0.0f); // Min, Max, Default (center)
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

  // Spectrum Analyzer
  spectrumAnalyzer_ = std::make_unique<SkiaSpectrumComponent>();
  // Use vertical bars for small channel strip view
  spectrumAnalyzer_->setDisplayMode(SkiaSpectrumComponent::DisplayMode::FilledCurve);
  addAndMakeVisible(spectrumAnalyzer_.get());
  
  // Link to track's audio processing
  if (track_) {
      track_->getMixerChannel().setSpectrumFifo(&spectrumAnalyzer_->getAudioFifo());
  }

  // Level meter
  addAndMakeVisible(meter_);

  // Start timer for meter updates (30 Hz)
  startTimer(33);

  setSize(80, 400);
}

MixerChannelComponent::~MixerChannelComponent() {
  if (track_)
    track_->removeChangeListener(this);
  zenith::design::ThemeManager::getInstance().removeChangeListener(this);
  stopTimer();
}

void MixerChannelComponent::changeListenerCallback(
    juce::ChangeBroadcaster *source) {
  if (source == track_) {
    // UI update on message thread
    updateFromTrack();
  } else if (source == &zenith::design::ThemeManager::getInstance()) {
    repaint();
  }
}

//==============================================================================
void MixerChannelComponent::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();
  SkRect skBounds = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());

  // Background (Glassmorphic)
  zenith::GlassmorphicPanel::draw(canvas, skBounds, zenith::GlassmorphicPanel::Style::Elevated);

  // Children are drawn by SkiaComponent::drawChildren
  drawChildren(canvas);
}




void MixerChannelComponent::resized() {
  auto bounds = getLocalBounds().reduced(8);

  // Track name at top
  nameLabel_.setBounds(bounds.removeFromTop(30));
  bounds.removeFromTop(4); // Spacing
  
  // Spectrum Analyzer
  if (spectrumAnalyzer_) {
      spectrumAnalyzer_->setBounds(bounds.removeFromTop(60).reduced(2));
      bounds.removeFromTop(4);
  }

  // Mute/Solo buttons at bottom
  auto buttonArea = bounds.removeFromBottom(64);
  muteButton_.setBounds(buttonArea.removeFromTop(30).reduced(2));
  buttonArea.removeFromTop(4); // Spacing
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
void MixerChannelComponent::timerCallback() {
  // Update meter from track level (thread-safe read via atomic)
  if (track_ != nullptr) {
    float level = track_->getCurrentLevel();
    meter_.setLevel(level);
    meter_.repaint();
  }
}

void MixerChannelComponent::updateFromTrack() {
  if (track_ == nullptr)
    return;

  updatingControls_ = true;

  // Update fader
  faderSlider_.setValue(track_->getVolume());

  // Update pan knob
  panKnob_.setValue(track_->getPan());

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
void MixerChannelComponent::onFaderChanged() {
  if (updatingControls_ || track_ == nullptr)
    return;

  // Update track volume (thread-safe via atomic)
  float newVolume = faderSlider_.getValue();
  track_->setVolume(newVolume);
}

void MixerChannelComponent::onPanChanged() {
  if (updatingControls_ || track_ == nullptr)
    return;

  // Update track pan (thread-safe via atomic)
  float newPan = panKnob_.getValue();
  track_->setPan(newPan);
}

void MixerChannelComponent::onMuteClicked() {
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

void MixerChannelComponent::onSoloClicked() {
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

MixerChannelComponent::LevelMeter::LevelMeter() {
  // Start animation timer at 60 Hz for smooth meter ballistics
  startTimerHz(60);
}

MixerChannelComponent::LevelMeter::~LevelMeter() { stopTimer(); }

void MixerChannelComponent::LevelMeter::timerCallback() {
  // Smooth meter ballistics (attack fast, decay slower)
  float target = targetLevel_.load();
  const float attackSpeed = 0.8f; // Fast attack
  const float decaySpeed = 0.05f; // Slow decay

  if (target > currentLevel_) {
    // Attack: rise quickly
    currentLevel_ += (target - currentLevel_) * attackSpeed;
  } else {
    // Decay: fall slowly
    currentLevel_ += (target - currentLevel_) * decaySpeed;
  }

  // Peak hold logic
  if (currentLevel_ > peakLevel_) {
    peakLevel_ = currentLevel_;
    peakHoldCounter_ = 120; // Hold peak for 2 seconds (at 60 Hz)
  } else if (peakHoldCounter_ > 0) {
    peakHoldCounter_--;
  } else {
    // Peak slowly decays after hold time
    peakLevel_ *= 0.95f;
  }

  // Repaint only if level changed significantly
  if (std::abs(currentLevel_ - target) > 0.001f || peakLevel_ > 0.001f) {
    repaint();
  }
}

void MixerChannelComponent::LevelMeter::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();
  SkRect rect = SkRect::MakeXYWH(bounds.getX(), bounds.getY(),
                                  bounds.getWidth(), bounds.getHeight());

  // Background (Subtle Glass)
  zenith::GlassmorphicPanel::draw(canvas, rect, zenith::GlassmorphicPanel::Style::Subtle);

  // Level bar
  if (currentLevel_ > 0.001f) {
    float levelDb = juce::Decibels::gainToDecibels(currentLevel_);
    float normalizedLevel = juce::jmap(levelDb, -60.0f, 0.0f, 0.0f, 1.0f);
    normalizedLevel = juce::jlimit(0.0f, 1.0f, normalizedLevel);

    if (normalizedLevel > 0.01f) {
        float barHeight = bounds.getHeight() * normalizedLevel;
        auto meterRect = SkRect::MakeXYWH(bounds.getX(), bounds.getBottom() - barHeight, 
                                          bounds.getWidth(), barHeight);
        meterRect.inset(2.0f, 2.0f);

        // Gradient based on level
        SkColor topColor = zenith::design::colors::NEON_GREEN;
        if (normalizedLevel > 0.9f) topColor = zenith::design::colors::RED;
        else if (normalizedLevel > 0.7f) topColor = zenith::design::colors::AMBER;
        else if (normalizedLevel > 0.4f) topColor = zenith::design::colors::BLUE; // Blue -> Greenish

        SkPoint pts[2] = {{meterRect.centerX(), meterRect.bottom()}, {meterRect.centerX(), meterRect.top()}};
        SkColor colors[2] = {zenith::design::colors::NEON_GREEN, topColor};
        
        SkPaint paint;
        paint.setShader(SkGradientShader::MakeLinear(pts, colors, nullptr, 2, SkTileMode::kClamp));
        paint.setAntiAlias(true);
        canvas->drawRoundRect(meterRect, 2.0f, 2.0f, paint);

        // Peak Glow utilizing helper
        zenith::NeonGlow::drawVUMeterGlow(canvas, rect, normalizedLevel, true);
    }
  }

  // Peak indicator
  if (peakLevel_ > 0.001f) {
    float peakDb = juce::Decibels::gainToDecibels(peakLevel_);
    float normalizedPeak = juce::jmap(peakDb, -60.0f, 0.0f, 0.0f, 1.0f);
    normalizedPeak = juce::jlimit(0.0f, 1.0f, normalizedPeak);
    
    // Draw simple line for peak
    float peakY = bounds.getBottom() - (bounds.getHeight() * normalizedPeak);
    SkPaint peakPaint;
    peakPaint.setColor(normalizedPeak > 0.95f ? zenith::design::colors::RED : SK_ColorWHITE);
    peakPaint.setAntiAlias(true);
    canvas->drawRect(SkRect::MakeXYWH(bounds.getX(), peakY, bounds.getWidth(), 1.0f), peakPaint);
  }
}

void MixerChannelComponent::LevelMeter::setLevel(float level) {
  targetLevel_.store(juce::jlimit(0.0f, 1.0f, level));
}

} // namespace zenith
