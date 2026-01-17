/*
  ==============================================================================
    MixerChannelRendering.cpp
    MixerChannelComponent rendering implementation
  ==============================================================================
*/

#include "MixerChannelComponent.h"
#include "GlassmorphicPanel.h"
#include "ZenithDesignSystem.h"
#include "../design-system/ZenithTypography.h"
#include "../design-system/MeterRenderer.h"
#include "../design-system/ZenithTheme.h"
#include "ZenithSkia.h"

namespace zenith {

void MixerChannelComponent::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();
  SkRect skBounds = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());

  if (isSelected_) {
    zenith::GlassmorphicPanel::drawWithAccent(
        canvas, skBounds,
        isMaster_ ? design::colors::ACCENT_SECONDARY
                  : design::colors::ACCENT_PRIMARY,
        GlassmorphicPanel::Style::ActiveGlow);
  } else {
    zenith::GlassmorphicPanel::draw(canvas, skBounds,
                                    isMaster_
                                        ? GlassmorphicPanel::Style::Floating
                                        : GlassmorphicPanel::Style::Elevated);
  }

  drawChildren(canvas);

  if (!insertHeaderBounds_.isEmpty()) {
    SkPaint labelPaint;
    labelPaint.setColor(design::colors::TEXT_TERTIARY);
    labelPaint.setAntiAlias(true);
    SkFont labelFont =
        design::typography::getSkFont(10.0f, design::FontWeight::Medium);
    canvas->drawString("INSERTS", insertHeaderBounds_.x(), insertHeaderBounds_.bottom(), labelFont, labelPaint);
  }

  if (!sendHeaderBounds_.isEmpty()) {
    SkPaint labelPaint;
    labelPaint.setColor(design::colors::TEXT_TERTIARY);
    labelPaint.setAntiAlias(true);
    SkFont labelFont =
        design::typography::getSkFont(10.0f, design::FontWeight::Medium);
    canvas->drawString("SENDS", sendHeaderBounds_.x(), sendHeaderBounds_.bottom(), labelFont, labelPaint);
  }

  if (isMaster_) {
    SkPaint badgePaint;
    badgePaint.setColor(design::withAlpha(design::colors::ACCENT_SECONDARY, design::opacity::GLASS_SOLID));
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

  if (hasFocus_) {
    SkPaint focusPaint;
    focusPaint.setAntiAlias(true);
    focusPaint.setStyle(SkPaint::kStroke_Style);
    focusPaint.setStrokeWidth(design::accessibility::FOCUS_RING_WIDTH);
    focusPaint.setColor(design::accessibility::FOCUS_RING_COLOR);
    
    SkRect focusRect = skBounds;
    float inset = design::accessibility::FOCUS_RING_OFFSET;
    focusRect.inset(inset, inset);
    
    canvas->drawRoundRect(focusRect, design::dimensions::RADIUS_SM + 2, 
                         design::dimensions::RADIUS_SM + 2, focusPaint);
  }
}

//==============================================================================
// LevelMeter Rendering
//==============================================================================

void MixerChannelComponent::LevelMeter::drawMeterBar(SkCanvas *canvas,
                                                     const SkRect &bounds,
                                                     float level, float peak) {
  zenith::design::MeterRenderer::drawVerticalLevelMeter(canvas, bounds, level, peak);
}

void MixerChannelComponent::LevelMeter::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();
  SkRect skBounds = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());

  if (stereo_) {
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

//==============================================================================
// InsertSlotIndicator Rendering
//==============================================================================

void MixerChannelComponent::InsertSlotIndicator::drawSkia(SkCanvas *canvas) {
  using namespace design;

  auto bounds = getLocalBounds().toFloat();
  SkRect skBounds = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());

  SkPaint bgPaint;
  bgPaint.setAntiAlias(true);

  if (isOccupied_) {
    bgPaint.setColor(design::colors::BG_02);
  } else {
    bgPaint.setColor(design::colors::BG_03);
  }

  canvas->drawRoundRect(skBounds, design::dimensions::RADIUS_SM, design::dimensions::RADIUS_SM, bgPaint);

  SkPaint borderPaint;
  borderPaint.setStyle(SkPaint::kStroke_Style);
  borderPaint.setStrokeWidth(0.5f);
  borderPaint.setColor(design::colors::BORDER_SUBTLE);
  borderPaint.setAntiAlias(true);
  canvas->drawRoundRect(skBounds, design::dimensions::RADIUS_SM, design::dimensions::RADIUS_SM, borderPaint);

  SkPaint textPaint;
  textPaint.setAntiAlias(true);
  SkFont font = typography::getSkFont(9.0f, design::FontWeight::Regular);

  if (isOccupied_) {
    textPaint.setColor(design::colors::TEXT_PRIMARY);
    juce::String displayName = pluginName_.substring(0, 12); // Using hardcoded 12 for now
    if (pluginName_.length() > 12)
      displayName += "...";
    canvas->drawString(displayName.toRawUTF8(), 4, skBounds.centerY() + 3, font,
                       textPaint);
  } else {
    textPaint.setColor(design::colors::TEXT_TERTIARY);
    canvas->drawString(("Slot " + juce::String(slotIndex_ + 1)).toRawUTF8(), 4,
                       skBounds.centerY() + 3, font, textPaint);
  }

  if (isOccupied_) {
    SkPaint dotPaint;
    dotPaint.setColor(design::colors::ACCENT_PRIMARY);
    dotPaint.setAntiAlias(true);
    canvas->drawCircle(skBounds.right() - 6, skBounds.centerY(), 3, dotPaint);
  }
}

//==============================================================================
// SendIndicator Rendering
//==============================================================================

void MixerChannelComponent::SendIndicator::drawSkia(SkCanvas *canvas) {
  using namespace design;

  auto bounds = getLocalBounds().toFloat();
  SkRect skBounds = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());

  SkPaint bgPaint;
  bgPaint.setColor(design::colors::BG_03);
  bgPaint.setAntiAlias(true);
  canvas->drawRoundRect(skBounds, design::dimensions::RADIUS_SM, design::dimensions::RADIUS_SM, bgPaint);

  if (sendLevel_ > 0.01f) {
    float barWidth = (skBounds.width() - 4) * sendLevel_;
    SkRect barRect = SkRect::MakeXYWH(2, skBounds.bottom() - 4, barWidth, 2);

    SkPaint barPaint;
    barPaint.setColor(design::colors::ACCENT_SECONDARY);
    barPaint.setAntiAlias(true);
    canvas->drawRoundRect(barRect, dimensions::RADIUS_XXS, dimensions::RADIUS_XXS, barPaint);
  }

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
    // Fallback if ZenithTheme is not available
    textPaint.setColor(design::withAlpha(design::colors::TEXT_PRIMARY, design::opacity::SECONDARY));
    displayText = "Send " + juce::String(sendIndex_ + 1);
  }

  canvas->drawString(displayText.toRawUTF8(), 4, skBounds.centerY() + 2, font,
                     textPaint);

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

} // namespace zenith
