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

#include "MixerChannelComponent.h"
#include "../../effects/ConsoleEmulation.h"
#include "../../engine/EngineConstants.h"
#include "../common/PluginEditorWindow.h"
#include "../../engine/Track.h"
#include "../../engine/Engine.h"
#include "../../engine/ProjectState.h"
#include "../../engine/AuxBus.h"
#include "GlassmorphicPanel.h"
#include "NeonGlow.h"
#include "ZenithDesignSystem.h"
#include "../design-system/ZenithIcons.h"
// #include "../design-system/ZenithTheme.h" // Deprecated
#include "../controls/SkiaPopupMenu.h"
#include "../controls/ContextMenuManager.h"
#include "../design-system/ColorBridge.h"
#include "../design-system/ZenithDesignSystem.h"
#include "PluginBrowser.h"
#include <JuceHeader.h>

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
constexpr float kInsertSlotHeight = 16.0f;
constexpr float kSendIndicatorHeight = 20.0f;
constexpr float kChannelStripWidth = 100.0f;
constexpr float kMasterStripWidth = 140.0f;
constexpr int kTopHeightMaster = 40;
constexpr int kTopHeightNormal = 34;
constexpr int kSpectrumHeight = 50;
constexpr int kMaxPluginNameLength = 12;
} // namespace

//==============================================================================
// MixerChannelComponent Implementation
//==============================================================================

#include "../../engine/Engine.h"
#include "../../engine/ProjectState.h"

//==============================================================================
// MixerChannelComponent Implementation
//==============================================================================

MixerChannelComponent::MixerChannelComponent(Track *track, ProjectState& state, Engine& engine, bool isMaster)
    : track_(track), projectState_(state), engine_(engine), isMaster_(isMaster), faderSlider_("Vol", design::colors::CYAN), panKnob_("Pan", design::colors::CYAN),
      muteButton_("Mute"), soloButton_("Solo"), armButton_("Record") {
  jassert(track_ != nullptr);
  track_->addChangeListener(this);
  zenith::design::ThemeManager::getInstance().addChangeListener(this);

  // Initialize UI from track
  updateFromTrack();

  // Track name label
  nameLabel_.setText(track_->getName(), juce::dontSendNotification);
  nameLabel_.setJustificationType(juce::Justification::centred);
  nameLabel_.setFont(isMaster_ 
      ? design::typography::getJuceFont(16.0f, design::typography::FontWeight::Bold) 
      : design::typography::getJuceFont(14.0f, design::typography::FontWeight::Bold));
  nameLabel_.setEditable(true, true, false);
  nameLabel_.onTextChange = [this]() {
    if (track_) {
      track_->setName(nameLabel_.getText());
    }
  };
  addAndMakeVisible(nameLabel_);

  // GPU-accelerated volume fader with spring physics
  faderSlider_.setOrientation(zenith::ZenithSlider::Vertical);
  // ZenithSlider uses setRange instead of setStyle/setDisplayRange
  faderSlider_.setRange(0.0f, 1.0f, 1.0f);
  faderSlider_.setValue(track_->getVolume());
  faderSlider_.onValueChange = [this](float value) { juce::ignoreUnused(value); onFaderChanged(); };
  addAndMakeVisible(faderSlider_);

  // GPU-accelerated pan knob with spring physics
  panKnob_.setRange(-1.0f, 1.0f, 0.0f);
  panKnob_.setValue(track_->getPan());
  panKnob_.onValueChange = [this]() { onPanChanged(); };
  addAndMakeVisible(panKnob_);

  // Mute button - Red when active
  muteButton_.setIconPath(zenith::icons::Mute());
  muteButton_.setIconPosition(ZenithButton::IconPosition::Only);
  muteButton_.setToggleable(true);
  muteButton_.setToggleState(track_->isMuted());
  muteButton_.setStyle(SkiaButton::Style::Secondary);
  muteButton_.onClick = [this]() { onMuteClicked(); };
  addAndMakeVisible(muteButton_);

  // Solo button - Yellow/Amber when active
  soloButton_.setIconPath(zenith::icons::Solo());
  soloButton_.setIconPosition(ZenithButton::IconPosition::Only);
  soloButton_.setToggleable(true);
  soloButton_.setToggleState(track_->isSolo());
  soloButton_.setStyle(SkiaButton::Style::Secondary);
  soloButton_.onClick = [this]() { onSoloClicked(); };
  addAndMakeVisible(soloButton_);

  // Record Arm button - Red when armed
  armButton_.setIconPath(zenith::icons::Arm());
  armButton_.setIconPosition(ZenithButton::IconPosition::Only);
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
    auto slot = std::make_unique<InsertSlotIndicator>(*this, i);
    addAndMakeVisible(slot.get());
    insertSlots_.push_back(std::move(slot));
  }

  // Create send indicators
  for (int i = 0; i < zenith::constants::kNumSends; ++i) {
    auto send = std::make_unique<SendIndicator>(*this, i);
    addAndMakeVisible(send.get());
    sendIndicators_.push_back(std::move(send));
  }

  // Start timer for meter updates (30 Hz)
  if (juce::MessageManager::getInstanceWithoutCreating() != nullptr) startTimer(33);

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
  if (e.mods.isRightButtonDown()) {
    auto menu = ContextMenuManager::createMenu();
    juce::String trackId = track_ ? track_->getTrackId() : "";
    
    menu->addSectionHeader("Channel");
    
    menu->addItem(1, "Rename...", true, false, [this]() {
      nameLabel_.showEditor();
    });
    
    menu->addItem(2, "Duplicate Channel", !isMaster_, false, [this]() {
      if (track_) {
        projectState_.duplicateTrack(track_->getTrackId(), "Duplicate Track");
      }
    });
    
    menu->addSeparator();
    
    // Color submenu
    auto colorMenu = ContextMenuManager::createMenu();
    // Using simple names, but ideally these would set actual Zenith colors
    colorMenu->addItem(100, "Red", true, false, [this]() { track_->setColor(juce::Colour(0xFFFF4444)); });
    colorMenu->addItem(101, "Orange", true, false, [this]() { track_->setColor(juce::Colour(0xFFFF8844)); });
    colorMenu->addItem(102, "Yellow", true, false, [this]() { track_->setColor(juce::Colour(0xFFFFDD44)); });
    colorMenu->addItem(103, "Green", true, false, [this]() { track_->setColor(juce::Colour(0xFF44FF88)); });
    colorMenu->addItem(104, "Cyan", true, false, [this]() { track_->setColor(juce::Colour(0xFF44DDFF)); });
    colorMenu->addItem(105, "Blue", true, false, [this]() { track_->setColor(juce::Colour(0xFF4488FF)); });
    menu->addSubMenu("Change Color", std::move(colorMenu));
    
    menu->addSeparator();
    
    menu->addSectionHeader("Routing");
    
    auto routeMenu = ContextMenuManager::createMenu();
    routeMenu->addItem(200, "Master", true, track_->getOutputId() == "master", [this]() {
      track_->setOutputId("master");
    });
    // Dynamic list of aux buses
    int numAux = engine_.getNumAuxBuses();
    for (int auxIdx = 0; auxIdx < numAux; ++auxIdx) {
      if (auto* bus = engine_.getAuxBus(auxIdx)) {
        juce::String busId = bus->getName();
        bool isCurrent = (track_->getOutputId() == busId);
        routeMenu->addItem(201 + auxIdx, bus->getName(), true, isCurrent, [this, busId]() {
          track_->setOutputId(busId);
        });
      }
    }
    menu->addSubMenu("Route To", std::move(routeMenu));
    
    menu->addSeparator();
    
    menu->addItem(3, "Reset Channel", true, false, [this]() {
      faderSlider_.setValue(1.0f);
      panKnob_.setValue(0.0f);
      muteButton_.setToggleState(false);
      soloButton_.setToggleState(false);
      onFaderChanged();
      onPanChanged();
      onMuteClicked();
      onSoloClicked();
    });
    
    menu->addSeparator();
    menu->addItemComplete(99, "Delete Channel", SkPath(), "Del", !isMaster_, false, true, [this]() {
      if (track_) {
        projectState_.removeTrack(track_->getTrackId(), "Delete Track");
      }
    });
    
    ContextMenuManager::getInstance().showMenuAt(std::move(menu), this, e.x, e.y);
    return;
  }

  if (onClick) {
    onClick();
  }
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
        isMaster_ ? design::colors::ACCENT_SECONDARY
                  : design::colors::ACCENT_PRIMARY,
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
  if (!insertHeaderBounds_.isEmpty()) {
    SkPaint labelPaint;
    labelPaint.setColor(design::colors::TEXT_TERTIARY);
    labelPaint.setAntiAlias(true);
    SkFont labelFont =
        design::typography::getSkFont(10.0f, design::FontWeight::Medium);
    canvas->drawString("INSERTS", insertHeaderBounds_.x(), insertHeaderBounds_.bottom(), labelFont, labelPaint);
  }

  // Draw send section header
  if (!sendHeaderBounds_.isEmpty()) {
    SkPaint labelPaint;
    labelPaint.setColor(design::colors::TEXT_TERTIARY);
    labelPaint.setAntiAlias(true);
    SkFont labelFont =
        design::typography::getSkFont(10.0f, design::FontWeight::Medium);
    canvas->drawString("SENDS", sendHeaderBounds_.x(), sendHeaderBounds_.bottom(), labelFont, labelPaint);
  }

  // Master channel: Draw "MASTER" badge
  if (isMaster_) {
    SkPaint badgePaint;
    badgePaint.setColor(design::withAlpha(design::colors::ACCENT_SECONDARY, 0.3f));
    badgePaint.setAntiAlias(true);

    SkRect badgeRect = SkRect::MakeXYWH(bounds.getWidth() / 2 - 30, 4, 60, 18);
    canvas->drawRoundRect(badgeRect, design::dimensions::RADIUS_SM, design::dimensions::RADIUS_SM, badgePaint);

    SkPaint badgeTextPaint;
    badgeTextPaint.setColor(design::colors::ACCENT_SECONDARY);
    badgeTextPaint.setAntiAlias(true);
    SkFont badgeFont =
        design::typography::getSkFont(10.0f, design::FontWeight::Bold);
    canvas->drawString("MASTER", bounds.getWidth() / 2 - 22, 16, badgeFont,
                       badgeTextPaint);
  }
}

void MixerChannelComponent::resized() {
  auto bounds = getLocalBounds();
  const int totalHeight = bounds.getHeight();
  const int minFaderHeight = 60; // Absolute minimum for usability
  
  // -- 1. High Priority: Track Name (Top) --
  int topHeight = isMaster_ ? kTopHeightMaster : kTopHeightNormal;
  nameLabel_.setBounds(bounds.removeFromTop(topHeight));
  bounds.removeFromTop(4);

  // -- 2. High Priority: Buttons (Bottom) --
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

  // Determine available vertical space for optional components
  int availableHeight = bounds.getHeight();
  
  // -- 3. Medium Priority: Spectrum Analyzer (Top) --
  // Hide spectrum if we are crunched for space (< 450px total, or < 150px remaining)
  bool showSpectrum = (totalHeight > 450 && availableHeight > 150);
  
  if (spectrumAnalyzer_) {
    spectrumAnalyzer_->setVisible(showSpectrum);
    if (showSpectrum) {
      spectrumAnalyzer_->setBounds(
          bounds.removeFromTop(kSpectrumHeight).reduced(2));
      bounds.removeFromTop(4);
    }
  }

  // Reload available height
  availableHeight = bounds.getHeight();

  // -- 4. Medium Priority: Sends (Bottom) --
  // Show sends only if we have space. 
  float sendHeight = kSendIndicatorHeight;
  int requiredSendTotal = static_cast<int>(sendHeight * zenith::constants::kNumSends + 8);
  
  // Ensure we leave room for fader + inserts
  // Ensure we leave room for fader + inserts
  bool showSends = (availableHeight > requiredSendTotal + minFaderHeight + 40);
  
  // Reset header bounds
  sendHeaderBounds_ = SkRect::MakeEmpty();

  if (showSends) {
    // Calculate header position (just above the sends)
    sendHeaderBounds_ = SkRect::MakeXYWH(8, bounds.getBottom() - requiredSendTotal - 14, 100, 10);

    auto sendArea = bounds.removeFromBottom(requiredSendTotal);
    sendArea.removeFromBottom(4);
    for (auto &send : sendIndicators_) {
        send->setVisible(true);
        send->setBounds(sendArea.removeFromTop(static_cast<int>(sendHeight)).reduced(2, 1));
    }
    bounds.removeFromBottom(8);
  } else {
     for (auto &send : sendIndicators_) send->setVisible(false);
  }

  // Reload available height
  availableHeight = bounds.getHeight();

  // -- 5. Low Priority: Insert Slots (Bottom) --
  // Calculate how many inserts we can fit while keeping minFaderHeight
  int maxInsertAreaHeight = availableHeight - minFaderHeight - 8; // 8 for spacing
  float insertHeight = kInsertSlotHeight;
  
  // Reset header bounds
  insertHeaderBounds_ = SkRect::MakeEmpty();

  // Always reserve space for at least 0 inserts. 
  // If we have space, fill 'er up.
  if (maxInsertAreaHeight > 0) {
      int usableInsertHeight = std::min(maxInsertAreaHeight, static_cast<int>(insertHeight * kNumInsertSlots + 8));
      
      // Calculate header position (just above the inserts)
      if (usableInsertHeight >= insertHeight) {
           insertHeaderBounds_ = SkRect::MakeXYWH(8, bounds.getBottom() - usableInsertHeight - 14, 100, 10);
      }

      auto insertArea = bounds.removeFromBottom(usableInsertHeight);
      insertArea.removeFromBottom(4);
      
      for (size_t i = 0; i < insertSlots_.size(); ++i) {
          if (insertArea.getHeight() >= insertHeight) {
              insertSlots_[i]->setVisible(true);
              insertSlots_[i]->setBounds(insertArea.removeFromTop(static_cast<int>(insertHeight)).reduced(2, 0));
          } else {
              insertSlots_[i]->setVisible(false);
          }
      }
      bounds.removeFromBottom(8);
  } else {
      for (auto &slot : insertSlots_) slot->setVisible(false);
  }

  // -- 6. Remaining: Meter and Fader --
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
    soloButton_.setStyle(SkiaButton::Style::Warning);
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
    soloButton_.setStyle(SkiaButton::Style::Warning);
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

MixerChannelComponent::LevelMeter::LevelMeter() { if (juce::MessageManager::getInstanceWithoutCreating() != nullptr) startTimerHz(60); }

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
  bgPaint.setColor(design::colors::BG_04);
  bgPaint.setAntiAlias(true);
  canvas->drawRoundRect(bounds, design::dimensions::RADIUS_SM, design::dimensions::RADIUS_SM, bgPaint);

  if (level < 0.001f && peak < 0.001f)
    return;

  // Calculate normalized level
  float levelDb = juce::Decibels::gainToDecibels(level);
  float normalizedLevel = juce::jmap(levelDb, -60.0f, 0.0f, 0.0f, 1.0f);
  normalizedLevel = juce::jlimit(0.0f, 1.0f, normalizedLevel);

  // Define colors at function scope for reuse
  SkColor cGreen = design::colors::SUCCESS;
  SkColor cAmber = design::colors::WARNING;
  SkColor cRed = design::colors::DANGER;

  if (normalizedLevel > 0.01f) {
    float barHeight = bounds.height() * normalizedLevel;
    SkRect meterRect =
        SkRect::MakeXYWH(bounds.x() + 2, bounds.bottom() - barHeight - 2,
                         bounds.width() - 4, barHeight);

    // Gradient: green -> yellow -> red based on level
    SkColor topColor = cGreen;
    if (normalizedLevel > 0.9f) {
      topColor = cRed;
    } else if (normalizedLevel > 0.7f) {
      topColor = cAmber;
    } else if (normalizedLevel > 0.5f) {
      topColor = design::colors::NEON_YELLOW; // Yellow-green
    }

    SkPoint pts[2] = {{meterRect.centerX(), meterRect.bottom()},
                      {meterRect.centerX(), meterRect.top()}};
    SkColor gradColors[3] = {cGreen, design::colors::NEON_YELLOW,
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
      glowPaint.setColor(design::withAlpha(topColor, 0.3f));
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
    peakPaint.setColor(normalizedPeak > 0.95f ? cRed : SK_ColorWHITE);
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

void MixerChannelComponent::LevelMeter::mouseDown(const juce::MouseEvent& e) {
  if (e.mods.isRightButtonDown()) {
    auto menu = ContextMenuManager::createMenu();
    menu->addItem(1, "Reset Peak", true, false, [this]() {
        peakLevel_ = 0;
        peakLevelL_ = 0;
        peakLevelR_ = 0;
        repaint();
    });
    ContextMenuManager::getInstance().showMenuAt(std::move(menu), this, e.x, e.y);
  }
}

void MixerChannelComponent::LevelMeter::setLevel(float level) {
  targetLevel_.store(juce::jlimit(0.0f, 1.0f, level));
}

//==============================================================================
// InsertSlotIndicator Implementation
//==============================================================================

MixerChannelComponent::InsertSlotIndicator::InsertSlotIndicator(MixerChannelComponent& owner, int slotIndex)
    : owner_(owner), slotIndex_(slotIndex) {
  setSize(80, static_cast<int>(kInsertSlotHeight));
}

void MixerChannelComponent::InsertSlotIndicator::mouseDown(const juce::MouseEvent& e) {
  if (e.mods.isRightButtonDown()) {
    auto menu = ContextMenuManager::createMenu();
    
    if (isOccupied_) {
      menu->addSectionHeader(pluginName_);
      
      menu->addItem(1, "Bypass", true, false, [this]() {
          if (auto* track = this->owner_.getTrack()) {
              if (auto* plugin = track->getPlugin(slotIndex_)) {
                  plugin->suspendProcessing(!plugin->isSuspended());
                  // Force mismatch-check repaint to show bypass state
                  owner_.repaint();
              }
          }
      });
      
      menu->addItem(2, "Show Editor", true, false, [this]() {
          if (auto* track = this->owner_.getTrack()) {
              if (auto* plugin = track->getPlugin(slotIndex_)) {
                  this->owner_.getEngine().getPluginEditorWindowManager().openEditor(plugin, "", slotIndex_);
              }
          }
      });
      
      menu->addSeparator();
      
      menu->addItem(3, "Replace Plugin...", true, false, [this]() {
          auto* engine = &this->owner_.getEngine();
          auto descArr = engine->getPluginHost().getPluginDescriptions();
          if (descArr.size() > 0) {
              if (auto* track = this->owner_.getTrack()) {
                  if (auto plugin = engine->getPluginHost().createPlugin(descArr[0])) {
                      track->removePlugin(slotIndex_);
                      track->addPlugin(std::move(plugin));
                  }
              }
          }
      });
      
      menu->addItemComplete(4, "Remove Plugin", SkPath(), "", true, false, true, [this]() {
          if (auto* track = this->owner_.getTrack()) {
              track->removePlugin(slotIndex_);
          }
      });
    } else {
      menu->addItem(1, "Add Plugin...", true, false, [this]() {
          auto& engine = this->owner_.getEngine();
          
          auto* browser = new PluginBrowser(engine.getPluginHost(), [this](const juce::PluginDescription& desc) {
              if (auto* track = this->owner_.getTrack()) {
                  // Load plugin
                  juce::String error;
                  // Note: creating instance is blocking for now, ideally async
                  if (auto plugin = this->owner_.getEngine().getPluginHost().createInstance(desc, 44100, 512, error)) {
                       track->addPlugin(std::move(plugin));
                  }
              }
          });

          browser->setSize(350, 450);
          
          juce::CallOutBox::launchAsynchronously(std::unique_ptr<juce::Component>(browser), this->getScreenBounds(), nullptr);
      });
    }
    
    ContextMenuManager::getInstance().showMenuAt(std::move(menu), this, e.x, e.y);
  }
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
    bgPaint.setColor(design::colors::BG_02);
  } else {
    // Empty slot: very subtle
    bgPaint.setColor(design::colors::BG_03);
  }

  canvas->drawRoundRect(skBounds, design::dimensions::RADIUS_SM, design::dimensions::RADIUS_SM, bgPaint);

  // Border
  SkPaint borderPaint;
  borderPaint.setStyle(SkPaint::kStroke_Style);
  borderPaint.setStrokeWidth(0.5f);
  borderPaint.setColor(design::colors::BORDER_SUBTLE);
  borderPaint.setAntiAlias(true);
  canvas->drawRoundRect(skBounds, design::dimensions::RADIUS_SM, design::dimensions::RADIUS_SM, borderPaint);

  // Text
  SkPaint textPaint;
  textPaint.setAntiAlias(true);
  SkFont font = typography::getSkFont(9.0f, design::FontWeight::Regular);

  if (isOccupied_) {
    textPaint.setColor(design::colors::TEXT_PRIMARY);
    // Truncate plugin name if needed
    juce::String displayName = pluginName_.substring(0, kMaxPluginNameLength);
    if (pluginName_.length() > kMaxPluginNameLength)
      displayName += "...";
    canvas->drawString(displayName.toRawUTF8(), 4, skBounds.centerY() + 3, font,
                       textPaint);
  } else {
    textPaint.setColor(design::colors::TEXT_TERTIARY);
    canvas->drawString(("Slot " + juce::String(slotIndex_ + 1)).toRawUTF8(), 4,
                       skBounds.centerY() + 3, font, textPaint);
  }

  // Occupied indicator dot
  if (isOccupied_) {
    SkPaint dotPaint;
    dotPaint.setColor(design::colors::ACCENT_PRIMARY);
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

MixerChannelComponent::SendIndicator::SendIndicator(MixerChannelComponent& owner, int sendIndex)
    : owner_(owner), sendIndex_(sendIndex) {
  setSize(80, static_cast<int>(kSendIndicatorHeight));
}

void MixerChannelComponent::SendIndicator::mouseDown(const juce::MouseEvent& e) {
  if (e.mods.isRightButtonDown()) {
    auto menu = ContextMenuManager::createMenu();
    menu->addSectionHeader("Send " + juce::String(sendIndex_ + 1));
    
    bool isPre = false;
    if (auto* track = this->owner_.getTrack()) {
        isPre = track->isSendPreFader(sendIndex_);
    }

    menu->addItem(1, "Pre-Fader", true, isPre, [this]() {
        if (auto* track = this->owner_.getTrack()) {
            track->setSendPreFader(sendIndex_, true);
        }
    });
    menu->addItem(2, "Post-Fader", true, !isPre, [this]() {
        if (auto* track = this->owner_.getTrack()) {
            track->setSendPreFader(sendIndex_, false);
        }
    });
    
    menu->addSeparator();
    
    auto destMenu = ContextMenuManager::createMenu();
    destMenu->addItem(100, "None", true, destinationName_.isEmpty(), [this]() {
        if (auto* track = this->owner_.getTrack()) {
            track->setSendDestination(sendIndex_, -1); // -1 for none
        }
        this->setDestination("");
        setSendLevel(0.0f);
    });
    
    // Dynamic list of aux buses
    int numAux = this->owner_.getEngine().getNumAuxBuses();
    if (numAux > 0) {
        for (int i = 0; i < numAux; ++i) {
            if (auto* bus = this->owner_.getEngine().getAuxBus(i)) {
                 bool isCurrent = destinationName_ == bus->getName();
                 auto* busPtr = bus;
                 destMenu->addItem(200 + i, bus->getName(), true, isCurrent, [=, this]() {
                     if (auto* track = this->owner_.getTrack()) {
                         track->setSendDestination(sendIndex_, i); 
                     }
                     this->setDestination(busPtr->getName());
                 });
            }
        }
    } else {
        destMenu->addItem(999, "No Aux Buses", false, false, nullptr);
    }

    menu->addSubMenu("Set Destination", std::move(destMenu));
    
    menu->addSeparator();
    menu->addItemComplete(3, "Remove Send", SkPath(), "", true, false, true, [this]() {
         if (auto* track = this->owner_.getTrack()) {
            track->setSendDestination(sendIndex_, -1);
            track->setSendLevel(sendIndex_, 0.0f);
        }
        setDestination("");
        setSendLevel(0.0f);
    });
    
    ContextMenuManager::getInstance().showMenuAt(std::move(menu), this, e.x, e.y);
  }
}

void MixerChannelComponent::SendIndicator::drawSkia(SkCanvas *canvas) {
  using namespace design;

  auto bounds = getLocalBounds().toFloat();
  SkRect skBounds = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());

  // Background
  SkPaint bgPaint;
  bgPaint.setColor(design::colors::BG_03);
  bgPaint.setAntiAlias(true);
  canvas->drawRoundRect(skBounds, design::dimensions::RADIUS_SM, design::dimensions::RADIUS_SM, bgPaint);

  // Send level bar
  if (sendLevel_ > 0.01f) {
    float barWidth = (skBounds.width() - 4) * sendLevel_;
    SkRect barRect = SkRect::MakeXYWH(2, skBounds.bottom() - 4, barWidth, 2);

    SkPaint barPaint;
    barPaint.setColor(design::colors::ACCENT_SECONDARY);
    barPaint.setAntiAlias(true);
    canvas->drawRoundRect(barRect, 1.0f, 1.0f, barPaint);
  }

  // Text: Send destination or "Send X"
  SkPaint textPaint;
  textPaint.setAntiAlias(true);
  SkFont font = typography::getSkFont(9.0f, design::FontWeight::Regular);

  juce::String displayText;
  if (destinationName_.isNotEmpty()) {
    textPaint.setColor(design::colors::TEXT_PRIMARY);
    displayText = destinationName_.substring(0, 10);
    if (destinationName_.length() > 10)
      displayText += "...";
  } else {
    juce::Colour txt = ZenithTheme::Colors::text_tertiary;
    textPaint.setColor(SkColorSetARGB(txt.getAlpha(), txt.getRed(), txt.getGreen(), txt.getBlue()));
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
    levelPaint.setColor(design::colors::TEXT_SECONDARY);
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
