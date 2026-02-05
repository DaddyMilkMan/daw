# Zenith Skia Component Specifications
## Implementation-Ready Technical Reference

---

## 🔄 ViewSwitcher.h

The brain of the navigation system. Handles Tab/Shift+Tab seamlessly.

```cpp
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace zenith::ui {

enum class ViewType {
    Arrangement,
    Session,
    AIJam
};

class ViewSwitcher : public juce::Component,
                     public juce::KeyListener {
public:
    ViewSwitcher();
    ~ViewSwitcher() override = default;
    
    void setActiveView(ViewType view);
    ViewType getActiveView() const { return currentView_; }
    
    // Animation progress (0.0 - 1.0)
    float getTransitionProgress() const { return transitionProgress_; }
    
    // Listener pattern for view changes
    struct Listener {
        virtual ~Listener() = default;
        virtual void viewWillChange(ViewType from, ViewType to) {}
        virtual void viewDidChange(ViewType newView) {}
    };
    void addListener(Listener* l) { listeners_.add(l); }
    void removeListener(Listener* l) { listeners_.remove(l); }
    
    // juce::Component
    void resized() override;
    void paint(juce::Graphics& g) override;
    
    // juce::KeyListener
    bool keyPressed(const juce::KeyPress& key, juce::Component* origin) override;
    
private:
    ViewType currentView_ = ViewType::Arrangement;
    ViewType targetView_ = ViewType::Arrangement;
    float transitionProgress_ = 1.0f;  // 1.0 = complete
    
    // Views are owned here
    std::unique_ptr<class SkiaArrangementView> arrangementView_;
    std::unique_ptr<class SkiaSessionView> sessionView_;
    std::unique_ptr<class SkiaAIJamView> aiJamView_;
    
    juce::ListenerList<Listener> listeners_;
    
    // Animation timer
    class TransitionAnimator;
    std::unique_ptr<TransitionAnimator> animator_;
    
    void startTransition(ViewType to);
    void updateTransition(float deltaTime);
    
    static constexpr float kTransitionDuration = 0.25f;  // seconds
};

} // namespace zenith::ui
```

### Key Bindings Logic
```cpp
bool ViewSwitcher::keyPressed(const juce::KeyPress& key, juce::Component*) {
    if (key == juce::KeyPress::tabKey) {
        if (key.getModifiers().isShiftDown()) {
            // Shift+Tab: Toggle AI Jam overlay
            if (currentView_ == ViewType::AIJam) {
                // Return to previous non-AIJam view
                setActiveView(previousMainView_);
            } else {
                previousMainView_ = currentView_;
                setActiveView(ViewType::AIJam);
            }
        } else {
            // Tab: Toggle between Arrangement and Session
            if (currentView_ == ViewType::Arrangement) {
                setActiveView(ViewType::Session);
            } else if (currentView_ == ViewType::Session) {
                setActiveView(ViewType::Arrangement);
            }
            // If in AIJam, Tab does nothing (use Shift+Tab to exit)
        }
        return true;
    }
    return false;
}
```

---

## 🎹 SkiaArrangementView.h

```cpp
#pragma once
#include "../skia/SkiaComponent.h"
#include "../design-system/ZenithTheme.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkPicture.h"
#include "include/core/SkPictureRecorder.h"

namespace zenith::ui {

class SkiaArrangementView : public SkiaComponent {
public:
    SkiaArrangementView();
    ~SkiaArrangementView() override = default;
    
    // View state
    void setScrollPosition(float x, float y);
    void setZoom(float horizontal, float vertical);
    void setPlayheadPosition(double beatsPosition);
    
    // Track management
    void setTracks(const std::vector<TrackRef>& tracks);
    void setSelection(const SelectionState& sel);
    
    // SkiaComponent
    void drawSkia(SkCanvas* canvas) override;
    
    // Mouse handling
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e,
                        const juce::MouseWheelDetails& wheel) override;
    
private:
    // Layout constants
    static constexpr float kTrackHeaderWidth = 280.0f;
    static constexpr float kTimelineHeight = 40.0f;
    static constexpr float kDefaultTrackHeight = 80.0f;
    static constexpr float kMinTrackHeight = 40.0f;
    static constexpr float kMaxTrackHeight = 200.0f;
    
    // View state
    float scrollX_ = 0.0f;
    float scrollY_ = 0.0f;
    float zoomX_ = 1.0f;  // pixels per beat
    float zoomY_ = 1.0f;
    double playheadBeats_ = 0.0;
    
    // Cached render layers
    sk_sp<SkPicture> gridPicture_;
    sk_sp<SkPicture> trackHeadersPicture_;
    bool needsGridRedraw_ = true;
    bool needsHeadersRedraw_ = true;
    
    // Drawing methods
    void drawBackground(SkCanvas* canvas);
    void drawGrid(SkCanvas* canvas);
    void drawTimeline(SkCanvas* canvas);
    void drawTrackHeaders(SkCanvas* canvas);
    void drawTrackLanes(SkCanvas* canvas);
    void drawClips(SkCanvas* canvas);
    void drawPlayhead(SkCanvas* canvas);
    void drawSelection(SkCanvas* canvas);
    
    // Cache management
    void rebuildGridCache();
    void rebuildHeadersCache();
    
    // Hit testing
    struct HitResult {
        enum Type { None, TrackHeader, Clip, Timeline, Grid };
        Type type = None;
        int trackIndex = -1;
        int clipIndex = -1;
        float beatPosition = 0.0f;
    };
    HitResult hitTest(float x, float y);
    
    // Interaction state
    HitResult currentHit_;
    bool isDragging_ = false;
    juce::Point<float> dragStart_;
    enum class DragMode { None, Scroll, Select, MoveClip, ResizeClip };
    DragMode dragMode_ = DragMode::None;
};

} // namespace zenith::ui
```

### Drawing Implementation

```cpp
void SkiaArrangementView::drawSkia(SkCanvas* canvas) {
    SkAutoCanvasRestore acr(canvas, true);
    
    // 1. Background (solid)
    drawBackground(canvas);
    
    // 2. Grid (cached SkPicture when zoom/scroll stable)
    canvas->save();
    canvas->translate(-scrollX_, 0);
    if (needsGridRedraw_) rebuildGridCache();
    canvas->drawPicture(gridPicture_);
    canvas->restore();
    
    // 3. Track lanes with clips (scrolled)
    canvas->save();
    canvas->translate(kTrackHeaderWidth, kTimelineHeight - scrollY_);
    canvas->clipRect(SkRect::MakeXYWH(
        0, 0,
        getWidth() - kTrackHeaderWidth,
        getHeight() - kTimelineHeight
    ));
    canvas->translate(-scrollX_, 0);
    drawTrackLanes(canvas);
    drawClips(canvas);
    canvas->restore();
    
    // 4. Timeline (fixed Y, scrolled X)
    canvas->save();
    canvas->translate(kTrackHeaderWidth - scrollX_, 0);
    drawTimeline(canvas);
    canvas->restore();
    
    // 5. Track headers (fixed X, scrolled Y)
    canvas->save();
    canvas->translate(0, kTimelineHeight - scrollY_);
    canvas->clipRect(SkRect::MakeXYWH(0, 0, kTrackHeaderWidth, getHeight() - kTimelineHeight));
    if (needsHeadersRedraw_) rebuildHeadersCache();
    canvas->drawPicture(trackHeadersPicture_);
    canvas->restore();
    
    // 6. Playhead (on top of everything)
    drawPlayhead(canvas);
    
    // 7. Selection rectangle
    if (isDragging_ && dragMode_ == DragMode::Select) {
        drawSelection(canvas);
    }
}

void SkiaArrangementView::drawPlayhead(SkCanvas* canvas) {
    float x = kTrackHeaderWidth + beatsToPixels(playheadBeats_) - scrollX_;
    
    if (x < kTrackHeaderWidth || x > getWidth()) return;
    
    // Glow effect
    SkPaint glowPaint;
    glowPaint.setColor(ZenithTheme::accent_primary);
    glowPaint.setAlphaf(0.3f);
    glowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 8.0f));
    canvas->drawLine(x, 0, x, getHeight(), glowPaint);
    
    // Main line
    SkPaint linePaint;
    linePaint.setColor(ZenithTheme::accent_primary);
    linePaint.setStrokeWidth(2.0f);
    linePaint.setAntiAlias(true);
    canvas->drawLine(x, 0, x, getHeight(), linePaint);
    
    // Top handle
    SkPath handlePath;
    handlePath.moveTo(x - 6, 0);
    handlePath.lineTo(x + 6, 0);
    handlePath.lineTo(x, 10);
    handlePath.close();
    canvas->drawPath(handlePath, linePaint);
}
```

---

## 🔲 SkiaSessionView.h

```cpp
#pragma once
#include "../skia/SkiaComponent.h"

namespace zenith::ui {

class SkiaSessionView : public SkiaComponent {
public:
    SkiaSessionView();
    
    void drawSkia(SkCanvas* canvas) override;
    
private:
    // Layout
    static constexpr float kSceneLauncherWidth = 120.0f;
    static constexpr float kTrackWidth = 140.0f;
    static constexpr float kClipSlotHeight = 80.0f;
    static constexpr float kMixerHeight = 100.0f;
    static constexpr float kHeaderHeight = 40.0f;
    
    // View state
    float scrollX_ = 0.0f;
    float scrollY_ = 0.0f;
    int focusedTrack_ = -1;
    int focusedScene_ = -1;
    
    // Drawing
    void drawBackground(SkCanvas* canvas);
    void drawSceneLauncher(SkCanvas* canvas);
    void drawTrackHeaders(SkCanvas* canvas);
    void drawClipGrid(SkCanvas* canvas);
    void drawMixerStrip(SkCanvas* canvas);
    void drawClipSlot(SkCanvas* canvas, int track, int scene, const ClipSlotState& state);
};

// Clip slot states for rendering
struct ClipSlotState {
    enum State { Empty, Stopped, Playing, Queued, Recording };
    State state = Empty;
    juce::String name;
    juce::Colour color;
    bool isSelected = false;
    float playProgress = 0.0f;  // 0-1 for playing clips
};

} // namespace zenith::ui
```

### Clip Slot Rendering

```cpp
void SkiaSessionView::drawClipSlot(SkCanvas* canvas, int track, int scene, 
                                    const ClipSlotState& state) {
    float x = kSceneLauncherWidth + track * kTrackWidth + 4;
    float y = kHeaderHeight + scene * kClipSlotHeight + 4;
    float w = kTrackWidth - 8;
    float h = kClipSlotHeight - 8;
    
    SkRRect slotRect = SkRRect::MakeRectXY(SkRect::MakeXYWH(x, y, w, h), 8, 8);
    
    // Background
    SkPaint bgPaint;
    bgPaint.setAntiAlias(true);
    
    switch (state.state) {
        case ClipSlotState::Empty:
            bgPaint.setColor(ZenithTheme::bg_02);
            canvas->drawRRect(slotRect, bgPaint);
            
            // Draw empty circle indicator
            SkPaint circlePaint;
            circlePaint.setColor(ZenithTheme::text_tertiary);
            circlePaint.setStyle(SkPaint::kStroke_Style);
            circlePaint.setStrokeWidth(1.5f);
            canvas->drawCircle(x + w/2, y + h/2 - 8, 8, circlePaint);
            break;
            
        case ClipSlotState::Stopped:
            bgPaint.setColor(SkColorSetA(state.color.getARGB(), 180));
            canvas->drawRRect(slotRect, bgPaint);
            
            // Play triangle
            drawPlayTriangle(canvas, x + w/2, y + h/2 - 8);
            
            // Clip name
            drawText(canvas, state.name, x + 8, y + h - 16, ZenithTheme::text_primary);
            break;
            
        case ClipSlotState::Playing: {
            // Animated border glow
            float glowIntensity = 0.5f + 0.5f * std::sin(juce::Time::getMillisecondCounter() * 0.005f);
            
            bgPaint.setColor(SkColorSetA(state.color.getARGB(), 220));
            canvas->drawRRect(slotRect, bgPaint);
            
            // Outer glow
            SkPaint glowPaint;
            glowPaint.setColor(SkColorSetA(state.color.getARGB(), uint8_t(100 * glowIntensity)));
            glowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 6.0f));
            canvas->drawRRect(slotRect, glowPaint);
            
            // Playing indicator (filled circle)
            drawPlayingCircle(canvas, x + w/2, y + h/2 - 8, state.playProgress);
            
            // Border
            SkPaint borderPaint;
            borderPaint.setStyle(SkPaint::kStroke_Style);
            borderPaint.setStrokeWidth(2.0f);
            borderPaint.setColor(SkColorSetRGB(255, 255, 255));
            borderPaint.setAlphaf(0.3f + 0.2f * glowIntensity);
            canvas->drawRRect(slotRect, borderPaint);
            break;
        }
            
        case ClipSlotState::Queued: {
            // Blinking effect
            float blink = std::fmod(juce::Time::getMillisecondCounter() * 0.003f, 1.0f);
            bool isOn = blink > 0.5f;
            
            bgPaint.setColor(SkColorSetA(state.color.getARGB(), isOn ? 200 : 120));
            canvas->drawRRect(slotRect, bgPaint);
            
            // Queued ring indicator
            drawQueuedRing(canvas, x + w/2, y + h/2 - 8);
            break;
        }
            
        case ClipSlotState::Recording:
            bgPaint.setColor(ZenithTheme::record);
            canvas->drawRRect(slotRect, bgPaint);
            
            // Pulsing red
            float pulse = 0.7f + 0.3f * std::sin(juce::Time::getMillisecondCounter() * 0.008f);
            
            SkPaint pulsePaint;
            pulsePaint.setColor(ZenithTheme::record);
            pulsePaint.setAlphaf(pulse * 0.5f);
            pulsePaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 8.0f));
            canvas->drawRRect(slotRect, pulsePaint);
            
            // Record circle
            SkPaint recPaint;
            recPaint.setColor(SkColorSetRGB(255, 255, 255));
            canvas->drawCircle(x + w/2, y + h/2 - 8, 8, recPaint);
            break;
    }
}
```

---

## 🤖 SkiaAIJamView.h

```cpp
#pragma once
#include "../skia/SkiaComponent.h"

namespace zenith::ui {

class SkiaAIJamView : public SkiaComponent {
public:
    SkiaAIJamView();
    
    void setVisible(bool visible);
    void setBackgroundView(SkiaComponent* bgView);  // For blur effect
    
    void drawSkia(SkCanvas* canvas) override;
    
    // AI interface
    void setPromptText(const juce::String& text);
    void setGeneratedStems(const std::vector<StemInfo>& stems);
    void addChatMessage(bool fromUser, const juce::String& message);
    void setThinking(bool thinking);
    
private:
    // Animation
    float revealProgress_ = 0.0f;  // 0 = hidden, 1 = fully visible
    bool isThinking_ = false;
    
    // Layout
    static constexpr float kPanelPadding = 40.0f;
    static constexpr float kPromptBarHeight = 80.0f;
    static constexpr float kStemCardHeight = 140.0f;
    static constexpr float kQuickActionsHeight = 120.0f;
    static constexpr float kChatHeight = 200.0f;
    
    // Content
    juce::String promptText_;
    std::vector<StemInfo> stems_;
    std::vector<ChatMessage> chatHistory_;
    
    // Drawing
    void drawGlassBackground(SkCanvas* canvas);
    void drawPromptBar(SkCanvas* canvas);
    void drawStemCards(SkCanvas* canvas);
    void drawQuickActions(SkCanvas* canvas);
    void drawJamLoop(SkCanvas* canvas);
    void drawChatPanel(SkCanvas* canvas);
    void drawThinkingIndicator(SkCanvas* canvas);
};

struct StemInfo {
    enum Type { Drums, Bass, Chords, Melody, Other };
    Type type;
    juce::String name;
    bool isSoloed = false;
    bool isMuted = false;
    bool isPlaying = false;
    float level = 0.0f;  // For meter
    // Waveform preview data
    std::vector<float> waveformPreview;
};

struct ChatMessage {
    bool fromUser;
    juce::String text;
    double timestamp;
};

} // namespace zenith::ui
```

### Glassmorphism Background

```cpp
void SkiaAIJamView::drawGlassBackground(SkCanvas* canvas) {
    SkRect bounds = SkRect::MakeXYWH(
        kPanelPadding, kPanelPadding,
        getWidth() - 2 * kPanelPadding,
        getHeight() - 2 * kPanelPadding
    );
    SkRRect panel = SkRRect::MakeRectXY(bounds, 24, 24);
    
    // Save layer for blur effect
    canvas->save();
    canvas->clipRRect(panel);
    
    // Blur the background (expensive - should be cached)
    SkPaint blurPaint;
    auto blur = SkImageFilters::Blur(30, 30, nullptr);
    blurPaint.setImageFilter(blur);
    
    // Draw underlying content with blur
    if (backgroundView_) {
        canvas->saveLayer(nullptr, &blurPaint);
        backgroundView_->drawSkia(canvas);
        canvas->restore();
    }
    
    // Semi-transparent dark overlay
    SkPaint overlayPaint;
    overlayPaint.setColor(SkColorSetARGB(200, 8, 8, 14));  // #08080e @ 78%
    canvas->drawRRect(panel, overlayPaint);
    
    // Inner light border
    SkPaint innerBorderPaint;
    innerBorderPaint.setStyle(SkPaint::kStroke_Style);
    innerBorderPaint.setStrokeWidth(1.0f);
    innerBorderPaint.setColor(SkColorSetARGB(20, 255, 255, 255));
    canvas->drawRRect(panel, innerBorderPaint);
    
    // Subtle gradient at top for depth
    SkRect topGradientRect = SkRect::MakeXYWH(bounds.fLeft, bounds.fTop, bounds.width(), 100);
    SkPaint gradientPaint;
    SkPoint gradientPoints[2] = {{0, bounds.fTop}, {0, bounds.fTop + 100}};
    SkColor gradientColors[2] = {
        SkColorSetARGB(15, 255, 255, 255),
        SkColorSetARGB(0, 255, 255, 255)
    };
    gradientPaint.setShader(SkGradientShader::MakeLinear(
        gradientPoints, gradientColors, nullptr, 2, SkTileMode::kClamp
    ));
    canvas->save();
    canvas->clipRRect(panel);
    canvas->drawRect(topGradientRect, gradientPaint);
    canvas->restore();
    
    canvas->restore();
    
    // Title
    drawTitle(canvas, "AI JAM MODE", bounds.fLeft + 24, bounds.fTop + 40);
    drawSubtitle(canvas, "Press Shift+Tab to exit", bounds.fLeft + 24, bounds.fTop + 64);
}

void SkiaAIJamView::drawStemCards(SkCanvas* canvas) {
    float startY = kPromptBarHeight + 100;
    float cardWidth = (getWidth() - 2 * kPanelPadding - 60) / 5;  // 4 stems + variations
    
    for (size_t i = 0; i < stems_.size() && i < 4; i++) {
        float x = kPanelPadding + 20 + i * (cardWidth + 10);
        drawStemCard(canvas, x, startY, cardWidth, kStemCardHeight, stems_[i]);
    }
    
    // Variations panel
    float varX = kPanelPadding + 20 + 4 * (cardWidth + 10);
    drawVariationsPanel(canvas, varX, startY, cardWidth, kStemCardHeight);
}

void SkiaAIJamView::drawStemCard(SkCanvas* canvas, float x, float y, 
                                  float w, float h, const StemInfo& stem) {
    SkRRect card = SkRRect::MakeRectXY(SkRect::MakeXYWH(x, y, w, h), 12, 12);
    
    // Card background
    SkPaint bgPaint;
    bgPaint.setColor(ZenithTheme::bg_03);
    canvas->drawRRect(card, bgPaint);
    
    // Hover glow (when stem.isSelected or hovered)
    if (stem.isPlaying) {
        SkPaint glowPaint;
        glowPaint.setColor(ZenithTheme::accent_primary);
        glowPaint.setAlphaf(0.2f);
        glowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 8.0f));
        canvas->drawRRect(card, glowPaint);
    }
    
    // Type label
    const char* typeLabel = [&]() {
        switch (stem.type) {
            case StemInfo::Drums: return "DRUMS";
            case StemInfo::Bass: return "BASS";
            case StemInfo::Chords: return "CHORDS";
            case StemInfo::Melody: return "MELODY";
            default: return "OTHER";
        }
    }();
    drawLabel(canvas, typeLabel, x + 12, y + 20);
    
    // Play button
    float buttonCenterX = x + w/2;
    float buttonCenterY = y + 55;
    
    if (stem.isPlaying) {
        // Filled circle
        SkPaint playPaint;
        playPaint.setColor(ZenithTheme::accent_primary);
        canvas->drawCircle(buttonCenterX, buttonCenterY, 16, playPaint);
    } else {
        // Play triangle
        drawPlayTriangle(canvas, buttonCenterX, buttonCenterY, 12);
    }
    
    // Mini waveform
    float waveY = y + 90;
    float waveH = 30;
    drawMiniWaveform(canvas, x + 8, waveY, w - 16, waveH, 
                     stem.waveformPreview, stem.type == StemInfo::Drums || 
                     stem.type == StemInfo::Bass ? ZenithTheme::audio : ZenithTheme::midi);
    
    // Solo button
    drawButton(canvas, "Solo", x + 8, y + h - 28, 50, 20, stem.isSoloed);
}
```

---

## 🎨 Design System Components

### SkiaButton.h

```cpp
void SkiaButton::drawSkia(SkCanvas* canvas) {
    SkRRect bounds = SkRRect::MakeRectXY(getLocalBounds().toSkRect(), radius_, radius_);
    
    // Determine state colors
    SkColor bgColor, textColor;
    switch (state_) {
        case State::Normal:
            bgColor = SkColorSetARGB(0, 0, 0, 0);  // Transparent
            textColor = ZenithTheme::text_primary;
            break;
        case State::Hover:
            bgColor = ZenithTheme::glass_medium;
            textColor = ZenithTheme::text_primary;
            break;
        case State::Pressed:
            bgColor = ZenithTheme::glass_strong;
            textColor = ZenithTheme::text_primary;
            break;
        case State::Active:
            bgColor = ZenithTheme::accent_primary;
            textColor = SkColorSetRGB(255, 255, 255);
            break;
        case State::Disabled:
            bgColor = SkColorSetARGB(0, 0, 0, 0);
            textColor = SkColorSetA(ZenithTheme::text_tertiary, 100);
            break;
    }
    
    // Scale animation
    float scale = 1.0f;
    if (state_ == State::Hover) scale = 1.02f;
    if (state_ == State::Pressed) scale = 0.98f;
    
    canvas->save();
    canvas->scale(scale, scale);
    
    // Background
    SkPaint bgPaint;
    bgPaint.setColor(bgColor);
    bgPaint.setAntiAlias(true);
    canvas->drawRRect(bounds, bgPaint);
    
    // Border
    SkPaint borderPaint;
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.0f);
    borderPaint.setColor(ZenithTheme::glass_border);
    canvas->drawRRect(bounds, borderPaint);
    
    // Text
    SkFont font(ZenithTheme::getFont(), 14);
    SkPaint textPaint;
    textPaint.setColor(textColor);
    
    SkRect textBounds;
    font.measureText(label_.toRawUTF8(), label_.length(), SkTextEncoding::kUTF8, &textBounds);
    
    float textX = (getWidth() - textBounds.width()) / 2;
    float textY = (getHeight() + textBounds.height()) / 2;
    canvas->drawString(label_.toRawUTF8(), textX, textY, font, textPaint);
    
    canvas->restore();
}
```

### SkiaMeter.h

```cpp
void SkiaMeter::drawSkia(SkCanvas* canvas) {
    float levelDb = currentLevel_;  // -60 to +6 dB typically
    float peakDb = peakHold_;
    
    // Convert dB to 0-1 range
    float levelNorm = dbToNormalized(levelDb);
    float peakNorm = dbToNormalized(peakDb);
    
    SkRect bounds = getLocalBounds().toSkRect();
    bool isVertical = bounds.height() > bounds.width();
    
    // Background
    SkPaint bgPaint;
    bgPaint.setColor(ZenithTheme::bg_01);
    canvas->drawRect(bounds, bgPaint);
    
    // Gradient stops: green -> yellow -> red
    SkColor gradientColors[4] = {
        SkColorSetRGB(34, 197, 94),   // Green
        SkColorSetRGB(234, 179, 8),    // Yellow
        SkColorSetRGB(239, 68, 68),    // Red
        SkColorSetRGB(255, 100, 100)   // Bright red (clipping)
    };
    float gradientPositions[4] = { 0.0f, 0.75f, 0.9f, 1.0f };
    
    SkPoint gradientPoints[2];
    if (isVertical) {
        gradientPoints[0] = {0, bounds.fBottom};
        gradientPoints[1] = {0, bounds.fTop};
    } else {
        gradientPoints[0] = {bounds.fLeft, 0};
        gradientPoints[1] = {bounds.fRight, 0};
    }
    
    SkPaint meterPaint;
    meterPaint.setShader(SkGradientShader::MakeLinear(
        gradientPoints, gradientColors, gradientPositions, 4, SkTileMode::kClamp
    ));
    
    // Draw level bar
    SkRect levelRect;
    if (isVertical) {
        float levelHeight = bounds.height() * levelNorm;
        levelRect = SkRect::MakeXYWH(
            bounds.fLeft, bounds.fBottom - levelHeight,
            bounds.width(), levelHeight
        );
    } else {
        float levelWidth = bounds.width() * levelNorm;
        levelRect = SkRect::MakeXYWH(
            bounds.fLeft, bounds.fTop,
            levelWidth, bounds.height()
        );
    }
    canvas->drawRect(levelRect, meterPaint);
    
    // Peak hold line
    if (showPeakHold_) {
        SkPaint peakPaint;
        peakPaint.setColor(SkColorSetRGB(255, 255, 255));
        peakPaint.setStrokeWidth(2.0f);
        
        if (isVertical) {
            float peakY = bounds.fBottom - bounds.height() * peakNorm;
            canvas->drawLine(bounds.fLeft, peakY, bounds.fRight, peakY, peakPaint);
        } else {
            float peakX = bounds.fLeft + bounds.width() * peakNorm;
            canvas->drawLine(peakX, bounds.fTop, peakX, bounds.fBottom, peakPaint);
        }
    }
    
    // Scale markers
    drawScaleMarkers(canvas, bounds, isVertical);
}

float SkiaMeter::dbToNormalized(float db) {
    // -60 dB to +6 dB range
    constexpr float minDb = -60.0f;
    constexpr float maxDb = 6.0f;
    return std::clamp((db - minDb) / (maxDb - minDb), 0.0f, 1.0f);
}
```

---

## ⌨️ Keyboard Shortcuts Reference

| Key | Context | Action |
|-----|---------|--------|
| `Tab` | Any | Toggle Arrangement ↔ Session |
| `Shift+Tab` | Any | Toggle AI Jam overlay |
| `Space` | Any | Play/Stop |
| `Enter` | Any | Play from start |
| `R` | Any | Toggle Record |
| `L` | Any | Toggle Loop |
| `M` | Track selected | Mute track |
| `S` | Track selected | Solo track |
| `Cmd+Z` | Any | Undo |
| `Cmd+Shift+Z` | Any | Redo |
| `Cmd+C` | Clip selected | Copy |
| `Cmd+V` | Any | Paste |
| `Delete` | Clip selected | Delete clip |
| `Cmd+D` | Clip selected | Duplicate |
| `1-8` | Session View | Trigger scene 1-8 |
| `Cmd+1` | Any | Go to Arrangement |
| `Cmd+2` | Any | Go to Session |
| `Cmd+3` | Any | Go to AI Jam |
| `+/-` | Arrangement | Zoom in/out |
| Arrow keys | Any | Navigate |

---

## 📊 Performance Optimizations

### Render Strategy

```cpp
// In SkiaComponent base class
void SkiaComponent::render(SkCanvas* canvas) {
    // Check if we need full redraw
    if (isDirty_) {
        // Record to SkPicture for caching
        SkPictureRecorder recorder;
        SkCanvas* recordCanvas = recorder.beginRecording(getWidth(), getHeight());
        drawSkia(recordCanvas);
        cachedPicture_ = recorder.finishRecordingAsPicture();
        isDirty_ = false;
    }
    
    // Play back cached picture
    canvas->drawPicture(cachedPicture_);
    
    // Draw animated elements on top (always fresh)
    drawAnimatedElements(canvas);
}

// Mark dirty when state changes
void SkiaComponent::markDirty() {
    isDirty_ = true;
    repaint();
}
```

### Waveform Caching

```cpp
class WaveformCache {
public:
    sk_sp<SkImage> getWaveform(const AudioBuffer& buffer, 
                                float width, float height,
                                SkColor color) {
        CacheKey key{buffer.hashCode(), int(width), int(height), color};
        
        auto it = cache_.find(key);
        if (it != cache_.end()) {
            return it->second;
        }
        
        // Generate waveform
        auto image = renderWaveform(buffer, width, height, color);
        cache_[key] = image;
        
        // Evict old entries if cache too large
        trimCache();
        
        return image;
    }
    
private:
    std::map<CacheKey, sk_sp<SkImage>> cache_;
    static constexpr size_t kMaxCacheSize = 100;  // images
};
```

---

*These specs are implementation-ready. Copy. Paste. Build. Ship.*
