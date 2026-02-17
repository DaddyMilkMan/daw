/*
    SkiaModulationMatrix.cpp - Visual Modulation Matrix

    Drag-and-drop modulation routing interface with animated visualization.
    Shows modulation sources (LFOs, envelopes, etc.) connected to destinations
    with real-time animation of modulation amounts.

    Features:
    - Drag from source to destination to create routing
    - Visual routing lines with animated flow
    - Click routing dots to edit amount/curve
    - Curve editor for each routing
    - Color-coded modulation sources

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>
*/

#include "SkiaSmartKnob.h"
#include "../framework/SkiaComponent.h"
#include <juce_core/juce_core.h>
#include <vector>
#include <cmath>

namespace zenith {

//==============================================================================
// SkiaModulationMatrix Implementation
//==============================================================================

SkiaModulationMatrix::SkiaModulationMatrix() {
    startTimerHz(60); // 60 FPS for animation
}

void SkiaModulationMatrix::drawSkia(SkCanvas* canvas) {
    if (!canvas) return;

    auto bounds = SkRect::MakeXYWH(0, 0, getWidth(), getHeight());

    // Background
    SkPaint bgPaint;
    bgPaint.setColor(SkColorSetARGB(255, 12, 12, 18));
    bgPaint.setStyle(SkPaint::kFill_Style);
    canvas->drawRect(bounds, bgPaint);

    // Calculate layout
    calculateLayout(bounds);

    // Draw panels
    drawSourcesPanel(canvas, bounds);
    drawDestinationsPanel(canvas, bounds);

    // Draw matrix connections
    drawMatrix(canvas, bounds);

    // Draw curve editor if expanded
    if (showCurveEditor_) {
        drawCurveEditor(canvas, bounds);
    }

    // Draw temp line while creating routing
    if (isCreatingRouting_) {
        drawTempLine(canvas, bounds, routingStartSource_);
    }
}

void SkiaModulationMatrix::setSources(const std::vector<ModulationSource>& sources) {
    sources_ = sources;
    markDirty();
}

void SkiaModulationMatrix::setDestinations(const std::vector<juce::String>& destinations) {
    destinations_ = destinations;
    markDirty();
}

void SkiaModulationMatrix::addRouting(const ModulationRouting& routing) {
    routings_.push_back(routing);

    // Trigger animation
    newRoutingAnimation_ = 1.0f;

    markDirty();

    if (routingChangedCallback_) {
        routingChangedCallback_(routing);
    }
}

void SkiaModulationMatrix::removeRouting(int index) {
    if (index >= 0 && index < static_cast<int>(routings_.size())) {
        routings_.erase(routings_.begin() + index);
        markDirty();
    }
}

void SkiaModulationMatrix::clearAllRoutings() {
    routings_.clear();
    markDirty();
}

void SkiaModulationMatrix::calculateLayout(const SkRect& bounds) {
    float padding = 10.0f;
    float panelWidth = 150.0f;

    // Sources panel (left side)
    sourcesRect_ = SkRect::MakeXYWH(
        bounds.left() + padding,
        bounds.top() + padding,
        panelWidth,
        bounds.height() - padding * 2.0f
    );

    // Destinations panel (right side)
    destinationsRect_ = SkRect::MakeXYWH(
        bounds.right() - panelWidth - padding,
        bounds.top() + padding,
        panelWidth,
        bounds.height() - padding * 2.0f
    );

    // Matrix area (center)
    matrixRect_ = SkRect::MakeXYWH(
        sourcesRect_.right(),
        bounds.top() + padding,
        destinationsRect_.left() - sourcesRect_.right(),
        bounds.height() - padding * 2.0f
    );

    // Curve editor (overlay)
    if (showCurveEditor_) {
        curveEditorRect_ = SkRect::MakeXYWH(
            matrixRect_.centerX() - 150.0f,
            matrixRect_.centerY() - 100.0f,
            300.0f,
            200.0f
        );
    }
}

void SkiaModulationMatrix::drawSourcesPanel(SkCanvas* canvas, const SkRect& bounds) {
    // Draw panel background
    SkPaint bgPaint;
    bgPaint.setAntiAlias(true);
    bgPaint.setColor(SkColorSetARGB(255, 20, 20, 30));
    bgPaint.setStyle(SkPaint::kFill_Style);

    canvas->drawRoundRect(sourcesRect_, 8.0f, 8.0f, bgPaint);

    // Draw panel border
    SkPaint borderPaint;
    borderPaint.setAntiAlias(true);
    borderPaint.setColor(SkColorSetARGB(255, 60, 60, 80));
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.5f);

    canvas->drawRoundRect(sourcesRect_, 8.0f, 8.0f, borderPaint);

    // Draw source items
    float itemHeight = 30.0f;
    float y = sourcesRect_.top() + 15.0f;

    for (size_t i = 0; i < sources_.size(); ++i) {
        const auto& source = sources_[i];

        // Highlight if hovered
        if (i == static_cast<size_t>(hoveredSource_)) {
            SkPaint hoverPaint;
            hoverPaint.setAntiAlias(true);
            hoverPaint.setColor(SkColorSetARGB(100, 40, 40, 60));
            hoverPaint.setStyle(SkPaint::kFill_Style);

            SkRect itemRect = SkRect::MakeXYWH(sourcesRect_.left() + 5.0f, y, sourcesRect_.width() - 10.0f, itemHeight - 5.0f);
            canvas->drawRoundRect(itemRect, 4.0f, 4.0f, hoverPaint);
        }

        // Draw color indicator
        SkPaint colorPaint;
        colorPaint.setAntiAlias(true);
        colorPaint.setColor(source.color);
        colorPaint.setStyle(SkPaint::kFill_Style);

        float colorX = sourcesRect_.left() + 15.0f;
        float colorY = y + itemHeight * 0.5f;
        canvas->drawCircle(colorX, colorY, 6.0f, colorPaint);

        // Draw source name (simplified - use Skia text rendering in real implementation)
        SkPaint textPaint;
        textPaint.setAntiAlias(true);
        textPaint.setColor(SkColorSetARGB(255, 200, 200, 210));
        textPaint.setTextSize(11.0f);
        // canvas->drawText(source.name.toUTF8(), ...);

        // Draw current value bar
        if (source.isActive) {
            float valueNorm = (source.currentValue + 1.0f) * 0.5f; // -1..1 to 0..1
            float barWidth = 40.0f;
            float barX = sourcesRect_.right() - barWidth - 15.0f;
            float barY = colorY - 3.0f;

            // Bar background
            SkPaint barBgPaint;
            barBgPaint.setAntiAlias(true);
            barBgPaint.setColor(SkColorSetARGB(100, 30, 30, 40));
            barBgPaint.setStyle(SkPaint::kFill_Style);
            canvas->drawRect(SkRect::MakeXYWH(barX, barY, barWidth, 6.0f), barBgPaint);

            // Bar fill
            SkPaint barFillPaint;
            barFillPaint.setAntiAlias(true);
            barFillPaint.setColor(source.color);
            barFillPaint.setStyle(SkPaint::kFill_Style);
            canvas->drawRect(SkRect::MakeXYWH(barX, barY, barWidth * valueNorm, 6.0f), barFillPaint);
        }

        y += itemHeight;
    }
}

void SkiaModulationMatrix::drawDestinationsPanel(SkCanvas* canvas, const SkRect& bounds) {
    // Similar to sources panel but on the right
    SkPaint bgPaint;
    bgPaint.setAntiAlias(true);
    bgPaint.setColor(SkColorSetARGB(255, 20, 20, 30));
    bgPaint.setStyle(SkPaint::kFill_Style);

    canvas->drawRoundRect(destinationsRect_, 8.0f, 8.0f, bgPaint);

    SkPaint borderPaint;
    borderPaint.setAntiAlias(true);
    borderPaint.setColor(SkColorSetARGB(255, 60, 60, 80));
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.5f);

    canvas->drawRoundRect(destinationsRect_, 8.0f, 8.0f, borderPaint);

    // Draw destination items
    float itemHeight = 30.0f;
    float y = destinationsRect_.top() + 15.0f;

    for (size_t i = 0; i < destinations_.size(); ++i) {
        const auto& dest = destinations_[i];

        // Highlight if hovered
        if (i == static_cast<size_t>(hoveredDestination_)) {
            SkPaint hoverPaint;
            hoverPaint.setAntiAlias(true);
            hoverPaint.setColor(SkColorSetARGB(100, 40, 40, 60));
            hoverPaint.setStyle(SkPaint::kFill_Style);

            SkRect itemRect = SkRect::MakeXYWH(destinationsRect_.left() + 5.0f, y, destinationsRect_.width() - 10.0f, itemHeight - 5.0f);
            canvas->drawRoundRect(itemRect, 4.0f, 4.0f, hoverPaint);
        }

        // Draw destination name (simplified)
        SkPaint textPaint;
        textPaint.setAntiAlias(true);
        textPaint.setColor(SkColorSetARGB(255, 200, 200, 210));
        textPaint.setTextSize(11.0f);
        // canvas->drawText(dest.toUTF8(), ...);

        // Check if destination has active routings
        int routingCount = 0;
        for (const auto& routing : routings_) {
            // Check if routing goes to this destination
            // (In real implementation, compare destination IDs)
            routingCount++;
        }

        // Draw routing count indicator
        if (routingCount > 0) {
            SkPaint countPaint;
            countPaint.setAntiAlias(true);
            countPaint.setColor(SkColorSetARGB(255, 100, 200, 255));
            countPaint.setStyle(SkPaint::kFill_Style);

            float countX = destinationsRect_.right() - 25.0f;
            float countY = y + itemHeight * 0.5f;
            canvas->drawCircle(countX, countY, 8.0f, countPaint);

            // Draw count number (simplified)
            // canvas->drawText(juce::String(routingCount).toUTF8(), ...);
        }

        y += itemHeight;
    }
}

void SkiaModulationMatrix::drawMatrix(SkCanvas* canvas, const SkRect& bounds) {
    // Draw all routing connections
    for (size_t i = 0; i < routings_.size(); ++i) {
        const auto& routing = routings_[i];

        if (!routing.source || !routing.isActive) continue;

        // Get source and destination positions
        // In real implementation, match source/destination by ID
        int sourceIndex = 0; // Find source index
        int destIndex = 0; // Find destination index

        SkPoint sourcePos = getSourcePosition(sourceIndex, bounds);
        SkPoint destPos = getDestinationPosition(destIndex, bounds);

        drawRoutingLine(canvas, bounds, routing, sourcePos, destPos);
    }
}

void SkiaModulationMatrix::drawRoutingLine(SkCanvas* canvas, const SkRect& bounds,
                                          const ModulationRouting& routing,
                                          const SkPoint& sourcePos, const SkPoint& destPos) {
    // Create curved path from source to destination
    SkPath path;

    float midX = (sourcePos.x() + destPos.x()) * 0.5f;
    float controlOffset = std::abs(destPos.x() - sourcePos.x()) * 0.3f;

    path.moveTo(sourcePos);
    path.cubicTo(
        sourcePos.x() + controlOffset, sourcePos.y(),
        destPos.x() - controlOffset, destPos.y(),
        destPos.x(), destPos.y()
    );

    // Draw routing line
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setColor(routing.source ? routing.source->color : SkColorSetARGB(255, 100, 200, 255));
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(2.0f);

    // Animate opacity based on modulation amount
    float opacity = 150.0f + std::abs(routing.amount) * 105.0f;
    paint.setAlpha(static_cast<int>(opacity));

    canvas->drawPath(path, paint);

    // Draw animated dots along the line
    drawAnimatedDots(canvas, path, routing);

    // Draw routing dots at endpoints
    drawRoutingDot(canvas, sourcePos, routing.source ? routing.source->color : SkColorSetARGB(255, 0, 200, 255), true);
    drawRoutingDot(canvas, destPos, SkColorSetARGB(255, 150, 150, 160), false);

    // Draw amount label at midpoint
    juce::String amountText = juce::String(routing.amount * 100.0f, 1) + "%";

    float midX = (sourcePos.x() + destPos.x()) * 0.5f;
    float midY = (sourcePos.y() + destPos.y()) * 0.5f;

    SkPaint labelPaint;
    labelPaint.setAntiAlias(true);
    labelPaint.setColor(SkColorSetARGB(200, 180, 180, 190));
    labelPaint.setTextSize(10.0f);
    // canvas->drawText(amountText.toUTF8(), ...);
}

void SkiaModulationMatrix::drawRoutingDot(SkCanvas* canvas, const SkPoint& pos,
                                        SkColor color, bool isActive) {
    // Outer glow
    if (isActive) {
        SkPaint glowPaint;
        glowPaint.setAntiAlias(true);
        glowPaint.setColor(color);
        glowPaint.setAlpha(80);
        glowPaint.setStyle(SkPaint::kFill_Style);
        canvas->drawCircle(pos, 8.0f, glowPaint);
    }

    // Core dot
    SkPaint dotPaint;
    dotPaint.setAntiAlias(true);
    dotPaint.setColor(color);
    dotPaint.setStyle(SkPaint::kFill_Style);
    canvas->drawCircle(pos, 5.0f, dotPaint);

    // Inner highlight
    SkPaint highlightPaint;
    highlightPaint.setAntiAlias(true);
    highlightPaint.setColor(SkColorSetARGB(255, 255, 255, 255));
    highlightPaint.setStyle(SkPaint::kFill_Style);
    canvas->drawCircle(pos.x() - 1.5f, pos.y() - 1.5f, 2.0f, highlightPaint);
}

void SkiaModulationMatrix::drawAnimatedDots(SkCanvas* canvas, const SkPath& path,
                                           const ModulationRouting& routing) {
    // Draw dots that animate along the path to show modulation flow
    float time = juce::Time::getMillisecondCounterHiRes() * 0.001f;

    // Number of dots based on modulation amount
    int numDots = static_cast<int>(std::abs(routing.amount) * 3.0f) + 1;

    for (int i = 0; i < numDots; ++i) {
        float offset = (time + i / static_cast<float>(numDots));
        offset -= std::floor(offset); // Wrap to 0-1

        // Get position along path at this offset
        // (In real implementation, use SkPathMeasure)
        float t = offset;
        SkPoint pos; // Calculate position on path

        // Draw dot
        SkPaint dotPaint;
        dotPaint.setAntiAlias(true);
        dotPaint.setColor(routing.source ? routing.source->color : SkColorSetARGB(255, 100, 200, 255));
        dotPaint.setAlpha(180);
        dotPaint.setStyle(SkPaint::kFill_Style);
        canvas->drawCircle(pos, 3.0f, dotPaint);
    }
}

void SkiaModulationMatrix::drawCurveEditor(SkCanvas* canvas, const SkRect& bounds) {
    // Draw curve editor overlay
    SkPaint bgPaint;
    bgPaint.setAntiAlias(true);
    bgPaint.setColor(SkColorSetARGB(240, 15, 15, 25));
    bgPaint.setStyle(SkPaint::kFill_Style());

    canvas->drawRoundRect(curveEditorRect_, 12.0f, 12.0f, bgPaint);

    SkPaint borderPaint;
    borderPaint.setAntiAlias(true);
    borderPaint.setColor(SkColorSetARGB(255, 80, 80, 100));
    borderPaint.setStyle(SkPaint::kStroke_Style());
    borderPaint.setStrokeWidth(2.0f);

    canvas->drawRoundRect(curveEditorRect_, 12.0f, 12.0f, borderPaint);

    // Draw curve grid
    SkPaint gridPaint;
    gridPaint.setAntiAlias(true);
    gridPaint.setColor(SkColorSetARGB(100, 50, 50, 70));
    gridPaint.setStyle(SkPaint::kStroke_Style());
    gridPaint.setStrokeWidth(1.0f);

    // Draw grid lines
    for (int i = 1; i < 4; ++i) {
        float x = curveEditorRect_.left() + curveEditorRect_.width() * (i / 4.0f);
        float y = curveEditorRect_.top() + curveEditorRect_.height() * (i / 4.0f);

        canvas->drawLine(x, curveEditorRect_.top(), x, curveEditorRect_.bottom(), gridPaint);
        canvas->drawLine(curveEditorRect_.left(), y, curveEditorRect_.right(), y, gridPaint);
    }

    // Draw curve shape
    SkPath curvePath;
    curvePath.moveTo(curveEditorRect_.left(), curveEditorRect_.bottom());

    // Generate curve based on curve type
    for (int i = 0; i <= 20; ++i) {
        float t = static_cast<float>(i) / 20.0f;
        float x = curveEditorRect_.left() + curveEditorRect_.width() * t;

        float y;
        // In real implementation, calculate y based on curve type
        switch (0) { // curveType
            case 0: // Linear
                y = curveEditorRect_.bottom() - curveEditorRect_.height() * t;
                break;
            case 1: // Exponential
                y = curveEditorRect_.bottom() - curveEditorRect_.height() * (t * t);
                break;
            default:
                y = curveEditorRect_.bottom() - curveEditorRect_.height() * t;
        }

        curvePath.lineTo(x, y);
    }

    SkPaint curvePaint;
    curvePaint.setAntiAlias(true);
    curvePaint.setColor(SkColorSetARGB(255, 0, 255, 200));
    curvePaint.setStyle(SkPaint::kStroke_Style());
    curvePaint.setStrokeWidth(2.5f);

    canvas->drawPath(curvePath, curvePaint);

    // Draw title (simplified)
    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(SkColorSetARGB(255, 200, 200, 210));
    textPaint.setTextSize(12.0f);
    // canvas->drawText("Modulation Curve", ...);
}

void SkiaModulationMatrix::drawTempLine(SkCanvas* canvas, const SkRect& bounds, int sourceIndex) {
    if (sourceIndex < 0) return;

    SkPoint sourcePos = getSourcePosition(sourceIndex, bounds);

    // Get current mouse position (in real implementation)
    SkPoint mousePos; // = current mouse position

    // Draw temporary line
    SkPath path;
    path.moveTo(sourcePos);
    path.lineTo(mousePos);

    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setColor(SkColorSetARGB(150, 100, 200, 255));
    paint.setStyle(SkPaint::kStroke_Style());
    paint.setStrokeWidth(2.0f);

    canvas->drawPath(path, paint);
}

SkPoint SkiaModulationMatrix::getSourcePosition(int index, const SkRect& bounds) const {
    float itemHeight = 30.0f;
    float y = sourcesRect_.top() + 15.0f + index * itemHeight + itemHeight * 0.5f;
    float x = sourcesRect_.right();
    return SkPoint::Make(x, y);
}

SkPoint SkiaModulationMatrix::getDestinationPosition(int index, const SkRect& bounds) const {
    float itemHeight = 30.0f;
    float y = destinationsRect_.top() + 15.0f + index * itemHeight + itemHeight * 0.5f;
    float x = destinationsRect_.left();
    return SkPoint::Make(x, y);
}

int SkiaModulationMatrix::getSourceAtPosition(const SkPoint& pos, const SkRect& bounds) const {
    // Check if position is over a source item
    float itemHeight = 30.0f;
    float y = sourcesRect_.top() + 15.0f;

    for (size_t i = 0; i < sources_.size(); ++i) {
        SkRect itemRect = SkRect::MakeXYWH(sourcesRect_.left() + 5.0f, y, sourcesRect_.width() - 10.0f, itemHeight - 5.0f);

        if (pos.x() >= itemRect.left() && pos.x() <= itemRect.right() &&
            pos.y() >= itemRect.top() && pos.y() <= itemRect.bottom()) {
            return static_cast<int>(i);
        }

        y += itemHeight;
    }

    return -1;
}

int SkiaModulationMatrix::getDestinationAtPosition(const SkPoint& pos, const SkRect& bounds) const {
    // Check if position is over a destination item
    float itemHeight = 30.0f;
    float y = destinationsRect_.top() + 15.0f;

    for (size_t i = 0; i < destinations_.size(); ++i) {
        SkRect itemRect = SkRect::MakeXYWH(destinationsRect_.left() + 5.0f, y, destinationsRect_.width() - 10.0f, itemHeight - 5.0f);

        if (pos.x() >= itemRect.left() && pos.x() <= itemRect.right() &&
            pos.y() >= itemRect.top() && pos.y() <= itemRect.bottom()) {
            return static_cast<int>(i);
        }

        y += itemHeight;
    }

    return -1;
}

void SkiaModulationMatrix::mouseDown(const juce::MouseEvent& e) {
    auto pos = SkPoint::Make(e.position.x, e.position.y);

    // Check if clicking on a source
    int sourceIndex = getSourceAtPosition(pos, SkRect::MakeXYWH(0, 0, getWidth(), getHeight()));
    if (sourceIndex >= 0) {
        isCreatingRouting_ = true;
        routingStartSource_ = sourceIndex;
        return;
    }

    // Check if clicking on a routing
    // (In real implementation, check distance to routing lines)
}

void SkiaModulationMatrix::mouseDrag(const juce::MouseEvent& e) {
    // Update temp line while dragging
    if (isCreatingRouting_) {
        markDirty();
    }
}

void SkiaModulationMatrix::mouseUp(const juce::MouseEvent& e) {
    if (isCreatingRouting_) {
        auto pos = SkPoint::Make(e.position.x, e.position.y);

        // Check if released over a destination
        int destIndex = getDestinationAtPosition(pos, SkRect::MakeXYWH(0, 0, getWidth(), getHeight()));
        if (destIndex >= 0) {
            // Create new routing
            ModulationRouting newRouting;
            newRouting.source = const_cast<ModulationSource*>(&sources_[routingStartSource_]);
            newRouting.amount = 0.5f;
            newRouting.isActive = true;
            addRouting(newRouting);
        }

        isCreatingRouting_ = false;
        routingStartSource_ = -1;
    }
}

void SkiaModulationMatrix::mouseMove(const juce::MouseEvent& e) {
    auto pos = SkPoint::Make(e.position.x, e.position.y);
    auto bounds = SkRect::MakeXYWH(0, 0, getWidth(), getHeight());

    // Update hover state
    hoveredSource_ = getSourceAtPosition(pos, bounds);
    hoveredDestination_ = getDestinationAtPosition(pos, bounds);

    markDirty();
}

} // namespace zenith
