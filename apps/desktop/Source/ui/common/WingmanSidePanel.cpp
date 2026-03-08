/*
  ==============================================================================

    WingmanSidePanel.cpp
    Created: 2025-11-28
    Author:  David Chen + Isabella Moretti

  ==============================================================================
*/

#include "WingmanSidePanel.h"
#include "UndoHistoryPanel.h"
#include "../engine/ZenithLogger.h"
#include "../design-system/ZenithLayout.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../framework/GlassmorphicPanel.h"
#include "../controls/SpectraAnalyzerComponent.h"

#ifdef ZENITH_USE_SKIA
#include <core/SkCanvas.h>
#include <core/SkColor.h>
#include <core/SkFont.h>
#include <core/SkPaint.h>
#include <core/SkRect.h>

#endif

namespace zenith {

#ifdef ZENITH_USE_SKIA

WingmanSidePanel::WingmanSidePanel(CommandAPI &api, Engine &engine, ProjectState &projectState) {
  ZENITH_LOG_UI(zenith::LogLevel::Info, "WingmanSidePanel: Constructor started"); // Initialize WingmanPanel
  ZENITH_LOG_UI(zenith::LogLevel::Info, "WingmanSidePanel: Creating WingmanPanel...");
  wingmanPanel_ = std::make_unique<WingmanPanel>(api, engine);
  ZENITH_LOG_UI(zenith::LogLevel::Info, "WingmanSidePanel: WingmanPanel created. Adding child...");
  addChildComponent(wingmanPanel_.get());
  wingmanPanel_->setVisible(true);

  /*
  // Initialize SpectraAnalyzer
  spectraAnalyzer_ = std::make_unique<SpectraAnalyzerComponent>(engine);
  addChildComponent(spectraAnalyzer_.get());
  spectraAnalyzer_->setVisible(true);

  // Initialize UndoHistoryPanel
  undoHistoryPanel_ = std::make_unique<UndoHistoryPanel>(projectState);
  addChildComponent(undoHistoryPanel_.get());
  undoHistoryPanel_->setVisible(true);
  */

  setSize(300, 600);
  ZENITH_LOG_UI(zenith::LogLevel::Info, "WingmanSidePanel: Constructor complete");
}

WingmanSidePanel::~WingmanSidePanel() {}

void WingmanSidePanel::drawSkia(SkCanvas *canvas) {
  if (canvas == nullptr) return;
  
  auto bounds = getLocalBounds().toFloat();
  SkRect skBounds = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());

  // Glassmorphic background
  GlassmorphicPanel::Options opts;
  opts.style = GlassmorphicPanel::Style::Elevated;
  opts.cornerRadius = 0.0f; // Sharp rectangle for pop-out
  GlassmorphicPanel::drawWithOptions(canvas, skBounds, opts);

  // Left accent border highlight (the "pop out" edge)
  SkPaint accentPaint;
  accentPaint.setAntiAlias(true);
  accentPaint.setStrokeWidth(0.8f);
  accentPaint.setColor(design::withAlpha(design::colors::ACCENT_PRIMARY, 0.4f));
  canvas->drawLine(0.4f, 0.0f, 0.4f, skBounds.height(), accentPaint);
  
  // Draw children (WingmanPanel, etc.)
  drawChildren(canvas);
}

void WingmanSidePanel::resized() {
  if (!wingmanPanel_) return; // Null check only Wingman for now
  // if (!wingmanPanel_ || !spectraAnalyzer_ || !undoHistoryPanel_) return;
  
  auto bounds = getLocalBounds();

  ZenithLayout::begin()
      .withBounds(bounds)
      .withGap(5.0f)
      //.addFixedItem(spectraAnalyzer_.get(), (float)bounds.getWidth(), 150.0f)
      //.addFixedItem(undoHistoryPanel_.get(), (float)bounds.getWidth(), 180.0f)
      .addItem(wingmanPanel_.get())
      .applyColumn();
}

#endif // ZENITH_USE_SKIA

} // namespace zenith
