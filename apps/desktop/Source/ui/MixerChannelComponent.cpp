/**
 * @file MixerChannelComponent.cpp
 * @brief Full-featured mixer channel strip implementation
 *
 * Implements a professional-grade channel strip with:
 * - GPU-accelerated Skia rendering
 * - Glassmorphic panel design
 * - Peak-hold metering with gradient
 * - Insert slot and send indicators
 * - Smooth animations via spring physics
 */

#include "../../include/ui/MixerChannelComponent.h"
#include "../../Source/engine/EngineConstants.h"
#include "../../Source/engine/Track.h"
#include "../../Source/ui/skia/GlassmorphicPanel.h"
#include "../../Source/ui/skia/NeonGlow.h"
#include "../../Source/ui/skia/SkiaTheme.h"
#include "../../Source/ui/skia/ZenithDesignSystem.h"


#include <core/SkCanvas.h>
#include <core/SkMaskFilter.h>
#include <core/SkRRect.h>
#include <effects/SkGradientShader.h>

namespace zenith {

//==============================================================================
// Constants
//==============================================================================

namespace {
constexpr int kNumInsertSlots = 8;
constexpr int kNumSends = 4;
constexpr float kInsertSlotHeight = 16.0f;
constexpr float kSendIndicatorHeight = 20.0f;
constexpr float kChannelStripWidth = 100.0f;
constexpr float kMasterStripWidth = 140.0f;
} // namespace

//==============================================================================
// MixerChannelComponent Implementation
//==============================================================================

MixerChannelComponent::MixerChannelComponent(Track *track, bool isMaster)
    : track_(track), isMaster_(isMaster), faderSlider_("Vol"), panKnob_("Pan"),
      muteButton_("M"), soloButton_("S"), armButton_("R") {
  jassert(track_ != nullptr);
  track_->addChangeListener(this);
  zenith::design::ThemeManager::getInstance().addChangeListener(this);

  // Initialize UI from track
  updateFromTrack();

  // Track name label
  nameLabel_.setText(track_->getName(), juce::dontSendNotification);
  nameLabel_.setJustificationType(juce::Justification::centred);
  nameLabel_.setFont(
      juce::FontOptions(isMaster_ ? 16.0f : 14.0f, juce::Font::bold));
  nameLabel_.setEditable(true, true, false);
  nameLabel_.onTextChange = [this]() {
    if (track_) {
      track_->setName(nameLabel_.getText());
    }
  };
  addAndMakeVisible(nameLabel_);

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
  panKnob_.onValueChange = [this](float value) { onPanChanged(); };
  addAndMakeVisible(panKnob_);

  // Mute button - Red when active
  muteButton_.setToggleable(true);
  muteButton_.setToggleState(track_->isMuted());
  muteButton_.setStyle(SkiaButton::Style::Secondary);
  muteButton_.onClick = [this]() { onMuteClicked(); };
  addAndMakeVisible(muteButton_);

  // Solo button - Yellow/Amber when active
  soloButton_.setToggleable(true);
  soloButton_.setToggleState(track_->isSolo());
  soloButton_.setStyle(SkiaButton::Style::Secondary);
  soloButton_.onClick = [this]() { onSoloClicked(); };
  addAndMakeVisible(soloButton_);

  // Record Arm button - Red when armed
  armButton_.setToggleable(true);
  armButton_.setToggleState(track_->isArmed());
  armButton_.setStyle(SkiaButton::Style::Danger);
  armButton_.onClick = [this]() { onArmClicked(); };
  addAndMakeVisible(armButton_);

  // Spectrum Analyzer
  spectrumAnalyzer_ = std::make_unique<SkiaSpectrumComponent>();
  spectrumAnalyzer_->setDisplayMode(
      SkiaSpectrumComponent::DisplayMode::FilledCurve);
  addAndMakeVisible(spectrumAnalyzer_.get());

  // Link to track's audio processing
  if (track_) {
    track_->getMixerChannel().setSpectrumFifo(
        &spectrumAnalyzer_->getAudioFifo());
  }

  // Level meter
  meter_.setStereo(isMaster_); // Master channel has stereo metering
  addAndMakeVisible(meter_);

  // Create insert slot indicators
  for (int i = 0; i < kNumInsertSlots; ++i) {
    auto slot = std::make_unique<InsertSlotIndicator>(i);
    addAndMakeVisible(slot.get());
    insertSlots_.push_back(std::move(slot));
  }

  // Create send indicators
  for (int i = 0; i < kNumSends; ++i) {
    auto send = std::make_unique<SendIndicator>(i);
    addAndMakeVisible(send.get());
    sendIndicators_.push_back(std::move(send));
  }

  // Start timer for meter updates (30 Hz)
  startTimer(33);

  // Set size based on channel type
  setSize(isMaster_ ? static_cast<int>(kMasterStripWidth)
                    : static_cast<int>(kChannelStripWidth),
          500);
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

void MixerChannelComponent::mouseDown(const juce::MouseEvent &e) {
  if (onClick) {
    onClick();
  }
  // Don't consume event so children can receive it if needed (though they
  // usually handle their own mouse events)
  SkiaComponent::mouseDown(e);
}

//==============================================================================
// Selection State
//==============================================================================

void MixerChannelComponent::setSelected(bool selected) {
  if (isSelected_ != selected) {
    isSelected_ = selected;
    repaint();
  }
}

//==============================================================================
// Rendering
//==============================================================================

void MixerChannelComponent::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();
  SkRect skBounds = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());

  // Draw background panel with glassmorphism
  if (isSelected_) {
    // Selected channel: Use accent glow
    zenith::GlassmorphicPanel::drawWithAccent(
        canvas, skBounds,
        isMaster_ ? design::colors::MAGENTA : design::colors::CYAN,
        GlassmorphicPanel::Style::ActiveGlow);
  } else {
    // Normal channel: Elevated glass panel
    zenith::GlassmorphicPanel::draw(canvas, skBounds,
                                    isMaster_
                                        ? GlassmorphicPanel::Style::Floating
                                        : GlassmorphicPanel::Style::Elevated);
  }

  // Draw children (all the controls)
  drawChildren(canvas);

  // Draw insert slots section header
  float insertSectionY = bounds.getHeight() * 0.55f;
  SkPaint labelPaint;
  labelPaint.setColor(design::colors::TEXT_TERTIARY);
  labelPaint.setAntiAlias(true);
  SkFont labelFont =
      design::typography::getSkFont(10.0f, design::FontWeight::Medium);
  canvas->drawString("INSERTS", 8, insertSectionY - 4, labelFont, labelPaint);

  // Draw send section header
  float sendSectionY = bounds.getHeight() * 0.78f;
  canvas->drawString("SENDS", 8, sendSectionY - 4, labelFont, labelPaint);

  // Master channel: Draw "MASTER" badge
  if (isMaster_) {
    SkPaint badgePaint;
    badgePaint.setColor(design::withAlpha(design::colors::MAGENTA, 0.3f));
    badgePaint.setAntiAlias(true);

    SkRect badgeRect = SkRect::MakeXYWH(bounds.getWidth() / 2 - 30, 4, 60, 18);
    canvas->drawRoundRect(badgeRect, 4, 4, badgePaint);

    SkPaint badgeTextPaint;
    badgeTextPaint.setColor(design::colors::MAGENTA);
    badgeTextPaint.setAntiAlias(true);
    SkFont badgeFont =
        design::typography::getSkFont(10.0f, design::FontWeight::Bold);
    canvas->drawString("MASTER", bounds.getWidth() / 2 - 22, 16, badgeFont,
                       badgeTextPaint);
  }
}

void MixerChannelComponent::resized() {
  auto bounds = getLocalBounds().reduced(8);
  float stripWidth = static_cast<float>(bounds.getWidth());

  // Top section: Track name
  int topHeight = isMaster_ ? 40 : 34;
  nameLabel_.setBounds(bounds.removeFromTop(topHeight));
  bounds.removeFromTop(4);

  // Spectrum Analyzer
  if (spectrumAnalyzer_) {
    spectrumAnalyzer_->setBounds(bounds.removeFromTop(50).reduced(2));
    bounds.removeFromTop(4);
  }

  // Bottom section: Mute/Solo/Arm buttons
  auto buttonArea = bounds.removeFromBottom(isMaster_ ? 100 : 80);

  // Arm button (only for non-master)
  if (!isMaster_) {
    armButton_.setBounds(buttonArea.removeFromTop(24).reduced(2));
    buttonArea.removeFromTop(2);
  }

  // Mute/Solo buttons side by side
  auto msRow = buttonArea.removeFromTop(28);
  muteButton_.setBounds(msRow.removeFromLeft(msRow.getWidth() / 2).reduced(2));
  soloButton_.setBounds(msRow.reduced(2));
  buttonArea.removeFromTop(4);

  // Pan knob
  int panSize = isMaster_ ? 70 : 60;
  panKnob_.setBounds(buttonArea.withSizeKeepingCentre(panSize, panSize));

  // Send indicators (above buttons)
  float sendHeight = kSendIndicatorHeight;
  auto sendArea =
      bounds.removeFromBottom(static_cast<int>(sendHeight * kNumSends + 8));
  sendArea.removeFromBottom(4);

  for (auto &send : sendIndicators_) {
    send->setBounds(
        sendArea.removeFromTop(static_cast<int>(sendHeight)).reduced(2, 1));
  }
  bounds.removeFromBottom(8);

  // Insert slot indicators
  float insertHeight = kInsertSlotHeight;
  auto insertArea = bounds.removeFromBottom(
      static_cast<int>(insertHeight * kNumInsertSlots + 8));
  insertArea.removeFromBottom(4);

  for (auto &slot : insertSlots_) {
    slot->setBounds(
        insertArea.removeFromTop(static_cast<int>(insertHeight)).reduced(2, 0));
  }
  bounds.removeFromBottom(8);

  // Remaining space: meter and fader side by side
  int meterWidth = isMaster_ ? 24 : 18;
  auto meterBounds = bounds.removeFromLeft(meterWidth);
  meter_.setBounds(meterBounds.reduced(0, 4));

  bounds.removeFromLeft(4);

  // Fader takes remaining space
  faderSlider_.setBounds(bounds.reduced(2, 4));
}

//==============================================================================
// Timer Callback
//==============================================================================

void MixerChannelComponent::timerCallback() {
  if (track_ != nullptr) {
    float level = track_->getCurrentLevel();
    meter_.setLevel(level);

    // For master channel, use same level for both L/R (no stereo API yet)
    if (isMaster_) {
      meter_.setLeftLevel(level);
      meter_.setRightLevel(level);
    }

    meter_.repaint();
  }
}

//==============================================================================
// State Update
//==============================================================================

void MixerChannelComponent::updateFromTrack() {
  if (track_ == nullptr)
    return;

  updatingControls_ = true;

  // Update fader
  faderSlider_.setValue(track_->getVolume());

  // Update pan knob
  panKnob_.setValue(track_->getPan());

  // Update buttons
  muteButton_.setToggleState(track_->isMuted());
  soloButton_.setToggleState(track_->isSolo());
  armButton_.setToggleState(track_->isArmed());

  // Update button styles based on state
  if (track_->isMuted()) {
    muteButton_.setStyle(SkiaButton::Style::Danger);
  } else {
    muteButton_.setStyle(SkiaButton::Style::Secondary);
  }

  if (track_->isSolo()) {
    soloButton_.setStyle(SkiaButton::Style::Warn);
  } else {
    soloButton_.setStyle(SkiaButton::Style::Secondary);
  }

  // Update name
  nameLabel_.setText(track_->getName(), juce::dontSendNotification);

  // Update insert slots (Plugins are managed by Track, not MixerChannel)
  for (size_t i = 0; i < insertSlots_.size(); ++i) {
    if (i < static_cast<size_t>(track_->getNumPlugins())) {
      auto *plugin = track_->getPlugin(static_cast<int>(i));
      if (plugin) {
        insertSlots_[i]->setOccupied(true, plugin->getName());
      } else {
        insertSlots_[i]->setOccupied(false);
      }
    } else {
      insertSlots_[i]->setOccupied(false);
    }
  }

  // Update send indicators
  auto &mixer = track_->getMixerChannel();
  // Use kNumSends from constants/MixerChannel
  const int numSends = zenith::constants::kNumSends;

  for (size_t i = 0; i < sendIndicators_.size(); ++i) {
    if (i < static_cast<size_t>(numSends)) {
      sendIndicators_[i]->setSendLevel(mixer.getSendLevel(static_cast<int>(i)));
      // For now, hardcode destination names as "Aux X" since MixerChannel
      // doesn't know about the Graph
      sendIndicators_[i]->setDestination("Aux " + juce::String(i + 1));
    } else {
      sendIndicators_[i]->setSendLevel(0.0f);
      sendIndicators_[i]->setDestination("");
    }
  }

  updatingControls_ = false;
}

//==============================================================================
// Control Callbacks
//==============================================================================

void MixerChannelComponent::onFaderChanged() {
  if (updatingControls_ || track_ == nullptr)
    return;

  float newVolume = faderSlider_.getValue();
  track_->setVolume(newVolume);
}

void MixerChannelComponent::onPanChanged() {
  if (updatingControls_ || track_ == nullptr)
    return;

  float newPan = panKnob_.getValue();
  track_->setPan(newPan);
}

void MixerChannelComponent::onMuteClicked() {
  if (updatingControls_ || track_ == nullptr)
    return;

  bool newMuted = muteButton_.getToggleState();
  track_->setMuted(newMuted);

  if (newMuted) {
    muteButton_.setStyle(SkiaButton::Style::Danger);
  } else {
    muteButton_.setStyle(SkiaButton::Style::Secondary);
  }
}

void MixerChannelComponent::onSoloClicked() {
  if (updatingControls_ || track_ == nullptr)
    return;

  bool newSolo = soloButton_.getToggleState();
  track_->setSolo(newSolo);

  if (newSolo) {
    soloButton_.setStyle(SkiaButton::Style::Warn);
  } else {
    soloButton_.setStyle(SkiaButton::Style::Secondary);
  }
}

void MixerChannelComponent::onArmClicked() {
  if (updatingControls_ || track_ == nullptr)
    return;

  bool newArmed = armButton_.getToggleState();
  track_->setArmed(newArmed);
}

//==============================================================================
// LevelMeter Implementation
//==============================================================================

MixerChannelComponent::LevelMeter::LevelMeter() { startTimerHz(60); }

MixerChannelComponent::LevelMeter::~LevelMeter() { stopTimer(); }

void MixerChannelComponent::LevelMeter::timerCallback() {
  // Smooth meter ballistics
  auto smoothLevel = [](float target, float &current, float &peak,
                        int &peakHold) {
    const float attackSpeed = 0.8f;
    const float decaySpeed = 0.05f;

    if (target > current) {
      current += (target - current) * attackSpeed;
    } else {
      current += (target - current) * decaySpeed;
    }

    // Peak hold logic
    if (current > peak) {
      peak = current;
      peakHold = 120; // 2 seconds at 60 Hz
    } else if (peakHold > 0) {
      peakHold--;
    } else {
      peak *= 0.95f;
    }
  };

  if (stereo_) {
    smoothLevel(targetLevelL_.load(), currentLevelL_, peakLevelL_,
                peakHoldCounterL_);
    smoothLevel(targetLevelR_.load(), currentLevelR_, peakLevelR_,
                peakHoldCounterR_);

    if (std::abs(currentLevelL_ - targetLevelL_.load()) > 0.001f ||
        std::abs(currentLevelR_ - targetLevelR_.load()) > 0.001f ||
        peakLevelL_ > 0.001f || peakLevelR_ > 0.001f) {
      repaint();
    }
  } else {
    smoothLevel(targetLevel_.load(), currentLevel_, peakLevel_,
                peakHoldCounter_);

    if (std::abs(currentLevel_ - targetLevel_.load()) > 0.001f ||
        peakLevel_ > 0.001f) {
      repaint();
    }
  }
}

void MixerChannelComponent::LevelMeter::drawMeterBar(SkCanvas *canvas,
                                                     const SkRect &bounds,
                                                     float level, float peak) {
  using namespace design;

  // Draw background
  SkPaint bgPaint;
  bgPaint.setColor(colors::BG_DARKEST);
  bgPaint.setAntiAlias(true);
  canvas->drawRoundRect(bounds, 2.0f, 2.0f, bgPaint);

  if (level < 0.001f && peak < 0.001f)
    return;

  // Calculate normalized level
  float levelDb = juce::Decibels::gainToDecibels(level);
  float normalizedLevel = juce::jmap(levelDb, -60.0f, 0.0f, 0.0f, 1.0f);
  normalizedLevel = juce::jlimit(0.0f, 1.0f, normalizedLevel);

  if (normalizedLevel > 0.01f) {
    float barHeight = bounds.height() * normalizedLevel;
    SkRect meterRect =
        SkRect::MakeXYWH(bounds.x() + 2, bounds.bottom() - barHeight - 2,
                         bounds.width() - 4, barHeight);

    // Gradient: green -> yellow -> red based on level
    SkColor topColor = colors::NEON_GREEN;
    if (normalizedLevel > 0.9f) {
      topColor = colors::RED;
    } else if (normalizedLevel > 0.7f) {
      topColor = colors::AMBER;
    } else if (normalizedLevel > 0.5f) {
      topColor = SkColorSetRGB(180, 255, 0); // Yellow-green
    }

    SkPoint pts[2] = {{meterRect.centerX(), meterRect.bottom()},
                      {meterRect.centerX(), meterRect.top()}};
    SkColor gradColors[3] = {colors::NEON_GREEN, SkColorSetRGB(200, 255, 0),
                             topColor};
    SkScalar positions[3] = {0.0f, 0.6f, 1.0f};

    SkPaint meterPaint;
    meterPaint.setShader(SkGradientShader::MakeLinear(
        pts, gradColors, positions, 3, SkTileMode::kClamp));
    meterPaint.setAntiAlias(true);
    canvas->drawRoundRect(meterRect, 1.0f, 1.0f, meterPaint);

    // Glow effect for high levels
    if (normalizedLevel > 0.7f) {
      SkPaint glowPaint;
      glowPaint.setColor(withAlpha(topColor, 0.3f));
      glowPaint.setMaskFilter(
          SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 4.0f));
      glowPaint.setAntiAlias(true);
      canvas->drawRoundRect(meterRect, 1.0f, 1.0f, glowPaint);
    }
  }

  // Peak indicator
  if (peak > 0.001f) {
    float peakDb = juce::Decibels::gainToDecibels(peak);
    float normalizedPeak = juce::jmap(peakDb, -60.0f, 0.0f, 0.0f, 1.0f);
    normalizedPeak = juce::jlimit(0.0f, 1.0f, normalizedPeak);

    float peakY = bounds.bottom() - (bounds.height() * normalizedPeak) - 2;
    SkPaint peakPaint;
    peakPaint.setColor(normalizedPeak > 0.95f ? colors::RED : SK_ColorWHITE);
    peakPaint.setAntiAlias(true);
    canvas->drawRect(
        SkRect::MakeXYWH(bounds.x() + 2, peakY, bounds.width() - 4, 2.0f),
        peakPaint);
  }
}

void MixerChannelComponent::LevelMeter::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();
  SkRect skBounds = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());

  if (stereo_) {
    // Draw two meter bars side by side
    float barWidth = (skBounds.width() - 2) / 2;
    SkRect leftBounds = SkRect::MakeXYWH(0, 0, barWidth, skBounds.height());
    SkRect rightBounds =
        SkRect::MakeXYWH(barWidth + 2, 0, barWidth, skBounds.height());

    drawMeterBar(canvas, leftBounds, currentLevelL_, peakLevelL_);
    drawMeterBar(canvas, rightBounds, currentLevelR_, peakLevelR_);
  } else {
    drawMeterBar(canvas, skBounds, currentLevel_, peakLevel_);
  }
}

void MixerChannelComponent::LevelMeter::setLevel(float level) {
  targetLevel_.store(juce::jlimit(0.0f, 1.0f, level));
}

//==============================================================================
// InsertSlotIndicator Implementation
//==============================================================================

MixerChannelComponent::InsertSlotIndicator::InsertSlotIndicator(int slotIndex)
    : slotIndex_(slotIndex) {
  setSize(80, static_cast<int>(kInsertSlotHeight));
}

void MixerChannelComponent::InsertSlotIndicator::drawSkia(SkCanvas *canvas) {
  using namespace design;

  auto bounds = getLocalBounds().toFloat();
  SkRect skBounds = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());

  // Background
  SkPaint bgPaint;
  bgPaint.setAntiAlias(true);

  if (isOccupied_) {
    // Filled slot: subtle gradient
    bgPaint.setColor(colors::BG_MEDIUM);
  } else {
    // Empty slot: very subtle
    bgPaint.setColor(colors::BG_DARKER);
  }

  canvas->drawRoundRect(skBounds, 2.0f, 2.0f, bgPaint);

  // Border
  SkPaint borderPaint;
  borderPaint.setStyle(SkPaint::kStroke_Style);
  borderPaint.setStrokeWidth(0.5f);
  borderPaint.setColor(colors::BORDER_SUBTLE);
  borderPaint.setAntiAlias(true);
  canvas->drawRoundRect(skBounds, 2.0f, 2.0f, borderPaint);

  // Text
  SkPaint textPaint;
  textPaint.setAntiAlias(true);
  SkFont font = typography::getSkFont(9.0f, design::FontWeight::Regular);

  if (isOccupied_) {
    textPaint.setColor(colors::TEXT_PRIMARY);
    // Truncate plugin name if needed
    juce::String displayName = pluginName_.substring(0, 12);
    if (pluginName_.length() > 12)
      displayName += "...";
    canvas->drawString(displayName.toRawUTF8(), 4, skBounds.centerY() + 3, font,
                       textPaint);
  } else {
    textPaint.setColor(colors::TEXT_TERTIARY);
    canvas->drawString(("Slot " + juce::String(slotIndex_ + 1)).toRawUTF8(), 4,
                       skBounds.centerY() + 3, font, textPaint);
  }

  // Occupied indicator dot
  if (isOccupied_) {
    SkPaint dotPaint;
    dotPaint.setColor(colors::CYAN);
    dotPaint.setAntiAlias(true);
    canvas->drawCircle(skBounds.right() - 6, skBounds.centerY(), 3, dotPaint);
  }
}

void MixerChannelComponent::InsertSlotIndicator::setOccupied(
    bool occupied, const juce::String &pluginName) {
  if (isOccupied_ != occupied || pluginName_ != pluginName) {
    isOccupied_ = occupied;
    pluginName_ = pluginName;
    repaint();
  }
}

//==============================================================================
// SendIndicator Implementation
//==============================================================================

MixerChannelComponent::SendIndicator::SendIndicator(int sendIndex)
    : sendIndex_(sendIndex) {
  setSize(80, static_cast<int>(kSendIndicatorHeight));
}

void MixerChannelComponent::SendIndicator::drawSkia(SkCanvas *canvas) {
  using namespace design;

  auto bounds = getLocalBounds().toFloat();
  SkRect skBounds = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());

  // Background
  SkPaint bgPaint;
  bgPaint.setColor(colors::BG_DARKER);
  bgPaint.setAntiAlias(true);
  canvas->drawRoundRect(skBounds, 2.0f, 2.0f, bgPaint);

  // Send level bar
  if (sendLevel_ > 0.01f) {
    float barWidth = (skBounds.width() - 4) * sendLevel_;
    SkRect barRect = SkRect::MakeXYWH(2, skBounds.bottom() - 4, barWidth, 2);

    SkPaint barPaint;
    barPaint.setColor(colors::VIOLET);
    barPaint.setAntiAlias(true);
    canvas->drawRoundRect(barRect, 1.0f, 1.0f, barPaint);
  }

  // Text: Send destination or "Send X"
  SkPaint textPaint;
  textPaint.setAntiAlias(true);
  SkFont font = typography::getSkFont(9.0f, design::FontWeight::Regular);

  juce::String displayText;
  if (destinationName_.isNotEmpty()) {
    textPaint.setColor(colors::TEXT_PRIMARY);
    displayText = destinationName_.substring(0, 10);
    if (destinationName_.length() > 10)
      displayText += "...";
  } else {
    textPaint.setColor(colors::TEXT_TERTIARY);
    displayText = "Send " + juce::String(sendIndex_ + 1);
  }

  canvas->drawString(displayText.toRawUTF8(), 4, skBounds.centerY() + 2, font,
                     textPaint);

  // Level value on right
  if (sendLevel_ > 0.01f) {
    juce::String levelStr =
        juce::String(static_cast<int>(sendLevel_ * 100)) + "%";
    SkFont smallFont =
        typography::getMonoFont(8.0f, design::FontWeight::Regular);
    SkPaint levelPaint;
    levelPaint.setColor(colors::TEXT_SECONDARY);
    levelPaint.setAntiAlias(true);
    canvas->drawString(levelStr.toRawUTF8(), skBounds.right() - 24,
                       skBounds.centerY() + 2, smallFont, levelPaint);
  }
}

void MixerChannelComponent::SendIndicator::setSendLevel(float level) {
  if (std::abs(sendLevel_ - level) > 0.001f) {
    sendLevel_ = juce::jlimit(0.0f, 1.0f, level);
    repaint();
  }
}

void MixerChannelComponent::SendIndicator::setDestination(
    const juce::String &destName) {
  if (destinationName_ != destName) {
    destinationName_ = destName;
    repaint();
  }
}

} // namespace zenith
