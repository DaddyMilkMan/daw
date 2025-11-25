/**
 * @file SkiaMainWindowIntegration.cpp
 * @brief Implementation of GPU-accelerated Skia DAW rendering
 */

#include "SkiaMainWindowIntegration.h"

#ifdef ZENITH_USE_SKIA

// Define colors for the DAW UI
#define SK_COLOR_BG_DARK     SkColorSetRGB(18, 18, 20)
#define SK_COLOR_PANEL       SkColorSetRGB(30, 30, 35)
#define SK_COLOR_ACCENT      SkColorSetRGB(0, 160, 255)
#define SK_COLOR_TEXT_MAIN   SkColorSetRGB(220, 220, 220)
#define SK_COLOR_TEXT_DIM    SkColorSetRGB(150, 150, 150)
#define SK_COLOR_GRID_LINE   SkColorSetRGB(50, 50, 55)

namespace zenith {

//==============================================================================
// Constructor / Destructor
//==============================================================================

SkiaMainWindowIntegration::SkiaMainWindowIntegration()
    : isPlaying_(true), frameCounter_(0)
{
    // 1. Configure and attach OpenGL context
    openGLContext.setRenderer(this);
    openGLContext.setContinuousRepainting(true); // VSYNC enabled
    openGLContext.setComponentPaintingEnabled(true);
    openGLContext.attachTo(*this);

    // 2. Start animation timer at 60 Hz
    startTimerHz(60);

    DBG("SkiaMainWindowIntegration: Initialized");
}

SkiaMainWindowIntegration::~SkiaMainWindowIntegration()
{
    openGLContext.detach();
    shutdownSkiaRendering();
}

//==============================================================================
// OpenGL Lifecycle
//==============================================================================

void SkiaMainWindowIntegration::newOpenGLContextCreated()
{
    DBG("newOpenGLContextCreated - Initializing Skia on render thread");

    try {
        renderer_ = std::make_unique<SkiaRenderer>(*this, SkiaRenderer::Backend::OpenGL, true);
        if (renderer_->initialize()) {
            skiaInitialized_ = true;
            DBG("✓ Skia initialized successfully");
        } else {
            DBG("✗ SkiaRenderer initialization failed");
        }
    } catch (const std::exception& e) {
        DBG("✗ Exception: " << e.what());
    }
}

void SkiaMainWindowIntegration::openGLContextClosing()
{
    DBG("openGLContextClosing - Cleaning up Skia");
    renderer_.reset();
    skiaInitialized_ = false;
}

void SkiaMainWindowIntegration::renderOpenGL()
{
    if (!skiaInitialized_ || !renderer_) return;

    // Handle DPI scaling (Windows typically 1.0, 1.25, or 1.5)
    const float scale = (float)openGLContext.getRenderingScale();

    // Update Animation State
    if (isPlaying_) {
        playheadPosition_ += 2.0f;
        if (playheadPosition_ > (float)getWidth()) {
            playheadPosition_ = 0.0f;
        }
    }
    frameCounter_++;

    // RENDER THE FRAME
    renderer_->render([this, scale](SkCanvas* canvas) {
        canvas->save();
        canvas->scale(scale, scale); // Apply DPI scaling

        // 1. Main Background
        canvas->clear(SK_COLOR_BG_DARK);

        // 2. Define Layout Areas (Logical Pixels)
        float w = (float)getWidth() / scale;
        float h = (float)getHeight() / scale;
        float topBarH = 50.0f;
        float sideW = 250.0f;
        float rightW = 280.0f;

        SkRect transportRect = SkRect::MakeXYWH(0, 0, w, topBarH);
        SkRect browserRect   = SkRect::MakeXYWH(0, topBarH, sideW, h - topBarH);
        SkRect inspectorRect = SkRect::MakeXYWH(w - rightW, topBarH, rightW, h - topBarH);
        SkRect timelineRect  = SkRect::MakeXYWH(sideW, topBarH, w - sideW - rightW, h - topBarH);

        // 3. Draw Layout Components
        drawTimelineArea(canvas, timelineRect, scale);
        drawBrowserPanel(canvas, browserRect, scale);
        drawInspectorPanel(canvas, inspectorRect, scale);
        drawTransportBar(canvas, transportRect, scale); // Top stays on top

        canvas->restore();
    });
}

//==============================================================================
// Drawing Implementations
//==============================================================================

void SkiaMainWindowIntegration::drawTransportBar(SkCanvas* canvas, const SkRect& bounds, float scale)
{
    SkPaint paint;

    // Background
    paint.setColor(SK_COLOR_PANEL);
    canvas->drawRect(bounds, paint);

    // Bottom Border
    paint.setColor(SkColorSetRGB(60, 60, 65));
    paint.setStrokeWidth(1.0f);
    canvas->drawLine(bounds.left(), bounds.bottom(), bounds.right(), bounds.bottom(), paint);

    // Title Text
    SkFont font(nullptr, 18.0f);
    paint.setColor(SK_COLOR_TEXT_MAIN);
    canvas->drawString("ZENITH DAW", bounds.left() + 20, bounds.centerY() + 6, font, paint);

    // Play Button (Center)
    SkRect playBtn = SkRect::MakeXYWH(bounds.centerX() - 40, bounds.centerY() - 15, 80, 30);
    SkRRect rrect;
    rrect.setRectXY(playBtn, 4, 4);

    paint.setColor(SK_COLOR_ACCENT);
    canvas->drawRRect(rrect, paint);

    paint.setColor(SK_ColorWHITE);
    font.setSize(12.0f);
    canvas->drawString(isPlaying_ ? "PLAYING" : "STOPPED", playBtn.left() + 15, playBtn.bottom() - 10, font, paint);

    // BPM Counter
    paint.setColor(SK_COLOR_TEXT_DIM);
    canvas->drawString("128.00 BPM", bounds.right() - 150, bounds.centerY() + 5, font, paint);
}

void SkiaMainWindowIntegration::drawBrowserPanel(SkCanvas* canvas, const SkRect& bounds, float scale)
{
    SkPaint paint;

    // Background
    paint.setColor(SkColorSetRGB(22, 22, 24));
    canvas->drawRect(bounds, paint);

    // Border (Right edge)
    paint.setColor(SK_COLOR_GRID_LINE);
    paint.setStrokeWidth(1.0f);
    canvas->drawLine(bounds.right(), bounds.top(), bounds.right(), bounds.bottom(), paint);

    // Mock List Items
    SkFont font(nullptr, 13.0f);
    float itemY = bounds.top() + 30;

    const char* items[] = { " Audio Files", " Instruments", " MIDI Effects", " Audio Effects", " Presets" };

    for (int i = 0; i < 5; ++i) {
        paint.setColor(i == 1 ? SK_COLOR_ACCENT : SK_COLOR_TEXT_MAIN); // Highlight 'Instruments'
        if (i == 1) {
            // Draw selection highlight
            SkPaint bgPaint;
            bgPaint.setColor(SkColorSetARGB(30, 0, 160, 255));
            canvas->drawRect(SkRect::MakeXYWH(bounds.left(), itemY - 15, bounds.width() - 1, 30), bgPaint);
        }
        canvas->drawString(items[i], bounds.left() + 20, itemY, font, paint);
        itemY += 35;
    }
}

void SkiaMainWindowIntegration::drawTimelineArea(SkCanvas* canvas, const SkRect& bounds, float scale)
{
    // Clip to timeline area
    canvas->save();
    canvas->clipRect(bounds);

    SkPaint paint;

    // 1. Draw Grid Lines
    paint.setColor(SK_COLOR_GRID_LINE);
    paint.setStrokeWidth(1.0f);

    // Vertical Grid (Beats)
    float beatWidth = 100.0f;
    for (float x = bounds.left(); x < bounds.right(); x += beatWidth) {
        canvas->drawLine(x, bounds.top(), x, bounds.bottom(), paint);
    }

    // Horizontal Grid (Tracks) and track headers
    float trackHeight = 80.0f;
    for (float y = bounds.top(); y < bounds.bottom(); y += trackHeight) {
        canvas->drawLine(bounds.left(), y, bounds.right(), y, paint);

        // Mock Track Header Background
        SkPaint headerPaint;
        headerPaint.setColor(SkColorSetRGB(35, 35, 40));
        canvas->drawRect(SkRect::MakeXYWH(bounds.left(), y, 150, trackHeight), headerPaint);

        // Track Name
        SkFont font(nullptr, 12.0f);
        SkPaint textPaint;
        textPaint.setColor(SK_COLOR_TEXT_DIM);
        canvas->drawString("Track Name", bounds.left() + 10, y + trackHeight / 2 + 4, font, textPaint);
    }

    // 2. Draw a Sample Clip
    SkRect clipRect = SkRect::MakeXYWH(bounds.left() + 250, bounds.top() + 10, 200, 60);
    SkRRect clipRRect;
    clipRRect.setRectXY(clipRect, 6, 6);

    paint.setColor(SkColorSetARGB(200, 0, 160, 255)); // Semi-transparent Blue
    canvas->drawRRect(clipRRect, paint);

    paint.setColor(SK_ColorWHITE);
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(2.0f);
    canvas->drawRRect(clipRRect, paint);

    // 3. Draw Playhead
    drawPlayhead(canvas, bounds, scale);

    canvas->restore();
}

void SkiaMainWindowIntegration::drawPlayhead(SkCanvas* canvas, const SkRect& bounds, float scale)
{
    // Animated playhead position
    float x = bounds.left() + 250 + (sin(frameCounter_ * 0.05f) * 100 + 100);

    SkPaint paint;
    paint.setColor(SkColorSetRGB(255, 50, 50)); // Red
    paint.setStrokeWidth(2.0f);

    // Vertical Line
    canvas->drawLine(x, bounds.top(), x, bounds.bottom(), paint);

    // Triangle Cap
    SkPath path;
    path.moveTo(x - 6, bounds.top());
    path.lineTo(x + 6, bounds.top());
    path.lineTo(x, bounds.top() + 12);
    path.close();
    canvas->drawPath(path, paint);
}

void SkiaMainWindowIntegration::drawInspectorPanel(SkCanvas* canvas, const SkRect& bounds, float scale)
{
    SkPaint paint;
    paint.setColor(SkColorSetRGB(25, 25, 28));
    canvas->drawRect(bounds, paint);

    paint.setColor(SK_COLOR_GRID_LINE);
    paint.setStrokeWidth(1.0f);
    canvas->drawLine(bounds.left(), bounds.top(), bounds.left(), bounds.bottom(), paint);

    // Header Text
    SkFont font(nullptr, 14.0f);
    paint.setColor(SK_COLOR_TEXT_MAIN);
    canvas->drawString("PROPERTIES", bounds.left() + 20, bounds.top() + 30, font, paint);

    // Mock Knobs
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(3.0f);
    paint.setColor(SK_COLOR_ACCENT);

    canvas->drawCircle(bounds.left() + 50, bounds.top() + 80, 20, paint);
    canvas->drawCircle(bounds.left() + 120, bounds.top() + 80, 20, paint);
}

//==============================================================================
// Standard Component Methods
//==============================================================================

void SkiaMainWindowIntegration::paint(juce::Graphics& g)
{
    // Usually empty - OpenGL handles rendering
    // But show loading message if Skia not ready
    if (!skiaInitialized_) {
        g.fillAll(juce::Colours::black);
        g.setColour(juce::Colours::white);
        g.drawText("Initializing GPU Engine...", getLocalBounds(), juce::Justification::centred, true);
    }
}

void SkiaMainWindowIntegration::resized()
{
    // No-op - renderOpenGL handles bounds
}

void SkiaMainWindowIntegration::timerCallback()
{
    // Trigger render updates
    openGLContext.triggerRepaint();
}

//==============================================================================
// Control Methods
//==============================================================================

bool SkiaMainWindowIntegration::initializeSkiaRendering()
{
    // Initialization moved to newOpenGLContextCreated
    return true;
}

void SkiaMainWindowIntegration::shutdownSkiaRendering()
{
    renderer_.reset();
    skiaInitialized_ = false;
}

} // namespace zenith

#endif // ZENITH_USE_SKIA
