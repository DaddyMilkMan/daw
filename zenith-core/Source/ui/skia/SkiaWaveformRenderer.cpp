/**
 * @file SkiaWaveformRenderer.cpp
 * @brief Implementation of GPU-accelerated waveform rendering
 */

#include "SkiaWaveformRenderer.h"
#include "SkiaTheme.h"

#ifdef ZENITH_USE_SKIA
    #include <include/core/SkPaint.h>
    #include <include/core/SkPath.h>
    #include <include/core/SkShader.h>
    #include <include/core/SkGradientShader.h>
    #include <include/core/SkMaskFilter.h>
    #include <include/effects/SkGradientShader.h>
#endif

#include <algorithm>
#include <cmath>

namespace zenith {

//==============================================================================
// WaveformData implementation
//==============================================================================

void WaveformData::generateFromAudioBuffer(const juce::AudioBuffer<float>& buffer, int detailLevel)
{
    clear();

    if (buffer.getNumSamples() == 0)
        return;

    numChannels = buffer.getNumChannels();
    sampleRate = 44100; // Default, should be passed in from actual source
    durationSeconds = buffer.getNumSamples() / static_cast<double>(sampleRate);

    // Calculate samples per segment based on detail level
    // Detail level 1 (low): ~1000 segments
    // Detail level 5 (high): ~10000 segments
    int targetSegments = 1000 * detailLevel;
    int samplesPerSegment = std::max(1, buffer.getNumSamples() / targetSegments);

    int numSegments = (buffer.getNumSamples() + samplesPerSegment - 1) / samplesPerSegment;
    samples.reserve(numSegments);

    // Generate min/max/RMS for each segment
    for (int segment = 0; segment < numSegments; ++segment)
    {
        int startSample = segment * samplesPerSegment;
        int endSample = std::min(startSample + samplesPerSegment, buffer.getNumSamples());

        MinMaxPair pair;
        pair.min = 0.0f;
        pair.max = 0.0f;
        float sumSquares = 0.0f;
        int sampleCount = 0;

        // Analyze all channels
        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float* channelData = buffer.getReadPointer(ch);

            for (int i = startSample; i < endSample; ++i)
            {
                float sample = channelData[i];
                pair.min = std::min(pair.min, sample);
                pair.max = std::max(pair.max, sample);
                sumSquares += sample * sample;
                ++sampleCount;
            }
        }

        // Calculate RMS
        if (sampleCount > 0)
        {
            pair.rms = std::sqrt(sumSquares / sampleCount);
        }
        else
        {
            pair.rms = 0.0f;
        }

        samples.push_back(pair);
    }
}

void WaveformData::generateFromAudioSource(juce::AudioFormatReader* source, int detailLevel)
{
    if (!source)
        return;

    // Read entire file into buffer (for simplicity; could be optimized for large files)
    const int bufferSize = static_cast<int>(source->lengthInSamples);
    juce::AudioBuffer<float> buffer(static_cast<int>(source->numChannels), bufferSize);

    source->read(&buffer, 0, bufferSize, 0, true, true);

    sampleRate = static_cast<int>(source->sampleRate);
    durationSeconds = source->lengthInSamples / source->sampleRate;

    generateFromAudioBuffer(buffer, detailLevel);
}

void WaveformData::clear()
{
    samples.clear();
    durationSeconds = 0.0;
    numChannels = 1;
}

#ifdef ZENITH_USE_SKIA

//==============================================================================
// SkiaWaveformRenderer implementation
//==============================================================================

SkiaWaveformRenderer::SkiaWaveformRenderer()
{
    lastAnimationTime_ = juce::Time::getCurrentTime();
}

SkiaWaveformRenderer::~SkiaWaveformRenderer() = default;

void SkiaWaveformRenderer::setAnimationProgress(float progress)
{
    animationProgress_ = juce::jlimit(0.0f, 1.0f, progress);
}

//==============================================================================
// Main rendering methods
//==============================================================================

void SkiaWaveformRenderer::render(SkCanvas* canvas,
                                  const WaveformData& waveform,
                                  const SkRect& bounds,
                                  const WaveformRenderOptions& options)
{
    if (!canvas || !waveform.isValid() || bounds.isEmpty())
        return;

    renderTimeRange(canvas, waveform, bounds, 0.0, waveform.durationSeconds, options);
}

void SkiaWaveformRenderer::renderTimeRange(SkCanvas* canvas,
                                          const WaveformData& waveform,
                                          const SkRect& bounds,
                                          double startTime,
                                          double endTime,
                                          const WaveformRenderOptions& options)
{
    if (!canvas || !waveform.isValid() || bounds.isEmpty())
        return;

    // Convert time range to sample indices
    size_t startSample = getSampleIndexForTime(waveform, startTime);
    size_t endSample = getSampleIndexForTime(waveform, endTime);

    if (startSample >= waveform.samples.size())
        return;

    endSample = std::min(endSample, waveform.samples.size());

    // Render based on style
    switch (options.style)
    {
        case WaveformStyle::Filled:
            renderFilled(canvas, waveform, bounds, startSample, endSample, options);
            break;

        case WaveformStyle::Outline:
            renderOutline(canvas, waveform, bounds, startSample, endSample, options);
            break;

        case WaveformStyle::Peaks:
            renderPeaks(canvas, waveform, bounds, startSample, endSample, options);
            break;

        case WaveformStyle::RMS:
            renderRMS(canvas, waveform, bounds, startSample, endSample, options);
            break;

        case WaveformStyle::Hybrid:
            renderHybrid(canvas, waveform, bounds, startSample, endSample, options);
            break;
    \n    default: break;\n}

    // Draw center line if enabled
    if (options.showCenterLine)
    {
        SkPaint centerLinePaint;
        centerLinePaint.setAntiAlias(true);
        centerLinePaint.setColor(SkColorSetA(options.outlineColor, 80));
        centerLinePaint.setStrokeWidth(0.5f);

        float centerY = bounds.centerY();
        canvas->drawLine(bounds.left(), centerY, bounds.right(), centerY, centerLinePaint);
    }
}

//==============================================================================
// Style-specific rendering
//==============================================================================

void SkiaWaveformRenderer::renderFilled(SkCanvas* canvas,
                                       const WaveformData& waveform,
                                       const SkRect& bounds,
                                       size_t startSample,
                                       size_t endSample,
                                       const WaveformRenderOptions& options)
{
    SkPath path = createWaveformPath(waveform, bounds, startSample, endSample, true);

    // Apply glow effect if enabled
    if (options.enableGlow)
    {
        applyGlowEffect(canvas, path, options);
    }

    // Create fill paint
    SkPaint fillPaint;
    fillPaint.setAntiAlias(options.antiAlias);
    fillPaint.setStyle(SkPaint::kFill_Style);

    if (options.useGradient)
    {
        // Vertical gradient from top to bottom
        SkPoint points[2] = {
            SkPoint::Make(bounds.centerX(), bounds.top()),
            SkPoint::Make(bounds.centerX(), bounds.bottom())
        };
        SkColor colors[2] = {options.gradientTop, options.gradientBottom};
        SkScalar positions[2] = {0.0f, 1.0f};

        sk_sp<SkShader> shader = SkGradientShader::MakeLinear(
            points, colors, positions, 2, SkTileMode::kClamp
        );
        fillPaint.setShader(shader);
    }
    else
    {
        fillPaint.setColor(options.fillColor);
    }

    fillPaint.setAlpha(static_cast<uint8_t>(255 * options.opacity * options.animationProgress));

    canvas->drawPath(path, fillPaint);
}

void SkiaWaveformRenderer::renderOutline(SkCanvas* canvas,
                                        const WaveformData& waveform,
                                        const SkRect& bounds,
                                        size_t startSample,
                                        size_t endSample,
                                        const WaveformRenderOptions& options)
{
    SkPath path = createWaveformPath(waveform, bounds, startSample, endSample, false);

    SkPaint outlinePaint;
    outlinePaint.setAntiAlias(options.antiAlias);
    outlinePaint.setStyle(SkPaint::kStroke_Style);
    outlinePaint.setStrokeWidth(options.outlineWidth);
    outlinePaint.setColor(options.outlineColor);
    outlinePaint.setAlpha(static_cast<uint8_t>(255 * options.opacity * options.animationProgress));

    canvas->drawPath(path, outlinePaint);
}

void SkiaWaveformRenderer::renderPeaks(SkCanvas* canvas,
                                      const WaveformData& waveform,
                                      const SkRect& bounds,
                                      size_t startSample,
                                      size_t endSample,
                                      const WaveformRenderOptions& options)
{
    SkPaint peakPaint;
    peakPaint.setAntiAlias(options.antiAlias);
    peakPaint.setStyle(SkPaint::kStroke_Style);
    peakPaint.setStrokeWidth(options.peakWidth);
    peakPaint.setColor(options.peakColor);
    peakPaint.setAlpha(static_cast<uint8_t>(255 * options.opacity * options.animationProgress));

    size_t numSamples = endSample - startSample;
    float centerY = bounds.centerY();
    float heightScale = bounds.height() / 2.0f;

    for (size_t i = 0; i < numSamples; ++i)
    {
        size_t sampleIdx = startSample + i;
        if (sampleIdx >= waveform.samples.size())
            break;

        float x = bounds.left() + (i / static_cast<float>(numSamples)) * bounds.width();
        float yMin = centerY - waveform.samples[sampleIdx].min * heightScale;
        float yMax = centerY - waveform.samples[sampleIdx].max * heightScale;

        canvas->drawLine(x, yMin, x, yMax, peakPaint);
    }
}

void SkiaWaveformRenderer::renderRMS(SkCanvas* canvas,
                                    const WaveformData& waveform,
                                    const SkRect& bounds,
                                    size_t startSample,
                                    size_t endSample,
                                    const WaveformRenderOptions& options)
{
    size_t numSamples = endSample - startSample;
    float centerY = bounds.centerY();
    float heightScale = bounds.height() / 2.0f;

    SkPath rmsPath;
    bool first = true;

    // Create RMS envelope path
    for (size_t i = 0; i < numSamples; ++i)
    {
        size_t sampleIdx = startSample + i;
        if (sampleIdx >= waveform.samples.size())
            break;

        float x = bounds.left() + (i / static_cast<float>(numSamples)) * bounds.width();
        float rms = waveform.samples[sampleIdx].rms;
        float yTop = centerY - rms * heightScale;
        float yBottom = centerY + rms * heightScale;

        if (first)
        {
            rmsPath.moveTo(x, yTop);
            first = false;
        }
        else
        {
            rmsPath.lineTo(x, yTop);
        }
    }

    // Add bottom envelope
    for (int i = static_cast<int>(numSamples) - 1; i >= 0; --i)
    {
        size_t sampleIdx = startSample + i;
        if (sampleIdx >= waveform.samples.size())
            continue;

        float x = bounds.left() + (i / static_cast<float>(numSamples)) * bounds.width();
        float rms = waveform.samples[sampleIdx].rms;
        float yBottom = centerY + rms * heightScale;

        rmsPath.lineTo(x, yBottom);
    }

    rmsPath.close();

    SkPaint rmsPaint;
    rmsPaint.setAntiAlias(options.antiAlias);
    rmsPaint.setStyle(SkPaint::kFill_Style);
    rmsPaint.setColor(options.rmsColor);
    rmsPaint.setAlpha(static_cast<uint8_t>(255 * options.opacity * options.animationProgress * 0.6f));

    canvas->drawPath(rmsPath, rmsPaint);
}

void SkiaWaveformRenderer::renderHybrid(SkCanvas* canvas,
                                       const WaveformData& waveform,
                                       const SkRect& bounds,
                                       size_t startSample,
                                       size_t endSample,
                                       const WaveformRenderOptions& options)
{
    // Draw RMS first (semi-transparent)
    WaveformRenderOptions rmsOpts = options;
    rmsOpts.opacity *= 0.4f;
    renderRMS(canvas, waveform, bounds, startSample, endSample, rmsOpts);

    // Draw peaks on top
    renderPeaks(canvas, waveform, bounds, startSample, endSample, options);
}

//==============================================================================
// Helper methods
//==============================================================================

SkPath SkiaWaveformRenderer::createWaveformPath(const WaveformData& waveform,
                                               const SkRect& bounds,
                                               size_t startSample,
                                               size_t endSample,
                                               bool includeBottom)
{
    SkPath path;

    size_t numSamples = endSample - startSample;
    if (numSamples == 0)
        return path;

    float centerY = bounds.centerY();
    float heightScale = bounds.height() / 2.0f;

    // Draw top envelope (max values)
    bool first = true;
    for (size_t i = 0; i < numSamples; ++i)
    {
        size_t sampleIdx = startSample + i;
        if (sampleIdx >= waveform.samples.size())
            break;

        float x = bounds.left() + (i / static_cast<float>(numSamples)) * bounds.width();
        float y = centerY - waveform.samples[sampleIdx].max * heightScale;

        if (first)
        {
            path.moveTo(x, y);
            first = false;
        }
        else
        {
            path.lineTo(x, y);
        }
    }

    if (includeBottom)
    {
        // Draw bottom envelope (min values) in reverse
        for (int i = static_cast<int>(numSamples) - 1; i >= 0; --i)
        {
            size_t sampleIdx = startSample + i;
            if (sampleIdx >= waveform.samples.size())
                continue;

            float x = bounds.left() + (i / static_cast<float>(numSamples)) * bounds.width();
            float y = centerY - waveform.samples[sampleIdx].min * heightScale;

            path.lineTo(x, y);
        }

        path.close();
    }

    return path;
}

void SkiaWaveformRenderer::applyGlowEffect(SkCanvas* canvas,
                                          const SkPath& path,
                                          const WaveformRenderOptions& options)
{
    // Draw multiple glow layers
    const int glowLayers = 2;

    for (int i = 0; i < glowLayers; ++i)
    {
        SkPaint glowPaint;
        glowPaint.setAntiAlias(true);
        glowPaint.setStyle(SkPaint::kStroke_Style);

        float layerOpacity = options.glowOpacity * (1.0f - i * 0.3f);
        float layerBlur = options.glowRadius * (1.0f + i * 0.4f);

        glowPaint.setColor(options.fillColor);
        glowPaint.setAlpha(static_cast<uint8_t>(255 * layerOpacity * options.animationProgress));
        glowPaint.setStrokeWidth(options.outlineWidth + i * 2.0f);

        if (layerBlur > 0.0f)
        {
            glowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, layerBlur));
        }

        canvas->drawPath(path, glowPaint);
    }
}

size_t SkiaWaveformRenderer::getSampleIndexForTime(const WaveformData& waveform, double timeSeconds) const
{
    if (waveform.durationSeconds <= 0.0 || waveform.samples.empty())
        return 0;

    double ratio = timeSeconds / waveform.durationSeconds;
    size_t index = static_cast<size_t>(ratio * waveform.samples.size());
    return std::min(index, waveform.samples.size() - 1);
}

double SkiaWaveformRenderer::getTimeForSampleIndex(const WaveformData& waveform, size_t index) const
{
    if (waveform.samples.empty())
        return 0.0;

    double ratio = index / static_cast<double>(waveform.samples.size());
    return ratio * waveform.durationSeconds;
}

void SkiaWaveformRenderer::renderMidiNotes(SkCanvas* canvas,
                                          const std::vector<std::tuple<int, double, double, int>>& notes,
                                          const SkRect& bounds,
                                          SkColor color)
{
    if (!canvas || notes.empty())
        return;

    SkPaint notePaint;
    notePaint.setAntiAlias(true);
    notePaint.setColor(color);

    // Simple MIDI note visualization (rectangles for now)
    for (const auto& note : notes)
    {
        // int pitch = std::get<0>(note);
        double start = std::get<1>(note);
        double length = std::get<2>(note);
        int velocity = std::get<3>(note);

        // Map note to bounds (simplified - would need proper pitch mapping)
        float x = bounds.left() + static_cast<float>(start / 10.0) * bounds.width();
        float width = static_cast<float>(length / 10.0) * bounds.width();
        float height = 4.0f;
        float y = bounds.centerY() - height / 2.0f;

        // Alpha based on velocity
        uint8_t alpha = static_cast<uint8_t>((velocity / 127.0f) * 255);
        notePaint.setAlpha(alpha);

        SkRect noteRect = SkRect::MakeXYWH(x, y, width, height);
        canvas->drawRect(noteRect, notePaint);
    }
}

#endif // ZENITH_USE_SKIA

} // namespace zenith

