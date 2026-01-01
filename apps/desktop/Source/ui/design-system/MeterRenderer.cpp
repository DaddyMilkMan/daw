/*
  ==============================================================================

    MeterRenderer.cpp
    Created: 2025-12-31
    Author:  Zenith DAW

  ==============================================================================
*/

#include "MeterRenderer.h"
#include <effects/SkGradientShader.h>
#include <core/SkMaskFilter.h>
#include <core/SkBlurTypes.h>

namespace zenith::design {

void MeterRenderer::drawMeter(SkCanvas* canvas, const SkRect& bounds, 
                              float level, float peak, 
                              const Options& options) {
  
  // Background
  SkPaint bgPaint;
  bgPaint.setColor(design::colors::BG_04);
  bgPaint.setAntiAlias(true);
  canvas->drawRoundRect(bounds, design::dimensions::RADIUS_SM, design::dimensions::RADIUS_SM, bgPaint);

  if (level < 0.001f && peak < 0.001f) return;

  // Draw Bar
  SkRect contentBounds = bounds;
  contentBounds.inset(2, 2); // Padding
  
  // Draw Label if requested (horizontal only for now usually)
  if (options.showLabel && options.labelText.isNotEmpty()) {
      SkPaint textPaint;
      textPaint.setColor(design::colors::TEXT_SECONDARY);
      textPaint.setAntiAlias(true);
      SkFont font = typography::getSkFont(10.0f, FontWeight::Medium);
      
      canvas->drawString(options.labelText.toRawUTF8(), 
                         contentBounds.left(), 
                         contentBounds.centerY() + 3, 
                         font, textPaint);
      
      // Shift content to right
      contentBounds.fLeft += 60.0f; 
  }

  // Bar Rect Calculation
  SkRect barRect = contentBounds;
  float normalizedLevel = juce::jlimit(0.0f, 1.0f, level);

  if (normalizedLevel > 0.001f) {
      drawBar(canvas, barRect, normalizedLevel, options.isHorizontal);
  }

  // Peak Indicator
  if (options.showPeak && peak > 0.001f && !options.isHorizontal) { // Peak usually for vertical meters
      float normalizedPeak = juce::jlimit(0.0f, 1.0f, peak);
      
      float peakY = contentBounds.bottom() - (contentBounds.height() * normalizedPeak);
      SkPaint peakPaint;
      peakPaint.setColor(normalizedPeak > 0.95f ? design::colors::DANGER : SK_ColorWHITE);
      peakPaint.setAntiAlias(true);
      
      // Draw 2px high line
      canvas->drawRect(SkRect::MakeXYWH(contentBounds.left(), peakY, contentBounds.width(), 2.0f), peakPaint);
  }
}

void MeterRenderer::drawBar(SkCanvas* canvas, const SkRect& bounds, float level, bool isHorizontal) {
    SkColor cGreen = design::colors::SUCCESS;
    SkColor cAmber = design::colors::WARNING;
    SkColor cRed = design::colors::DANGER;
    SkColor cYellow = design::colors::NEON_YELLOW;

    // Calculate filled rect
    SkRect fillRect = bounds;
    if (isHorizontal) {
        fillRect.fRight = fillRect.fLeft + (fillRect.width() * level);
    } else {
        float h = fillRect.height() * level;
        fillRect.fTop = fillRect.bottom() - h;
    }

    // Gradient Setup
    SkPoint pts[2];
    if (isHorizontal) {
        pts[0] = {bounds.left(), bounds.centerY()};
        pts[1] = {bounds.right(), bounds.centerY()};
    } else {
        pts[0] = {bounds.centerX(), bounds.bottom()};
        pts[1] = {bounds.centerX(), bounds.top()};
    }

    SkColor colors[3] = {cGreen, cYellow, cRed};
    SkScalar pos[3] = {0.0f, 0.6f, 1.0f};
    
    // Dynamic top color for glow
    SkColor topColor = cGreen;
    if (level > 0.9f) topColor = cRed;
    else if (level > 0.7f) topColor = cAmber;
    else if (level > 0.5f) topColor = cYellow;

    SkPaint barPaint;
    barPaint.setShader(SkGradientShader::MakeLinear(pts, colors, pos, 3, SkTileMode::kClamp));
    barPaint.setAntiAlias(true);
    
    // Draw Main Bar
    canvas->drawRoundRect(fillRect, 1.0f, 1.0f, barPaint);

    // Draw Glow for high levels
    if (level > 0.7f) {
        SkPaint glowPaint;
        glowPaint.setColor(design::withAlpha(topColor, 0.3f));
        glowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 4.0f));
        glowPaint.setAntiAlias(true);
        canvas->drawRoundRect(fillRect, 1.0f, 1.0f, glowPaint);
    }
}

void MeterRenderer::drawVerticalLevelMeter(SkCanvas* canvas, const SkRect& bounds, float level, float peak) {
    Options opts;
    opts.isHorizontal = false;
    opts.showPeak = true;
    drawMeter(canvas, bounds, level, peak, opts);
}

// Default options overload
void MeterRenderer::drawMeter(SkCanvas* canvas, const SkRect& bounds, float level, float peak) {
    Options opts;
    drawMeter(canvas, bounds, level, peak, opts);
}

void MeterRenderer::drawHorizontalMeter(SkCanvas* canvas, const SkRect& bounds, float value, const juce::String& label) {
    Options opts;
    opts.isHorizontal = true;
    opts.showPeak = false;
    opts.showLabel = true;
    opts.labelText = label;
    drawMeter(canvas, bounds, value, 0.0f, opts);
}

} // namespace zenith::design
