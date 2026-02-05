/*
  ==============================================================================

    RightSidePanel.cpp
    Created: 2025-11-28
    Author:  David Chen + Isabella Moretti

    Wingman/AI panel container. Positioned on LEFT side as copilot.
    (Named "RightSidePanel" for legacy reasons - should be "SidePanel")

  ==============================================================================
*/

#include "RightSidePanel.h"
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

RightSidePanel::RightSidePanel(CommandAPI &api, Engine &engine, ProjectState &projectState) {
  ZENITH_LOG_UI(zenith::LogLevel::Info, "RightSidePanel: Constructor started"); // Initialize WingmanPanel
  ZENITH_LOG_UI(zenith::LogLevel::Info, "RightSidePanel: Creating WingmanPanel...");
  wingmanPanel_ = std::make_unique<WingmanPanel>(api, engine);
  ZENITH_LOG_UI(zenith::LogLevel::Info, "RightSidePanel: WingmanPanel created. Adding child...");
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
  ZENITH_LOG_UI(zenith::LogLevel::Info, "RightSidePanel: Constructor complete");
}

RightSidePanel::~RightSidePanel() {}

void RightSidePanel::drawSkia(SkCanvas *canvas) {
  if (canvas == nullptr) return;
  
  auto bounds = getLocalBounds().toFloat();
  SkRect skBounds = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());

  // Glassmorphic background
  GlassmorphicPanel::Options opts;
  opts.style = GlassmorphicPanel::Style::Elevated;
  opts.cornerRadius = 0.0f; // Sharp rectangle for pop-out
  GlassmorphicPanel::drawWithOptions(canvas, skBounds, opts);

  // Right accent border highlight (the "pop out" edge)
  // Panel is on the left side, so border is on the right
  SkPaint accentPaint;
  accentPaint.setAntiAlias(true);
  accentPaint.setStrokeWidth(0.8f);
  accentPaint.setColor(design::withAlpha(design::colors::ACCENT_PRIMARY, 0.4f));
  canvas->drawLine(skBounds.width() - 0.4f, 0.0f, skBounds.width() - 0.4f, skBounds.height(), accentPaint);
  
  // Manually draw Skia children (WingmanPanel, etc.)
  // Note: drawChildren() doesn't work with Skia rendering, we need to call drawSkia directly
  if (wingmanPanel_ && wingmanPanel_->isVisible()) {
    canvas->save();
    canvas->translate(wingmanPanel_->getX(), wingmanPanel_->getY());
    canvas->clipRect(SkRect::MakeWH(wingmanPanel_->getWidth(), wingmanPanel_->getHeight()));
    wingmanPanel_->drawSkia(canvas);
    canvas->restore();
  }
}

void RightSidePanel::resized() {
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
