/*
  ToastNotificationManager.h

  Toast notification system for displaying non-intrusive messages to users

  Features:
  - Animated toast notifications with slide-in/out effects
  - Multiple toast positions (top-left, top-right, bottom-left, bottom-right, center)
  - Auto-dismiss with configurable duration
  - Action buttons on toasts
  - Priority-based stacking
  - Keyboard accessibility
  - Theme integration with Zenith Design System
*/

#pragma once

#include "../framework/SkiaComponent.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../framework/UIErrorHandler.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_core/juce_core.h>
#include <functional>
#include <queue>
#include <memory>
#include <atomic>

namespace zenith::UI {

/**
 * Toast notification types for styling and behavior
 */
enum class ToastType {
    Info,       // Blue toasts for informational messages
    Warning,    // Yellow toasts for warnings
    Error,      // Red toasts for errors
    Success,    // Green toasts for success messages
    Critical    // Bold red toasts for critical errors
};

/**
 * Toast position on screen
 */
enum class ToastPosition {
    TopLeft,        // Top-left corner
    TopRight,       // Top-right corner
    BottomLeft,     // Bottom-left corner
    BottomRight,    // Bottom-right corner
    TopCenter,      // Top center
    BottomCenter,   // Bottom center
    Center          // Screen center
};

/**
 * Individual toast notification
 */
class ToastNotification : public SkiaComponent {
public:
    using Callback = std::function<void()>;

    ToastNotification(const juce::String& message, ToastType type,
                     const Callback& onAction = nullptr,
                     const juce::String& actionText = "OK",
                     int duration = 5000);

    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    void mouseDown(const juce::MouseEvent& event) override;
    bool keyPressed(const juce::KeyPress& key) override;
    void drawSkia(SkCanvas* canvas) override;

    // Public methods
    void show();
    void dismiss();
    bool isActive() const;

    // Animation methods
    void animateIn();
    void animateOut();
    void updateAnimation(float progress);

    // Position setters
    void setPosition(ToastPosition position);
    void setMargin(int margin);

    // Configuration
    void setDuration(int durationMs);
    void setAction(const Callback& callback, const juce::String& text);
    void setDismissible(bool dismissible);

    // Getters
    ToastType getType() const;
    juce::String getMessage() const;
    int getDuration() const;

private:
    struct ToastConfig {
        juce::Colour background;
        juce::Colour border;
        juce::Colour text;
        juce::Colour actionText;
        juce::String icon;
        int shadowBlur;
        juce::Colour shadowColour;
    };

    juce::String message_;
    ToastType type_;
    Callback onAction_;
    juce::String actionText_;
    int durationMs_;
    juce::Timer dismissTimer_;
    std::atomic<bool> isActive_;
    ToastPosition position_;
    int margin_;
    bool isHovered_;
    float animationProgress_;
    juce::Rectangle<int> targetBounds_;
    juce::Rectangle<int> currentBounds_;

    // Calculate toast config based on type
    ToastConfig getConfig() const;

    // Calculate target position
    juce::Rectangle<int> calculateTargetPosition() const;

    // Timer callback
    void timerCallback() override;

    // Animation helper
    void animateToTarget();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ToastNotification)
};

/**
 * Toast Notification Manager Singleton
 *
 * Manages the display of toast notifications with:
 * - Queue management for multiple toasts
 * - Animation coordination
 * - Position stacking
 * - Keyboard accessibility
 */
class ToastNotificationManager : public juce::Component, public juce::Timer {
public:
    /**
     * Get the singleton instance
     */
    static ToastNotificationManager& getInstance();

    /**
     * Show a toast notification
     */
    void showToast(const juce::String& message,
                  const Callback& onAction = nullptr,
                  ToastType type = ToastType::Info,
                  const juce::String& actionText = "OK",
                  int duration = 5000);

    /**
     * Show an error toast from UIError
     */
    void showToast(const UIError& error, const Callback& onAction = nullptr);

    /**
     * Show success toast
     */
    void showSuccess(const juce::String& message, const Callback& onAction = nullptr) {
        showToast(message, onAction, ToastType::Success, "OK", 4000);
    }

    /**
     * Show warning toast
     */
    void showWarning(const juce::String& message, const Callback& onAction = nullptr) {
        showToast(message, onAction, ToastType::Warning, "OK", 6000);
    }

    /**
     * Show error toast
     */
    void showError(const juce::String& message, const Callback& onAction = nullptr) {
        showToast(message, onAction, ToastType::Error, "OK", 7000);
    }

    /**
     * Show info toast
     */
    void showInfo(const juce::String& message, const Callback& onAction = nullptr) {
        showToast(message, onAction, ToastType::Info, "OK", 5000);
    }

    /**
     * Clear all active toasts
     */
    void clearAll();

    /**
     * Dismiss a specific toast
     */
    void dismissToast(ToastNotification* toast);

    /**
     * Set the maximum number of toasts to display at once
     */
    void setMaxVisibleToasts(int max);

    /**
     * Set the default toast position
     */
    void setDefaultPosition(ToastPosition position);

    /**
     * Set the toast animation duration
     */
    void setAnimationDuration(int durationMs);

    /**
     * Set margin between toasts
     */
    void setToastMargin(int margin);

    /**
     * Check if manager has active toasts
     */
    bool hasActiveToasts() const;

    /**
     * Get the number of active toasts
     */
    int getActiveToastCount() const;

    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

    // Timer callback
    void timerCallback() override;

private:
    ToastNotificationManager();
    ~ToastNotificationManager() override;

    // Prevent copying
    ToastNotificationManager(const ToastNotificationManager&) = delete;
    ToastNotificationManager& operator=(const ToastNotificationManager&) = delete;

    // Private implementation
    struct ToastQueueItem {
        std::unique_ptr<ToastNotification> toast;
        juce::Timestamp timestamp;
        bool isProcessing;
    };

    std::vector<ToastQueueItem> toastQueue_;
    std::vector<ToastNotification*> activeToasts_;
    juce::Component::SafePointer<juce::Component> parent_;
    int maxVisibleToasts_;
    ToastPosition defaultPosition_;
    int animationDurationMs_;
    int toastMargin_;
    mutable std::mutex mutex_;

    // Position management
    struct PositionStack {
        std::vector<juce::Rectangle<int>> positions;
        int getAvailablePosition();
        void pushPosition(const juce::Rectangle<int>& pos);
        void clear();
    };

    std::map<ToastPosition, PositionStack> positionStacks_;

    // Internal methods
    void showToastInternal(const ToastNotification::Callback& onAction,
                          ToastType type, const juce::String& actionText,
                          int duration);

    void processNextToast();
    void positionActiveToasts();
    void removeToast(ToastNotification* toast);
    void createParentIfNeeded();

    // Position helpers
    juce::Rectangle<int> getDefaultBounds(ToastPosition position) const;
    juce::Rectangle<int> calculateStackedPosition(ToastPosition position, int index) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ToastNotificationManager)
};

// Global helper functions
inline void showToast(const juce::String& message, ToastType type = ToastType::Info) {
    ToastNotificationManager::getInstance().showToast(message, nullptr, type);
}

inline void showSuccess(const juce::String& message) {
    ToastNotificationManager::getInstance().showSuccess(message);
}

inline void showWarning(const juce::String& message) {
    ToastNotificationManager::getInstance().showWarning(message);
}

inline void showError(const juce::String& message) {
    ToastNotificationManager::getInstance().showError(message);
}

inline void showInfo(const juce::String& message) {
    ToastNotificationManager::getInstance().showInfo(message);
}

} // namespace zenith::UI