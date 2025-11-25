/**
 * @file BottomBar.cpp
 * @brief Implementation of bottom bar with keyboard and mixer
 */

// POLISH: spacing normalized to 8px grid (fader width 8, radius 4px, text 12pt)

#include "BottomBar.h"
// #include "views/PianoKeyboardViewSkia.h"  // TODO: File missing - needs investigation
#include <include/core/SkFont.h>
#include <include/core/SkPaint.h>
#include <include/core/SkPath.h>
#include <include/core/SkRRect.h>


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

  SkRect skBounds = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());
  canvas.drawRect(skBounds, bgPaint);

  // Top border
  SkPaint borderPaint;
  borderPaint.setAntiAlias(true);
  borderPaint.setColor(colors.borderSubtle);
  borderPaint.setStyle(SkPaint::kStroke_Style);
  borderPaint.setStrokeWidth(1.0f);

  canvas.drawLine(0, 0, skBounds.width(), 0, borderPaint);

  // Draw mixer channels
  float channelWidth = skBounds.width() / channelCount_;
  float padding = 8.0f;

  for (int i = 0; i < channelCount_; ++i) {
    float x = i * channelWidth;

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
        SkRect::MakeXYWH(channelRect.centerX() - 4, faderY, 8,
                         faderHeight); // 8px grid: -3→-4, 6→8

    // Track background
    SkPaint trackPaint;
    trackPaint.setAntiAlias(true);
    trackPaint.setColor(colors.bg1);

    SkRRect trackRRect =
        SkRRect::MakeRectXY(faderTrackRect, 4.0f, 4.0f); // 8px grid: 3→4
    canvas.drawRRect(trackRRect, trackPaint);

    // Fill level
    float fillHeight = faderHeight * faderLevel;
    SkRect fillRect = SkRect::MakeXYWH(faderTrackRect.left(),
                                       faderTrackRect.bottom() - fillHeight,
                                       faderTrackRect.width(), fillHeight);

    SkPaint fillPaint;
    fillPaint.setAntiAlias(true);
    fillPaint.setColor(colors.accentMain);

    SkRRect fillRRect =
        SkRRect::MakeRectXY(fillRect, 4.0f, 4.0f); // 8px grid: 3→4
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
  // keyboard_ = std::make_unique<PianoKeyboardViewSkia>(keyboardState);  // TODO: PianoKeyboardViewSkia file missing
  // addAndMakeVisible(keyboard_.get());

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
    // keyboard_->setVisible(visible);  // TODO: keyboard_ member missing
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
    // keyboard_->setBounds(bounds);  // TODO: keyboard_ member missing
  } else if (!keyboardVisible_ && mixerStripVisible_) {
    // Mixer strip takes full width
    mixerStrip_->setBounds(bounds);
  } else {
    // Both visible: split horizontally
    auto mixerBounds = bounds.removeFromRight(MIXER_STRIP_WIDTH);
    mixerStrip_->setBounds(mixerBounds);
    // keyboard_->setBounds(bounds);  // TODO: keyboard_ member missing
  }
}

void BottomBar::paint(juce::Graphics &g) {
  auto &colors = SkiaTheme::getInstance().getColors();

  // Draw top border
  g.setColour(juce::Colour(colors.borderSubtle));
  g.fillRect(0, 0, getWidth(), 1);

  // Background
  g.setColour(juce::Colour(colors.bg2));
  g.fillRect(getLocalBounds().withTrimmedTop(1));
}

void BottomBar::paintToSkia(SkCanvas *canvas, SkRect bounds) {
  auto &colors = SkiaTheme::getInstance().getColors();

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

  // Draw children
  for (auto *child : getChildren()) {
    if (!child->isVisible())
      continue;

    auto childBounds = child->getBounds();
    SkRect childSkBounds = SkRect::MakeXYWH(
        bounds.left() + childBounds.getX(), bounds.top() + childBounds.getY(),
        childBounds.getWidth(), childBounds.getHeight());

    auto *skiaChild = dynamic_cast<SkiaComponent *>(child);
    if (skiaChild && skiaChild->supportsSkiaRendering()) {
      // Native Skia rendering
      canvas->save();
      canvas->clipRect(childSkBounds);
      skiaChild->paintToSkia(canvas, childSkBounds);
      canvas->restore();
    } else {
      // JUCE Fallback rendering
      juce::Image componentImage(juce::Image::ARGB,
                                 juce::jmax(1, childBounds.getWidth()),
                                 juce::jmax(1, childBounds.getHeight()), true);

      juce::Graphics componentGraphics(componentImage);
      componentGraphics.setOrigin(-childBounds.getX(), -childBounds.getY());
      child->paint(componentGraphics);

      juce::Image::BitmapData bitmapData(componentImage,
                                         juce::Image::BitmapData::readOnly);

      SkImageInfo imageInfo = SkImageInfo::MakeN32Premul(
          componentImage.getWidth(), componentImage.getHeight());

      sk_sp<SkImage> skiaImage = SkImages::RasterFromPixmapCopy(
          SkPixmap(imageInfo, bitmapData.data, bitmapData.lineStride));

      if (skiaImage) {
        SkPaint paint;
        paint.setAntiAlias(true);
        canvas->drawImage(skiaImage, childSkBounds.left(), childSkBounds.top(),
                          SkSamplingOptions(SkFilterMode::kLinear), &paint);
      }
    }
  }
}

} // namespace zenith
