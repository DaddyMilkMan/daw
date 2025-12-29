/*
  ==============================================================================

    RightSidePanel.cpp
    Created: 2025-11-28
    Author:  David Chen + Isabella Moretti

  ==============================================================================
*/

#include "RightSidePanel.h"
#include "UndoHistoryPanel.h"
#include "../engine/ZenithLogger.h"
#include "../design-system/ZenithLayout.h"
#include "../controls/SpectraAnalyzerComponent.h"

#ifdef ZENITH_USE_SKIA
#include <core/SkCanvas.h>
#include <core/SkColor.h>
#include <core/SkFont.h>
#include <core/SkPaint.h>
#include <core/SkRect.h>

#include <effects/SkGradientShader.h>
#endif

namespace zenith {

#ifdef ZENITH_USE_SKIA

RightSidePanel::RightSidePanel(CommandAPI &api, Engine &engine, ProjectState &projectState) {
  ZENITH_LOG_UI(zenith::LogLevel::Info, "RightSidePanel: Constructor started");
  setSize(300, 600);

  ZENITH_LOG_UI(zenith::LogLevel::Info, "RightSidePanel: Creating WingmanPanel...");
  wingmanPanel_ = std::make_unique<WingmanPanel>(api, engine);
  ZENITH_LOG_UI(zenith::LogLevel::Info, "RightSidePanel: WingmanPanel created. Adding child...");
  addChildComponent(wingmanPanel_.get());
  wingmanPanel_->setVisible(true);

  // Initialize SpectraAnalyzer
  spectraAnalyzer_ = std::make_unique<SpectraAnalyzerComponent>(engine);
  addChildComponent(spectraAnalyzer_.get());
  spectraAnalyzer_->setVisible(true);

  // Initialize UndoHistoryPanel
  undoHistoryPanel_ = std::make_unique<UndoHistoryPanel>(projectState);
  addChildComponent(undoHistoryPanel_.get());
  undoHistoryPanel_->setVisible(true);

  ZENITH_LOG_UI(zenith::LogLevel::Info, "RightSidePanel: Starting timer...");
  if (juce::MessageManager::getInstanceWithoutCreating() != nullptr) startTimerHz(60); // Animation timer
  ZENITH_LOG_UI(zenith::LogLevel::Info, "RightSidePanel: Constructor complete");
}

RightSidePanel::~RightSidePanel() { stopTimer(); }

void RightSidePanel::timerCallback() {
  animationPhase_ += 0.05f;
  repaint();
}

void RightSidePanel::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();
  SkRect skBounds = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());

  if (skBounds != cachedBounds_) {
    updateCachedPaints(skBounds);
    cachedBounds_ = skBounds;
  }

  // Draw Glassmorphic Panel Background
  float radius = design::dimensions::RADIUS_LG;
  SkRRect rrect;
  rrect.setRectXY(skBounds, radius, radius);

  // Background Glass (High-Alpha for white canvas contrast)
  canvas->drawRRect(rrect, bgPaint_);

  // Left Border Glow (Cyberpunk feel)
  SkPaint glowPaint;
  glowPaint.setAntiAlias(true);
  glowPaint.setStyle(SkPaint::kStroke_Style);
  glowPaint.setStrokeWidth(2.0f);
  
  SkPoint pts[2] = { {0.0f, 0.0f}, {0.0f, skBounds.height()} };
  SkColor colors[2] = { design::colors::ACCENT_PRIMARY, 
                        design::withAlpha(design::colors::ACCENT_SECONDARY, 0.0f) };
  
  glowPaint.setShader(SkGradientShader::MakeLinear(
      pts, colors, nullptr, 2, SkTileMode::kClamp));
  canvas->drawLine(0.0f, radius, 0.0f, skBounds.height() - radius, glowPaint);

  // Subtle Outer Border
  canvas->drawRRect(rrect, borderPaint_);

  // Recursively draw children (WingmanPanel, Spectra, Undo)
  drawChildren(canvas);
}

void RightSidePanel::updateCachedPaints(const SkRect &bounds) {
  juce::ignoreUnused(bounds);
  
  // 1. Background Paint - Premium Glass for White Canvas
  bgPaint_.setAntiAlias(true);
  bgPaint_.setColor(design::withAlpha(design::colors::BG_01, 0.95f)); // Deep Charcoal but slightly transparent
  bgPaint_.setStyle(SkPaint::kFill_Style);

  // 2. Border Paint - Subtle Cyan Glow
  borderPaint_.setAntiAlias(true);
  borderPaint_.setStyle(SkPaint::kStroke_Style);
  borderPaint_.setStrokeWidth(1.0f);
  borderPaint_.setColor(design::withAlpha(design::colors::ACCENT_PRIMARY, 0.3f));

  // 3. Text Paints - Standardized Typography
  textPaint_.setAntiAlias(true);
  textPaint_.setStyle(SkPaint::kFill_Style);
  textPaint_.setColor(design::colors::TEXT_PRIMARY);

  subTextPaint_.setAntiAlias(true);
  subTextPaint_.setStyle(SkPaint::kFill_Style);
  subTextPaint_.setColor(design::colors::TEXT_SECONDARY);

  // 4. Fonts - Modern Inter & JetBrains Mono Integration
  headerFont_ = design::typography::getSkFont(design::typography::FONT_LG, 
                                            design::typography::FontWeight::Bold);
  bodyFont_ = design::typography::getSkFont(design::typography::FONT_MD, 
                                         design::typography::FontWeight::Regular);
  labelFont_ = design::typography::getSkFont(design::typography::FONT_SM, 
                                          design::typography::FontWeight::Regular);

  // 5. Meter Paints
  meterBgPaint_.setAntiAlias(true);
  meterBgPaint_.setColor(SkColorSetARGB(100, 10, 10, 10));
  meterBgPaint_.setStyle(SkPaint::kFill_Style);

  meterPeakPaint_.setAntiAlias(true);
  meterPeakPaint_.setStyle(SkPaint::kFill_Style);

  meterRmsPaint_.setAntiAlias(true);
  meterRmsPaint_.setColor(SkColorSetARGB(200, 255, 255, 255));
  meterRmsPaint_.setStyle(SkPaint::kFill_Style);
}

void RightSidePanel::resized() {
  auto bounds = getLocalBounds();

  ZenithLayout::begin()
      .withBounds(bounds)
      .withGap(5.0f)
      .addFixedItem(spectraAnalyzer_.get(), (float)bounds.getWidth(), 150.0f)
      .addFixedItem(undoHistoryPanel_.get(), (float)bounds.getWidth(), 180.0f)
      .addItem(wingmanPanel_.get())
      .applyColumn();
}

#endif // ZENITH_USE_SKIA

} // namespace zenith
