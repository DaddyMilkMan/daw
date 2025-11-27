/**
 * @file BottomBar.cpp
 * @brief Implementation of bottom bar with keyboard and mixer
 */

#include "BottomBar.h"

#ifdef ZENITH_USE_SKIA
#include <include/core/SkFont.h>
#include <include/core/SkPaint.h>
#include <include/core/SkPath.h>
#include <include/core/SkRRect.h>
#endif

namespace zenith {

//==============================================================================
// MixerStrip Implementation
//==============================================================================

BottomBar::MixerStrip::MixerStrip() { setSize(600, 96); }

void BottomBar::MixerStrip::setChannelCount(int count) {
  channelCount_ = juce::jlimit(1, 16, count);
  repaint();
}

void BottomBar::MixerStrip::paintSkia(SkCanvas &canvas,
                                      const juce::Rectangle<int> &bounds) {
  auto &colors = SkiaTheme::getInstance().getColors();

  // Background
  SkPaint bgPaint;
  bgPaint.setAntiAlias(true);
  bgPaint.setColor(colors.bg2);

  SkRect skBounds = SkRect::MakeWH(static_cast<float>(bounds.getWidth()), 
                                    static_cast<float>(bounds.getHeight()));
  canvas.drawRect(skBounds, bgPaint);

  // Top border
  SkPaint borderPaint;
  borderPaint.setAntiAlias(true);
  borderPaint.setColor(colors.borderSubtle);
  borderPaint.setStyle(SkPaint::kStroke_Style);
  borderPaint.setStrokeWidth(1.0f);

  canvas.drawLine(0, 0, skBounds.width(), 0, borderPaint);

  // Draw mixer channels
  float channelWidth = skBounds.width() / static_cast<float>(channelCount_);
  float padding = 8.0f;

  for (int i = 0; i < channelCount_; ++i) {
    float x = static_cast<float>(i) * channelWidth;

    // Channel background
    SkRect channelRect =
        SkRect::MakeXYWH(x + padding, padding, channelWidth - padding * 2,
                         skBounds.height() - padding * 2);

    SkPaint channelPaint;
    channelPaint.setAntiAlias(true);
    channelPaint.setColor(colors.bg3);

    SkRRect channelRRect = SkRRect::MakeRectXY(channelRect, 4.0f, 4.0f);
    canvas.drawRRect(channelRRect, channelPaint);

    // Fader (vertical bar)
    float faderHeight = channelRect.height() * 0.6f;
    float faderY = channelRect.centerY() - faderHeight / 2;
    float faderLevel = 0.7f; // Placeholder level

    SkRect faderTrackRect =
        SkRect::MakeXYWH(channelRect.centerX() - 4, faderY, 8, faderHeight);

    // Track background
    SkPaint trackPaint;
    trackPaint.setAntiAlias(true);
    trackPaint.setColor(colors.bg1);

    SkRRect trackRRect = SkRRect::MakeRectXY(faderTrackRect, 4.0f, 4.0f);
    canvas.drawRRect(trackRRect, trackPaint);

    // Fill level
    float fillHeight = faderHeight * faderLevel;
    SkRect fillRect = SkRect::MakeXYWH(faderTrackRect.left(),
                                       faderTrackRect.bottom() - fillHeight,
                                       faderTrackRect.width(), fillHeight);

    SkPaint fillPaint;
    fillPaint.setAntiAlias(true);
    fillPaint.setColor(colors.accentMain);

    SkRRect fillRRect = SkRRect::MakeRectXY(fillRect, 4.0f, 4.0f);
    canvas.drawRRect(fillRRect, fillPaint);

    // Channel number
    auto &typo = SkiaTheme::getInstance().getTypography();
    SkFont font;
    font.setSize(typo.small.size);

    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(colors.textMuted);

    juce::String channelLabel = juce::String(i + 1);
    SkRect textBounds;
    font.measureText(channelLabel.toRawUTF8(), channelLabel.length(),
                     SkTextEncoding::kUTF8, &textBounds);

    float textX = channelRect.centerX() - textBounds.width() / 2;
    float textY = channelRect.bottom() - 8;

    canvas.drawString(channelLabel.toRawUTF8(), textX, textY, font, textPaint);
  }
}

//==============================================================================
// BottomBar Construction
//==============================================================================

BottomBar::BottomBar(juce::MidiKeyboardState &keyboardState) {
  // Create piano keyboard
  keyboard_ = std::make_unique<PianoKeyboardViewSkia>(keyboardState);
  addAndMakeVisible(keyboard_.get());

  // Create mixer strip
  mixerStrip_ = std::make_unique<MixerStrip>();
  addChildComponent(mixerStrip_.get());

  setSize(800, KEYBOARD_HEIGHT);
}

//==============================================================================
// Visibility Control
//==============================================================================

void BottomBar::setKeyboardVisible(bool visible) {
  if (keyboardVisible_ != visible) {
    keyboardVisible_ = visible;
    if (keyboard_) {
      keyboard_->setVisible(visible);
    }
    resized();
  }
}

void BottomBar::setMixerStripVisible(bool visible) {
  if (mixerStripVisible_ != visible) {
    mixerStripVisible_ = visible;
    mixerStrip_->setVisible(visible);
    resized();
  }
}

//==============================================================================
// Component Overrides
//==============================================================================

void BottomBar::resized() {
  auto bounds = getLocalBounds();

  if (!keyboardVisible_ && !mixerStripVisible_) {
    return;
  }

  if (keyboardVisible_ && !mixerStripVisible_) {
    // Keyboard takes full width
    if (keyboard_) {
      keyboard_->setBounds(bounds);
    }
  } else if (!keyboardVisible_ && mixerStripVisible_) {
    // Mixer strip takes full width
    mixerStrip_->setBounds(bounds);
  } else {
    // Both visible: split horizontally
    auto mixerBounds = bounds.removeFromRight(MIXER_STRIP_WIDTH);
    mixerStrip_->setBounds(mixerBounds);
    if (keyboard_) {
      keyboard_->setBounds(bounds);
    }
  }
}

#ifndef ZENITH_USE_SKIA
void BottomBar::paint(juce::Graphics &g) {
  auto &colors = SkiaTheme::getInstance().getColors();

  // Draw top border
  g.setColour(juce::Colour(colors.borderSubtle));
  g.fillRect(0, 0, getWidth(), 1);

  // Background
  g.setColour(juce::Colour(colors.bg2));
  g.fillRect(getLocalBounds().withTrimmedTop(1));
}
#endif

#ifdef ZENITH_USE_SKIA
void BottomBar::drawSkia(SkCanvas *canvas) {
  auto &colors = SkiaTheme::getInstance().getColors();
  auto bounds = SkRect::MakeWH(static_cast<float>(getWidth()), 
                                static_cast<float>(getHeight()));

  // Draw top border
  SkPaint borderPaint;
  borderPaint.setAntiAlias(true);
  borderPaint.setColor(colors.borderSubtle);
  canvas->drawRect(
      SkRect::MakeXYWH(bounds.left(), bounds.top(), bounds.width(), 1.0f),
      borderPaint);

  // Background
  SkPaint bgPaint;
  bgPaint.setAntiAlias(true);
  bgPaint.setColor(colors.bg2);
  canvas->drawRect(SkRect::MakeXYWH(bounds.left(), bounds.top() + 1.0f,
                                    bounds.width(), bounds.height() - 1.0f),
                   bgPaint);

  // Note: Children are rendered recursively by SkiaMainWindowIntegration::renderComponentRecursively
  // We don't need to manually render them here
}
#endif

} // namespace zenith
