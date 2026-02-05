#include "AudioReactiveVisualizer.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>

namespace Zenith {

AudioReactiveVisualizer::AudioReactiveVisualizer()
    : audioEngine(nullptr)
    , transportController(nullptr)
    , isEnabled(false)
    , nextTouchId(1)
    , lastGestureTime(0.0f)
    , animationTime(0.0f)
    , visualSmoothingBuffer(0.0f) {

    // Initialize default configuration
    visualConfig.visualSensitivity = 1.0f;
    visualConfig.bassFrequency = 20.0f;
    visualConfig.midFrequency = 1000.0f;
    visualConfig.trebleFrequency = 8000.0f;
    visualConfig.spectrumDecay = 0.95f;
    visualConfig.peakHoldTime = 0.5f;
    visualConfig.enableSmoothing = true;
    visualConfig.enablePeakHold = true;
    visualConfig.backgroundOpacity = 0.8f;
    visualConfig.colorSaturation = 0.7f;
    visualConfig.colorBrightness = 0.8f;

    // Initialize visual effects
    activeEffects.resize(8); // One for each effect type
    for (auto& effect : activeEffects) {
        effect.smoothing = 0.8f;
        effect.threshold = 0.1f;
    }

    // Set initial bounds
    visualBounds[0] = SkPoint{0, 0};
    visualBounds[1] = SkPoint{800, 600};
}

AudioReactiveVisualizer::~AudioReactiveVisualizer() {
    shutdown();
}

bool AudioReactiveVisualizer::initialize(std::shared_ptr<IAudioEngine> engine,
                                       std::shared_ptr<ITransportController> transport) {
    audioEngine = engine;
    transportController = transport;

    // Initialize FFT analyzer
    if (!audioEngine) return false;

    analyzer = std::make_unique<AudioFFTAnalyzer>();
    analyzer->initialize(44100, 2048);
    analyzer->setSmoothingTime(30.0f); // 30ms smoothing for visuals

    // Initialize visual bounds
    updateVisualBounds();

    isEnabled = true;
    return true;
}

void AudioReactiveVisualizer::shutdown() {
    analyzer.reset();
    touchPoints.clear();
    touchIdMap.clear();
    customEffects.clear();
    isEnabled = false;
}

void AudioReactiveVisualizer::setEffectType(VisualEffect::Type type, bool enabled) {
    if (type >= 0 && type < activeEffects.size()) {
        activeEffects[type].enabled = enabled;
        activeEffects[type].data.resize(256); // Initialize data buffer
        std::fill(activeEffects[type].data.begin(), activeEffects[type].data.end(), 0.0f);
    }
}

void AudioReactiveVisualizer::processAudio(const float* audioData, int numSamples) {
    if (!isEnabled || !analyzer) return;

    // Process audio through FFT analyzer
    analyzer->processAudio(audioData, numSamples);

    // Update visual effects based on audio features
    updateVisualEffects();
}

void AudioReactiveVisualizer::updateVisualization(float deltaTime) {
    if (!isEnabled) return;

    animationTime += deltaTime;

    // Update touch points
    float currentTime = animationTime;
    touchPoints.erase(
        std::remove_if(touchPoints.begin(), touchPoints.end(),
            [currentTime](const TouchPoint& point) {
                return currentTime - point.timestamp > 1.0f; // Remove after 1 second
            }),
        touchPoints.end()
    );

    // Recognize gestures
    recognizeGestures();

    // Update spectral history
    const auto& features = analyzer->getAudioFeatures();
    spectralHistory.push_back(features.frequencyBands.fullSpectrum);
    if (spectralHistory.size() > 60) { // Keep 1 second of history at 60fps
        spectralHistory.erase(spectralHistory.begin());
    }
}

void AudioReactiveVisualizer::render(SkCanvas* canvas, const SkRect& bounds) {
    if (!isEnabled || !canvas) return;

    // Set viewport
    viewport = bounds;
    updateVisualBounds();

    // Clear background
    SkPaint bgPaint;
    bgPaint.setColor(SkColor4f{0.1f, 0.1f, 0.15f, visualConfig.backgroundOpacity});
    canvas->drawRect(bounds, bgPaint);

    const auto& features = analyzer->getAudioFeatures();

    // Render each enabled effect
    for (const auto& effect : activeEffects) {
        if (!effect.enabled) continue;

        switch (effect.type) {
            case VisualEffect::SPECTRUM_ANALYZER:
                processSpectrumAnalyzer(canvas, features);
                break;
            case VisualEffect::WAVEFORM:
                processWaveform(canvas, features);
                break;
            case VisualEffect::CIRCULAR_SCOPE:
                processCircularScope(canvas, features);
                break;
            case VisualEffect::BASS_PULSER:
                processBassPulser(canvas, features);
                break;
            case VisualEffect::FREQUENCY_BARS:
                processFrequencyBars(canvas, features);
                break;
            case VisualEffect::PARTICLE_SYSTEM:
                processParticleSystem(canvas, features);
                break;
            case VisualEffect::RIPPLE_EFFECT:
                processRippleEffect(canvas, features);
                break;
            case VisualEffect::GLOW_EFFECT:
                processGlowEffect(canvas, features);
                break;
        }
    }

    // Render custom effects
    for (const auto& effect : customEffects) {
        effect(canvas, features);
    }

    // Render touch points
    renderTouchPoints(canvas);
}

void AudioReactiveVisualizer::addTouchPoint(const TouchPoint& point) {
    touchPoints.push_back(point);
}

void AudioReactiveVisualizer::updateTouchPoint(int touchId, float x, float y, float pressure) {
    for (auto& point : touchPoints) {
        if (point.touchId == touchId) {
            point.x = x;
            point.y = y;
            point.pressure = pressure;
            point.timestamp = animationTime;
            break;
        }
    }
}

void AudioReactiveVisualizer::removeTouchPoint(int touchId) {
    touchPoints.erase(
        std::remove_if(touchPoints.begin(), touchPoints.end(),
            [touchId](const TouchPoint& point) {
                return point.touchId == touchId;
            }),
        touchPoints.end()
    );
}

void AudioReactiveVisualizer::clearTouchPoints() {
    touchPoints.clear();
}

void AudioReactiveVisualizer::recognizeGestures() {
    if (touchPoints.size() < 1 || !gestureCallback) return;

    if (touchPoints.size() == 1) {
        // Single touch gestures
        detectTapGesture();
        detectSwipeGesture();
    } else if (touchPoints.size() >= 2) {
        // Multi-touch gestures
        detectPinchGesture();
        detectRotateGesture();
    }
}

void AudioReactiveVisualizer::detectSwipeGesture() {
    if (touchPoints.size() != 2) return;

    const TouchPoint& start = touchPoints[0];
    const TouchPoint& current = touchPoints.back();

    float dx = current.x - start.x;
    float dy = current.y - start.y;
    float distance = std::sqrt(dx * dx + dy * dy);
    float timeElapsed = animationTime - start.timestamp;

    if (timeElapsed > 0.1f && distance > 50.0f) {
        currentGesture.type = Gesture::NONE;
        float absDx = std::abs(dx);
        float absDy = std::abs(dy);

        if (absDx > absDy) {
            currentGesture.type = dx > 0 ? Gesture::SWIPE_RIGHT : Gesture::SWIPE_LEFT;
        } else {
            currentGesture.type = dy > 0 ? Gesture::SWIPE_DOWN : Gesture::SWIPE_UP;
        }

        currentGesture.confidence = distance / 100.0f;
        currentGesture.startX = start.x;
        currentGesture.startY = start.y;
        currentGesture.endX = current.x;
        currentGesture.endY = current.y;
        currentGesture.points = touchPoints;

        if (currentGesture.confidence > 0.5f) {
            gestureCallback(currentGesture);
            gestureHistory.push_back(currentGesture);
            lastGestureTime = animationTime;
        }
    }
}

void AudioReactiveVisualizer::detectPinchGesture() {
    if (touchPoints.size() != 2) return;

    const TouchPoint& p1 = touchPoints[0];
    const TouchPoint& p2 = touchPoints[1];

    float centerX = (p1.x + p2.x) * 0.5f;
    float centerY = (p1.y + p2.y) * 0.5f;
    float distance = computeGestureDistance(p1, p2);
    float initialDistance = distance; // In a real implementation, track initial distance

    // Simple pinch detection based on change in distance
    if (initialDistance > 0.0f) {
        float normalizedDistance = distance / initialDistance;
        float timeElapsed = animationTime - p1.timestamp;

        if (timeElapsed > 0.1f) {
            currentGesture.type = normalizedDistance > 1.0f ? Gesture::PINCH_OUT : Gesture::PINCH_IN;
            currentGesture.confidence = std::abs(normalizedDistance - 1.0f) * 2.0f;
            currentGesture.points = touchPoints;

            if (currentGesture.confidence > 0.3f) {
                gestureCallback(currentGesture);
                gestureHistory.push_back(currentGesture);
                lastGestureTime = animationTime;
            }
        }
    }
}

void AudioReactiveVisualizer::detectRotateGesture() {
    if (touchPoints.size() != 2) return;

    const TouchPoint& p1 = touchPoints[0];
    const TouchPoint& p2 = touchPoints[1];

    float centerX = (p1.x + p2.x) * 0.5f;
    float centerY = (p1.y + p2.y) * 0.5f;

    float angle1 = computeGestureAngle({centerX, centerY, 0, 0}, p1);
    float angle2 = computeGestureAngle({centerX, centerY, 0, 0}, p2);
    float angleDiff = angle2 - angle1;

    // Normalize angle difference to [-π, π]
    while (angleDiff > M_PI) angleDiff -= 2 * M_PI;
    while (angleDiff < -M_PI) angleDiff += 2 * M_PI;

    if (std::abs(angleDiff) > 0.1f) {
        currentGesture.type = angleDiff > 0 ? Gesture::ROTATE_CLOCKWISE : Gesture::ROTATE_COUNTER_CLOCKWISE;
        currentGesture.confidence = std::abs(angleDiff) / M_PI;
        currentGesture.rotationAngle = angleDiff;
        currentGesture.points = touchPoints;

        if (currentGesture.confidence > 0.2f) {
            gestureCallback(currentGesture);
            gestureHistory.push_back(currentGesture);
            lastGestureTime = animationTime;
        }
    }
}

void AudioReactiveVisualizer::detectTapGesture() {
    if (touchPoints.size() != 1) return;

    const TouchPoint& point = touchPoints.back();
    float timeSincePress = animationTime - point.timestamp;

    if (timeSincePress > 0.1f && timeSincePress < 0.3f) {
        // Check if this is a tap (quick press and release)
        currentGesture.type = Gesture::TAP;
        currentGesture.confidence = 1.0f;
        currentGesture.startX = point.x;
        currentGesture.startY = point.y;
        currentGesture.endX = point.x;
        currentGesture.endY = point.y;
        currentGesture.points = touchPoints;

        gestureCallback(currentGesture);
        gestureHistory.push_back(currentGesture);
        lastGestureTime = animationTime;
    }
}

void AudioReactiveVisualizer::computeGestureCentroid(const std::vector<TouchPoint>& points, float& centerX, float& centerY) {
    if (points.empty()) {
        centerX = centerY = 0.0f;
        return;
    }

    float sumX = 0.0f, sumY = 0.0f;
    for (const auto& point : points) {
        sumX += point.x;
        sumY += point.y;
    }
    centerX = sumX / points.size();
    centerY = sumY / points.size();
}

float AudioReactiveVisualizer::computeGestureDistance(const TouchPoint& p1, const TouchPoint& p2) {
    float dx = p2.x - p1.x;
    float dy = p2.y - p1.y;
    return std::sqrt(dx * dx + dy * dy);
}

float AudioReactiveVisualizer::computeGestureAngle(const TouchPoint& center, const TouchPoint& point) {
    float dx = point.x - center.x;
    float dy = point.y - center.y;
    return std::atan2(dy, dx);
}

void AudioReactiveVisualizer::updateVisualBounds() {
    if (!viewport.isEmpty()) {
        visualBounds[0] = SkPoint{viewport.fLeft, viewport.fTop};
        visualBounds[1] = SkPoint{viewport.fRight, viewport.fBottom};
    }
}

void AudioReactiveVisualizer::processSpectrumAnalyzer(SkCanvas* canvas, const AudioFeatures& features) {
    SkPath path;
    int barCount = 64;
    float barWidth = viewport.width() / barCount;

    for (int i = 0; i < barCount; ++i) {
        float frequency = (float)i * audioEngine->getSampleRate() / 2048;
        float magnitude = analyzer->getFrequencies()[std::min(i, (int)analyzer->getFrequencies().size() - 1)];

        // Apply smoothing and threshold
        float smoothed = lerp(activeEffects[VisualEffect::SPECTRUM_ANALYZER].data[i], magnitude, 0.3f);
        activeEffects[VisualEffect::SPECTRUM_ANALYZER].data[i] = smoothed;

        float height = smoothed * viewport.height() * visualConfig.visualSensitivity;
        float x = viewport.fLeft + i * barWidth;
        float y = viewport.fBottom - height;

        // Draw bar
        SkRect barRect = SkRect::MakeXYWH(x, y, barWidth - 2, height);
        SkPaint barPaint;
        barPaint.setColor(getColorFromFrequency(frequency, smoothed));
        barPaint.setAntiAlias(true);
        canvas->drawRect(barRect, barPaint);
    }
}

void AudioReactiveVisualizer::processWaveform(SkCanvas* canvas, const AudioFeatures& features) {
    SkPath path;
    const auto& fft = analyzer->getFrequencies();

    if (fft.empty()) return;

    path.moveTo(viewport.fLeft, viewport.fBottom / 2);

    for (size_t i = 0; i < fft.size(); ++i) {
        float x = viewport.fLeft + (float)i / fft.size() * viewport.width();
        float y = viewport.fBottom / 2 - fft[i] * 100 * visualConfig.visualSensitivity;
        path.lineTo(x, y);
    }

    SkPaint paint;
    paint.setColor(SkColor4f{0.0f, 1.0f, 1.0f, 1.0f});
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(2.0f);
    paint.setAntiAlias(true);
    canvas->drawPath(path, paint);
}

void AudioReactiveVisualizer::processCircularScope(SkCanvas* canvas, const AudioFeatures& features) {
    float centerX = viewport.centerX();
    float centerY = viewport.centerY();
    float radius = std::min(viewport.width(), viewport.height()) * 0.3f;

    const auto& fft = analyzer->getFrequencies();
    int points = std::min(256, (int)fft.size());

    SkPath path;
    for (int i = 0; i <= points; ++i) {
        float angle = (float)i / points * 2.0f * M_PI;
        float magnitude = fft[i % fft.size()] * visualConfig.visualSensitivity;
        float pointRadius = radius + magnitude * 50;

        float x = centerX + std::cos(angle) * pointRadius;
        float y = centerY + std::sin(angle) * pointRadius;

        if (i == 0) {
            path.moveTo(x, y);
        } else {
            path.lineTo(x, y);
        }
    }

    SkPaint paint;
    paint.setColor(SkColor4f{1.0f, 0.5f, 0.0f, 1.0f});
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(2.0f);
    paint.setAntiAlias(true);
    canvas->drawPath(path, paint);
}

void AudioReactiveVisualizer::processBassPulser(SkCanvas* canvas, const AudioFeatures& features) {
    float bassEnergy = analyzer->getBassEnergy();
    float pulseRadius = bassEnergy * 100 * visualConfig.visualSensitivity;

    SkPaint paint;
    paint.setColor(getBassColor(bassEnergy));
    paint.setAntiAlias(true);

    // Create gradient
    SkPoint center = SkPoint{viewport.centerX(), viewport.centerY()};
    SkColor colors[] = {SkColor4f{0.0f, 0.0f, 0.0f, 0.0f}, getBassColor(bassEnergy)};
    SkShader* shader = SkGradientShader::MakeRadial(center, 0, center, pulseRadius, colors, nullptr, SkShader::kClamp_TileMode);
    paint.setShader(shader);

    canvas->drawCircle(viewport.centerX(), viewport.centerY(), pulseRadius, paint);
}

void AudioReactiveVisualizer::processFrequencyBars(SkCanvas* canvas, const AudioFeatures& features) {
    const auto& fft = analyzer->getFrequencies();
    int barCount = 32;
    float barWidth = viewport.width() / barCount;

    for (int i = 0; i < barCount; ++i) {
        float magnitude = fft[i % fft.size()];
        float height = magnitude * viewport.height() * visualConfig.visualSensitivity;

        // Color based on frequency range
        SkColor color;
        float frequency = (float)i * audioEngine->getSampleRate() / 2048;
        if (frequency < 250) {
            color = getBassColor(magnitude);
        } else if (frequency < 2000) {
            color = getMidColor(magnitude);
        } else {
            color = getTrebleColor(magnitude);
        }

        SkRect barRect = SkRect::MakeXYWH(
            viewport.fLeft + i * barWidth,
            viewport.fBottom - height,
            barWidth - 4,
            height
        );

        SkPaint paint;
        paint.setColor(color);
        paint.setAntiAlias(true);
        canvas->drawRect(barRect, paint);
    }
}

void AudioReactiveVisualizer::processParticleSystem(SkCanvas* canvas, const AudioFeatures& features) {
    // Simple particle system based on audio energy
    int particleCount = static_cast<int>(features.frequencyBands.fullSpectrum * 100);
    particleCount = std::min(particleCount, 100); // Cap at 100 particles

    SkPaint paint;
    paint.setColor(SkColor4f{1.0f, 1.0f, 1.0f, 0.8f});
    paint.setAntiAlias(true);

    for (int i = 0; i < particleCount; ++i) {
        float x = viewport.fLeft + std::rand() % static_cast<int>(viewport.width());
        float y = viewport.fBottom + std::rand() % static_cast<int>(viewport.height());
        float size = 2.0f + features.frequencyBands.bass * 10;

        canvas->drawCircle(x, y, size, paint);
    }
}

void AudioReactiveVisualizer::processRippleEffect(SkCanvas* canvas, const AudioFeatures& features) {
    float centerX = viewport.centerX();
    float centerY = viewport.centerY();

    // Create multiple ripples based on bass energy
    float rippleCount = analyzer->getBassEnergy() * 5;

    for (int i = 0; i < rippleCount; ++i) {
        float radius = (animationTime * 200 + i * 50) % 300;
        float alpha = 1.0f - (radius / 300.0f);

        if (alpha > 0.0f) {
            SkPaint paint;
            paint.setColor(SkColor4f{0.0f, 0.5f, 1.0f, alpha});
            paint.setStyle(SkPaint::kStroke_Style);
            paint.setStrokeWidth(2.0f);
            paint.setAntiAlias(true);

            canvas->drawCircle(centerX, centerY, radius, paint);
        }
    }
}

void AudioReactiveVisualizer::processGlowEffect(SkCanvas* canvas, const AudioFeatures& features) {
    // Create glow effect based on overall audio energy
    float glowIntensity = features.frequencyBands.fullSpectrum * visualConfig.visualSensitivity;
    float glowRadius = 50 + glowIntensity * 100;

    // Multiple glow layers for better effect
    for (int i = 0; i < 5; ++i) {
        float layerAlpha = 0.2f * (1.0f - i / 5.0f) * glowIntensity;
        float layerRadius = glowRadius * (1.0f + i * 0.2f);

        SkPaint paint;
        paint.setColor(SkColor4f{1.0f, 0.8f, 0.2f, layerAlpha});
        paint.setAntiAlias(true);

        canvas->drawCircle(viewport.centerX(), viewport.centerY(), layerRadius, paint);
    }
}

SkColor AudioReactiveVisualizer::getColorFromFrequency(float frequency, float amplitude) {
    float hue = (frequency / 20000.0f) * 360.0f; // Map frequency to hue
    float saturation = visualConfig.colorSaturation;
    float value = std::min(1.0f, amplitude * visualConfig.colorBrightness);

    // Convert HSV to RGB
    float c = value * saturation;
    float x = c * (1 - std::abs(std::fmod(hue / 60.0f, 2.0f) - 1));
    float m = value - c;

    float r, g, b;
    if (hue < 60) {
        r = c; g = x; b = 0;
    } else if (hue < 120) {
        r = x; g = c; b = 0;
    } else if (hue < 180) {
        r = 0; g = c; b = x;
    } else if (hue < 240) {
        r = 0; g = x; b = c;
    } else if (hue < 300) {
        r = x; g = 0; b = c;
    } else {
        r = c; g = 0; b = x;
    }

    return SkColorSetARGB(255,
        (int)((r + m) * 255),
        (int)((g + m) * 255),
        (int)((b + m) * 255));
}

SkColor AudioReactiveVisualizer::getBassColor(float energy) {
    float intensity = std::min(1.0f, energy * visualConfig.visualSensitivity);
    return SkColorSetARGB(255,
        (int)(255 * intensity),
        (int)(100 * intensity),
        (int)(50 * intensity));
}

SkColor AudioReactiveVisualizer::getMidColor(float energy) {
    float intensity = std::min(1.0f, energy * visualConfig.visualSensitivity);
    return SkColorSetARGB(255,
        (int)(100 * intensity),
        (int)(255 * intensity),
        (int)(100 * intensity));
}

SkColor AudioReactiveVisualizer::getTrebleColor(float energy) {
    float intensity = std::min(1.0f, energy * visualConfig.visualSensitivity);
    return SkColorSetARGB(255,
        (int)(50 * intensity),
        (int)(150 * intensity),
        (int)(255 * intensity));
}

SkColor AudioReactiveVisualizer::getGradientColor(float position, const SkColor& color1, const SkColor& color2) {
    int r = (int)(SkColorGetR(color1) + (SkColorGetR(color2) - SkColorGetR(color1)) * position);
    int g = (int)(SkColorGetG(color1) + (SkColorGetG(color2) - SkColorGetG(color1)) * position);
    int b = (int)(SkColorGetB(color1) + (SkColorGetB(color2) - SkColorGetB(color1)) * position);
    return SkColorSetRGB(r, g, b);
}

float AudioReactiveVisualizer::easeInOut(float t) {
    return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
}

float AudioReactiveVisualizer::clamp(float value, float min, float max) {
    return std::max(min, std::min(max, value));
}

float AudioReactiveVisualizer::lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

void AudioReactiveVisualizer::addCustomEffect(VisualEffect::Type type, std::function<void(SkCanvas*, const AudioFeatures&)> effect) {
    customEffects.push_back(effect);
}

void AudioReactiveVisualizer::setVisualSensitivity(float sensitivity) {
    visualConfig.visualSensitivity = clamp(sensitivity, 0.0f, 5.0f);
}

void AudioReactiveVisualizer::setSmoothingFactor(float smoothing) {
    visualConfig.enableSmoothing = smoothing > 0.0f;
    float smoothingCoeff = clamp(smoothing, 0.0f, 1.0f);

    for (auto& effect : activeEffects) {
        effect.smoothing = smoothingCoeff;
    }
}

void AudioReactiveVisualizer::setThreshold(float threshold) {
    visualConfig.threshold = clamp(threshold, 0.0f, 1.0f);

    for (auto& effect : activeEffects) {
        effect.threshold = visualConfig.threshold;
    }
}

void AudioReactiveVisualizer::updateVisualEffects() {
    const auto& features = analyzer->getAudioFeatures();

    // Update effect data with smoothing
    for (auto& effect : activeEffects) {
        if (effect.enabled && !effect.data.empty()) {
            // Apply smoothing to effect data
            for (size_t i = 0; i < effect.data.size() && i < analyzer->getFrequencies().size(); ++i) {
                float currentFreq = analyzer->getFrequencies()[i];
                effect.data[i] = lerp(effect.data[i], currentFreq, 1.0f - effect.smoothing);
            }
        }
    }
}

void AudioReactiveVisualizer::renderTouchPoints(SkCanvas* canvas) {
    SkPaint paint;
    paint.setColor(SkColor4f{1.0f, 0.0f, 0.0f, 0.8f});
    paint.setAntiAlias(true);

    // Render touch points
    for (const auto& point : touchPoints) {
        float radius = 10.0f + point.pressure * 20.0f;
        canvas->drawCircle(point.x, point.y, radius, paint);

        // Draw touch trail
        if (point.touchId > 1) { // Skip the first touch point
            canvas->drawCircle(point.x, point.y, radius * 0.5f, paint);
        }
    }
}

} // namespace Zenith