/*
  ==============================================================================

    TransportBar.cpp
    Created: 2025-11-28
    Author:  Leo "Lil Bit" Rossi
    Redesigned: 2025-12-29 (Modern Sleek Design v2)

    Ultra-modern transport bar with:
    - Full-width glassmorphic background
    - Circular buttons with scale animations
    - Smooth hover/press micro-interactions
    - Refined glow effects

  ==============================================================================
*/

#include "TransportBar.h"

#include <core/SkBlurTypes.h>
#include <core/SkCanvas.h>
#include <core/SkColor.h>
#include <core/SkFont.h>
#include <core/SkFontTypes.h>
#include <core/SkMaskFilter.h>
#include <core/SkPaint.h>
#include <core/SkRRect.h>
#include <effects/SkGradientShader.h>

#ifdef ZENITH_USE_SKIA
#include "../design-system/ZenithIcons.h"
#include "../framework/GlassmorphicPanel.h"
#include "../framework/NeonGlow.h"
#include "../design-system/ColorBridge.h"
#include "../design-system/ZenithDesignSystem.h"

namespace zenith {

// ============================================================================
// CONSTANTS - Modern Design Tokens
// ============================================================================

namespace {
    constexpr float TRANSPORT_HEIGHT = 56.0f;       // Slightly thinner, more modern
    constexpr float BUTTON_SIZE = 40.0f;            // Smaller, refined
    constexpr float BUTTON_GAP = 12.0f;             // More breathing room
    constexpr int NUM_TRANSPORT_BUTTONS = 4;
    
    // Animation
    constexpr float HOVER_SCALE = 1.08f;            // Subtle scale up on hover
    constexpr float PRESS_SCALE = 0.94f;            // Subtle scale down on press
    constexpr float ANIMATION_SPEED = 10.0f;        // Fast and snappy
    
    // Visual - Moderate glow (visible but not overwhelming)
    constexpr float ICON_SIZE_RATIO = 0.45f;        // Smaller icons = more refined
    constexpr float GLOW_RADIUS_HOVER = 8.0f;       // Visible hover glow
    constexpr float GLOW_RADIUS_ACTIVE = 14.0f;     // Nice active glow
}

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

TransportBar::TransportBar() {
    setSize(800, static_cast<int>(TRANSPORT_HEIGHT));
}

TransportBar::~TransportBar() = default;

// ============================================================================
// LIFECYCLE
// ============================================================================

void TransportBar::visibilityChanged() {
    if (isVisible() && getPeer() != nullptr && !isTimerRunning()) {
        if (juce::MessageManager::getInstanceWithoutCreating() != nullptr) {
            startTimerHz(60);
        }
    }
}

// ============================================================================
// LAYOUT - Full Width, Centered Controls
// ============================================================================

void TransportBar::resized() {
    using namespace design;

    const int width = getWidth();
    const int height = getHeight();

    // Calculate total width of button group
    const float totalButtonWidth = (BUTTON_SIZE * NUM_TRANSPORT_BUTTONS) + 
                                   (BUTTON_GAP * (NUM_TRANSPORT_BUTTONS - 1));

    // Center the button group
    const float groupStartX = (width - totalButtonWidth) / 2.0f;
    const float buttonY = (height - BUTTON_SIZE) / 2.0f;

    float currentX = groupStartX;

    // Return to Start
    returnToStartButtonBounds_ = juce::Rectangle<int>(
        static_cast<int>(currentX), static_cast<int>(buttonY),
        static_cast<int>(BUTTON_SIZE), static_cast<int>(BUTTON_SIZE)
    );
    currentX += BUTTON_SIZE + BUTTON_GAP;

    // Play
    playButtonBounds_ = juce::Rectangle<int>(
        static_cast<int>(currentX), static_cast<int>(buttonY),
        static_cast<int>(BUTTON_SIZE), static_cast<int>(BUTTON_SIZE)
    );
    currentX += BUTTON_SIZE + BUTTON_GAP;

    // Stop
    stopButtonBounds_ = juce::Rectangle<int>(
        static_cast<int>(currentX), static_cast<int>(buttonY),
        static_cast<int>(BUTTON_SIZE), static_cast<int>(BUTTON_SIZE)
    );
    currentX += BUTTON_SIZE + BUTTON_GAP;

    // Record
    recordButtonBounds_ = juce::Rectangle<int>(
        static_cast<int>(currentX), static_cast<int>(buttonY),
        static_cast<int>(BUTTON_SIZE), static_cast<int>(BUTTON_SIZE)
    );

    // Update cached paints
    SkRect skBounds = SkRect::MakeWH(static_cast<float>(width), static_cast<float>(height));
    updateCachedPaints(skBounds);
    cachedBounds_ = skBounds;
}

// ============================================================================
// RENDERING - Modern Sleek Design
// ============================================================================

void TransportBar::drawSkia(SkCanvas *canvas) {
    auto bounds = getLocalBounds().toFloat();
    SkRect skBounds = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());

    // 1. Full-width glassmorphic background with subtle gradient
    drawModernBackground(canvas, skBounds);

    // 2. Draw transport buttons with modern styling
    drawModernButton(canvas, returnToStartButtonBounds_, icons::SkipBack(), 
                     false, design::colors::CYAN, returnToStartState_);
    
    drawModernButton(canvas, playButtonBounds_, 
                     isPlaying_ ? icons::Pause() : icons::Play(), 
                     isPlaying_, design::colors::NEON_GREEN, playState_);
    
    drawModernButton(canvas, stopButtonBounds_, icons::Stop(), 
                     false, design::colors::CYAN, stopState_);
    
    drawModernButton(canvas, recordButtonBounds_, icons::Record(),
                     isRecording_, design::colors::NEON_RED, recordState_);
}

void TransportBar::drawModernBackground(SkCanvas *canvas, const SkRect &bounds) {
    // Subtle gradient background
    SkPaint bgPaint;
    bgPaint.setAntiAlias(true);
    
    SkPoint pts[2] = {{0, 0}, {0, bounds.height()}};
    SkColor colors[2] = {
        design::withAlpha(design::colors::BG_01, 0.85f),
        design::withAlpha(design::colors::BG_00, 0.95f)
    };
    bgPaint.setShader(SkGradientShader::MakeLinear(pts, colors, nullptr, 2, SkTileMode::kClamp));
    canvas->drawRect(bounds, bgPaint);

    // Subtle top highlight line
    SkPaint highlightPaint;
    highlightPaint.setColor(design::withAlpha(SK_ColorWHITE, 0.03f));
    highlightPaint.setAntiAlias(true);
    canvas->drawRect(SkRect::MakeXYWH(0, 0, bounds.width(), 1), highlightPaint);

    // Bottom border with subtle glow
    SkPaint borderPaint;
    borderPaint.setColor(design::withAlpha(design::colors::ACCENT_PRIMARY, 0.15f));
    borderPaint.setAntiAlias(true);
    canvas->drawRect(SkRect::MakeXYWH(0, bounds.height() - 1, bounds.width(), 1), borderPaint);
}

void TransportBar::drawModernButton(SkCanvas *canvas,
                                    const juce::Rectangle<int> &bounds,
                                    const SkPath &iconPath, 
                                    bool isActive,
                                    uint32_t accentColor,
                                    const InteractionState &state) {
    if (bounds.isEmpty()) return;

    // Calculate animated scale
    float targetScale = 1.0f;
    if (state.pressAmount > 0.01f) {
        targetScale = PRESS_SCALE;
    } else if (state.hoverAmount > 0.01f) {
        targetScale = 1.0f + (HOVER_SCALE - 1.0f) * state.hoverAmount;
    }
    
    float scale = targetScale;
    
    // Center point
    float centerX = bounds.getCentreX();
    float centerY = bounds.getCentreY();
    float radius = BUTTON_SIZE / 2.0f * scale;

    // === OUTER GLOW (for hover and active) - Moderate ===
    if (isActive || state.hoverAmount > 0.05f) {
        float glowRadius = isActive ? GLOW_RADIUS_ACTIVE : GLOW_RADIUS_HOVER * state.hoverAmount;
        float glowOpacity = isActive ? 0.35f : 0.18f * state.hoverAmount;  // Visible glow
        
        SkPaint glowPaint;
        glowPaint.setAntiAlias(true);
        glowPaint.setColor(design::withAlpha(accentColor, glowOpacity));
        glowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, glowRadius / 2.0f));
        
        canvas->drawCircle(centerX, centerY, radius + glowRadius * 0.4f, glowPaint);
    }

    // === BUTTON BACKGROUND - Circular ===
    SkPaint buttonPaint;
    buttonPaint.setAntiAlias(true);
    
    if (isActive) {
        // Active: filled with accent color gradient
        SkPoint pts[2] = {{centerX, centerY - radius}, {centerX, centerY + radius}};
        SkColor colors[2] = {
            design::lighten(accentColor, 0.2f),
            accentColor
        };
        buttonPaint.setShader(SkGradientShader::MakeLinear(pts, colors, nullptr, 2, SkTileMode::kClamp));
    } else if (state.hoverAmount > 0.01f) {
        // Hover: subtle fill fading in
        uint8_t alpha = static_cast<uint8_t>(25 * state.hoverAmount);
        buttonPaint.setColor(SkColorSetA(SK_ColorWHITE, alpha));
    } else {
        // Default: very subtle background
        buttonPaint.setColor(design::withAlpha(SK_ColorWHITE, 0.04f));
    }
    
    canvas->drawCircle(centerX, centerY, radius, buttonPaint);

    // === BUTTON RING (border) ===
    SkPaint ringPaint;
    ringPaint.setAntiAlias(true);
    ringPaint.setStyle(SkPaint::kStroke_Style);
    ringPaint.setStrokeWidth(1.5f);
    
    if (isActive) {
        ringPaint.setColor(design::withAlpha(SK_ColorWHITE, 0.3f));
    } else if (state.hoverAmount > 0.01f) {
        SkColor ringColor = design::interpolateColor(
            design::withAlpha(SK_ColorWHITE, 0.08f),
            design::withAlpha(accentColor, 0.5f),
            state.hoverAmount
        );
        ringPaint.setColor(ringColor);
    } else {
        ringPaint.setColor(design::withAlpha(SK_ColorWHITE, 0.08f));
    }
    
    canvas->drawCircle(centerX, centerY, radius - 0.75f, ringPaint);

    // === ICON ===
    float iconSize = BUTTON_SIZE * ICON_SIZE_RATIO * scale;
    SkRect iconBounds = SkRect::MakeXYWH(
        centerX - iconSize / 2.0f,
        centerY - iconSize / 2.0f,
        iconSize, iconSize
    );

    icons::IconStyle style;
    
    if (isActive) {
        style.color = SK_ColorWHITE;
        style.filled = true;
    } else {
        // Brighten on hover
        style.color = design::interpolateColor(
            design::colors::TEXT_SECONDARY,
            SK_ColorWHITE,
            state.hoverAmount * 0.8f
        );
        style.filled = false;
    }
    
    style.strokeWidth = icons::STROKE_LIGHT;  // Thinner strokes = more refined
    
    // Icon glow on active
    if (isActive) {
        style.glowRadius = 4.0f;
        style.glowColor = accentColor;
    }

    icons::drawIconCentered(canvas, iconPath, iconBounds, iconSize, style);
}

void TransportBar::updateCachedPaints(const SkRect &bounds) {
    // Minimal caching - most rendering is dynamic now
    font_ = design::getMonoFont(16.0f, design::FontWeight::Medium);
    smallFont_ = design::getSkFont(11.0f, design::FontWeight::Regular);
}

// ============================================================================
// MOUSE INPUT
// ============================================================================

void TransportBar::mouseDown(const juce::MouseEvent &e) {
    if (e.mods.isRightButtonDown()) return;

    auto pos = e.getPosition();

    returnToStartState_.isPressed = returnToStartButtonBounds_.contains(pos);
    playState_.isPressed = playButtonBounds_.contains(pos);
    stopState_.isPressed = stopButtonBounds_.contains(pos);
    recordState_.isPressed = recordButtonBounds_.contains(pos);

    if (returnToStartButtonBounds_.contains(pos) && onReturnToStart) {
        onReturnToStart();
    } else if (playButtonBounds_.contains(pos) && onPlayClicked) {
        onPlayClicked();
    } else if (stopButtonBounds_.contains(pos) && onStopClicked) {
        onStopClicked();
    } else if (recordButtonBounds_.contains(pos) && onRecordClicked) {
        onRecordClicked();
    }
    
    repaint();
}

void TransportBar::mouseUp(const juce::MouseEvent &e) {
    juce::ignoreUnused(e);
    returnToStartState_.isPressed = false;
    playState_.isPressed = false;
    stopState_.isPressed = false;
    recordState_.isPressed = false;
    repaint();
}

void TransportBar::mouseMove(const juce::MouseEvent &e) {
    auto pos = e.getPosition();
    returnToStartState_.isHovered = returnToStartButtonBounds_.contains(pos);
    playState_.isHovered = playButtonBounds_.contains(pos);
    stopState_.isHovered = stopButtonBounds_.contains(pos);
    recordState_.isHovered = recordButtonBounds_.contains(pos);

    // Contextual help
    if (globalHelpCallback) {
        if (returnToStartState_.isHovered) {
            globalHelpCallback("Return to Start", "Jump to position 0");
        } else if (playState_.isHovered) {
            globalHelpCallback("Play/Pause", "Space");
        } else if (stopState_.isHovered) {
            globalHelpCallback("Stop", "Stop playback");
        } else if (recordState_.isHovered) {
            globalHelpCallback("Record", "R");
        }
    }
}

void TransportBar::mouseEnter(const juce::MouseEvent &e) { 
    mouseMove(e); 
}

void TransportBar::mouseExit(const juce::MouseEvent &e) {
    juce::ignoreUnused(e);
    returnToStartState_.isHovered = false;
    playState_.isHovered = false;
    stopState_.isHovered = false;
    recordState_.isHovered = false;
}

// ============================================================================
// ANIMATION - Smooth 60fps Updates
// ============================================================================

void TransportBar::timerCallback() {
    SkiaComponent::timerCallback();

    float dt = 1.0f / 60.0f;
    
    // Use faster animation speed for snappier feel
    float speed = ANIMATION_SPEED;
    
    // Custom smooth update for each state
    auto smoothUpdate = [speed, dt](InteractionState& state) {
        float targetHover = state.isHovered ? 1.0f : 0.0f;
        float targetPress = state.isPressed ? 1.0f : 0.0f;
        
        state.hoverAmount += (targetHover - state.hoverAmount) * speed * dt;
        state.pressAmount += (targetPress - state.pressAmount) * speed * dt * 2.0f; // Press is faster
        
        // Clamp
        state.hoverAmount = juce::jlimit(0.0f, 1.0f, state.hoverAmount);
        state.pressAmount = juce::jlimit(0.0f, 1.0f, state.pressAmount);
    };
    
    smoothUpdate(returnToStartState_);
    smoothUpdate(playState_);
    smoothUpdate(stopState_);
    smoothUpdate(recordState_);

    // Always repaint for smooth animations (could optimize later)
    bool needsRepaint = returnToStartState_.isAnimating() || 
                        playState_.isAnimating() ||
                        stopState_.isAnimating() || 
                        recordState_.isAnimating();
    
    if (needsRepaint) {
        repaint();
    }
}

// ============================================================================
// ACCESSIBILITY
// ============================================================================

std::unique_ptr<juce::AccessibilityHandler> TransportBar::createAccessibilityHandler() {
    return std::make_unique<juce::AccessibilityHandler>(*this, juce::AccessibilityRole::group);
}

} // namespace zenith

#endif // ZENITH_USE_SKIA
