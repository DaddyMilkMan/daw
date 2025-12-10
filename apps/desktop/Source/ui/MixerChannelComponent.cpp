/**
 * @file MixerChannelComponent.cpp
 * @brief Mixer channel strip implementation
 */

#include "../../include/ui/MixerChannelComponent.h"
#include "../../Source/engine/Track.h"
#include "../../Source/ui/skia/SkiaTheme.h" // For SkiaTheme

#include <skia/include/core/SkCanvas.h>
#include <skia/include/core/SkRRect.h>

//==============================================================================
namespace zenith {

MixerChannelComponent::MixerChannelComponent(Track *track)
    : track_(track), faderSlider_("Vol"), panKnob_("Pan"), muteButton_("M"),
      soloButton_("S") {
  jassert(track_ != nullptr);
  track_->addChangeListener(this);

  // Initialize UI from track
  // Initialize UI from track
  updateFromTrack();
  updateNameCache(); // Initialize cache

  // Track name label
  // Track name is drawn in drawSkia

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

  // Level meter
  addAndMakeVisible(meter_);

  // Start timer for meter updates (30 Hz)
  startTimer(33);

  setSize(80, 400);
}

MixerChannelComponent::~MixerChannelComponent() {
  if (track_)
    track_->removeChangeListener(this);
  stopTimer();
}

void MixerChannelComponent::changeListenerCallback(
    juce::ChangeBroadcaster *source) {
  if (source == track_) {
    // UI update on message thread
    updateFromTrack();
  }
}

//==============================================================================
void MixerChannelComponent::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();
  using namespace zenith;

  // Background with Ableton-style gradient
  SkPoint bgGradPoints[2] = {{bounds.getCentreX(), bounds.getY()},
                             {bounds.getCentreX(), bounds.getBottom()}};
  SkColor bgGradColors[2] = {design::colors::BG_DARK,
                             design::colors::BG_DARKER};
  auto bgGradient = SkGradientShader::MakeLinear(
      bgGradPoints, bgGradColors, nullptr, 2, SkTileMode::kClamp);

  SkPaint bgPaint;
  bgPaint.setShader(bgGradient);
  bgPaint.setAntiAlias(true);

  SkRRect rrect = SkRRect::MakeRectXY(
      SkRect::MakeXYWH(bounds.getX(), bounds.getY(), bounds.getWidth(),
                       bounds.getHeight()),
      6.0f, 6.0f);
  canvas->drawRRect(rrect, bgPaint);

  // Inner highlight at top (subtle)
  SkRect highlightBounds =
      SkRect::MakeXYWH(bounds.getX(), bounds.getY(), bounds.getWidth(),
                       bounds.getHeight() * 0.2f);
  // SkiaTheme::getInstance().getColors().white.withAlpha(0.03f)
  SkPaint highlightPaint;
  highlightPaint.setColor(SkColorSetARGB(7, 255, 255, 255)); // 3% white
  SkRRect highlightRRect = SkRRect::MakeRectXY(highlightBounds, 6.0f, 6.0f);
  canvas->drawRRect(highlightRRect, highlightPaint);
  // Subtle border
  SkPaint borderPaint;
  borderPaint.setStyle(SkPaint::kStroke_Style);
  borderPaint.setStrokeWidth(1.0f);
  borderPaint.setColor(SkColorSetARGB(128, 58, 58, 58)); // 50% grey
  borderPaint.setAntiAlias(true);
  SkRRect borderRect = rrect;
  borderRect.inset(0.5f, 0.5f);
  canvas->drawRRect(borderRect, borderPaint);

  // Draw Track Name
  if (track_ != nullptr) {
    // Draw Cached Track Name (Critique #2: Zero allocations in paint)
    SkFont font; // Font is light, but ideally cached too. Skia handles font
                 // cache well.
    font.setSize(design::dimensions::MIXER_TRACK_HEADER_TEXT_SIZE);
    font.setEdging(SkFont::Edging::kAntiAlias);

    SkPaint textPaint;
    textPaint.setColor(SK_ColorWHITE);
    textPaint.setAntiAlias(true);

    canvas->drawSimpleText(cachedName_.c_str(), cachedName_.length(),
                           SkTextEncoding::kUTF8, cachedNameX_, cachedNameY_,
                           font, textPaint);
  }

  // Children are drawn by SkiaComponent::drawChildren
  drawChildren(canvas);
}

void MixerChannelComponent::resized() {
  auto bounds = getLocalBounds().reduced(8);
  updateNameCache(); // Re-calculate text position on resize

  // Track name at top
  // Track name is drawn directly, just reserve space
  bounds.removeFromTop(30);
  bounds.removeFromTop(4); // Spacing

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

  // Skia buttons: toggle state provides visual feedback
  muteButton_.setToggleState(track_->isMuted());
  soloButton_.setToggleState(track_->isSolo());

  // Update name
  // Update name cache
  updateNameCache(); // Recalculate text content/width if name changed

  updatingControls_ = false;
}

void MixerChannelComponent::updateNameCache() {
  if (track_ == nullptr)
    return;

  // Convert string once
  cachedName_ = track_->getName().toStdString();

  // Calculate layout once
  auto bounds = getLocalBounds().toFloat(); // JUCE float rect
  // Convert to Skia Layout
  SkRect skBounds = SkRect::MakeXYWH(bounds.getX(), bounds.getY(),
                                     bounds.getWidth(), bounds.getHeight());

  SkFont font;
  font.setSize(design::dimensions::MIXER_TRACK_HEADER_TEXT_SIZE);

  SkRect textBounds;
  font.measureText(cachedName_.c_str(), cachedName_.length(),
                   SkTextEncoding::kUTF8, &textBounds);

  cachedNameWidth_ = textBounds.width();
  cachedNameX_ = skBounds.centerX() - (cachedNameWidth_ / 2.0f);
  // Use Design Token for Padding (Critique #3 Fix)
  cachedNameY_ =
      skBounds.top() + design::dimensions::MIXER_TRACK_HEADER_PADDING_TOP;
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
}

void MixerChannelComponent::onSoloClicked() {
  if (updatingControls_ || track_ == nullptr)
    return;

  // Toggle solo (thread-safe via atomic)
  bool newSolo = soloButton_.getToggleState();
  track_->setSolo(newSolo);
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
  using namespace zenith;

  // Background with gradient (darker at top, lighter at bottom)
  SkPoint bgGradPoints[2] = {{bounds.getCentreX(), bounds.getY()},
                             {bounds.getCentreX(), bounds.getBottom()}};
  SkColor bgGradColors[2] = {design::colors::BG_DARK,
                             design::colors::BG_DARKER};
  auto bgGradient = SkGradientShader::MakeLinear(
      bgGradPoints, bgGradColors, nullptr, 2, SkTileMode::kClamp);

  SkPaint bgPaint;
  bgPaint.setShader(bgGradient);
  bgPaint.setAntiAlias(true);

  SkRRect rrect = SkRRect::MakeRectXY(
      SkRect::MakeXYWH(bounds.getX(), bounds.getY(), bounds.getWidth(),
                       bounds.getHeight()),
      3.0f, 3.0f);
  canvas->drawRRect(rrect, bgPaint);

  // Inner shadow at top
  bgPaint.setShader(nullptr);
  bgPaint.setColor(SkColorSetARGB(255, 0, 0, 0)); // Black
  bgPaint.setMaskFilter(SkMaskFilter::MakeBlur((SkBlurStyle)0, 2.0f));
  SkRRect shadowRRect = rrect;
  shadowRRect.inset(1.0f, 1.0f);
  canvas->drawRRect(shadowRRect.makeOffset(0, 1), bgPaint);
  bgPaint.setMaskFilter(nullptr);

  // Level bar (using smoothed animated level)
  if (currentLevel_ > 0.001f) {
    float levelDb = juce::Decibels::gainToDecibels(currentLevel_);

    // Map -60dB to 0dB -> 0.0 to 1.0
    float normalizedLevel = juce::jmap(levelDb, -60.0f, 0.0f, 0.0f, 1.0f);
    normalizedLevel = juce::jlimit(0.0f, 1.0f, normalizedLevel);

    float barHeight = bounds.getHeight() * normalizedLevel;
    auto meterBounds =
        bounds.withTop(bounds.getBottom() - barHeight).reduced(2.0f);

    // Beautiful gradient based on level
    SkColor topColor, bottomColor;
    if (normalizedLevel > 0.9f) {
      // Clipping warning - red gradient
      topColor = design::colors::RED;
      bottomColor = design::colors::RED;
    } else if (normalizedLevel > 0.7f) {
      // Hot - orange/yellow gradient
      topColor = design::colors::AMBER;
      bottomColor = design::colors::AMBER;
    } else if (normalizedLevel > 0.4f) {
      // Moderate - green/yellow gradient
      topColor = design::colors::NEON_GREEN;
      bottomColor = design::colors::NEON_GREEN;
    } else {
      // Normal - blue/green gradient
      topColor = design::colors::BLUE;
      bottomColor = design::colors::BLUE;
    }

    // Apply gradient to meter bar
    SkPoint meterGradPoints[2] = {
        {meterBounds.getCentreX(), meterBounds.getY()},
        {meterBounds.getCentreX(), meterBounds.getBottom()}};
    SkColor meterGradColors[2] = {topColor, bottomColor};
    auto meterGradient = SkGradientShader::MakeLinear(
        meterGradPoints, meterGradColors, nullptr, 2, SkTileMode::kClamp);

    SkPaint meterPaint;
    meterPaint.setShader(meterGradient);
    meterPaint.setAntiAlias(true);
    canvas->drawRoundRect(
        SkRect::MakeXYWH(meterBounds.getX(), meterBounds.getY(),
                         meterBounds.getWidth(), meterBounds.getHeight()),
        2.0f, 2.0f, meterPaint);

    // Highlight at top of bar for 3D effect
    meterPaint.setShader(nullptr);
    meterPaint.setColor(SkColorSetARGB(51, 255, 255, 255)); // 20% white
    canvas->drawRRect(
        SkRRect::MakeRectXY(SkRect::MakeXYWH(meterBounds.getX(),
                                             meterBounds.getY(),
                                             meterBounds.getWidth(),
                                             meterBounds.getHeight() * 0.3f),
                            2.0f, 2.0f),
        meterPaint);
  }

  // Peak indicator (thin line at peak level)
  if (peakLevel_ > 0.001f) {
    float peakDb = juce::Decibels::gainToDecibels(peakLevel_);
    float normalizedPeak = juce::jmap(peakDb, -60.0f, 0.0f, 0.0f, 1.0f);
    normalizedPeak = juce::jlimit(0.0f, 1.0f, normalizedPeak);

    float peakY = bounds.getBottom() - (bounds.getHeight() * normalizedPeak);
    auto peakBounds = SkRect::MakeXYWH(bounds.getX() + 2.0f, peakY - 1.0f,
                                       bounds.getWidth() - 4.0f, 2.0f);

    // Peak color (red if clipping, otherwise white)
    SkColor peakColor = normalizedPeak > 0.95f
                            ? design::colors::RED // Red for clipping
                            : SK_ColorWHITE;      // White for normal

    SkPaint peakPaint;
    peakPaint.setColor(peakColor);
    peakPaint.setAntiAlias(true);
    canvas->drawRoundRect(peakBounds, 1.0f, 1.0f, peakPaint);
  }

  // Subtle border
  SkPaint borderPaint;
  borderPaint.setColor(SkColorSetARGB(128, 58, 58, 58)); // 50% grey
  borderPaint.setStyle(SkPaint::kStroke_Style);
  borderPaint.setStrokeWidth(1.0f);
  borderPaint.setAntiAlias(true);
  SkRRect borderRect = rrect;
  borderRect.inset(0.5f, 0.5f);
  canvas->drawRRect(borderRect, borderPaint);
}

void MixerChannelComponent::LevelMeter::setLevel(float level) {
  targetLevel_.store(juce::jlimit(0.0f, 1.0f, level));
}

} // namespace zenith
