/*
    SkiaSmartKnob.cpp - Smart Knob with Modulation Visualization

    Renders a rotary knob with animated modulation rings showing
    real-time modulation sources affecting the parameter.

    Features:
    - Animated modulation rings (60fps)
    - Multiple modulation sources with different colors
    - Value tooltip on hover
    - Mini display for current modulated value
    - Smooth parameter transitions

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>
*/

#include "SkiaSmartKnob.h"
#include "../framework/SkiaComponent.h"
#include <cmath>
#include <algorithm>

namespace zenith {

//==============================================================================
// SkiaSmartKnob Implementation
//==============================================================================

SkiaSmartKnob::SkiaSmartKnob() {
    // Enable animation for smooth modulation visualization
    startTimerHz(60); // 60 FPS animation
}

void SkiaSmartKnob::drawSkia(SkCanvas* canvas) {
    if (!canvas) return;

    auto bounds = SkRect::MakeXYWH(0.0f, 0.0f, (float) getWidth(), (float) getHeight());

    // Draw base knob (inherited from SkiaRotaryKnob)
    SkiaRotaryKnob::drawSkia(canvas);

    // Draw modulation rings on top
    if (showModulationRing_ && !modulationRoutings_.empty()) {
        drawModulationRings(canvas, bounds);
    }

    // Draw value tooltip if hovering
    if (showValueTooltip_ && isMouseOver()) {
        drawValueTooltip(canvas, bounds);
    }

    // Draw mini display if enabled
    if (showMiniDisplay_) {
        drawMiniDisplay(canvas, bounds);
    }
}

void SkiaSmartKnob::addModulationSource(ModulationSource* source) {
    if (source && !source->id.isEmpty()) {
        // Check if already exists
        for (auto* s : modulationSources_) {
            if (s && s->id == source->id) {
                return; // Already added
            }
        }
        modulationSources_.push_back(source);
        markDirty();
    }
}

void SkiaSmartKnob::removeModulationSource(const juce::String& sourceId) {
    modulationSources_.erase(
        std::remove_if(modulationSources_.begin(), modulationSources_.end(),
            [&sourceId](ModulationSource* s) { return s && s->id == sourceId; }),
        modulationSources_.end());
    markDirty();
}

void SkiaSmartKnob::clearModulationSources() {
    modulationSources_.clear();
    markDirty();
}

void SkiaSmartKnob::setModulationRouting(const ModulationRouting& routing) {
    modulationRoutings_.push_back(routing);
    markDirty();
}

void SkiaSmartKnob::removeModulationRouting(int index) {
    if (index >= 0 && index < static_cast<int>(modulationRoutings_.size())) {
        modulationRoutings_.erase(modulationRoutings_.begin() + index);
        markDirty();
    }
}

void SkiaSmartKnob::clearModulationRoutings() {
    modulationRoutings_.clear();
    markDirty();
}

float SkiaSmartKnob::getModulatedValue() const {
    float baseValue = getValue();
    float totalModulation = calculateTotalModulation();
    return juce::jlimit(minValue_, maxValue_, baseValue + totalModulation);
}

void SkiaSmartKnob::drawModulationRings(SkCanvas* canvas, const SkRect& bounds) {
    if (modulationRoutings_.empty()) return;

    float centerX = bounds.centerX();
    float centerY = bounds.centerY();
    float radius = juce::jmin(bounds.width(), bounds.height()) * 0.35f;

    // Draw each modulation routing as a colored ring
    for (size_t i = 0; i < modulationRoutings_.size(); ++i) {
        const auto& routing = modulationRoutings_[i];

        if (!routing.isActive || !routing.source) continue;

        // Calculate modulation amount as visual ring
        float modAmount = std::abs(routing.amount);
        float normalizedAmount = juce::jlimit(0.0f, 1.0f, modAmount);

        // Ring radius increases with index (concentric rings)
        float ringRadius = radius + 10.0f + (i * modulationRingWidth_ * 2.0f);

        // Get source color
        SkColor modColor = routing.source->color;

        // Draw modulation arc
        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setColor(modColor);
        paint.setAlpha(static_cast<int>(255 * modulationOpacity_));
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setStrokeWidth(modulationRingWidth_);

        // Calculate arc angles based on modulation amount
        float startAngle = arcStartAngle_;
        float sweepAngle = (arcEndAngle_ - arcStartAngle_) * normalizedAmount;

        if (routing.isInverted) {
            // Draw from end going backwards
            startAngle = arcEndAngle_ - sweepAngle;
        }

        SkRect arcRect = SkRect::MakeLTRB(
            centerX - ringRadius,
            centerY - ringRadius,
            centerX + ringRadius,
            centerY + ringRadius
        );

        canvas->drawArc(arcRect, startAngle, sweepAngle, false, paint);

        // Draw small indicator dot at end of arc
        float endAngle = startAngle + sweepAngle;
        float dotX = centerX + std::cos(endAngle) * ringRadius;
        float dotY = centerY + std::sin(endAngle) * ringRadius;

        SkPaint dotPaint;
        dotPaint.setAntiAlias(true);
        dotPaint.setColor(modColor);
        dotPaint.setStyle(SkPaint::kFill_Style);
        canvas->drawCircle(dotX, dotY, modulationRingWidth_ * 0.8f, dotPaint);
    }
}

void SkiaSmartKnob::drawModulationRing(SkCanvas* canvas, const SkRect& bounds, float modulatedValue) {
    // Draw single modulation ring showing current modulated value
    float centerX = bounds.centerX();
    float centerY = bounds.centerY();
    float radius = juce::jmin(bounds.width(), bounds.height()) * 0.35f;
    float ringRadius = radius + 8.0f;

    // Normalize value to 0-1
    float normalizedValue = (modulatedValue - minValue_) / (maxValue_ - minValue_);

    // Draw ring
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setColor(getModulationColor());
    paint.setAlpha(static_cast<int>(255 * modulationOpacity_));
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(modulationRingWidth_);

    float startAngle = arcStartAngle_;
    float sweepAngle = (arcEndAngle_ - arcStartAngle_) * normalizedValue;

    SkRect arcRect = SkRect::MakeLTRB(
        centerX - ringRadius,
        centerY - ringRadius,
        centerX + ringRadius,
        centerY + ringRadius
    );

    canvas->drawArc(arcRect, startAngle, sweepAngle, false, paint);
}

void SkiaSmartKnob::drawValueTooltip(SkCanvas* canvas, const SkRect& bounds) {
    float modulatedValue = getModulatedValue();
    juce::String valueText = juce::String(modulatedValue, 2);

    // Tooltip dimensions
    float padding = 6.0f;
    float cornerRadius = 4.0f;

    // Measure text (simplified - in real implementation use Skia text measurement)
    float textWidth = 60.0f; // Approximate
    float textHeight = 16.0f;

    float tooltipWidth = textWidth + padding * 2.0f;
    float tooltipHeight = textHeight + padding * 2.0f;

    // Position tooltip above knob
    float tooltipX = bounds.centerX() - tooltipWidth * 0.5f;
    float tooltipY = bounds.top() - tooltipHeight - 8.0f;

    SkRect tooltipRect = SkRect::MakeXYWH(tooltipX, tooltipY, tooltipWidth, tooltipHeight);

    // Draw tooltip background
    SkPaint bgPaint;
    bgPaint.setAntiAlias(true);
    bgPaint.setColor(SkColorSetARGB(230, 30, 30, 35));
    bgPaint.setStyle(SkPaint::kFill_Style);

    canvas->drawRoundRect(tooltipRect, cornerRadius, cornerRadius, bgPaint);

    // Draw tooltip border
    SkPaint borderPaint;
    borderPaint.setAntiAlias(true);
    borderPaint.setColor(SkColorSetARGB(255, 100, 100, 110));
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.0f);

    canvas->drawRoundRect(tooltipRect, cornerRadius, cornerRadius, borderPaint);

    // Draw text (simplified - use Skia text rendering in real implementation)
    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(SkColorSetARGB(255, 220, 220, 220));
    // canvas->drawText(valueText.toUTF8(), valueText.length(), ...);

    // Draw small arrow pointing to knob
    SkPath arrowPath;
    float arrowWidth = 8.0f;
    float arrowHeight = 6.0f;
    float arrowX = bounds.centerX();
    float arrowY = bounds.top() - 2.0f;

    arrowPath.moveTo(arrowX, arrowY);
    arrowPath.lineTo(arrowX - arrowWidth * 0.5f, arrowY - arrowHeight);
    arrowPath.lineTo(arrowX + arrowWidth * 0.5f, arrowY - arrowHeight);
    arrowPath.close();

    SkPaint arrowPaint;
    arrowPaint.setAntiAlias(true);
    arrowPaint.setColor(SkColorSetARGB(230, 30, 30, 35));
    arrowPaint.setStyle(SkPaint::kFill_Style);

    canvas->drawPath(arrowPath, arrowPaint);
}

void SkiaSmartKnob::drawMiniDisplay(SkCanvas* canvas, const SkRect& bounds) {
    float modulatedValue = getModulatedValue();
    juce::String valueText = juce::String(modulatedValue, 1);

    float displayRadius = juce::jmin(bounds.width(), bounds.height()) * 0.2f;
    float centerX = bounds.centerX();
    float centerY = bounds.centerY();

    // Draw circular background
    SkPaint bgPaint;
    bgPaint.setAntiAlias(true);
    bgPaint.setColor(SkColorSetARGB(200, 20, 20, 25));
    bgPaint.setStyle(SkPaint::kFill_Style);

    canvas->drawCircle(centerX, centerY, displayRadius, bgPaint);

    // Draw value text (simplified)
    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(SkColorSetARGB(255, 180, 180, 180));
    textPaint.setTextSize(12.0f);
    // canvas->drawText(valueText.toUTF8(), valueText.length(), ...);
}

float SkiaSmartKnob::calculateTotalModulation() const {
    float total = 0.0f;

    for (const auto& routing : modulationRoutings_) {
        if (routing.isActive && routing.source) {
            float modValue = routing.source->currentValue * routing.amount;
            if (routing.isInverted) {
                modValue = -modValue;
            }
            total += modValue;
        }
    }

    return total;
}

SkColor SkiaSmartKnob::getModulationColor() const {
    // Default modulation color (cyan)
    return SkColorSetARGB(255, 0, 200, 255);
}

void SkiaSmartKnob::mouseDown(const juce::MouseEvent& e) {
    SkiaRotaryKnob::mouseDown(e);

    if (e.mods.isRightButtonDown()) {
        // Show context menu
        // Implementation would show modulation routing menu
    }
}

void SkiaSmartKnob::mouseDrag(const juce::MouseEvent& e) {
    SkiaRotaryKnob::mouseDrag(e);
}

void SkiaSmartKnob::mouseUp(const juce::MouseEvent& e) {
    SkiaRotaryKnob::mouseUp(e);
}

void SkiaSmartKnob::mouseMove(const juce::MouseEvent& e) {
    SkiaRotaryKnob::mouseMove(e);

    if (showValueTooltip_) {
        markDirty(); // Redraw for tooltip
    }
}

//==============================================================================
// SkiaModulationDisplay Implementation
//==============================================================================

SkiaModulationDisplay::SkiaModulationDisplay() {
    startTimerHz(60); // 60 FPS animation
}

void SkiaModulationDisplay::drawSkia(SkCanvas* canvas) {
    if (!canvas) return;

    auto bounds = SkRect::MakeXYWH(0, 0, getWidth(), getHeight());

    // Clear background
    SkPaint bgPaint;
    bgPaint.setColor(SkColorSetARGB(255, 15, 15, 20));
    bgPaint.setStyle(SkPaint::kFill_Style);
    canvas->drawRect(bounds, bgPaint);

    // Collect modulation amounts
    std::vector<float> amounts;
    for (const auto* source : sources_) {
        if (source) {
            amounts.push_back(source->currentValue);
        }
    }

    // Draw based on layout style
    switch (layoutStyle_) {
        case ModulationRingStyle::Solid:
            drawSolidRing(canvas, bounds, amounts);
            break;
        case ModulationRingStyle::Segmented:
            drawSegmentedRing(canvas, bounds, amounts);
            break;
        case ModulationRingStyle::Dotted:
            drawDottedRing(canvas, bounds, amounts);
            break;
        case ModulationRingStyle::Waveform:
            drawWaveformRing(canvas, bounds, amounts);
            break;
        case ModulationRingStyle::Bars:
            drawBars(canvas, bounds, amounts);
            break;
        case ModulationRingStyle::Concentric:
            drawConcentricRings(canvas, bounds, amounts);
            break;
    }

    // Draw source indicators
    drawSourceIndicators(canvas, bounds);

    // Draw amount labels
    if (showAmountValues_) {
        drawAmountLabels(canvas, bounds);
    }
}

void SkiaModulationDisplay::setSources(const std::vector<ModulationSource*>& sources) {
    sources_ = sources;
    markDirty();
}

void SkiaModulationDisplay::addSource(ModulationSource* source) {
    if (source) {
        sources_.push_back(source);
        markDirty();
    }
}

void SkiaModulationDisplay::setRoutings(const std::vector<ModulationRouting>& routings) {
    routings_ = routings;
    markDirty();
}

void SkiaModulationDisplay::addRouting(const ModulationRouting& routing) {
    routings_.push_back(routing);
    markDirty();
}

void SkiaModulationDisplay::timerCallback() {
    // Update animation phase
    animationPhase_ += 0.02f * animationSpeed_;
    if (animationPhase_ > 1.0f) {
        animationPhase_ -= 1.0f;
    }

    // Update source values for animation
    for (auto* source : sources_) {
        if (source && source->isActive) {
            // Smooth interpolation toward current value
            float diff = source->currentValue - source->displayedValue;
            source->displayedValue += diff * 0.1f;
        }
    }

    markDirty(); // Trigger redraw
}

void SkiaModulationDisplay::drawSolidRing(SkCanvas* canvas, const SkRect& bounds,
                                        const std::vector<float>& amounts) {
    if (amounts.empty()) return;

    float centerX = bounds.centerX();
    float centerY = bounds.centerY();
    float maxRadius = juce::jmin(bounds.width(), bounds.height()) * 0.4f;

    for (size_t i = 0; i < amounts.size() && i < static_cast<size_t>(maxRoutings_); ++i) {
        float radius = maxRadius * (1.0f - static_cast<float>(i) / amounts.size());
        float amount = std::abs(amounts[i]);

        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setColor(getSourceColor(static_cast<int>(i)));
        paint.setAlpha(static_cast<int>(200 * amount));
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setStrokeWidth(3.0f);

        SkRect arcRect = SkRect::MakeLTRB(
            centerX - radius,
            centerY - radius,
            centerX + radius,
            centerY + radius
        );

        float sweepAngle = (juce::MathConstants<float>::twoPi * 0.8f) * amount;
        canvas->drawArc(arcRect, 0.0f, sweepAngle, false, paint);
    }
}

void SkiaModulationDisplay::drawSegmentedRing(SkCanvas* canvas, const SkRect& bounds,
                                            const std::vector<float>& amounts) {
    if (amounts.empty()) return;

    float centerX = bounds.centerX();
    float centerY = bounds.centerY();
    float maxRadius = juce::jmin(bounds.width(), bounds.height()) * 0.4f;
    int numSegments = 32;

    for (size_t i = 0; i < amounts.size() && i < static_cast<size_t>(maxRoutings_); ++i) {
        float radius = maxRadius * (1.0f - static_cast<float>(i) / amounts.size());
        float amount = std::abs(amounts[i]);
        int activeSegments = static_cast<int>(numSegments * amount);

        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setColor(getSourceColor(static_cast<int>(i)));
        paint.setStyle(SkPaint::kFill_Style);

        for (int j = 0; j < numSegments; ++j) {
            float angle = (static_cast<float>(j) / numSegments) * juce::MathConstants<float>::twoPi;

            // Fade inactive segments
            if (j >= activeSegments) {
                paint.setAlpha(50);
            } else {
                paint.setAlpha(200);
            }

            float segWidth = 8.0f;
            float segHeight = 12.0f;

            SkRect segRect = SkRect::MakeLTRB(
                centerX + std::cos(angle) * radius - segWidth * 0.5f,
                centerY + std::sin(angle) * radius - segHeight * 0.5f,
                centerX + std::cos(angle) * radius + segWidth * 0.5f,
                centerY + std::sin(angle) * radius + segHeight * 0.5f
            );

            canvas->drawRect(segRect, paint);
        }
    }
}

void SkiaModulationDisplay::drawDottedRing(SkCanvas* canvas, const SkRect& bounds,
                                          const std::vector<float>& amounts) {
    // Similar to segmented but with circles
    if (amounts.empty()) return;

    float centerX = bounds.centerX();
    float centerY = bounds.centerY();
    float maxRadius = juce::jmin(bounds.width(), bounds.height()) * 0.4f;
    int numDots = 24;

    for (size_t i = 0; i < amounts.size() && i < static_cast<size_t>(maxRoutings_); ++i) {
        float radius = maxRadius * (1.0f - static_cast<float>(i) / amounts.size());
        float amount = std::abs(amounts[i]);
        int activeDots = static_cast<int>(numDots * amount);

        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setColor(getSourceColor(static_cast<int>(i)));
        paint.setStyle(SkPaint::kFill_Style);

        for (int j = 0; j < numDots; ++j) {
            float angle = (static_cast<float>(j) / numDots) * juce::MathConstants<float>::twoPi;

            bool isActive = j < activeDots;
            paint.setAlpha(isActive ? 220 : 60);

            float dotX = centerX + std::cos(angle) * radius;
            float dotY = centerY + std::sin(angle) * radius;

            canvas->drawCircle(dotX, dotY, 3.0f, paint);
        }
    }
}

void SkiaModulationDisplay::drawWaveformRing(SkCanvas* canvas, const SkRect& bounds,
                                           const std::vector<float>& waveform) {
    if (waveform.empty()) return;

    float centerX = bounds.centerX();
    float centerY = bounds.centerY();
    float radius = juce::jmin(bounds.width(), bounds.height()) * 0.35f;

    SkPath path;
    bool firstPoint = true;

    for (size_t i = 0; i < waveform.size(); ++i) {
        float angle = (static_cast<float>(i) / waveform.size()) * juce::MathConstants<float>::twoPi;
        float amplitude = waveform[i] * 20.0f; // Scale amplitude

        float x = centerX + std::cos(angle) * (radius + amplitude);
        float y = centerY + std::sin(angle) * (radius + amplitude);

        if (firstPoint) {
            path.moveTo(x, y);
            firstPoint = false;
        } else {
            path.lineTo(x, y);
        }
    }

    path.close();

    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setColor(SkColorSetARGB(255, 0, 200, 255));
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(2.0f);

    canvas->drawPath(path, paint);
}

void SkiaModulationDisplay::drawBars(SkCanvas* canvas, const SkRect& bounds,
                                    const std::vector<float>& amounts) {
    if (amounts.empty()) return;

    float padding = 10.0f;
    float barWidth = (bounds.width() - padding * 2.0f) / amounts.size();
    float maxBarHeight = bounds.height() * 0.6f;

    for (size_t i = 0; i < amounts.size(); ++i) {
        float amount = std::abs(amounts[i]);
        float barHeight = maxBarHeight * amount;

        float x = padding + i * barWidth;
        float y = bounds.centerY() - barHeight * 0.5f;

        SkRect barRect = SkRect::MakeXYWH(x + 2.0f, y, barWidth - 4.0f, barHeight);

        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setColor(getSourceColor(static_cast<int>(i)));
        paint.setStyle(SkPaint::kFill_Style);

        canvas->drawRect(barRect, paint);
    }
}

void SkiaModulationDisplay::drawConcentricRings(SkCanvas* canvas, const SkRect& bounds,
                                               const std::vector<float>& amounts) {
    // Similar to solid ring but full circles with varying intensity
    if (amounts.empty()) return;

    float centerX = bounds.centerX();
    float centerY = bounds.centerY();
    float maxRadius = juce::jmin(bounds.width(), bounds.height()) * 0.4f;

    for (size_t i = 0; i < amounts.size() && i < static_cast<size_t>(maxRoutings_); ++i) {
        float radius = maxRadius * (1.0f - static_cast<float>(i) / amounts.size());
        float amount = std::abs(amounts[i]);

        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setColor(getSourceColor(static_cast<int>(i)));
        paint.setAlpha(static_cast<int>(150 * amount));
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setStrokeWidth(2.0f);

        canvas->drawCircle(centerX, centerY, radius, paint);
    }
}

void SkiaModulationDisplay::drawSourceIndicators(SkCanvas* canvas, const SkRect& bounds) {
    if (!showSourceNames_) return;

    float centerX = bounds.centerX();
    float y = bounds.bottom() - 20.0f;
    float spacing = 80.0f;
    float totalWidth = sources_.size() * spacing;
    float startX = centerX - totalWidth * 0.5f;

    for (size_t i = 0; i < sources_.size(); ++i) {
        const auto* source = sources_[i];
        if (!source) continue;

        float x = startX + i * spacing;

        // Draw color indicator
        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setColor(source->color);
        paint.setStyle(SkPaint::kFill_Style);

        canvas->drawCircle(x, y, 6.0f, paint);

        // Draw source name (simplified)
        // canvas->drawText(source->name.toUTF8(), source->name.length(), ...);
    }
}

void SkiaModulationDisplay::drawAmountLabels(SkCanvas* canvas, const SkRect& bounds) {
    // Draw numeric values for each modulation source
    if (sources_.empty()) return;

    float centerX = bounds.centerX();
    float y = bounds.bottom() - 35.0f;
    float spacing = 80.0f;
    float totalWidth = sources_.size() * spacing;
    float startX = centerX - totalWidth * 0.5f;

    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(SkColorSetARGB(255, 180, 180, 180));
    textPaint.setTextSize(11.0f);

    for (size_t i = 0; i < sources_.size(); ++i) {
        const auto* source = sources_[i];
        if (!source) continue;

        float x = startX + i * spacing;
        juce::String amountText = juce::String(source->displayedValue, 2);

        // Draw text (simplified)
        // canvas->drawText(amountText.toUTF8(), amountText.length(), ...);
    }
}

SkColor SkiaModulationDisplay::getSourceColor(int index) const {
    // Default color palette
    static const SkColor colors[] = {
        SkColorSetARGB(255, 0, 200, 255),    // Cyan
        SkColorSetARGB(255, 255, 100, 0),    // Orange
        SkColorSetARGB(255, 0, 255, 100),    // Green
        SkColorSetARGB(255, 200, 0, 255),    // Magenta
        SkColorSetARGB(255, 255, 255, 0),    // Yellow
        SkColorSetARGB(255, 100, 100, 255),  // Purple
        SkColorSetARGB(255, 255, 100, 100),  // Pink
        SkColorSetARGB(255, 100, 255, 255),  // Light cyan
    };

    int colorIndex = index % (sizeof(colors) / sizeof(colors[0]));
    return colors[colorIndex];
}

} // namespace zenith
