/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

// SkiaComponent.h


extern "C++" {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-warning-option"
#pragma clang diagnostic ignored "-Wattributes"
#include <core/SkCanvas.h>
#include <core/SkColor.h>
#include <core/SkFont.h>
#include <core/SkMaskFilter.h>
#include <core/SkPaint.h>
#include <core/SkPath.h>
#include <core/SkRRect.h>
#include <core/SkRect.h>
#include <core/SkShader.h>
#pragma clang diagnostic pop
}
#include <juce_gui_basics/juce_gui_basics.h>

#include "SkiaAccessibility.h"
#include "ZenithDesignSystem.h"
#include "DirtyRectManager.h"
#include "../design-system/ZenithTheme.h"
#include "TabOrderManager.h"
#include "KeyboardShortcutManager.h"
#include "../validation/Validator.h"
#include <functional>
#include <map>
#include <memory>
#include <vector>

#include "Animation.h"
#include "AnimationCoordinator.h"

namespace zenith {

  using AnimatedValue = animation::AnimatedValue<float>;

  template <typename T> class ValueHistory {
  // ... (keep ValueHistory as is)
  public:
    void push(T value) {
      // Remove redo history
      if (!history_.empty() && currentIndex_ < history_.size() - 1) {
        history_.erase(history_.begin() + currentIndex_ + 1, history_.end());
      }

      history_.push_back(value);
      currentIndex_ = history_.size() - 1;

      // Limit history size
      if (history_.size() > 50) {
        history_.erase(history_.begin());
        if (currentIndex_ > 0)
          currentIndex_--;
      }
    }

    bool canUndo() const { return !history_.empty() && currentIndex_ > 0; }
    bool canRedo() const {
      return !history_.empty() && currentIndex_ < history_.size() - 1;
    }
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

  class SkiaComponent : public juce::Component,
                        public virtual juce::Timer,
                        public juce::KeyListener,
                        public zenith::animation::AnimationListener,
                        public std::enable_shared_from_this<SkiaComponent> {
  public:
    struct AIElementInfo {
      SkRect bounds;
      juce::String type; // "knob", "fader", "button"
      juce::String id;
      juce::String label;
      juce::String parameterId;
      float currentValue = 0.0f;
    };

    explicit SkiaComponent();
    ~SkiaComponent() override;

    virtual void drawSkia(SkCanvas *canvas) = 0;

    // AI Vision Hook
    virtual std::vector<AIElementInfo> getInspectableElements() { return {}; }

#ifdef DEBUG
    virtual void drawDebug(SkCanvas *canvas);
#endif

    void setGlowEnabled(bool enabled) {
      glowEnabled_ = enabled;
      markDirty();
    }
    bool isGlowEnabled() const { return glowEnabled_; }

    void setGlowColor(SkColor color) {
      glowColor_ = color;
      markDirty();
    }
    SkColor getGlowColor() const { return glowColor_; }

    void setGlowRadius(float radius) {
      glowRadius_ = radius;
      markDirty();
    }
    float getGlowRadius() const { return glowRadius_; }

    // Lifecycle hooks
    virtual void onShow() {}
    virtual void onHide() {}
    virtual void onResize() {}
    virtual void onParentChanged() {}

    // Interaction hooks
    virtual void onHoverEnter() {}
    virtual void onHoverExit() {}
    virtual void onFocusGained() {}
    virtual void onFocusLost() {}

    // Widget delegation hooks
    virtual void onMouseDown(const juce::MouseEvent &e) {
      juce::ignoreUnused(e);
    }
    virtual void onMouseDrag(const juce::MouseEvent &e) {
      juce::ignoreUnused(e);
    }
    virtual void onMouseUp(const juce::MouseEvent &e) { juce::ignoreUnused(e); }

    // Force hit testing for transparent Skia components
    bool hitTest(int x, int y) override;

    void animateTo(const juce::String &property, float target, int durationMs);
    void animateWithSpring(const juce::String &property, float target,
                           float stiffness, float damping);
    void stopAnimation(const juce::String &property);
    void stopAllAnimations();

    float getAnimatedValue(const juce::String &property) const;
    bool isAnimating(const juce::String &property) const;

    bool isHovered() const { return isHovered_; }
    void setHovered(bool hovered) {
      isHovered_ = hovered;
      markDirty();
    }
    bool isFocused() const { return juce::Component::hasKeyboardFocus(true); }

    /**
     * @brief Mark entire component as needing repaint
     * 
     * Use markDirtyRect() for localized updates when possible.
     */
    void markDirty() {
      needsRepaint_ = true;
      dirtyManager_.markFullDirty(static_cast<float>(getWidth()), 
                                   static_cast<float>(getHeight()));
      repaint();
    }
    
    /**
     * @brief Mark specific region as needing repaint (PREFERRED)
     * @param rect The dirty region in local coordinates
     * 
     * Use this instead of markDirty() for efficient partial repaints.
     * The dirty region will be clipped during rendering.
     */
    void markDirtyRect(const juce::Rectangle<float>& rect) {
      dirtyManager_.addDirtyRect(rect);
      repaint(rect.toNearestInt());
    }
    
    void markDirtyRect(const SkRect& rect) {
      dirtyManager_.addDirtyRect(rect);
      juce::Rectangle<int> juceRect(
          static_cast<int>(rect.fLeft), static_cast<int>(rect.fTop),
          static_cast<int>(rect.width()), static_cast<int>(rect.height()));
      repaint(juceRect);
    }
    
    /**
     * @brief Get accumulated dirty regions for rendering
     */
    std::vector<SkRect> getDirtyRects() const {
      return dirtyManager_.getDirtyRects();
    }
    
    /**
     * @brief Check if any dirty regions exist
     */
    bool hasDirtyRegions() const {
      return dirtyManager_.hasDirtyRegions();
    }
    
    /**
     * @brief Clear dirty regions after rendering
     */
    void clearDirtyRects() {
      dirtyManager_.clearDirtyRects();
      needsRepaint_ = false;
    }
    
    bool isDirty() const { return needsRepaint_ || dirtyManager_.hasDirtyRegions(); }

    void paint(juce::Graphics &g) override;
    void resized() override;
    void mouseEnter(const juce::MouseEvent &e) override;
    void mouseExit(const juce::MouseEvent &e) override;
    void focusGained(juce::Component::FocusChangeType cause) override;
    void focusLost(juce::Component::FocusChangeType cause) override;
    void timerCallback() override;

    // AnimationListener interface
    void onAnimationTick(float deltaMs) override;

    /**
     * @brief Update internal property animations.
     * @return true if animations are still active, false if finished.
     */
    bool updateInternalAnimations(float deltaMs);

    // Avoid hiding Component::keyPressed
    using juce::Component::keyPressed;
    bool keyPressed(const juce::KeyPress &key,
                    juce::Component *origin) override;

    virtual void onEnterPressed() {}
    virtual void onEscapePressed() {}

    // Keyboard navigation support
    virtual void setTabGroup(zenith::UI::TabGroup group);
    virtual zenith::UI::TabGroup getTabGroup() const;
    virtual void setTabOrder(int order);
    virtual int getTabOrder() const;
    virtual void setFocusable(bool focusable);
    virtual bool isFocusable() const;
    virtual bool isInTabOrder() const;

    // Shortcut integration
    virtual void registerShortcut(const zenith::UI::KeyboardShortcut& shortcut);
    virtual void unregisterShortcut(const juce::KeyPress& key);
    virtual bool handleKeyPress(const juce::KeyPress& key);

    // Focus management
    virtual void grabFocusWithReason(juce::Component::FocusChangeType reason);
    virtual bool requestFocusNext();
    virtual bool requestFocusPrevious();

    virtual void showContextMenu();
    virtual void handleContextMenuResult(int result);

    // Context menu actions
    virtual void resetToDefault() {}
    virtual void copyValue() {}
    virtual void pasteValue() {}
    virtual void toggleMIDILearn() {}

    static void setTargetFPS(int fps);
    static int getTargetFPS();

    void setHelpText(const juce::String &title, const juce::String &description) {
      helpTitle_ = title;
      helpDescription_ = description;
    }

    // Global callback for "Info View" style help (Ableton-like)
    static std::function<void(const juce::String &, const juce::String &)>
        globalHelpCallback;

  protected:
    void drawChildren(SkCanvas *canvas); // Helper to draw child components
    SkCanvas *getSkiaCanvas(juce::Graphics &g);

    void applyGlow(SkPaint &paint, float intensity = 1.0f);
    void animateColorChange();

  private:
    bool isHovered_ = false;
    juce::String helpTitle_;
    juce::String helpDescription_;
    bool needsRepaint_ = true;
    SkColor glowColor_ = SkColorSetRGB(0, 255, 255);
    float glowRadius_ = 0.0f;
    bool glowEnabled_ = false;
    bool isMIDILearning_ = false;

    // Keyboard navigation state
    zenith::UI::TabGroup tabGroup_ = zenith::UI::TabGroup::None;
    int tabOrder_ = 0;
    bool isFocusable_ = true;
    bool isInTabOrder_ = true;
    std::map<juce::KeyPress, zenith::UI::KeyboardShortcut> registeredShortcuts_;
    std::shared_ptr<zenith::UI::ShortcutContext> shortcutContext_;

    static int systemRefreshRate_;
    static int targetFPS_;
    static int getSystemRefreshRate();

    DirtyRectManager dirtyManager_;
    std::map<juce::String, std::unique_ptr<AnimatedValue>> animations_;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaComponent)
  };

} // namespace zenith