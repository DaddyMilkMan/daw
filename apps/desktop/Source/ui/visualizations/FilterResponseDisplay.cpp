/*
  ==============================================================================

    FilterResponseDisplay.cpp
    Created: 2026-02-01
    Author:  Zenith DAW

    Implementation of filter frequency response visualizer.

  ==============================================================================
*/

#include "FilterResponseDisplay.h"
#include <cmath>
#include <algorithm>

namespace zenith {

//==============================================================================
// FilterResponseCalculator Implementation
//==============================================================================

void FilterResponseCalculator::computeSVFResponse(
    float cutoff, float resonance, FilterType type, double sampleRate,
    std::vector<float>& frequencies, std::vector<float>& magnitudes,
    std::vector<float>& phases)
{
    const size_t numPoints = frequencies.size();
    const float wc = 2.0f * juce::MathConstants<float>::pi * cutoff;
    const float k = 2.0f - 2.0f * juce::jlimit(0.0f, 1.0f, resonance);

    for (size_t i = 0; i < numPoints; ++i) {
        const float omega = 2.0f * juce::MathConstants<float>::pi * frequencies[i];
        std::complex<float> Hlp, Hbp, Hhp;
        computeSVFTransferFunction(omega, wc, k, Hlp, Hbp, Hhp);

        std::complex<float> H;
        switch (type) {
            case FilterType::Lowpass:
                H = Hlp;
                break;
            case FilterType::Bandpass:
                H = Hbp;
                break;
            case FilterType::Highpass:
                H = Hhp;
                break;
            default:
                H = Hlp;
        }

        magnitudes[i] = magnitudeToDb(std::abs(H));
        phases[i] = std::arg(H) * 180.0f / juce::MathConstants<float>::pi;
    }
}

void FilterResponseCalculator::computeMoogLadderResponse(
    float cutoff, float resonance, double sampleRate,
    std::vector<float>& frequencies, std::vector<float>& magnitudes,
    std::vector<float>& phases)
{
    const size_t numPoints = frequencies.size();
    const float wc = 2.0f * juce::MathConstants<float>::pi * cutoff;
    const float k = 4.0f * std::pow(resonance, 1.2f);

    // Moog ladder is 4 cascaded 1-pole lowpass filters with feedback
    for (size_t i = 0; i < numPoints; ++i) {
        const float omega = 2.0f * juce::MathConstants<float>::pi * frequencies[i];

        // Each 1-pole lowpass: H(s) = wc / (s + wc)
        // Discrete approximation for visualization
        const float g = wc / sampleRate / 2.0f;
        const float g_denom = g / (1.0f + g);

        // 4-pole cascade with feedback
        std::complex<float> H = onePoleLowpass(omega, wc);
        H = H * H * H * H;  // 4 poles

        // Feedback approximation (creates the resonance peak)
        const float denom = 1.0f + k * std::exp(-omega / wc);
        H = H / denom;

        magnitudes[i] = magnitudeToDb(std::abs(H));
        phases[i] = std::arg(H) * 180.0f / juce::MathConstants<float>::pi;
    }
}

void FilterResponseCalculator::computeMS20Response(
    float cutoff, float resonance, double sampleRate,
    std::vector<float>& frequencies, std::vector<float>& magnitudes,
    std::vector<float>& phases)
{
    const size_t numPoints = frequencies.size();
    const float wc = 2.0f * juce::MathConstants<float>::pi * cutoff;
    const float k = 2.0f * resonance;

    // MS-20 is a 3-pole diode ladder with aggressive resonance
    for (size_t i = 0; i < numPoints; ++i) {
        const float omega = 2.0f * juce::MathConstants<float>::pi * frequencies[i];

        // 3 cascaded 1-pole sections with diode nonlinearity approximation
        std::complex<float> H = onePoleLowpass(omega, wc);
        H = H * H * H;  // 3 poles

        // MS-20 feedback is more aggressive
        const float denom = 1.0f + k * std::exp(-0.8f * omega / wc);
        H = H / denom;

        magnitudes[i] = magnitudeToDb(std::abs(H));
        phases[i] = std::arg(H) * 180.0f / juce::MathConstants<float>::pi;
    }
}

void FilterResponseCalculator::computeProphetResponse(
    float cutoff, float resonance, double sampleRate,
    std::vector<float>& frequencies, std::vector<float>& magnitudes,
    std::vector<float>& phases)
{
    const size_t numPoints = frequencies.size();
    const float wc = 2.0f * juce::MathConstants<float>::pi * cutoff;

    // Prophet-5 CEM 3320: 4-pole with distinctive resonance
    for (size_t i = 0; i < numPoints; ++i) {
        const float omega = 2.0f * juce::MathConstants<float>::pi * frequencies[i];

        // CEM 3320 uses two 2-pole sections
        // Each 2-pole section creates a smoother response than 1-pole cascade
        const float g = std::tan(wc / sampleRate / 4.0f);
        const float k = resonance * (1.0f - 0.15f * g);

        // Approximate as 4-pole with smoother rolloff
        std::complex<float> H = onePoleLowpass(omega, wc);
        H = H * H * H * H;

        // Prophet has creamier resonance - slightly damped
        const float denom = 1.0f + k * std::exp(-0.9f * omega / wc);
        H = H / denom;

        magnitudes[i] = magnitudeToDb(std::abs(H));
        phases[i] = std::arg(H) * 180.0f / juce::MathConstants<float>::pi;
    }
}

void FilterResponseCalculator::computeSEMResponse(
    float cutoff, float resonance, FilterType type, double sampleRate,
    std::vector<float>& frequencies, std::vector<float>& magnitudes,
    std::vector<float>& phases)
{
    // SEM is a state variable filter - similar to SVF but different character
    const size_t numPoints = frequencies.size();
    const float wc = 2.0f * juce::MathConstants<float>::pi * cutoff;
    const float k = resonance * (1.0f + wc / sampleRate);

    for (size_t i = 0; i < numPoints; ++i) {
        const float omega = 2.0f * juce::MathConstants<float>::pi * frequencies[i];
        std::complex<float> Hlp, Hbp, Hhp;
        computeSVFTransferFunction(omega, wc, k, Hlp, Hbp, Hhp);

        std::complex<float> H;
        switch (type) {
            case FilterType::Lowpass:
                H = Hlp;
                break;
            case FilterType::Bandpass:
                H = Hbp;
                break;
            case FilterType::Highpass:
                H = Hhp;
                break;
            default:
                H = Hlp;
        }

        magnitudes[i] = magnitudeToDb(std::abs(H));
        phases[i] = std::arg(H) * 180.0f / juce::MathConstants<float>::pi;
    }
}

void FilterResponseCalculator::computeTB303Response(
    float cutoff, float resonance, double sampleRate,
    std::vector<float>& frequencies, std::vector<float>& magnitudes,
    std::vector<float>& phases)
{
    const size_t numPoints = frequencies.size();
    const float wc = 2.0f * juce::MathConstants<float>::pi * cutoff;

    // TB-303: 3-pole diode ladder with very aggressive resonance
    for (size_t i = 0; i < numPoints; ++i) {
        const float omega = 2.0f * juce::MathConstants<float>::pi * frequencies[i];

        std::complex<float> H = onePoleLowpass(omega, wc);
        H = H * H * H;  // 3 poles

        // TB-303 has the most aggressive resonance
        const float k = resonance * 2.0f;
        const float denom = 1.0f + k * std::exp(-0.7f * omega / wc);
        H = H / denom;

        magnitudes[i] = magnitudeToDb(std::abs(H));
        phases[i] = std::arg(H) * 180.0f / juce::MathConstants<float>::pi;
    }
}

std::complex<float> FilterResponseCalculator::onePoleLowpass(float omega, float wc) {
    // H(s) = wc / (s + wc)
    // H(jω) = wc / (jω + wc)
    const float denom = wc * wc + omega * omega;
    return std::complex<float>(wc * wc / denom, -wc * omega / denom);
}

std::complex<float> FilterResponseCalculator::onePoleHighpass(float omega, float wc) {
    // H(s) = s / (s + wc)
    // H(jω) = jω / (jω + wc)
    const float denom = wc * wc + omega * omega;
    return std::complex<float>(-wc * omega / denom, wc * omega / denom);
}

void FilterResponseCalculator::computeSVFTransferFunction(
    float omega, float wc, float k,
    std::complex<float>& Hlp, std::complex<float>& Hbp, std::complex<float>& Hhp)
{
    // State Variable Filter transfer functions
    // Based on Chamberlin's digital SVF with bilinear transform

    const float T = 1.0f;  // Normalized sample time
    const float g = std::tan(wc * T / 2.0f);

    // Transfer functions
    std::complex<float> s(0, omega / wc);  // Normalized frequency

    // SVF responses (continuous approximation)
    std::complex<float> denom = s * s + g * s + g * g;
    Hlp = (g * g) / denom;
    Hbp = (g * s) / denom;
    Hhp = (s * s) / denom;

    // Apply resonance feedback
    denom = s * s + g * (1.0f + k) * s + g * g;
    Hlp = (g * g) / denom;
    Hbp = (g * s) / denom;
    Hhp = (s * s) / denom;
}

//==============================================================================
// FilterResponseDisplay Implementation
//==============================================================================

FilterResponseDisplay::FilterResponseDisplay() {
    // Initialize frequency points (logarithmic spacing)
    response1_.resize(numFreqPoints_);
    response2_.resize(numFreqPoints_);

    float logMin = std::log(minFreq_);
    float logMax = std::log(maxFreq_);
    float logRange = logMax - logMin;

    for (int i = 0; i < numFreqPoints_; ++i) {
        float logFreq = logMin + (logRange * i / (numFreqPoints_ - 1));
        response1_.frequencies[i] = std::exp(logFreq);
        response2_.frequencies[i] = response1_.frequencies[i];
    }

    // Start timer for updates (60 FPS for smooth animations)
    startTimerHz(60);
}

FilterResponseDisplay::~FilterResponseDisplay() {
    stopTimer();
}

void FilterResponseDisplay::setFilterParameters(float cutoff, float resonance,
                                                FilterType type, FilterModelType model) {
    cutoff_.store(cutoff);
    resonance_.store(resonance);
    filterType_ = type;
    filterModel_ = model;
    updateResponse();
}

void FilterResponseDisplay::setFilter2Parameters(float cutoff, float resonance,
                                                 FilterType type, FilterModelType model) {
    cutoff2_.store(cutoff);
    resonance2_.store(resonance);
    filterType2_ = type;
    filterModel2_ = model;
    updateResponse();
}

void FilterResponseDisplay::setFrequencyRange(float minHz, float maxHz) {
    minFreq_ = juce::jmax(10.0f, minHz);
    maxFreq_ = juce::jmin(40000.0f, maxHz);

    // Regenerate frequency points
    float logMin = std::log(minFreq_);
    float logMax = std::log(maxFreq_);
    float logRange = logMax - logMin;

    for (int i = 0; i < numFreqPoints_; ++i) {
        float logFreq = logMin + (logRange * i / (numFreqPoints_ - 1));
        response1_.frequencies[i] = std::exp(logFreq);
        response2_.frequencies[i] = response1_.frequencies[i];
    }

    needsUpdate_ = true;
}

void FilterResponseDisplay::setDbRange(float minDb, float maxDb) {
    minDb_ = minDb;
    maxDb_ = maxDb;
    markDirty();
}

void FilterResponseDisplay::setSpectrumData(const std::vector<float>& spectrum) {
    spectrumData_ = spectrum;
    markDirty();
}

void FilterResponseDisplay::timerCallback() {
    // Update animation phase for smooth 60fps effects
    animationPhase_ += 0.016f; // ~60fps
    if (animationPhase_ > 1.0f) animationPhase_ -= 1.0f;

    // Pulsing glow effect
    glowPulse_ = 0.5f + 0.5f * std::sin(animationPhase_ * juce::MathConstants<float>::twoPi);

    if (needsUpdate_) {
        computeResponse();
        needsUpdate_ = false;
    }

    // Always mark dirty for smooth animations
    markDirty();
}

void FilterResponseDisplay::updateResponse() {
    needsUpdate_ = true;
}

void FilterResponseDisplay::computeResponse() {
    response1_.cutoffFrequency = cutoff_.load();
    response1_.resonanceValue = resonance_.load();
    response1_.filterType = filterType_;
    response1_.filterModel = filterModel_;

    // Compute response based on filter model
    switch (filterModel_) {
        case FilterModelType::MoogLadder:
            FilterResponseCalculator::computeMoogLadderResponse(
                response1_.cutoffFrequency, response1_.resonanceValue,
                sampleRate_, response1_.frequencies, response1_.magnitudes,
                response1_.phases);
            break;

        case FilterModelType::MS20:
            FilterResponseCalculator::computeMS20Response(
                response1_.cutoffFrequency, response1_.resonanceValue,
                sampleRate_, response1_.frequencies, response1_.magnitudes,
                response1_.phases);
            break;

        case FilterModelType::Prophet:
            FilterResponseCalculator::computeProphetResponse(
                response1_.cutoffFrequency, response1_.resonanceValue,
                sampleRate_, response1_.frequencies, response1_.magnitudes,
                response1_.phases);
            break;

        case FilterModelType::SEM:
            FilterResponseCalculator::computeSEMResponse(
                response1_.cutoffFrequency, response1_.resonanceValue,
                filterType_, sampleRate_, response1_.frequencies,
                response1_.magnitudes, response1_.phases);
            break;

        case FilterModelType::TB303:
            FilterResponseCalculator::computeTB303Response(
                response1_.cutoffFrequency, response1_.resonanceValue,
                sampleRate_, response1_.frequencies, response1_.magnitudes,
                response1_.phases);
            break;

        case FilterModelType::SVF:
        case FilterModelType::Ladder:
        default:
            FilterResponseCalculator::computeSVFResponse(
                response1_.cutoffFrequency, response1_.resonanceValue,
                filterType_, sampleRate_, response1_.frequencies,
                response1_.magnitudes, response1_.phases);
            break;
    }

    // Compute second filter if in dual mode
    if (dualFilterMode_) {
        response2_.cutoffFrequency = cutoff2_.load();
        response2_.resonanceValue = resonance2_.load();
        response2_.filterType = filterType2_;
        response2_.filterModel = filterModel2_;

        switch (filterModel2_) {
            case FilterModelType::MoogLadder:
                FilterResponseCalculator::computeMoogLadderResponse(
                    response2_.cutoffFrequency, response2_.resonanceValue,
                    sampleRate_, response2_.frequencies, response2_.magnitudes,
                    response2_.phases);
                break;

            case FilterModelType::MS20:
                FilterResponseCalculator::computeMS20Response(
                    response2_.cutoffFrequency, response2_.resonanceValue,
                    sampleRate_, response2_.frequencies, response2_.magnitudes,
                    response2_.phases);
                break;

            case FilterModelType::Prophet:
                FilterResponseCalculator::computeProphetResponse(
                    response2_.cutoffFrequency, response2_.resonanceValue,
                    sampleRate_, response2_.frequencies, response2_.magnitudes,
                    response2_.phases);
                break;

            case FilterModelType::SEM:
                FilterResponseCalculator::computeSEMResponse(
                    response2_.cutoffFrequency, response2_.resonanceValue,
                    filterType2_, sampleRate_, response2_.frequencies,
                    response2_.magnitudes, response2_.phases);
                break;

            case FilterModelType::TB303:
                FilterResponseCalculator::computeTB303Response(
                    response2_.cutoffFrequency, response2_.resonanceValue,
                    sampleRate_, response2_.frequencies, response2_.magnitudes,
                    response2_.phases);
                break;

            case FilterModelType::SVF:
            case FilterModelType::Ladder:
            default:
                FilterResponseCalculator::computeSVFResponse(
                    response2_.cutoffFrequency, response2_.resonanceValue,
                    filterType2_, sampleRate_, response2_.frequencies,
                    response2_.magnitudes, response2_.phases);
                break;
        }
    }
}

#ifdef ZENITH_USE_SKIA

void FilterResponseDisplay::drawSkia(SkCanvas* canvas) {
    SkRect bounds = SkRect::MakeWH(getWidth(), getHeight());

    // Draw background
    drawBackground(canvas);

    // Draw grid
    if (showGrid_) {
        drawGrid(canvas);
    }

    // Draw spectrum overlay if available
    if (!spectrumData_.empty()) {
        drawSpectrumOverlay(canvas);
    }

    // Draw filter response curves
    drawResponseCurve(canvas, response1_, curve1Color_, bounds);

    if (dualFilterMode_) {
        drawResponseCurve(canvas, response2_, curve2Color_, bounds);
    }

    // Draw phase if enabled
    if (showPhase_) {
        drawPhaseCurve(canvas, response1_, bounds);
        if (dualFilterMode_) {
            drawPhaseCurve(canvas, response2_, bounds);
        }
    }

    // Draw labels
    drawLabels(canvas);
}

void FilterResponseDisplay::drawBackground(SkCanvas* canvas) {
    SkPaint paint;
    paint.setColor(backgroundColor_);
    paint.setStyle(SkPaint::kFill_Style);
    canvas->drawRect(SkRect::MakeWH(getWidth(), getHeight()), paint);
}

void FilterResponseDisplay::drawGrid(SkCanvas* canvas) {
    if (getWidth() <= 0 || getHeight() <= 0) return;

    const float width = getWidth();
    const float height = getHeight();

    // Premium grid with subtle gradient
    SkPaint gridPaint, majorGridPaint;
    gridPaint.setColor(gridColor_);
    gridPaint.setStyle(SkPaint::kStroke_Style);
    gridPaint.setStrokeWidth(0.8f);
    gridPaint.setAntiAlias(true);

    majorGridPaint.setColor(gridMajorColor_);
    majorGridPaint.setStyle(SkPaint::kStroke_Style);
    majorGridPaint.setStrokeWidth(1.2f);
    majorGridPaint.setAntiAlias(true);

    // Frequency grid lines (logarithmic) with opacity based on importance
    const float freqs[] = {20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000};

    for (float freq : freqs) {
        if (freq < minFreq_ || freq > maxFreq_) continue;
        float x = freqToX(freq);

        bool isMajor = (freq == 100 || freq == 1000 || freq == 10000);

        if (isMajor) {
            canvas->drawLine(x, 0, x, height, majorGridPaint);
        } else {
            canvas->drawLine(x, 0, x, height, gridPaint);
        }
    }

    // dB grid lines with varying opacity
    const int dbStep = 10;
    for (int db = static_cast<int>(minDb_); db <= static_cast<int>(maxDb_); db += dbStep) {
        float y = dbToY(static_cast<float>(db));

        if (db == 0) {
            // 0 dB line - special treatment
            SkPaint zeroLinePaint;
            zeroLinePaint.setColor(SkColorSetARGB(200, 140, 140, 150));
            zeroLinePaint.setStyle(SkPaint::kStroke_Style);
            zeroLinePaint.setStrokeWidth(1.5f);
            zeroLinePaint.setAntiAlias(true);
            canvas->drawLine(0, y, width, y, zeroLinePaint);
        } else {
            canvas->drawLine(0, y, width, y, gridPaint);
        }
    }

    // Subtle vignette effect at edges
    SkRect vignetteRect = SkRect::MakeWH(width, height);
    SkPaint vignettePaint;
    vignettePaint.setColor(SkColorSetARGB(30, 0, 0, 0));
    vignettePaint.setStyle(SkPaint::kFill_Style);
    canvas->drawRect(vignetteRect, vignettePaint);
}

void FilterResponseDisplay::drawResponseCurve(SkCanvas* canvas,
                                               const FilterResponseData& data,
                                               SkColor curveColor,
                                               const SkRect& bounds) {
    if (data.magnitudes.empty() || getWidth() <= 0 || getHeight() <= 0) return;

    SkPath path;

    // Build the main path with smooth curves
    bool started = false;
    for (size_t i = 0; i < data.magnitudes.size(); ++i) {
        float x = freqToX(data.frequencies[i]);
        float y = dbToY(data.magnitudes[i]);

        if (!started) {
            path.moveTo(x, y);
            started = true;
        } else {
            path.lineTo(x, y);
        }
    }

    // ==================== PREMIUM GRADIENT FILL ====================
    SkPoint points[2] = {
        {0, dbToY(maxDb_)},
        {0, dbToY(minDb_)}
    };

    SkColor gradStart = SkColorSetARGB(120, SkColorGetR(curveColor),
                                       SkColorGetG(curveColor),
                                       SkColorGetB(curveColor));
    SkColor gradEnd = SkColorSetARGB(10, SkColorGetR(curveColor),
                                     SkColorGetG(curveColor),
                                     SkColorGetB(curveColor));

    SkColor colors[2] = {gradStart, gradEnd};
    float positions[2] = {0.0f, 1.0f};

    auto gradient = SkGradientShader::MakeLinear(points, colors, positions, 2,
                                                   SkTileMode::kClamp);

    SkPath fillPath = path;
    fillPath.lineTo(getWidth(), dbToY(minDb_));
    fillPath.lineTo(0, dbToY(minDb_));
    fillPath.close();

    SkPaint fillPaint;
    fillPaint.setShader(gradient);
    fillPaint.setAntiAlias(true);
    canvas->drawPath(fillPath, fillPaint);

    // ==================== GLOW EFFECTS ====================
    // Outer glow (larger blur)
    SkPath glowPath = path;
    SkPaint glowPaintOuter;
    glowPaintOuter.setColor(curveColor);
    glowPaintOuter.setStyle(SkPaint::kStroke_Style);
    glowPaintOuter.setStrokeWidth(8.0f);
    glowPaintOuter.setAlpha(static_cast<U8CPU>(40 + glowPulse_ * 30));
    glowPaintOuter.setAntiAlias(true);

    // Apply blur using mask filter
    auto blurFilter = SkMaskFilter::MakeBlur(SkBlurStyle::kNormal_SkBlurStyle,
                                             6.0f);
    glowPaintOuter.setMaskFilter(blurFilter);
    canvas->drawPath(glowPath, glowPaintOuter);

    // Inner glow (brighter core)
    SkPaint glowPaintInner;
    glowPaintInner.setColor(curveColor);
    glowPaintInner.setStyle(SkPaint::kStroke_Style);
    glowPaintInner.setStrokeWidth(3.0f);
    glowPaintInner.setAlpha(static_cast<U8CPU>(100 + glowPulse_ * 50));
    glowPaintInner.setAntiAlias(true);

    auto blurFilterInner = SkMaskFilter::MakeBlur(SkBlurStyle::kNormal_SkBlurStyle,
                                                   3.0f);
    glowPaintInner.setMaskFilter(blurFilterInner);
    canvas->drawPath(glowPath, glowPaintInner);

    // ==================== MAIN CURVE ====================
    // Bright core line
    SkPaint mainLinePaint;
    mainLinePaint.setColor(SkColorSetRGB(
        std::min(255u, SkColorGetR(curveColor) + 30),
        std::min(255u, SkColorGetG(curveColor) + 30),
        std::min(255u, SkColorGetB(curveColor) + 30)));
    mainLinePaint.setStyle(SkPaint::kStroke_Style);
    mainLinePaint.setStrokeWidth(2.0f);
    mainLinePaint.setAntiAlias(true);
    canvas->drawPath(path, mainLinePaint);

    // ==================== CUTOFF MARKER WITH ANIMATION ====================
    float cutoffX = freqToX(data.cutoffFrequency);
    size_t cutoffIdx = static_cast<size_t>(
        (std::log(data.cutoffFrequency) - std::log(minFreq_)) /
        (std::log(maxFreq_) - std::log(minFreq_)) * (numFreqPoints_ - 1));
    cutoffIdx = std::min(cutoffIdx, data.magnitudes.size() - 1);
    float cutoffY = dbToY(data.magnitudes[cutoffIdx]);

    // Animated ripple effect around cutoff
    float rippleRadius = 8.0f + 4.0f * glowPulse_;
    SkPaint ripplePaint;
    ripplePaint.setColor(curveColor);
    ripplePaint.setStyle(SkPaint::kStroke_Style);
    ripplePaint.setStrokeWidth(1.5f);
    ripplePaint.setAlpha(static_cast<U8CPU>(150 - glowPulse_ * 100));
    canvas->drawCircle(cutoffX, cutoffY, rippleRadius, ripplePaint);

    // Second ripple
    float rippleRadius2 = 12.0f + 6.0f * glowPulse_;
    ripplePaint.setAlpha(static_cast<U8CPU>(80 - glowPulse_ * 50));
    canvas->drawCircle(cutoffX, cutoffY, rippleRadius2, ripplePaint);

    // Center dot
    SkPaint centerDotPaint;
    centerDotPaint.setColor(SkColorSetRGB(255, 255, 255));
    centerDotPaint.setStyle(SkPaint::kFill_Style);
    canvas->drawCircle(cutoffX, cutoffY, 4.0f, centerDotPaint);

    // Glow around center dot
    SkPaint centerGlowPaint;
    centerGlowPaint.setColor(curveColor);
    centerGlowPaint.setStyle(SkPaint::kFill_Style);
    centerGlowPaint.setAlpha(180);
    auto centerBlur = SkMaskFilter::MakeBlur(SkBlurStyle::kNormal_SkBlurStyle, 4.0f);
    centerGlowPaint.setMaskFilter(centerBlur);
    canvas->drawCircle(cutoffX, cutoffY, 6.0f, centerGlowPaint);
}

void FilterResponseDisplay::drawSpectrumOverlay(SkCanvas* canvas) {
    if (spectrumData_.empty() || getWidth() <= 0 || getHeight() <= 0) return;

    SkPaint paint;
    paint.setColor(spectrumColor_);
    paint.setStyle(SkPaint::kFill_Style);
    paint.setAntiAlias(true);

    const float width = getWidth();
    const float barWidth = width / static_cast<float>(spectrumData_.size());

    for (size_t i = 0; i < spectrumData_.size(); ++i) {
        // Map spectrum bin to frequency
        float freq = minFreq_ * std::pow(maxFreq_ / minFreq_,
                                         static_cast<float>(i) / spectrumData_.size());
        float x = freqToX(freq);

        // Spectrum value is typically 0-1, map to dB range
        float db = minDb_ + spectrumData_[i] * (maxDb_ - minDb_);
        float y = dbToY(db);
        float barHeight = getHeight() - y;

        SkRect bar = SkRect::MakeXYWH(x, y, barWidth + 1, barHeight);
        canvas->drawRect(bar, paint);
    }
}

void FilterResponseDisplay::drawPhaseCurve(SkCanvas* canvas,
                                            const FilterResponseData& data,
                                            const SkRect& bounds) {
    if (data.phases.empty() || getWidth() <= 0 || getHeight() <= 0) return;

    SkPath path;
    SkPaint paint;
    paint.setColor(phaseColor_);
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(1.5f);
    paint.setAntiAlias(true);

    // Phase is displayed at bottom of graph
    const float phaseHeight = getHeight() * 0.2f;
    const float phaseBottom = getHeight();
    const float phaseTop = phaseBottom - phaseHeight;

    bool started = false;
    for (size_t i = 0; i < data.phases.size(); ++i) {
        float x = freqToX(data.frequencies[i]);
        // Map phase (-180 to 180) to Y range
        float normalizedPhase = (data.phases[i] + 180.0f) / 360.0f;
        float y = phaseBottom - normalizedPhase * phaseHeight;

        if (!started) {
            path.moveTo(x, y);
            started = true;
        } else {
            path.lineTo(x, y);
        }
    }

    canvas->drawPath(path, paint);
}

void FilterResponseDisplay::drawLabels(SkCanvas* canvas) {
    if (getWidth() <= 0 || getHeight() <= 0) return;

    SkFont font;
    font.setSize(11.0f);

    SkPaint textPaint;
    textPaint.setColor(textColor_);
    textPaint.setAntiAlias(true);

    const float width = getWidth();
    const float height = getHeight();
    const float margin = 5.0f;

    // Frequency labels
    const float freqs[] = {20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000};

    for (float freq : freqs) {
        if (freq < minFreq_ || freq > maxFreq_) continue;
        float x = freqToX(freq);

        juce::String label = formatFrequency(freq);
        canvas->drawSimpleText(label.toUTF8(), label.length(), SkTextEncoding::kUTF8,
                        x, height - margin, font, textPaint);
    }

    // dB labels
    const int dbStep = 20;
    for (int db = static_cast<int>(minDb_); db <= static_cast<int>(maxDb_); db += dbStep) {
        float y = dbToY(static_cast<float>(db));

        juce::String label = juce::String(db) + " dB";
        canvas->drawSimpleText(label.toUTF8(), label.length(), SkTextEncoding::kUTF8,
                        margin, y + 4, font, textPaint);
    }

    // Filter info label
    juce::String info;
    info += juce::String(response1_.cutoffFrequency) + " Hz, ";
    info += juce::String(int(response1_.resonanceValue * 100)) + "% Res";

    SkFont infoFont;
    infoFont.setSize(13.0f);
    SkPaint infoPaint;
    infoPaint.setColor(curve1Color_);
    infoPaint.setAntiAlias(true);
    canvas->drawSimpleText(info.toUTF8(), info.length(), SkTextEncoding::kUTF8,
                    width - 150, margin + 15, infoFont, infoPaint);
}

float FilterResponseDisplay::freqToX(float freq) const {
    float logFreq = std::log(freq);
    float logMin = std::log(minFreq_);
    float logMax = std::log(maxFreq_);
    return ((logFreq - logMin) / (logMax - logMin)) * getWidth();
}

float FilterResponseDisplay::xToFreq(float x) const {
    float logMin = std::log(minFreq_);
    float logMax = std::log(maxFreq_);
    float normalizedX = x / getWidth();
    return std::exp(logMin + normalizedX * (logMax - logMin));
}

float FilterResponseDisplay::dbToY(float db) const {
    float normalizedDb = (db - minDb_) / (maxDb_ - minDb_);
    return getHeight() * (1.0f - normalizedDb);
}

float FilterResponseDisplay::yToDb(float y) const {
    float normalizedY = 1.0f - (y / getHeight());
    return minDb_ + normalizedY * (maxDb_ - minDb_);
}

juce::String FilterResponseDisplay::formatFrequency(float freq) const {
    if (freq >= 1000.0f) {
        return juce::String(freq / 1000.0f, 1) + "k";
    }
    return juce::String(static_cast<int>(freq));
}

#endif // ZENITH_USE_SKIA

} // namespace zenith
