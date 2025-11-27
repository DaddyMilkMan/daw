/**
 * @file SkiaWaveformRenderer.cpp
 * @brief GPU-accelerated waveform rendering for audio visualization
 */

#include "SkiaWaveformRenderer.h"

#ifdef ZENITH_USE_SKIA

#include <algorithm>
#include <cmath>
#include "include/effects/SkGradientShader.h"

namespace zenith {

//==============================================================================
// Construction
//==============================================================================

SkiaWaveformRenderer::SkiaWaveformRenderer() = default;

SkiaWaveformRenderer::~SkiaWaveformRenderer() = default;

//==============================================================================
// Data Management
//==============================================================================

void WaveformData::generateFromAudioBuffer(const juce::AudioBuffer<float>& buffer, int detailLevel)
{
    numChannels = buffer.getNumChannels();
    sampleRate = 44100;  // Default, should be passed in
    durationSeconds = static_cast<double>(buffer.getNumSamples()) / sampleRate;

    samples.clear();

    if (buffer.getNumSamples() == 0)
        return;

    int samplesPerSegment = std::max(1, buffer.getNumSamples() / (detailLevel * 100));
    int numSegments = (buffer.getNumSamples() + samplesPerSegment - 1) / samplesPerSegment;

    samples.resize(numSegments);

    for (int seg = 0; seg < numSegments; ++seg) {
        int startSample = seg * samplesPerSegment;
        int endSample = std::min(startSample + samplesPerSegment, buffer.getNumSamples());

        MinMaxPair& pair = samples[seg];
        pair.min = 0.0f;
        pair.max = 0.0f;
        pair.rms = 0.0f;

        float rmsSum = 0.0f;
        int count = 0;

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            const float* channelData = buffer.getReadPointer(ch);

            for (int i = startSample; i < endSample; ++i) {
                float sample = channelData[i];
                pair.min = std::min(pair.min, sample);
                pair.max = std::max(pair.max, sample);
                rmsSum += sample * sample;
                count++;
            }
        }

        if (count > 0)
            pair.rms = std::sqrt(rmsSum / count);
    }
}

void WaveformData::generateFromAudioSource(juce::AudioFormatReader* source, int detailLevel)
{
    if (!source) return;

    numChannels = source->numChannels;
    sampleRate = static_cast<int>(source->sampleRate);
    durationSeconds = static_cast<double>(source->lengthInSamples) / source->sampleRate;

    samples.clear();

    int samplesPerSegment = std::max(1, static_cast<int>(source->lengthInSamples) / (detailLevel * 100));
    int numSegments = (source->lengthInSamples + samplesPerSegment - 1) / samplesPerSegment;

    samples.resize(numSegments);

    juce::AudioBuffer<float> tempBuffer(numChannels, samplesPerSegment);

    for (int seg = 0; seg < numSegments; ++seg) {
        int startSample = seg * samplesPerSegment;
        int readSamples = std::min(samplesPerSegment, static_cast<int>(source->lengthInSamples - startSample));

        source->read(&tempBuffer, 0, readSamples, startSample, true, true);

        MinMaxPair& pair = samples[seg];
        pair.min = 0.0f;
        pair.max = 0.0f;
        pair.rms = 0.0f;

        float rmsSum = 0.0f;
        int count = 0;

        for (int ch = 0; ch < numChannels; ++ch) {
            const float* channelData = tempBuffer.getReadPointer(ch);

            for (int i = 0; i < readSamples; ++i) {
                float sample = channelData[i];
                pair.min = std::min(pair.min, sample);
                pair.max = std::max(pair.max, sample);
                rmsSum += sample * sample;
                count++;
            }
        }

        if (count > 0)
            pair.rms = std::sqrt(rmsSum / count);
    }
}

void WaveformData::clear()
{
    samples.clear();
    durationSeconds = 0.0;
}

//==============================================================================
// Animation
//==============================================================================

void SkiaWaveformRenderer::setAnimationProgress(float progress)
{
    animationProgress_ = juce::jlimit(0.0f, 1.0f, progress);
}

//==============================================================================
// Rendering
//==============================================================================

void SkiaWaveformRenderer::render(SkCanvas* canvas,
                                  const WaveformData& waveform,
                                  const SkRect& bounds,
                                  const WaveformRenderOptions& options)
{
    if (!canvas || !waveform.isValid())
        return;

    // Apply animation scaling
    float animScale = options.animateEntry ? animationProgress_ : 1.0f;

    size_t startSample = 0;
    size_t endSample = waveform.samples.size();

    // Clamp to valid range
    endSample = std::min(endSample, waveform.samples.size());

    canvas->save();

    // Render based on style
    switch (options.style) {
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
    }

    canvas->restore();
}

void SkiaWaveformRenderer::renderTimeRange(SkCanvas* canvas,
                                          const WaveformData& waveform,
                                          const SkRect& bounds,
                                          double startTime,
                                          double endTime,
                                          const WaveformRenderOptions& options)
{
    if (!canvas || !waveform.isValid())
        return;

    // Convert time to sample indices
    size_t startSample = static_cast<size_t>((startTime / waveform.durationSeconds) * waveform.samples.size());
    size_t endSample = static_cast<size_t>((endTime / waveform.durationSeconds) * waveform.samples.size());

    startSample = std::min(startSample, waveform.samples.size());
    endSample = std::min(endSample, waveform.samples.size());

    if (startSample >= endSample)
        return;

    canvas->save();
    renderFilled(canvas, waveform, bounds, startSample, endSample, options);
    canvas->restore();
}

void SkiaWaveformRenderer::renderMidiNotes(SkCanvas* canvas,
                                          const std::vector<std::tuple<int, double, double, int>>& notes,
                                          const SkRect& bounds,
                                          SkColor color)
{
    if (!canvas || notes.empty())
        return;

    SkPaint notePaint;
    notePaint.setColor(color);
    notePaint.setAntiAlias(true);

    for (const auto& note : notes) {
        int pitch = std::get<0>(note);
        double startTime = std::get<1>(note);
        double duration = std::get<2>(note);
        int velocity = std::get<3>(note);

        // Map pitch to vertical position (0-127 MIDI notes)
        float y = bounds.top() + (127 - pitch) * (bounds.height() / 128.0f);
        float height = bounds.height() / 128.0f;

        // Map time to horizontal position
        float x = bounds.left() + (startTime / 10.0) * bounds.width();  // Assume 10 second duration
        float width = (duration / 10.0) * bounds.width();

        // Draw note as rectangle with velocity opacity
        uint8_t alpha = static_cast<uint8_t>((velocity / 127.0f) * 255.0f);
        SkColor noteColor = SkColorSetA(color, alpha);
        notePaint.setColor(noteColor);

        canvas->drawRect(SkRect::MakeXYWH(x, y, width, height), notePaint);
    }
}

//==============================================================================
// Internal Rendering Methods
//==============================================================================

void SkiaWaveformRenderer::renderFilled(SkCanvas* canvas,
                                       const WaveformData& waveform,
                                       const SkRect& bounds,
                                       size_t startSample,
                                       size_t endSample,
                                       const WaveformRenderOptions& options)
{
    if (startSample >= endSample || endSample > waveform.samples.size())
        return;

    SkPath path = createWaveformPath(waveform, bounds, startSample, endSample, true);

    SkPaint paint;
    paint.setAntiAlias(options.antiAlias);
    paint.setColor(options.fillColor);
    paint.setAlpha(static_cast<uint8_t>(options.opacity * 255.0f));

    if (options.useGradient) {
        SkPoint pts[2] = {{bounds.left(), bounds.top()}, {bounds.left(), bounds.bottom()}};
        SkColor colors[2] = {options.gradientTop, options.gradientBottom};
        sk_sp<SkShader> shader = SkGradientShader::MakeLinear(pts, colors, nullptr, 2, SkTileMode::kClamp);
        paint.setShader(shader);
    }

    canvas->drawPath(path, paint);

    if (options.enableGlow)
        applyGlowEffect(canvas, path, options);
}

void SkiaWaveformRenderer::renderOutline(SkCanvas* canvas,
                                        const WaveformData& waveform,
                                        const SkRect& bounds,
                                        size_t startSample,
                                        size_t endSample,
                                        const WaveformRenderOptions& options)
{
    if (startSample >= endSample || endSample > waveform.samples.size())
        return;

    SkPath path = createWaveformPath(waveform, bounds, startSample, endSample, false);

    SkPaint paint;
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(options.outlineWidth);
    paint.setAntiAlias(options.antiAlias);
    paint.setColor(options.outlineColor);
    paint.setAlpha(static_cast<uint8_t>(options.opacity * 255.0f));

    canvas->drawPath(path, paint);
}

void SkiaWaveformRenderer::renderPeaks(SkCanvas* canvas,
                                      const WaveformData& waveform,
                                      const SkRect& bounds,
                                      size_t startSample,
                                      size_t endSample,
                                      const WaveformRenderOptions& options)
{
    if (startSample >= endSample || endSample > waveform.samples.size())
        return;

    SkPaint paint;
    paint.setStrokeWidth(options.peakWidth);
    paint.setAntiAlias(options.antiAlias);
    paint.setColor(options.peakColor);
    paint.setAlpha(static_cast<uint8_t>(options.opacity * 255.0f));

    float centerY = bounds.centerY();
    float pixelWidth = bounds.width() / (endSample - startSample);

    for (size_t i = startSample; i < endSample; ++i) {
        float x = bounds.left() + (i - startSample) * pixelWidth;
        float maxVal = waveform.samples[i].max;
        float minVal = waveform.samples[i].min;

        float y1 = centerY - maxVal * bounds.height() / 2.0f;
        float y2 = centerY - minVal * bounds.height() / 2.0f;

        canvas->drawLine(x, y1, x, y2, paint);
    }
}

void SkiaWaveformRenderer::renderRMS(SkCanvas* canvas,
                                    const WaveformData& waveform,
                                    const SkRect& bounds,
                                    size_t startSample,
                                    size_t endSample,
                                    const WaveformRenderOptions& options)
{
    if (startSample >= endSample || endSample > waveform.samples.size())
        return;

    SkPath path;
    float centerY = bounds.centerY();
    float pixelWidth = bounds.width() / (endSample - startSample);

    path.moveTo(bounds.left(), centerY);

    for (size_t i = startSample; i < endSample; ++i) {
        float x = bounds.left() + (i - startSample) * pixelWidth;
        float rms = waveform.samples[i].rms;
        float y = centerY - rms * bounds.height() / 2.0f;
        path.lineTo(x, y);
    }

    path.lineTo(bounds.right(), centerY);
    path.close();

    SkPaint paint;
    paint.setAntiAlias(options.antiAlias);
    paint.setColor(options.rmsColor);
    paint.setAlpha(static_cast<uint8_t>(options.opacity * 255.0f));

    canvas->drawPath(path, paint);
}

void SkiaWaveformRenderer::renderHybrid(SkCanvas* canvas,
                                       const WaveformData& waveform,
                                       const SkRect& bounds,
                                       size_t startSample,
                                       size_t endSample,
                                       const WaveformRenderOptions& options)
{
    if (startSample >= endSample)
        return;

    // Draw RMS as filled area
    renderRMS(canvas, waveform, bounds, startSample, endSample, options);

    // Draw peaks as outline
    WaveformRenderOptions peakOpts = options;
    peakOpts.peakColor = options.outlineColor;
    renderPeaks(canvas, waveform, bounds, startSample, endSample, peakOpts);
}

//==============================================================================
// Helper Methods
//==============================================================================

SkPath SkiaWaveformRenderer::createWaveformPath(const WaveformData& waveform,
                                               const SkRect& bounds,
                                               size_t startSample,
                                               size_t endSample,
                                               bool includeBottom)
{
    SkPath path;
    float centerY = bounds.centerY();
    float pixelWidth = bounds.width() / (endSample - startSample);

    // Top path
    path.moveTo(bounds.left(), centerY);

    for (size_t i = startSample; i < endSample; ++i) {
        float x = bounds.left() + (i - startSample) * pixelWidth;
        float maxVal = waveform.samples[i].max;
        float y = centerY - maxVal * bounds.height() / 2.0f;
        path.lineTo(x, y);
    }

    if (includeBottom) {
        path.lineTo(bounds.right(), centerY);

        // Bottom path (in reverse)
        for (int i = endSample - 1; i >= static_cast<int>(startSample); --i) {
            float x = bounds.left() + (i - startSample) * pixelWidth;
            float minVal = waveform.samples[i].min;
            float y = centerY - minVal * bounds.height() / 2.0f;
            path.lineTo(x, y);
        }
    }

    path.close();
    return path;
}

void SkiaWaveformRenderer::applyGlowEffect(SkCanvas* canvas,
                                          const SkPath& path,
                                          const WaveformRenderOptions& options)
{
    // Draw glow by drawing the path with reduced opacity at a slight offset
    SkPaint glowPaint;
    glowPaint.setColor(options.fillColor);
    glowPaint.setAlpha(static_cast<uint8_t>(options.glowOpacity * 255.0f));

    // Draw multiple passes with decreasing opacity for glow effect
    for (float offset = options.glowRadius; offset > 0.5f; offset -= 1.0f) {
        uint8_t alpha = static_cast<uint8_t>((options.glowOpacity * 255.0f) * (1.0f - (offset / options.glowRadius)));
        glowPaint.setAlpha(alpha);
        canvas->drawPath(path, glowPaint);
    }
}

size_t SkiaWaveformRenderer::getSampleIndexForTime(const WaveformData& waveform, double timeSeconds) const
{
    if (waveform.durationSeconds <= 0.0)
        return 0;

    double ratio = timeSeconds / waveform.durationSeconds;
    return static_cast<size_t>(juce::jlimit(0.0, 1.0, ratio) * waveform.samples.size());
}

double SkiaWaveformRenderer::getTimeForSampleIndex(const WaveformData& waveform, size_t index) const
{
    if (waveform.samples.empty())
        return 0.0;

    double ratio = static_cast<double>(index) / waveform.samples.size();
    return ratio * waveform.durationSeconds;
}

} // namespace zenith

#endif // ZENITH_USE_SKIA
