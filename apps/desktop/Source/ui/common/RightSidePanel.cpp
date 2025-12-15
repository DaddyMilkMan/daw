/*
  ==============================================================================

    RightSidePanel.cpp
    Created: 2025-11-28
    Author:  David Chen + Isabella Moretti

  ==============================================================================
*/

#include "RightSidePanel.h"
#include "../../SimpleLogger.h"
#include "../views/SpectraAnalyzerComponent.h"

#ifdef ZENITH_USE_SKIA
#include <core/SkCanvas.h>
#include <core/SkColor.h>
#include <core/SkFont.h>
#include <core/SkPaint.h>
#include <core/SkRect.h>

#endif

namespace zenith {

#ifdef ZENITH_USE_SKIA

RightSidePanel::RightSidePanel(CommandAPI &api, AIBridgeClient &client,
                               Engine &engine) {
  logToFile("RightSidePanel: Constructor started");
  setSize(300, 600);

  logToFile("RightSidePanel: Creating WingmanPanel...");
  wingmanPanel_ = std::make_unique<WingmanPanel>(api, client, engine);
  logToFile("RightSidePanel: WingmanPanel created. Adding child...");
  addChildComponent(wingmanPanel_.get());
  wingmanPanel_->setVisible(true);

  // Initialize SpectraAnalyzer
  spectraAnalyzer_ = std::make_unique<SpectraAnalyzerComponent>(engine);
  addChildComponent(spectraAnalyzer_.get());
  spectraAnalyzer_->setVisible(true);

  logToFile("RightSidePanel: Starting timer...");
  startTimerHz(60); // Animation timer
  logToFile("RightSidePanel: Constructor complete");
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

  // Note: Old meter code removed. Visualizer handles it now.
}

void RightSidePanel::updateCachedPaints(const SkRect &bounds) {
  // 1. Background Paint
  bgPaint_.setAntiAlias(true);
  bgPaint_.setColor(SkColorSetARGB(240, 20, 20, 20)); // Almost opaque dark grey
  bgPaint_.setStyle(SkPaint::kFill_Style);

  // 2. Border Paint
  borderPaint_.setAntiAlias(true);
  borderPaint_.setStyle(SkPaint::kStroke_Style);
  borderPaint_.setStrokeWidth(1.0f);
  borderPaint_.setColor(SkColorSetARGB(100, 0, 170, 255)); // Cyan accent

  // 3. Text Paints
  textPaint_.setAntiAlias(true);
  textPaint_.setStyle(SkPaint::kFill_Style);
  textPaint_.setColor(SkColorSetARGB(255, 255, 255, 255)); // White text

  subTextPaint_.setAntiAlias(true);
  subTextPaint_.setStyle(SkPaint::kFill_Style);
  subTextPaint_.setColor(SkColorSetARGB(180, 200, 200, 200)); // Light grey text

  // 4. Fonts
  headerFont_.setSize(16.0f);
  headerFont_.setEmbolden(true);
  headerFont_.setSubpixel(true);

  bodyFont_.setSize(12.0f);
  bodyFont_.setEmbolden(false);
  bodyFont_.setSubpixel(true);

  labelFont_.setSize(10.0f);
  labelFont_.setSubpixel(true);

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

  // Spectra at Top (150px)
  if (spectraAnalyzer_) {
    spectraAnalyzer_->setBounds(bounds.removeFromTop(150).reduced(5));
  }

  // Wingman takes the rest
  if (wingmanPanel_) {
    wingmanPanel_->setBounds(bounds.reduced(5));
  }
}

#endif // ZENITH_USE_SKIA

} // namespace zenith
