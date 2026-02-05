#pragma once

#include "AudioFFTAnalyzer.h"
#include <modules/zenith_core/engine/core/IAudioEngine.h>
#include <modules/zenith_core/engine/core/ITransportController.h>
#include <vector>
#include <memory>
#include <functional>

namespace Zenith {

// Forward declarations
class SkiaRenderer;

class AudioReactiveVisualizer {
public:
    struct VisualEffect {
        enum Type {
            NONE = 0,
            SPECTRUM_ANALYZER,
            WAVEFORM,
            CIRCULAR_SCOPE,
            BASS_PULSER,
            FREQUENCY_BARS,
            PARTICLE_SYSTEM,
            RIPPLE_EFFECT,
            GLOW_EFFECT
        };

        Type type;
        bool enabled;
        float intensity;
        float smoothing;
        float threshold;
        std::vector<float> data;
        float animationPhase;
        float animationSpeed;

        VisualEffect() : type(NONE), enabled(false), intensity(1.0f),
                        smoothing(0.8f), threshold(0.1f), animationPhase(0.0f),
                        animationSpeed(1.0f) {}
    };

    struct VisualConfig {
        float visualSensitivity;
        float bassFrequency;
        float midFrequency;
        float trebleFrequency;
        float spectrumDecay;
        float peakHoldTime;
        bool enableSmoothing;
        bool enablePeakHold;
        float backgroundOpacity;
        float colorSaturation;
        float colorBrightness;
    };

    struct TouchPoint {
        float x, y;
        float pressure;
        float timestamp;
        int touchId;

        TouchPoint() : x(0), y(0), pressure(1.0f), timestamp(0), touchId(0) {}
        TouchPoint(float px, float py, float p, float t, int id)
            : x(px), y(py), pressure(p), timestamp(t), touchId(id) {}
    };

    AudioReactiveVisualizer();
    ~AudioReactiveVisualizer();

    // Initialization
    bool initialize(std::shared_ptr<IAudioEngine> audioEngine,
                    std::shared_ptr<ITransportController> transport);
    void shutdown();

    // Configuration
    void setConfig(const VisualConfig& config) { visualConfig = config; }
    void setEnabled(bool enabled) { isEnabled = enabled; }
    void setEffectType(VisualEffect::Type type, bool enabled);

    // Processing
    void processAudio(const float* audioData, int numSamples);
    void updateVisualization(float deltaTime);
    void render(SkiaRenderer* renderer, const SkRect& bounds);

    // Gesture handling
    void addTouchPoint(const TouchPoint& point);
    void updateTouchPoint(int touchId, float x, float y, float pressure);
    void removeTouchPoint(int touchId);
    void clearTouchPoints();

    // Multi-touch gesture recognition
    struct Gesture {
        enum Type {
            NONE = 0,
            SWIPE_LEFT,
            SWIPE_RIGHT,
            SWIPE_UP,
            SWIPE_DOWN,
            PINCH_IN,
            PINCH_OUT,
            ROTATE_CLOCKWISE,
            ROTATE_COUNTER_CLOCKWISE,
            TAP,
            DOUBLE_TAP,
            LONG_PRESS
        };

        Type type;
        float confidence;
        float startX, startY;
        float endX, endY;
        float centerDistance;
        float rotationAngle;
        float pressure;
        std::vector<TouchPoint> points;

        Gesture() : type(NONE), confidence(0.0f), centerDistance(0.0f),
                   rotationAngle(0.0f), pressure(1.0f) {}
    };

    // Gesture callbacks
    using GestureCallback = std::function<void(const Gesture&)>;
    void setGestureCallback(GestureCallback callback) { gestureCallback = callback; }

    // Visual effects
    void updateVisualEffects();
    void addCustomEffect(VisualEffect::Type type, std::function<void(SkCanvas*, const AudioFeatures&)> effect);

    // Dynamic control
    void setVisualSensitivity(float sensitivity);
    void setSmoothingFactor(float smoothing);
    void setThreshold(float threshold);

    // State access
    const AudioFeatures& getCurrentAudioFeatures() const { return analyzer->getAudioFeatures(); }
    const std::vector<VisualEffect>& getActiveEffects() const { return activeEffects; }
    const std::vector<TouchPoint>& getTouchPoints() const { return touchPoints; }

private:
    // Audio processing
    std::shared_ptr<IAudioEngine> audioEngine;
    std::shared_ptr<ITransportController> transportController;
    std::unique_ptr<AudioFFTAnalyzer> analyzer;

    // Visual effects
    VisualConfig visualConfig;
    std::vector<VisualEffect> activeEffects;
    std::vector<std::function<void(SkCanvas*, const AudioFeatures&)>> customEffects;
    bool isEnabled;

    // Touch handling
    std::vector<TouchPoint> touchPoints;
    std::map<int, int> touchIdMap; // Maps platform touch ID to our internal ID
    int nextTouchId;

    // Gesture recognition
    GestureCallback gestureCallback;
    Gesture currentGesture;
    std::vector<Gesture> gestureHistory;
    float lastGestureTime;

    // Animation
    float animationTime;
    float visualSmoothingBuffer;
    std::vector<float> spectralHistory;

    // Visual rendering
    SkPoint visualBounds[2]; // Top-left and bottom-right bounds for visualization
    SkRect viewport;

    // Private methods
    void recognizeGestures();
    void detectSwipeGesture();
    void detectPinchGesture();
    void detectRotateGesture();
    void detectTapGesture();
    void computeGestureCentroid(const std::vector<TouchPoint>& points, float& centerX, float& centerY);
    float computeGestureDistance(const TouchPoint& p1, const TouchPoint& p2);
    float computeGestureAngle(const TouchPoint& center, const TouchPoint& point);
    void updateVisualBounds();
    void processSpectrumAnalyzer(SkCanvas* canvas, const AudioFeatures& features);
    void processWaveform(SkCanvas* canvas, const AudioFeatures& features);
    void processCircularScope(SkCanvas* canvas, const AudioFeatures& features);
    void processBassPulser(SkCanvas* canvas, const AudioFeatures& features);
    void processFrequencyBars(SkCanvas* canvas, const AudioFeatures& features);
    void processParticleSystem(SkCanvas* canvas, const AudioFeatures& features);
    void processRippleEffect(SkCanvas* canvas, const AudioFeatures& features);
    void processGlowEffect(SkCanvas* canvas, const AudioFeatures& features);

    // Color generation
    SkColor getColorFromFrequency(float frequency, float amplitude);
    SkColor getBassColor(float energy);
    SkColor getMidColor(float energy);
    SkColor getTrebleColor(float energy);
    SkColor getGradientColor(float position, const SkColor& color1, const SkColor& color2);

    // Utility
    float easeInOut(float t);
    float clamp(float value, float min, float max);
    float lerp(float a, float b, float t);
};

} // namespace Zenith