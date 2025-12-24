/*
  ==============================================================================

    RightSidePanel.cpp
    Created: 2025-11-28
    Author:  David Chen + Isabella Moretti

  ==============================================================================
*/

#include "RightSidePanel.h"
#include "../design-system/ZenithLayout.h"
#include "../engine/ZenithLogger.h"
#include "../visualization/SpectraAnalyzerComponent.h"

#ifdef ZENITH_USE_SKIA
#include "ZenithSkia.h"

#endif

namespace zenith {

#ifdef ZENITH_USE_SKIA

RightSidePanel::RightSidePanel(CommandAPI &api, Engine &engine) {
  ZENITH_LOG_UI(zenith::LogLevel::Info, "RightSidePanel: Constructor started");
  setSize(304, 600); // 8px grid (304 / 8 = 38)

  ZENITH_LOG_UI(zenith::LogLevel::Info,
                "RightSidePanel: Creating WingmanPanel...");
  wingmanPanel_ = std::make_unique<WingmanPanel>(api, engine);
  ZENITH_LOG_UI(zenith::LogLevel::Info,
                "RightSidePanel: WingmanPanel created. Adding child...");
  addChildComponent(wingmanPanel_.get());
  wingmanPanel_->setVisible(true);

  // Initialize SpectraAnalyzer
  spectraAnalyzer_ = std::make_unique<SpectraAnalyzerComponent>(engine);
  addChildComponent(spectraAnalyzer_.get());
  spectraAnalyzer_->setVisible(true);

  ZENITH_LOG_UI(zenith::LogLevel::Info, "RightSidePanel: Starting timer...");
  startTimerHz(60); // Animation timer
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

  // Lazy update of cached resources on the Render Thread
  if (skBounds != cachedBounds_) {
    updateCachedPaints(skBounds);
    cachedBounds_ = skBounds;
  }

  // Glassmorphism Background (Frame)
  canvas->drawRect(skBounds, bgPaint_);

  // Left border glow
  canvas->drawLine(0.0f, 0.0f, 0.0f, skBounds.height(), borderPaint_);

  // Draw child components (WingmanPanel, SpectraAnalyzer, etc.)
  drawChildren(canvas);
}

void RightSidePanel::updateCachedPaints(const SkRect &bounds) {
  // 1. Background Paint
  bgPaint_.setAntiAlias(true);
  bgPaint_.setColor(design::withAlpha(design::colors::BG_DARKER, 0.94f));
  bgPaint_.setStyle(SkPaint::kFill_Style);

  // 2. Border Paint
  borderPaint_.setAntiAlias(true);
  borderPaint_.setStyle(SkPaint::kStroke_Style);
  borderPaint_.setStrokeWidth(1.0f);
  borderPaint_.setColor(design::withAlpha(design::colors::CYAN, 0.4f));

  // 3. Text Paints
  textPaint_.setAntiAlias(true);
  textPaint_.setStyle(SkPaint::kFill_Style);
  textPaint_.setColor(design::colors::TEXT_PRIMARY);

  subTextPaint_.setAntiAlias(true);
  subTextPaint_.setStyle(SkPaint::kFill_Style);
  subTextPaint_.setColor(SkColorSetARGB(180, 200, 200, 200)); // Light grey text

  // 4. Fonts
  headerFont_ = design::getSkFont(16.0f, design::FontWeight::Bold);
  bodyFont_ = design::getSkFont(12.0f);
  labelFont_ = design::getMonoFont(10.0f);

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
      .withGap(8.0f) // 8px grid
      .addFixedItem(spectraAnalyzer_.get(), (float)bounds.getWidth(),
                    152.0f) // 8px grid (152 / 8 = 19)
      .addItem(wingmanPanel_.get())
      .applyColumn();
}

#endif // ZENITH_USE_SKIA

} // namespace zenith
