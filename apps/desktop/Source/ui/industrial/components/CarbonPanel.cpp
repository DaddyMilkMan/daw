#include "CarbonPanel.h"
#include <cmath>
#include <core/SkFont.h>
#include <core/SkPath.h>
#include <effects/SkImageFilters.h>

namespace zenith::industrial {

CarbonPanel::CarbonPanel(std::string title, IndustrialTheme &theme,
                         const SkBitmap *texture, bool expertDecorations)
    : title_(std::move(title)), theme_(theme), texture_(texture),
      expertDecorations_(expertDecorations) {}

void CarbonPanel::render(SkCanvas *canvas) {
  if (!isVisible()) {
    return;
  }

  SkPaint paint;
  paint.setAntiAlias(true);

  auto rrect = SkRRect::MakeRectXY(bounds_, 8.0f, 8.0f);
  paint.setColor(IndustrialTheme::GRAPHITE);
  canvas->drawRRect(rrect, paint);

  if (texture_ && !texture_->drawsNothing()) {
    paint.setAlphaf(0.55f);
    if (auto image = texture_->asImage()) {
      canvas->drawImageRect(image, bounds_, SkSamplingOptions(), &paint);
    }
    paint.setAlphaf(1.0f);
  }

  paint.setStyle(SkPaint::kStroke_Style);
  paint.setStrokeWidth(1.0f);
  paint.setColor(IndustrialTheme::SILVER);
  canvas->drawRRect(rrect, paint);

  paint.setStyle(SkPaint::kStroke_Style);
  paint.setStrokeWidth(1.0f);
  paint.setColor(0x33000000);
  canvas->drawRRect(
      SkRRect::MakeRectXY(bounds_.makeInset(2.0f, 2.0f), 6.0f, 6.0f), paint);

  SkFont titleFont(theme_.getTypeface(IndustrialTheme::FontWeight::SemiBold),
                   16.0f);
  titleFont.setEdging(SkFont::Edging::kSubpixelAntiAlias);
  paint.setStyle(SkPaint::kFill_Style);
  paint.setColor(IndustrialTheme::WHITE);
  const juce::String uppercaseTitle = juce::String(title_).toUpperCase();
  canvas->drawString(uppercaseTitle.toRawUTF8(), bounds_.x() + 10.0f,
                     bounds_.y() + 18.0f, titleFont, paint);

  paint.setStrokeWidth(1.0f);
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setColor(0x446A6A6A);
  canvas->drawLine(bounds_.x() + 10.0f, bounds_.y() + 23.0f,
                   bounds_.right() - 10.0f, bounds_.y() + 23.0f, paint);

  if (expertDecorations_) {
    auto drawScrew = [&](float cx, float cy) {
      SkPath hex;
      constexpr float r = 5.0f;
      for (int i = 0; i < 6; ++i) {
        const float angle =
            juce::MathConstants<float>::twoPi * (static_cast<float>(i) / 6.0f) +
            juce::MathConstants<float>::pi * 0.5f;
        const float x = cx + std::cos(angle) * r;
        const float y = cy + std::sin(angle) * r;
        if (i == 0) {
          hex.moveTo(x, y);
        } else {
          hex.lineTo(x, y);
        }
      }
      hex.close();

      paint.setStyle(SkPaint::kFill_Style);
      paint.setColor(0xFF4A4A4A);
      canvas->drawPath(hex, paint);
      paint.setStyle(SkPaint::kStroke_Style);
      paint.setStrokeWidth(1.0f);
      paint.setColor(0xFF2A2A2A);
      canvas->drawPath(hex, paint);
      paint.setStyle(SkPaint::kFill_Style);
      paint.setColor(0xFF232323);
      canvas->drawCircle(cx, cy, 1.7f, paint);
    };

    drawScrew(bounds_.x() + 10.0f, bounds_.y() + 10.0f);
    drawScrew(bounds_.right() - 10.0f, bounds_.y() + 10.0f);
    drawScrew(bounds_.x() + 10.0f, bounds_.bottom() - 10.0f);
    drawScrew(bounds_.right() - 10.0f, bounds_.bottom() - 10.0f);

    if (bounds_.width() > 300.0f && bounds_.height() > 96.0f) {
      const SkRect serialRect = SkRect::MakeXYWH(
          bounds_.right() - 198.0f, bounds_.bottom() - 22.0f, 188.0f, 14.0f);
      paint.setStyle(SkPaint::kFill_Style);
      paint.setColor(0xCC1A1A1A);
      canvas->drawRRect(SkRRect::MakeRectXY(serialRect, 2.0f, 2.0f), paint);
      paint.setStyle(SkPaint::kStroke_Style);
      paint.setStrokeWidth(1.0f);
      paint.setColor(0x886A6A6A);
      canvas->drawRRect(SkRRect::MakeRectXY(serialRect, 2.0f, 2.0f), paint);

      SkFont plateFont(theme_.getTypeface(IndustrialTheme::FontWeight::Medium),
                       10.0f);
      plateFont.setEdging(SkFont::Edging::kSubpixelAntiAlias);
      paint.setStyle(SkPaint::kFill_Style);
      paint.setColor(0xFFAAAAAA);
      canvas->drawString("S/N: ZPS-2026-001   CAL: 2025-02-17",
                         serialRect.x() + 6.0f, serialRect.bottom() - 4.0f,
                         plateFont, paint);
    }

    if (bounds_.width() > 420.0f && bounds_.height() > 120.0f) {
      paint.setStyle(SkPaint::kStroke_Style);
      paint.setStrokeWidth(1.0f);
      paint.setColor(0x553A3A3A);
      float vx = bounds_.x() + 14.0f;
      const float vy = bounds_.bottom() - 18.0f;
      for (int i = 0; i < 22; ++i) {
        canvas->drawLine(vx, vy, vx + 5.0f, vy, paint);
        vx += 7.0f;
      }
    }
  }
}

} // namespace zenith::industrial
