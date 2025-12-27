/*
  ==============================================================================

    UndoHistoryPanel.cpp
    Created: 2025-12-25
    Author:  Zenith DAW Team

    Implementation of the visual undo/redo history panel.

  ==============================================================================
*/

#include "UndoHistoryPanel.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../framework/GlassmorphicPanel.h"
#include "../framework/NeonGlow.h"

#ifdef ZENITH_USE_SKIA

#include <core/SkRRect.h>
#include <effects/SkGradientShader.h>

namespace zenith {

UndoHistoryPanel::UndoHistoryPanel(ProjectState& projectState)
    : projectState_(projectState) {
    
    setSize(280, 200);
    updateCachedPaints();
    startTimerHz(10);  // Slower update rate - history doesn't change that often
}

UndoHistoryPanel::~UndoHistoryPanel() {
    stopTimer();
}

void UndoHistoryPanel::timerCallback() {
    rebuildHistory();
    repaint();
}

void UndoHistoryPanel::resized() {
    updateCachedPaints();
}

void UndoHistoryPanel::updateCachedPaints() {
    using namespace design;
    
    // Background
    bgPaint_.setAntiAlias(true);
    bgPaint_.setColor(colors::BG_DARKER);
    bgPaint_.setStyle(SkPaint::kFill_Style);
    
    // Header
    headerPaint_.setAntiAlias(true);
    headerPaint_.setColor(colors::BG_DARK);
    headerPaint_.setStyle(SkPaint::kFill_Style);
    
    // Text
    textPaint_.setAntiAlias(true);
    textPaint_.setColor(colors::TEXT_PRIMARY);
    textPaint_.setStyle(SkPaint::kFill_Style);
    
    dimTextPaint_.setAntiAlias(true);
    dimTextPaint_.setColor(colors::TEXT_TERTIARY);
    dimTextPaint_.setStyle(SkPaint::kFill_Style);
    
    // Current position highlight
    highlightPaint_.setAntiAlias(true);
    highlightPaint_.setColor(withAlpha(colors::CYAN, 0.25f));
    highlightPaint_.setStyle(SkPaint::kFill_Style);
    
    // Hover
    hoverPaint_.setAntiAlias(true);
    hoverPaint_.setColor(colors::GLASS_HOVER);
    hoverPaint_.setStyle(SkPaint::kFill_Style);
    
    // Border
    borderPaint_.setAntiAlias(true);
    borderPaint_.setColor(colors::BORDER_SUBTLE);
    borderPaint_.setStyle(SkPaint::kStroke_Style);
    borderPaint_.setStrokeWidth(1.0f);
    
    // Buttons
    buttonPaint_.setAntiAlias(true);
    buttonPaint_.setColor(colors::TEXT_SECONDARY);
    buttonPaint_.setStyle(SkPaint::kStroke_Style);
    buttonPaint_.setStrokeWidth(1.5f);
    
    // Fonts
    headerFont_ = typography::getSkFont(14.0f, FontWeight::SemiBold);
    itemFont_ = typography::getSkFont(12.0f, FontWeight::Regular);
    shortcutFont_ = typography::getMonoFont(10.0f, FontWeight::Regular);
}

void UndoHistoryPanel::rebuildHistory() {
    historyItems_.clear();
    
    auto& undoManager = projectState_.getUndoManager();
    
    // Get redo description (future action - if available)
    juce::String redoDesc = undoManager.getRedoDescription();
    if (redoDesc.isNotEmpty()) {
        HistoryItem item;
        item.description = redoDesc;
        item.isUndoable = false;
        item.index = 0;
        historyItems_.push_back(item);
    }
    
    // Current position marker
    currentIndex_ = (int)historyItems_.size();
    
    // Get undo description (past action - if available)
    juce::String undoDesc = undoManager.getUndoDescription();
    if (undoDesc.isNotEmpty()) {
        HistoryItem item;
        item.description = undoDesc;
        item.isUndoable = true;
        item.index = currentIndex_;
        historyItems_.push_back(item);
    }
    
    // Note: JUCE's UndoManager only exposes the immediate undo/redo descriptions,
    // not the full history stack. For a complete history, you'd need to track 
    // action names yourself when transactions are created.
}

void UndoHistoryPanel::drawSkia(SkCanvas* canvas) {
    if (!canvas) return;
    
    auto bounds = getLocalBounds().toFloat();
    SkRect skBounds = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());
    
    // Draw glassmorphic background
    GlassmorphicPanel::draw(canvas, skBounds, GlassmorphicPanel::Style::Subtle);
    
    // Header
    drawHeader(canvas, bounds.getWidth());
    
    // History list area
    SkRect listBounds = SkRect::MakeXYWH(
        0, HEADER_HEIGHT,
        bounds.getWidth(),
        bounds.getHeight() - HEADER_HEIGHT
    );
    drawHistoryList(canvas, listBounds);
}

void UndoHistoryPanel::drawHeader(SkCanvas* canvas, float width) {
    using namespace design;
    
    // Header background
    SkRect headerRect = SkRect::MakeWH(width, HEADER_HEIGHT);
    canvas->drawRect(headerRect, headerPaint_);
    
    // Title
    canvas->drawString("History", 12.0f, 24.0f, headerFont_, textPaint_);
    
    // Undo button
    float buttonY = (HEADER_HEIGHT - BUTTON_SIZE) / 2.0f;
    float undoX = width - BUTTON_SIZE * 2 - 16.0f;
    float redoX = width - BUTTON_SIZE - 8.0f;
    
    undoButtonBounds_ = juce::Rectangle<float>(undoX, buttonY, BUTTON_SIZE, BUTTON_SIZE);
    redoButtonBounds_ = juce::Rectangle<float>(redoX, buttonY, BUTTON_SIZE, BUTTON_SIZE);
    
    // Draw undo button (curved arrow left)
    {
        SkPaint paint = buttonPaint_;
        if (undoHovered_ && projectState_.canUndo()) {
            paint.setColor(colors::CYAN);
        } else if (!projectState_.canUndo()) {
            paint.setColor(colors::TEXT_DISABLED);
        }
        
        // Simple undo arrow
        SkPath path;
        float cx = undoX + BUTTON_SIZE / 2.0f;
        float cy = buttonY + BUTTON_SIZE / 2.0f;
        path.moveTo(cx + 6, cy - 4);
        path.lineTo(cx - 2, cy - 4);
        path.lineTo(cx - 2, cy + 4);
        path.moveTo(cx - 6, cy - 4);
        path.lineTo(cx - 2, cy - 4);
        path.lineTo(cx - 2, cy);
        canvas->drawPath(path, paint);
    }
    
    // Draw redo button (curved arrow right)
    {
        SkPaint paint = buttonPaint_;
        if (redoHovered_ && projectState_.canRedo()) {
            paint.setColor(colors::CYAN);
        } else if (!projectState_.canRedo()) {
            paint.setColor(colors::TEXT_DISABLED);
        }
        
        // Simple redo arrow
        SkPath path;
        float cx = redoX + BUTTON_SIZE / 2.0f;
        float cy = buttonY + BUTTON_SIZE / 2.0f;
        path.moveTo(cx - 6, cy - 4);
        path.lineTo(cx + 2, cy - 4);
        path.lineTo(cx + 2, cy + 4);
        path.moveTo(cx + 6, cy - 4);
        path.lineTo(cx + 2, cy - 4);
        path.lineTo(cx + 2, cy);
        canvas->drawPath(path, paint);
    }
    
    // Bottom border
    SkPaint linePaint;
    linePaint.setColor(colors::BORDER_SUBTLE);
    linePaint.setStrokeWidth(1.0f);
    canvas->drawLine(0, HEADER_HEIGHT, width, HEADER_HEIGHT, linePaint);
}

void UndoHistoryPanel::drawHistoryList(SkCanvas* canvas, const SkRect& bounds) {
    using namespace design;
    
    canvas->save();
    canvas->clipRect(bounds);
    canvas->translate(0, -scrollOffset_);
    
    float y = bounds.top();
    
    if (historyItems_.empty()) {
        // Empty state
        canvas->drawString("No history yet", 
                          bounds.width() / 2.0f - 40.0f, 
                          y + 40.0f, 
                          itemFont_, dimTextPaint_);
        
        // Show keyboard shortcuts
        SkPaint shortcutPaint = dimTextPaint_;
        canvas->drawString("Ctrl+Z to undo", 
                          bounds.width() / 2.0f - 35.0f, 
                          y + 70.0f, 
                          shortcutFont_, shortcutPaint);
        canvas->drawString("Ctrl+Shift+Z to redo", 
                          bounds.width() / 2.0f - 55.0f, 
                          y + 90.0f, 
                          shortcutFont_, shortcutPaint);
    } else {
        // Draw current position marker
        SkRect currentRect = SkRect::MakeXYWH(0, y, bounds.width(), ITEM_HEIGHT);
        
        // Draw "Now" marker
        canvas->drawRect(currentRect, highlightPaint_);
        
        SkPaint nowPaint = textPaint_;
        nowPaint.setColor(colors::CYAN);
        canvas->drawString("• Current State", 12.0f, y + 18.0f, itemFont_, nowPaint);
        y += ITEM_HEIGHT;
        
        // Draw history items
        for (size_t i = 0; i < historyItems_.size(); ++i) {
            const auto& item = historyItems_[i];
            bool isHovered = (hoveredIndex_ == static_cast<int>(i));
            drawHistoryItem(canvas, item, y, bounds.width(), isHovered);
            y += ITEM_HEIGHT;
        }
    }
    
    canvas->restore();
}

void UndoHistoryPanel::drawHistoryItem(SkCanvas* canvas, const HistoryItem& item,
                                       float y, float width, bool isHovered) {
    using namespace design;
    
    SkRect itemRect = SkRect::MakeXYWH(0, y, width, ITEM_HEIGHT);
    
    // Hover background
    if (isHovered) {
        canvas->drawRect(itemRect, hoverPaint_);
    }
    
    // Icon indicator
    SkPaint iconPaint = dimTextPaint_;
    if (item.isUndoable) {
        iconPaint.setColor(colors::TEXT_SECONDARY);
        canvas->drawString("↺", 12.0f, y + 18.0f, itemFont_, iconPaint);
    } else {
        iconPaint.setColor(colors::TEXT_TERTIARY);
        canvas->drawString("↻", 12.0f, y + 18.0f, itemFont_, iconPaint);
    }
    
    // Description text
    SkPaint textPaintLocal = item.isUndoable ? textPaint_ : dimTextPaint_;
    
    // Truncate long descriptions
    juce::String desc = item.description;
    if (desc.length() > 30) {
        desc = desc.substring(0, 27) + "...";
    }
    
    canvas->drawString(desc.toStdString().c_str(), 28.0f, y + 18.0f, 
                      itemFont_, textPaintLocal);
}

int UndoHistoryPanel::getItemAtY(float y) const {
    if (y < HEADER_HEIGHT) return -1;
    
    float adjustedY = y - HEADER_HEIGHT + scrollOffset_;
    
    // First item is "Current State"
    if (adjustedY < ITEM_HEIGHT) return -1;  // Current state, not clickable
    
    adjustedY -= ITEM_HEIGHT;
    int index = static_cast<int>(adjustedY / ITEM_HEIGHT);
    
    if (index >= 0 && index < static_cast<int>(historyItems_.size())) {
        return index;
    }
    return -1;
}

void UndoHistoryPanel::mouseDown(const juce::MouseEvent& e) {
    auto pos = e.position;
    
    // Check undo button
    if (undoButtonBounds_.contains(pos.x, pos.y) && projectState_.canUndo()) {
        projectState_.undo();
        return;
    }
    
    // Check redo button
    if (redoButtonBounds_.contains(pos.x, pos.y) && projectState_.canRedo()) {
        projectState_.redo();
        return;
    }
    
    // Check history item click
    int clickedIndex = getItemAtY(pos.y);
    if (clickedIndex >= 0) {
        jumpToHistoryIndex(clickedIndex);
    }
}

void UndoHistoryPanel::mouseMove(const juce::MouseEvent& e) {
    auto pos = e.position;
    
    undoHovered_ = undoButtonBounds_.contains(pos.x, pos.y);
    redoHovered_ = redoButtonBounds_.contains(pos.x, pos.y);
    
    int newHovered = getItemAtY(pos.y);
    if (newHovered != hoveredIndex_) {
        hoveredIndex_ = newHovered;
        repaint();
    }
}

void UndoHistoryPanel::mouseExit(const juce::MouseEvent& e) {
    juce::ignoreUnused(e);
    hoveredIndex_ = -1;
    undoHovered_ = false;
    redoHovered_ = false;
    repaint();
}

void UndoHistoryPanel::mouseWheelMove(const juce::MouseEvent& e,
                                       const juce::MouseWheelDetails& wheel) {
    juce::ignoreUnused(e);
    
    float totalHeight = ITEM_HEIGHT * (historyItems_.size() + 1);
    float visibleHeight = getHeight() - HEADER_HEIGHT;
    float maxScroll = std::max(0.0f, totalHeight - visibleHeight);
    
    scrollOffset_ -= wheel.deltaY * 40.0f;
    scrollOffset_ = juce::jlimit(0.0f, maxScroll, scrollOffset_);
    
    repaint();
}

void UndoHistoryPanel::jumpToHistoryIndex(int index) {
    if (index < 0 || index >= static_cast<int>(historyItems_.size())) return;
    
    const auto& item = historyItems_[index];
    
    if (item.isUndoable) {
        // This is in the undo stack - perform undo
        projectState_.undo();
    } else {
        // This is in the redo stack - perform redo
        projectState_.redo();
    }
    
    // Rebuild to reflect new state
    rebuildHistory();
    repaint();
}

} // namespace zenith

#endif // ZENITH_USE_SKIA
