/*
  ==============================================================================

    ModulationVisualizer.cpp
    Created: 2026-02-01
    Author:  Zenith DAW

    Implementation of modulation visualization component.

  ==============================================================================
*/

#include "ModulationVisualizer.h"
#include <cmath>
#include <algorithm>

namespace zenith {

//==============================================================================
// ModulationVisualizer Implementation
//==============================================================================

ModulationVisualizer::ModulationVisualizer() {
    // Initialize current mod values
    currentModValues_.fill(0.0f);

    // Initialize waveform buffers
    lfo1State_.waveformBuffer.resize(256);
    lfo2State_.waveformBuffer.resize(256);

    // Initialize pulse positions for each routing
    pulsePositions_.clear();

    // Start timer for animations (60 FPS for premium smoothness)
    startTimerHz(60);
}

ModulationVisualizer::~ModulationVisualizer() {
    stopTimer();
}

void ModulationVisualizer::setLFO1State(const LFOVisualState& state) {
    lfo1State_ = state;
    markDirty();
}

void ModulationVisualizer::setLFO2State(const LFOVisualState& state) {
    lfo2State_ = state;
    markDirty();
}

void ModulationVisualizer::setAmpEnvelopeState(const EnvelopeVisualState& state) {
    ampEnvState_ = state;
    markDirty();
}

void ModulationVisualizer::setModEnvelopeState(const EnvelopeVisualState& state) {
    modEnvState_ = state;
    markDirty();
}

void ModulationVisualizer::setModulationRoutings(const std::vector<ModulationRouting>& routings) {
    routings_ = routings;
    markDirty();
}

void ModulationVisualizer::addModulationRouting(const ModulationRouting& routing) {
    routings_.push_back(routing);
    markDirty();
}

void ModulationVisualizer::clearModulationRoutings() {
    routings_.clear();
    markDirty();
}

void ModulationVisualizer::setModulationValue(ModulationDestination dest, float value) {
    size_t index = static_cast<size_t>(dest);
    if (index < currentModValues_.size()) {
        currentModValues_[index] = value;
    }
}

void ModulationVisualizer::timerCallback() {
    // Animation timing (60 FPS)
    float dt = 1.0f / 60.0f;

    // Update animation phases
    animationPhase_ += dt * 2.0f; // Faster for visible animation
    if (animationPhase_ > 1.0f) animationPhase_ -= 1.0f;

    pulsePhase_ += dt * 1.5f; // Pulse travels along connections
    if (pulsePhase_ > 1.0f) pulsePhase_ -= 1.0f;

    // Update LFO phase based on rate
    float lfo1Phase = lfo1Phase_.load();
    lfo1Phase += lfo1State_.rate * dt;
    if (lfo1Phase > 1.0f) lfo1Phase -= 1.0f;
    lfo1Phase_.store(lfo1Phase);

    float lfo2Phase = lfo2Phase_.load();
    lfo2Phase += lfo2State_.rate * dt;
    if (lfo2Phase > 1.0f) lfo2Phase -= 1.0f;
    lfo2Phase_.store(lfo2Phase);

    // Always mark dirty for smooth 60fps animations
    markDirty();
}

#ifdef ZENITH_USE_SKIA

void ModulationVisualizer::drawSkia(SkCanvas* canvas) {
    // Draw background
    SkPaint bgPaint;
    bgPaint.setColor(backgroundColor_);
    bgPaint.setStyle(SkPaint::kFill_Style);
    canvas->drawRect(SkRect::MakeWH(getWidth(), getHeight()), bgPaint);

    drawAllSections(canvas);
}

void ModulationVisualizer::drawAllSections(SkCanvas* canvas) {
    const float width = getWidth();
    const float height = getHeight();
    const float padding = 8.0f;

    if (width <= 0 || height <= 0) return;

    if (displayMode_ == DisplayMode::MatrixOnly) {
        // Just show matrix
        SkRect matrixBounds = SkRect::MakeXYWH(padding, padding,
                                               width - 2 * padding,
                                               height - 2 * padding);
        drawModulationMatrix(canvas, matrixBounds);
        return;
    }

    if (displayMode_ == DisplayMode::Compact) {
        // LFOs and envelopes side by side
        float sectionWidth = (width - 3 * padding) / 2.0f;
        float sectionHeight = (height - 3 * padding) / 2.0f;

        if (showLFO_) {
            // LFO 1
            SkRect lfo1Bounds = SkRect::MakeXYWH(padding, padding,
                                                sectionWidth, sectionHeight);
            drawLFOWaveform(canvas, lfo1Bounds, lfo1State_.waveform,
                           lfo1Phase_.load(), lfo1Color_, "LFO 1");

            // LFO 2
            SkRect lfo2Bounds = SkRect::MakeXYWH(padding * 2 + sectionWidth, padding,
                                                sectionWidth, sectionHeight);
            drawLFOWaveform(canvas, lfo2Bounds, lfo2State_.waveform,
                           lfo2Phase_.load(), lfo2Color_, "LFO 2");
        }

        if (showEnvelopes_) {
            // Amp Envelope
            SkRect ampEnvBounds = SkRect::MakeXYWH(padding,
                                                   padding * 2 + sectionHeight,
                                                   sectionWidth, sectionHeight);
            drawEnvelope(canvas, ampEnvBounds, ampEnvState_, ampEnvColor_, "AMP ENV");

            // Mod Envelope
            SkRect modEnvBounds = SkRect::MakeXYWH(padding * 2 + sectionWidth,
                                                   padding * 2 + sectionHeight,
                                                   sectionWidth, sectionHeight);
            drawEnvelope(canvas, modEnvBounds, modEnvState_, modEnvColor_, "MOD ENV");
        }
    }
    else { // Full mode
        // Top: LFOs
        if (showLFO_) {
            float lfoHeight = height * 0.35f;
            float lfoWidth = (width - 3 * padding) / 2.0f;

            SkRect lfo1Bounds = SkRect::MakeXYWH(padding, padding,
                                                lfoWidth, lfoHeight);
            drawLFOWaveform(canvas, lfo1Bounds, lfo1State_.waveform,
                           lfo1Phase_.load(), lfo1Color_, "LFO 1");

            SkRect lfo2Bounds = SkRect::MakeXYWH(padding * 2 + lfoWidth, padding,
                                                lfoWidth, lfoHeight);
            drawLFOWaveform(canvas, lfo2Bounds, lfo2State_.waveform,
                           lfo2Phase_.load(), lfo2Color_, "LFO 2");
        }

        // Middle: Envelopes
        if (showEnvelopes_) {
            float envTop = showLFO_ ? height * 0.38f : padding;
            float envHeight = height * 0.25f;
            float envWidth = (width - 3 * padding) / 2.0f;

            SkRect ampEnvBounds = SkRect::MakeXYWH(padding, envTop,
                                                   envWidth, envHeight);
            drawEnvelope(canvas, ampEnvBounds, ampEnvState_, ampEnvColor_, "AMP ENV");

            SkRect modEnvBounds = SkRect::MakeXYWH(padding * 2 + envWidth, envTop,
                                                   envWidth, envHeight);
            drawEnvelope(canvas, modEnvBounds, modEnvState_, modEnvColor_, "MOD ENV");
        }

        // Bottom: Modulation Matrix
        if (showMatrix_) {
            float matrixTop = height * 0.68f;
            SkRect matrixBounds = SkRect::MakeXYWH(padding, matrixTop,
                                                   width - 2 * padding,
                                                   height - matrixTop - padding);
            drawModulationMatrix(canvas, matrixBounds);
        }
    }
}

void ModulationVisualizer::drawLFOWaveform(SkCanvas* canvas, const SkRect& bounds,
                                          LFOWaveform waveform, float phase,
                                          SkColor color, const juce::String& label) {
    if (bounds.isEmpty()) return;

    // ========== PREMIUM BACKGROUND ==========
    SkPaint bgPaint;
    bgPaint.setColor(SkColorSetARGB(60, 25, 25, 35));
    bgPaint.setStyle(SkPaint::kFill_Style);
    canvas->drawRoundRect(bounds, 10.0f, 10.0f, bgPaint);

    // Subtle inner glow border
    SkPaint borderPaint;
    borderPaint.setColor(SkColorSetARGB(120, 70, 70, 80));
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.5f);
    borderPaint.setAntiAlias(true);
    canvas->drawRoundRect(bounds, 10.0f, 10.0f, borderPaint);

    // Draw center line with glow
    SkPaint centerPaint;
    centerPaint.setColor(SkColorSetARGB(100, 70, 70, 80));
    centerPaint.setStyle(SkPaint::kStroke_Style);
    centerPaint.setStrokeWidth(1.0f);
    float centerY = bounds.centerY();
    canvas->drawLine(bounds.fLeft + 4, centerY, bounds.fRight - 4, centerY, centerPaint);

    // ========== GENERATE WAVEFORM ==========
    SkPath path;
    generateLFOPath(waveform, path, bounds, phase);

    // ========== GRADIENT FILL UNDER WAVEFORM ==========
    SkPoint gradPoints[2] = {
        {bounds.fLeft, bounds.fTop},
        {bounds.fLeft, bounds.fBottom}
    };

    SkColor gradColors[2] = {
        SkColorSetARGB(80, SkColorGetR(color), SkColorGetG(color), SkColorGetB(color)),
        SkColorSetARGB(10, SkColorGetR(color), SkColorGetG(color), SkColorGetB(color))
    };
    float gradPos[2] = {0.0f, 1.0f};

    auto gradient = SkGradientShader::MakeLinear(gradPoints, gradColors, gradPos, 2,
                                                   SkTileMode::kClamp);

    SkPath fillPath = path;
    // Close the path for filling
    if (!path.isEmpty()) {
        // Create fill by extending to center
        SkPath closedPath;
        SkPath::Iter iter(path, false);
        SkPoint firstPoint;
        SkPoint lastPoint;
        bool hasFirst = false;
        SkPoint pt;

        while (iter.next(&pt)) {
            if (iter.isCloseLine()) break;
            if (!hasFirst) {
                firstPoint = pt;
                hasFirst = true;
                closedPath.moveTo(pt.fX, centerY);
                closedPath.lineTo(pt.fX, pt.fY);
            } else {
                closedPath.lineTo(pt.fX, pt.fY);
                lastPoint = pt;
            }
        }
        if (hasFirst) {
            closedPath.lineTo(lastPoint.fX, centerY);
            closedPath.close();

            SkPaint fillPaint;
            fillPaint.setShader(gradient);
            fillPaint.setAntiAlias(true);
            canvas->drawPath(closedPath, fillPaint);
        }
    }

    // ========== OUTER GLOW ==========
    SkPaint glowOuter;
    glowOuter.setColor(color);
    glowOuter.setStyle(SkPaint::kStroke_Style);
    glowOuter.setStrokeWidth(6.0f);
    glowOuter.setAlpha(50);
    glowOuter.setAntiAlias(true);
    auto blurOuter = SkMaskFilter::MakeBlur(SkBlurStyle::kNormal_SkBlurStyle, 4.0f);
    glowOuter.setMaskFilter(blurOuter);
    canvas->drawPath(path, glowOuter);

    // ========== MAIN WAVEFORM ==========
    SkPaint wavePaint;
    wavePaint.setColor(SkColorSetRGB(
        std::min(255u, SkColorGetR(color) + 30),
        std::min(255u, SkColorGetG(color) + 30),
        std::min(255u, SkColorGetB(color) + 30)));
    wavePaint.setStyle(SkPaint::kStroke_Style);
    wavePaint.setStrokeWidth(2.5f);
    wavePaint.setAntiAlias(true);
    canvas->drawPath(path, wavePaint);

    // ========== ANIMATED PHASE INDICATOR ==========
    float phaseX = bounds.fLeft + phase * bounds.width();

    // Pulsing rings
    float pulseSize = 6.0f + 4.0f * std::sin(animationPhase_ * juce::MathConstants<float>::twoPi);
    SkPaint pulseRing;
    pulseRing.setColor(color);
    pulseRing.setStyle(SkPaint::kStroke_Style);
    pulseRing.setStrokeWidth(1.5f);
    pulseRing.setAlpha(static_cast<U8CPU>(120 - animationPhase_ * 80));
    canvas->drawCircle(phaseX, centerY, pulseSize, pulseRing);

    // Outer glow
    SkPaint phaseGlow;
    phaseGlow.setColor(color);
    phaseGlow.setStyle(SkPaint::kFill_Style);
    phaseGlow.setAlpha(100);
    auto phaseBlur = SkMaskFilter::MakeBlur(SkBlurStyle::kNormal_SkBlurStyle, 6.0f);
    phaseGlow.setMaskFilter(phaseBlur);
    canvas->drawCircle(phaseX, centerY, 8.0f, phaseGlow);

    // Bright center
    SkPaint centerDot;
    centerDot.setColor(SkColorSetRGB(255, 255, 255));
    centerDot.setStyle(SkPaint::kFill_Style);
    canvas->drawCircle(phaseX, centerY, 4.0f, centerDot);

    // ========== LABELS ==========
    SkFont font;
    font.setSize(12.0f);

    SkPaint textPaint;
    textPaint.setColor(textColor_);
    textPaint.setAntiAlias(true);

    // Label at top left with subtle shadow
    canvas->drawText(label.toUTF8(), label.length(),
                    bounds.fLeft + 10, bounds.fTop + 16, font, textPaint);

    // Rate label at bottom right
    float rate = (label == "LFO 1") ? lfo1State_.rate : lfo2State_.rate;
    juce::String rateLabel = juce::String(rate, 1) + " Hz";
    canvas->drawText(rateLabel.toUTF8(), rateLabel.length(),
                    bounds.fRight - 55, bounds.fBottom - 8, font, textPaint);
}

void ModulationVisualizer::drawEnvelope(SkCanvas* canvas, const SkRect& bounds,
                                       const EnvelopeVisualState& state,
                                       SkColor color, const juce::String& label) {
    if (bounds.isEmpty()) return;

    // Draw background
    SkPaint bgPaint;
    bgPaint.setColor(SkColorSetARGB(50, 30, 30, 40));
    bgPaint.setStyle(SkPaint::kFill_Style);
    canvas->drawRoundRect(bounds, 8.0f, 8.0f, bgPaint);

    // Draw border
    SkPaint borderPaint;
    borderPaint.setColor(SkColorSetARGB(100, 60, 60, 70));
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.0f);
    canvas->drawRoundRect(bounds, 8.0f, 8.0f, borderPaint);

    // Generate envelope path
    SkPath path;
    generateEnvelopePath(state, path, bounds);

    // Fill under envelope
    SkPaint fillPaint;
    fillPaint.setColor(SkColorSetARGB(40, SkColorGetR(color),
                                      SkColorGetG(color),
                                      SkColorGetB(color)));
    fillPaint.setStyle(SkPaint::kFill_Style);

    SkPath fillPath = path;
    fillPath.lineTo(bounds.fRight, bounds.fBottom);
    fillPath.lineTo(bounds.fLeft, bounds.fBottom);
    fillPath.close();
    canvas->drawPath(fillPath, fillPaint);

    // Draw envelope curve
    SkPaint wavePaint;
    wavePaint.setColor(color);
    wavePaint.setStyle(SkPaint::kStroke_Style);
    wavePaint.setStrokeWidth(2.0f);
    wavePaint.setAntiAlias(true);
    canvas->drawPath(path, wavePaint);

    // Draw current value indicator
    float currentY = bounds.fBottom - (state.currentValue * bounds.height() * 0.8f);

    // Draw stage indicator text
    SkFont font;
    font.setSize(11.0f);
    SkPaint textPaint;
    textPaint.setColor(textColor_);
    textPaint.setAntiAlias(true);

    canvas->drawText(label.toUTF8(), label.length(),
                    bounds.fLeft + 8, bounds.fTop + 14, font, textPaint);

    // Stage labels
    const char* stages[] = {"ATK", "DEC", "SUS", "REL", "IDLE"};
    juce::String stageText = stages[std::min(4, state.currentStage)];
    if (state.isActive) {
        stageText += juce::String::formatted(" %.0f%%", state.stageProgress * 100);
    }
    canvas->drawText(stageText.toUTF8(), stageText.length(),
                    bounds.fRight - 60, bounds.fTop + 14, font, textPaint);

    // ADSR values
    float x = bounds.fLeft + 8;
    float y = bounds.fBottom - 8;
    juce::String adsrText = juce::String::formatted("A:%.2f D:%.2f S:%.2f R:%.2f",
                                                    state.attack, state.decay,
                                                    state.sustain, state.release);
    canvas->drawText(adsrText.toUTF8(), adsrText.length(), x, y, font, textPaint);
}

void ModulationVisualizer::drawModulationMatrix(SkCanvas* canvas, const SkRect& bounds) {
    if (bounds.isEmpty()) return;

    // Draw background
    SkPaint bgPaint;
    bgPaint.setColor(SkColorSetARGB(50, 30, 30, 40));
    bgPaint.setStyle(SkPaint::kFill_Style);
    canvas->drawRoundRect(bounds, 8.0f, 8.0f, bgPaint);

    // Draw border
    SkPaint borderPaint;
    borderPaint.setColor(SkColorSetARGB(100, 60, 60, 70));
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.0f);
    canvas->drawRoundRect(bounds, 8.0f, 8.0f, borderPaint);

    // Layout: sources on left, destinations on right
    const float sourceWidth = bounds.width() * 0.25f;
    const float destWidth = bounds.width() * 0.25f;
    const float centerX = bounds.fLeft + bounds.width() * 0.5f;

    // Calculate node positions
    std::vector<SkRect> sourceNodes;
    std::vector<SkRect> destNodes;

    // Important sources to display
    std::vector<ModulationSource> sources = {
        ModulationSource::LFO1,
        ModulationSource::LFO2,
        ModulationSource::Env1,
        ModulationSource::Env2,
        ModulationSource::Velocity,
        ModulationSource::ModWheel,
        ModulationSource::Aftertouch
    };

    // Important destinations to display
    std::vector<ModulationDestination> destinations = {
        ModulationDestination::FilterCutoff,
        ModulationDestination::FilterResonance,
        ModulationDestination::Osc1Pitch,
        ModulationDestination::Osc2Pitch,
        ModulationDestination::Osc1Mix,
        ModulationDestination::AmpGain,
        ModulationDestination::Osc1Shape
    };

    const float nodeHeight = 18.0f;
    const float nodeSpacing = (bounds.height() - 20) / std::max(sources.size(), destinations.size());

    // Create source nodes
    for (size_t i = 0; i < sources.size(); ++i) {
        float y = bounds.fTop + 10 + i * nodeSpacing;
        SkRect nodeRect = SkRect::MakeXYWH(bounds.fLeft + 10, y,
                                          sourceWidth - 20, nodeHeight);
        sourceNodes.push_back(nodeRect);
        drawSourceNode(canvas, nodeRect, sources[i],
                      sourceColors_[static_cast<size_t>(sources[i])]);
    }

    // Create destination nodes
    for (size_t i = 0; i < destinations.size(); ++i) {
        float y = bounds.fTop + 10 + i * nodeSpacing;
        SkRect nodeRect = SkRect::MakeXYWH(bounds.fRight - destWidth + 10, y,
                                          destWidth - 20, nodeHeight);
        destNodes.push_back(nodeRect);

        float modValue = currentModValues_[static_cast<size_t>(destinations[i])];
        drawDestNode(canvas, nodeRect, destinations[i], modValue);
    }

    // Draw modulation connections
    for (const auto& routing : routings_) {
        if (!routing.isActive()) continue;

        // Find source and destination indices
        size_t sourceIdx = SIZE_MAX;
        size_t destIdx = SIZE_MAX;

        for (size_t i = 0; i < sources.size(); ++i) {
            if (sources[i] == routing.source) {
                sourceIdx = i;
                break;
            }
        }

        for (size_t i = 0; i < destinations.size(); ++i) {
            if (destinations[i] == routing.dest) {
                destIdx = i;
                break;
            }
        }

        if (sourceIdx < sourceNodes.size() && destIdx < destNodes.size()) {
            const SkRect& sourceRect = sourceNodes[sourceIdx];
            const SkRect& destRect = destNodes[destIdx];

            float currentVal = currentModValues_[static_cast<size_t>(routing.dest)];
            drawModulationConnection(canvas, sourceRect, destRect,
                                   routing.amount, currentVal, routing.color);
        }
    }

    // Title
    SkFont font;
    font.setSize(12.0f);
    SkPaint textPaint;
    textPaint.setColor(textColor_);
    textPaint.setAntiAlias(true);

    juce::String title = "MODULATION MATRIX";
    canvas->drawText(title.toUTF8(), title.length(),
                    bounds.fLeft + 15, bounds.fTop + 14, font, textPaint);
}

void ModulationVisualizer::drawModulationConnection(SkCanvas* canvas,
                                                    const SkRect& sourceRect,
                                                    const SkRect& destRect,
                                                    float amount, float currentValue,
                                                    SkColor color) {
    // Get connection points
    float startX = sourceRect.fRight;
    float startY = sourceRect.centerY();
    float endX = destRect.fLeft;
    float endY = destRect.centerY();

    // Create curved path
    SkPath path;
    path.moveTo(startX, startY);

    float midX = (startX + endX) / 2.0f;
    path.cubicTo(midX, startY, midX, endY, endX, endY);

    // ========== PREMIUM GLOW EFFECTS ==========
    // Outer glow layer
    SkPaint glowOuter;
    glowOuter.setColor(color);
    glowOuter.setStyle(SkPaint::kStroke_Style);
    float glowWidth = 4.0f + std::abs(amount) * 6.0f;
    glowOuter.setStrokeWidth(glowWidth);
    glowOuter.setAlpha(static_cast<U8CPU>(40 + std::abs(amount) * 60));
    glowOuter.setAntiAlias(true);
    auto blur = SkMaskFilter::MakeBlur(SkBlurStyle::kNormal_SkBlurStyle, 3.0f);
    glowOuter.setMaskFilter(blur);
    canvas->drawPath(path, glowOuter);

    // Inner bright core
    SkPaint corePaint;
    corePaint.setColor(SkColorSetRGB(
        std::min(255u, SkColorGetR(color) + 40),
        std::min(255u, SkColorGetG(color) + 40),
        std::min(255u, SkColorGetB(color) + 40)));
    corePaint.setStyle(SkPaint::kStroke_Style);
    corePaint.setStrokeWidth(2.0f);
    corePaint.setAntiAlias(true);
    canvas->drawPath(path, corePaint);

    // ========== ANIMATED PULSE TRAVELING ALONG CONNECTION ==========
    // Multiple pulses for more visual interest
    for (int pulseIdx = 0; pulseIdx < 3; ++pulseIdx) {
        float pulseT = pulsePhase_ + pulseIdx * 0.33f;
        while (pulseT > 1.0f) pulseT -= 1.0f;

        // Pulse fades at ends
        float pulseAlpha = 1.0f - std::abs(pulseT - 0.5f) * 2.0f;
        pulseAlpha = juce::jmax(0.0f, pulseAlpha);
        pulseAlpha *= (0.3f + 0.7f * std::abs(amount)); // Scale by modulation amount

        // Calculate point on bezier curve
        float invT = 1.0f - pulseT;
        float pulseX = invT * invT * invT * startX +
                       3.0f * invT * invT * pulseT * midX +
                       3.0f * invT * pulseT * pulseT * midX +
                       pulseT * pulseT * pulseT * endX;
        float pulseY = invT * invT * invT * startY +
                       3.0f * invT * invT * pulseT * startY +
                       3.0f * invT * pulseT * pulseT * endY +
                       pulseT * pulseT * pulseT * endY;

        // Draw pulse with glow
        float pulseRadius = 3.0f + 2.0f * pulseAlpha;

        SkPaint pulseGlow;
        pulseGlow.setColor(color);
        pulseGlow.setStyle(SkPaint::kFill_Style);
        pulseGlow.setAlpha(static_cast<U8CPU>(pulseAlpha * 150));
        auto pulseBlur = SkMaskFilter::MakeBlur(SkBlurStyle::kNormal_SkBlurStyle, 4.0f);
        pulseGlow.setMaskFilter(pulseBlur);
        canvas->drawCircle(pulseX, pulseY, pulseRadius * 2.0f, pulseGlow);

        SkPaint pulseCore;
        pulseCore.setColor(SkColorSetRGB(255, 255, 255));
        pulseCore.setStyle(SkPaint::kFill_Style);
        pulseCore.setAlpha(static_cast<U8CPU>(pulseAlpha * 200));
        canvas->drawCircle(pulseX, pulseY, pulseRadius, pulseCore);
    }

    // ========== CURRENT VALUE DOT (Pigments-style bubble) ==========
    float valueT = 0.5f + currentValue * 0.3f;
    valueT = juce::jlimit(0.0f, 1.0f, valueT);

    // Calculate point on bezier curve
    float invT = 1.0f - valueT;
    float dotX = invT * invT * invT * startX +
                 3.0f * invT * invT * valueT * midX +
                 3.0f * invT * valueT * valueT * midX +
                 valueT * valueT * valueT * endX;
    float dotY = invT * invT * invT * startY +
                 3.0f * invT * invT * valueT * startY +
                 3.0f * invT * valueT * valueT * endY +
                 valueT * valueT * valueT * endY;

    // Glow around value dot
    SkPaint dotGlow;
    dotGlow.setColor(color);
    dotGlow.setStyle(SkPaint::kFill_Style);
    dotGlow.setAlpha(150);
    auto dotBlur = SkMaskFilter::MakeBlur(SkBlurStyle::kNormal_SkBlurStyle, 5.0f);
    dotGlow.setMaskFilter(dotBlur);
    canvas->drawCircle(dotX, dotY, 8.0f, dotGlow);

    // White center
    SkPaint dotPaint;
    dotPaint.setColor(SkColorSetRGB(255, 255, 255));
    dotPaint.setStyle(SkPaint::kFill_Style);
    canvas->drawCircle(dotX, dotY, 4.0f, dotPaint);

    // Value bubble text (like Pigments)
    if (std::abs(currentValue) > 0.01f) {
        juce::String valStr = juce::String(currentValue * 100, 0) + "%";
        SkFont valFont;
        valFont.setSize(9.0f);

        SkPaint valPaint;
        valPaint.setColor(SkColorSetRGB(255, 255, 255));
        valPaint.setAntiAlias(true);

        // Draw bubble background
        float bubbleW = valFont.measureText(valStr.getCharPointer(), valStr.length(),
                                              SkTextEncoding::kUTF8) + 8;
        SkRect bubbleRect = SkRect::MakeXYWH(dotX - bubbleW/2, dotY - 20, bubbleW, 12);

        SkPaint bubblePaint;
        bubblePaint.setColor(SkColorSetARGB(200, 40, 40, 50));
        bubblePaint.setStyle(SkPaint::kFill_Style);
        canvas->drawRoundRect(bubbleRect, 4.0f, 4.0f, bubblePaint);

        canvas->drawSimpleText(valStr.toUTF8(), valStr.length(), SkTextEncoding::kUTF8,
                             dotX - bubbleW/2 + 4, dotY - 10, valFont, valPaint);
    }
}

void ModulationVisualizer::drawSourceNode(SkCanvas* canvas, const SkRect& bounds,
                                         ModulationSource source, SkColor color) {
    SkPaint bgPaint;
    bgPaint.setColor(SkColorSetARGB(150, SkColorGetR(color),
                                     SkColorGetG(color),
                                     SkColorGetB(color)));
    bgPaint.setStyle(SkPaint::kFill_Style);
    canvas->drawRoundRect(bounds, 4.0f, 4.0f, bgPaint);

    SkFont font;
    font.setSize(10.0f);
    SkPaint textPaint;
    textPaint.setColor(SkColorSetRGB(255, 255, 255));
    textPaint.setAntiAlias(true);

    juce::String name = getSourceName(source);
    canvas->drawText(name.toUTF8(), name.length(),
                    bounds.fLeft + 4, bounds.fTop + 12, font, textPaint);
}

void ModulationVisualizer::drawDestNode(SkCanvas* canvas, const SkRect& bounds,
                                       ModulationDestination dest, float currentValue) {
    // Color based on current modulation value
    float intensity = std::abs(currentValue);
    uint8_t r = static_cast<uint8_t>(80 + intensity * 100);
    uint8_t g = static_cast<uint8_t>(80 + intensity * 50);
    uint8_t b = static_cast<uint8_t>(100 + intensity * 50);

    SkPaint bgPaint;
    bgPaint.setColor(SkColorSetARGB(180, r, g, b));
    bgPaint.setStyle(SkPaint::kFill_Style);
    canvas->drawRoundRect(bounds, 4.0f, 4.0f, bgPaint);

    // Modulation indicator bar
    if (std::abs(currentValue) > 0.01f) {
        float barWidth = std::abs(currentValue) * (bounds.width() - 4);
        float barX = currentValue > 0 ? bounds.fLeft + 2 : bounds.fRight - 2 - barWidth;

        SkPaint barPaint;
        barPaint.setColor(SkColorSetARGB(200, 0, 200, 255));
        barPaint.setStyle(SkPaint::kFill_Style);
        SkRect barRect = SkRect::MakeXYWH(barX, bounds.fBottom - 3, barWidth, 2);
        canvas->drawRect(barRect, barPaint);
    }

    SkFont font;
    font.setSize(9.0f);
    SkPaint textPaint;
    textPaint.setColor(SkColorSetRGB(255, 255, 255));
    textPaint.setAntiAlias(true);

    juce::String name = getDestName(dest);
    canvas->drawText(name.toUTF8(), name.length(),
                    bounds.fLeft + 3, bounds.fTop + 11, font, textPaint);
}

void ModulationVisualizer::generateLFOPath(LFOWaveform waveform, SkPath& path,
                                          const SkRect& bounds, float phase) {
    const int numPoints = 128;
    const float width = bounds.width();
    const float height = bounds.height();

    bool started = false;
    for (int i = 0; i < numPoints; ++i) {
        float t = static_cast<float>(i) / (numPoints - 1);
        float phaseAtPoint = t + phase;
        if (phaseAtPoint > 1.0f) phaseAtPoint -= 1.0f;

        float value = calculateLFOValue(waveform, phaseAtPoint);
        float x = bounds.fLeft + t * width;
        float y = bounds.centerY() - value * height * 0.4f;

        if (!started) {
            path.moveTo(x, y);
            started = true;
        } else {
            path.lineTo(x, y);
        }
    }
}

void ModulationVisualizer::generateEnvelopePath(const EnvelopeVisualState& state,
                                               SkPath& path, const SkRect& bounds) {
    const float width = bounds.width();
    const float height = bounds.height();
    const float padding = 10.0f;

    // Calculate total time (normalized)
    float total = state.attack + state.decay + state.release + 0.1f; // Minimum sustain time
    if (total < 0.01f) total = 0.01f;

    float scaleX = (width - 2 * padding) / total;

    // Start point
    float x = bounds.fLeft + padding;
    float y = bounds.fBottom - padding;
    path.moveTo(x, y);

    // Attack
    x += state.attack * scaleX;
    y = bounds.fTop + padding;
    path.lineTo(x, y);

    // Decay
    x += state.decay * scaleX;
    y = bounds.fBottom - padding - (state.sustain * (height - 2 * padding));
    path.lineTo(x, y);

    // Sustain (show as horizontal line)
    float sustainEnd = x + 0.1f * scaleX; // Show a bit of sustain
    path.lineTo(sustainEnd, y);

    // Release
    x = sustainEnd + state.release * scaleX;
    y = bounds.fBottom - padding;
    path.lineTo(x, y);
}

float ModulationVisualizer::calculateLFOValue(LFOWaveform waveform, float phase) {
    float pi2 = 2.0f * juce::MathConstants<float>::pi;
    float t = phase * pi2;

    switch (waveform) {
        case LFOWaveform::Sine:
            return std::sin(t);

        case LFOWaveform::Triangle:
            return 2.0f * std::abs(2.0f * (phase - std::floor(phase + 0.5f))) - 1.0f;

        case LFOWaveform::Saw:
            return 2.0f * (phase - std::floor(phase + 0.5f));

        case LFOWaveform::Square:
            return (phase < 0.5f) ? 1.0f : -1.0f;

        case LFOWaveform::SampleAndHold:
            // Not easily deterministic without state
            return std::sin(t); // Fall back to sine

        default:
            return std::sin(t);
    }
}

SkColor ModulationVisualizer::getSourceColor(ModulationSource source) {
    size_t index = static_cast<size_t>(source);
    if (index < sourceColors_.size()) {
        return sourceColors_[index];
    }
    return SkColorSetRGB(150, 150, 150);
}

SkColor ModulationVisualizer::getDestColor(ModulationDestination dest) {
    // Destination colors are based on current modulation value
    juce::ignoreUnused(dest);
    return SkColorSetRGB(100, 150, 200);
}

juce::String ModulationVisualizer::getSourceName(ModulationSource source) {
    switch (source) {
        case ModulationSource::LFO1: return "LFO 1";
        case ModulationSource::LFO2: return "LFO 2";
        case ModulationSource::Env1: return "AMP ENV";
        case ModulationSource::Env2: return "MOD ENV";
        case ModulationSource::Velocity: return "VELOCITY";
        case ModulationSource::ModWheel: return "MOD WHEEL";
        case ModulationSource::Aftertouch: return "AFTERTOUCH";
        case ModulationSource::Timbre: return "TIMBRE";
        default: return "-";
    }
}

juce::String ModulationVisualizer::getDestName(ModulationDestination dest) {
    switch (dest) {
        case ModulationDestination::FilterCutoff: return "FILTER CUTOFF";
        case ModulationDestination::FilterResonance: return "RESONANCE";
        case ModulationDestination::Osc1Pitch: return "OSC 1 PITCH";
        case ModulationDestination::Osc2Pitch: return "OSC 2 PITCH";
        case ModulationDestination::Osc3Pitch: return "OSC 3 PITCH";
        case ModulationDestination::Osc1Mix: return "OSC 1 MIX";
        case ModulationDestination::Osc2Mix: return "OSC 2 MIX";
        case ModulationDestination::Osc3Mix: return "OSC 3 MIX";
        case ModulationDestination::AmpGain: return "AMP GAIN";
        case ModulationDestination::Osc1Shape: return "OSC 1 SHAPE";
        case ModulationDestination::Osc2Shape: return "OSC 2 SHAPE";
        case ModulationDestination::Osc3Shape: return "OSC 3 SHAPE";
        case ModulationDestination::LFO1Rate: return "LFO 1 RATE";
        case ModulationDestination::LFO2Rate: return "LFO 2 RATE";
        default: return "-";
    }
}

#endif // ZENITH_USE_SKIA

} // namespace zenith
