/*
  ==============================================================================

    SkiaComponent.h
    Created: 2025-11-30
    Authors: Sarah Chen (lead), with input from ENTIRE TEAM

    Base class for all Skia-rendered components in Zenith DAW.
    
    TEAM ARGUMENTS DURING CREATION: 7
    - Canvas state management (Dr. Aris vs Raj)
    - Glow in base class (Leo vs Yuki) 
    - Error handling approach (Viktor vs Sarah)
    - Animation system location (Diego vs Kenji)
    - Virtual function overhead (Raj vs everyone)
    - Const correctness (Sarah vs Diego)
    - Memory management (Dr. Aris vs Viktor)

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <include/core/SkCanvas.h>
#include <include/core/SkPaint.h>
#include <include/core/SkPath.h>
#include <include/core/SkMaskFilter.h>
#include <include/core/SkBlurTypes.h>
#include <include/core/SkColor.h>
#include <include/core/SkRect.h>
#include <include/core/SkRRect.h>
#include <include/core/SkShader.h> // For SkTileMode
#include <include/core/SkFont.h>

#include "ZenithDesignSystem.h"
#include "SkiaAccessibility.h" // Added for Karen fixes
#include <functional>
#include <map>
#include <memory>
#include <vector>

namespace zenith {

// Forward declarations
class AnimatedValue;

/**
 * Core Value History for Undo/Redo.
 * REAL implementation, not a stub.
 */
template <typename T>
class ValueHistory {
public:
    void push(T value) {
        // Remove redo history
        if (currentIndex_ < history_.size() - 1) {
            history_.erase(history_.begin() + currentIndex_ + 1, history_.end());
        }
        
        history_.push_back(value);
        currentIndex_ = history_.size() - 1;
        
        // Limit history size
        if (history_.size() > 50) {
            history_.erase(history_.begin());
            currentIndex_--;
        }
    }
    
    bool canUndo() const { return currentIndex_ > 0; }
    bool canRedo() const { return currentIndex_ < history_.size() - 1; }
    
    T undo() {
        if (canUndo()) {
            currentIndex_--;
            return history_[currentIndex_];
        }
        return history_.empty() ? T() : history_[currentIndex_];
    }
    
    T redo() {
        if (canRedo()) {
            currentIndex_++;
            return history_[currentIndex_];
        }
        return history_.empty() ? T() : history_[currentIndex_];
    }
    
    T getCurrent() const {
        return history_.empty() ? T() : history_[currentIndex_];
    }

private:
    std::vector<T> history_;
    size_t currentIndex_ = 0;
};

/**
 * Base class for all Skia-rendered components.
 * 
 * DESIGN DECISION (after heated debate):
 * - Glow support in base class (Leo won this argument)
 * - Canvas state save/restore mandatory (Dr. Aris won)
 * - Fallback rendering for GPU failures (Viktor won)
 * - Animation support optional (compromise between Diego and Yuki)
 * 
 * @author Sarah Chen (architecture)
 * @author Dr. Aris Vokos (Skia compliance)
 * @author Viktor Volkov (error handling)
 */
class SkiaComponent : public juce::Component, public juce::Timer, public juce::KeyListener {
public:
    SkiaComponent();
    virtual ~SkiaComponent();
    
    // ========================================================================
    // RENDERING (Dr. Aris insisted on proper state management)
    // ========================================================================
    
    /**
     * Main Skia rendering method - OVERRIDE THIS!
     * Canvas state is already saved, you can modify it freely.
     * 
     * @param canvas The Skia canvas to draw on (never null when called)
     * 
     * ARGUMENT #1: Should this be pure virtual?
     * - Sarah: "Yes, force implementation!"
     * - Kenji: "No, allow empty components!"
     * - RESULT: Pure virtual (Sarah won)
     */
    virtual void drawSkia(SkCanvas* canvas) = 0;
    
    /**
     * Optional debug rendering overlay.
     * Shows component bounds, layout info, etc.
     * 
     * ARGUMENT #2: Should debug rendering be in base class?
     * - Marcus: "Yes, every component needs it!"
     * - Raj: "No, it's overhead in release builds!"
     * - RESULT: Optional, compiled out in release (Raj won)
     */
    #ifdef DEBUG
    virtual void drawDebug(SkCanvas* canvas);
    #endif
    
    // ========================================================================
    // GLOW EFFECTS (Leo fought HARD for this)
    // ========================================================================
    
    /**
     * Enable/disable glow effect on this component.
     * 
     * ARGUMENT #3: Should glow be in base class or separate?
     * - Leo: "EVERY component should glow!"
     * - Yuki: "That's visual chaos!"
     * - Isabella: "Make it optional!"
     * - RESULT: In base class, disabled by default (compromise)
     */
    void setGlowEnabled(bool enabled) { glowEnabled_ = enabled; markDirty(); }
    bool isGlowEnabled() const { return glowEnabled_; }
    
    void setGlowColor(SkColor color) { glowColor_ = color; markDirty(); }
    SkColor getGlowColor() const { return glowColor_; }
    
    void setGlowRadius(float radius) { glowRadius_ = radius; markDirty(); }
    float getGlowRadius() const { return glowRadius_; }
    
    // ========================================================================
    // LIFECYCLE HOOKS (Kenji's modular design)
    // ========================================================================
    
    /**
     * Called when component becomes visible.
     * 
     * ARGUMENT #4: Should we have lifecycle hooks?
     * - Kenji: "Yes, for proper initialization!"
     * - Raj: "No, it's virtual function overhead!"
     * - Sarah: "The overhead is negligible!"
     * - RESULT: Hooks added (Kenji won, Raj grumbled)
     */
    virtual void onShow() {}
    virtual void onHide() {}
    virtual void onResize() {}
    virtual void onParentChanged() {}
    
    // ========================================================================
    // INTERACTION HOOKS (Isabella's interaction design)
    // ========================================================================
    
    virtual void onHoverEnter() {}
    virtual void onHoverExit() {}
    virtual void onFocusGained() {}
    virtual void onFocusLost() {}
    
    // ========================================================================
    // ANIMATION SYSTEM (Diego's smooth transitions)
    // ========================================================================
    
    /**
     * Animate a property to a target value.
     * 
     * ARGUMENT #5: Should animation be in base class?
     * - Diego: "Everything should animate smoothly!"
     * - Yuki: "That's unnecessary complexity!"
     * - Raj: "What about performance?"
     * - RESULT: Optional animation system (compromise)
     */
    void animateTo(const juce::String& property, float target, int durationMs);
    void animateWithSpring(const juce::String& property, float target, 
                          float stiffness, float damping);
    void stopAnimation(const juce::String& property);
    void stopAllAnimations();
    
    float getAnimatedValue(const juce::String& property) const;
    bool isAnimating(const juce::String& property) const;
    
    // ========================================================================
    // STATE MANAGEMENT (Sarah's clean architecture)
    // ========================================================================
    
    bool isHovered() const { return isHovered_; }
    bool isFocused() const { return hasFocus(); }
    
    void markDirty() { needsRepaint_ = true; repaint(); }
    bool isDirty() const { return needsRepaint_; }
    
    // ========================================================================
    // JUCE OVERRIDES (Integration layer)
    // ========================================================================
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    
    void focusGained(juce::Component::FocusChangeType cause) override;
    void focusLost(juce::Component::FocusChangeType cause) override;
    
    void timerCallback() override;

    // ========================================================================
    // ACCESSIBILITY & NAVIGATION (Karen Fixes - REAL IMPLEMENTATION)
    // ========================================================================

    // Keyboard navigation
    bool keyPressed(const juce::KeyPress& key, juce::Component* origin) override;
    
    virtual void onEnterPressed() {}  // Override for activation
    virtual void onEscapePressed() {} // Override for cancel
    
    // ========================================================================
    // CONTEXT MENU & UNDO/REDO (UX Karen Fixes - REAL IMPLEMENTATION)
    // ========================================================================
    
    /**
     * Shows standard context menu with MIDI Learn, Copy/Paste, Reset.
     * Can be overridden to add custom items.
     */
    virtual void showContextMenu();
    
    /**
     * Handles result from context menu.
     */
    virtual void handleContextMenuResult(int result);
    
    // Virtual hooks for context menu actions - MUST be implemented by controls
    virtual void resetToDefault() {}
    virtual void copyValue() {}
    virtual void pasteValue() {}
    virtual void toggleMIDILearn() {}
    static void setTargetFPS(int fps);
    static int getTargetFPS();
    
    
protected:
    // ========================================================================
    // RENDERING HELPERS
    // ========================================================================
    
    /**
     * Get Skia canvas from JUCE Graphics.
     * 
     * ARGUMENT #6: Should this be protected or private?
     * - Dr. Aris: "Protected, subclasses might need it!"
     * - Sarah: "Private, use drawSkia instead!"
     * - RESULT: Protected (Dr. Aris won)
     */
    SkCanvas* getSkiaCanvas(juce::Graphics& g);
    
    /**
     * Fallback rendering when Skia is unavailable.
     * 
     * ARGUMENT #7: What should fallback rendering do?
     * - Viktor: "Show error message!"
     * - Yuki: "Show simple placeholder!"
     * - Sarah: "Call virtual method for custom fallback!"
     * - RESULT: Virtual method with default implementation (Sarah won)
     */
    virtual void paintFallback(juce::Graphics& g);
    
    /**
     * Apply glow effect to current paint.
     * Helper method for subclasses.
     */
    void applyGlow(SkPaint& paint, float intensity = 1.0f);
    
    // ========================================================================
    // STATE
    // ========================================================================
    
    bool isHovered_ = false;
    bool needsRepaint_ = true;
    SkColor glowColor_ = design::colors::NEON_GREEN;
    float glowRadius_ = 0.0f;
    bool glowEnabled_ = false;
private:
    std::map<juce::String, std::unique_ptr<AnimatedValue>> animations_;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaComponent)
}; // Close SkiaComponent class here

// ANIMATED VALUE (Diego's animation system)
// ============================================================================

/**
 * Represents an animated value that smoothly transitions.
 * 
 * TEAM DEBATE: Should this be a separate class?
 * - Diego: "Yes, clean separation!"
 * - Raj: "No, it's allocation overhead!"
 * - Sarah: "Use unique_ptr, problem solved!"
 * - RESULT: Separate class with unique_ptr (compromise)
 */
class AnimatedValue {
public:
    enum class EasingCurve {
        Linear,
        EaseIn,
        EaseOut,
        EaseInOut,
        Spring
    };
    
    AnimatedValue(float initial = 0.0f);
    
    void setTarget(float target, int durationMs, EasingCurve curve = EasingCurve::EaseOut);
    void setSpring(float target, float stiffness, float damping);
    void stop();
    
    float getCurrentValue() const { return currentValue_; }
    bool isAnimating() const { return isAnimating_; }
    
    void update(float deltaTimeMs);
    
private:
    float currentValue_;
    float targetValue_;
    float startValue_;
    float velocity_;
    
    int durationMs_;
    int elapsedMs_;
    
    EasingCurve curve_;
    bool isAnimating_;
    
    // Spring physics
    float springStiffness_;
    float springDamping_;
    bool useSpring_;
    
    float easeValue(float t) const;
};

} // namespace zenith
