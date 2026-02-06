/*
  ToastNotificationManager.cpp

  Implementation of the toast notification system
*/

#include "ToastNotificationManager.h"
#include <zenith_core/utils/PlatformLogUtils.h>
#include <algorithm>

namespace zenith::UI {

ToastNotification::ToastNotification(const juce::String& message, ToastType type,
                                   const Callback& onAction,
                                   const juce::String& actionText,
                                   int duration)
    : message_(message)
    , type_(type)
    , onAction_(onAction)
    , actionText_(actionText)
    , durationMs_(duration)
    , isActive_(false)
    , position_(ToastPosition::BottomRight)
    , margin_(10)
    , isHovered_(false)
    , animationProgress_(0.0f) {

    setWantsKeyboardFocus(true);
    setInterceptsMouseClicks(true, true);

    // Set up dismiss timer
    if (durationMs_ > 0) {
        dismissTimer_.startTimer(durationMs_);
    }
}

void ToastNotification::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat();
    auto config = getConfig();

    // Draw shadow
    if (config.shadowBlur > 0) {
        g.setColour(config.shadowColour.withAlpha(0.3f));
        g.drawRoundedRectangle(bounds.reduced(2.0f),
                              6.0f,
                              config.shadowBlur + 2);
    }

    // Draw background
    g.setColour(config.background);
    g.fillRoundedRectangle(bounds, 6.0f);

    // Draw border
    g.setColour(config.border);
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

    // Draw text
    g.setColour(config.text);
    auto font = g.getCurrentFont().withHeight(14.0f);
    g.setFont(font);

    // Word wrap the message
    auto textBounds = bounds.reduced(12.0f, 8.0f);
    auto wrappedText = font.getHorizontalLayout().computeWrappedText(message_, textBounds.getWidth());
    g.drawMultiLineText(wrappedText, textBounds.getX(), textBounds.getY(), textBounds.getWidth());

    // Draw action button if present
    if (onAction_ && actionText_.isNotEmpty()) {
        auto buttonBounds = textBounds.withTrimmedTop(textBounds.getHeight() - 30);
        buttonBounds = buttonBounds.withWidth(80).withTrimmedRight(20);

        // Button background
        auto buttonColour = isHovered_ ? config.text : config.background;
        g.setColour(buttonColour);
        g.fillRoundedRectangle(buttonBounds.toFloat(), 4.0f);

        // Button text
        g.setColour(isHovered_ ? config.background : config.text);
        g.setFont(font.withHeight(12.0f));
        g.drawText(actionText_, buttonBounds, juce::Justification::centred);
    }

    // Draw icon if present
    if (config.icon.isNotEmpty()) {
        auto iconBounds = bounds.reduced(12.0f).withTrimmedRight(bounds.getWidth() - 30);
        g.setColour(config.text);
        // Placeholder for icon - would need actual icon drawing
        g.drawEllipse(iconBounds, 1.0f);
    }
}

void ToastNotification::resized() {
    auto config = getConfig();
    auto contentHeight = 40; // Approximate height based on content

    if (onAction_) {
        contentHeight += 20; // Add space for action button
    }

    setBounds(calculateTargetPosition().withHeight(contentHeight));
}

void ToastNotification::mouseEnter(const juce::MouseEvent& event) {
    isHovered_ = true;
    repaint();
}

void ToastNotification::mouseExit(const juce::MouseEvent& event) {
    isHovered_ = false;
    repaint();
}

void ToastNotification::mouseDown(const juce::MouseEvent& event) {
    // Check if action button was clicked
    if (onAction_) {
        auto textBounds = getLocalBounds().reduced(12.0f, 8.0f);
        auto buttonBounds = textBounds.withTrimmedTop(textBounds.getHeight() - 30);
        buttonBounds = buttonBounds.withWidth(80).withTrimmedRight(20);

        if (buttonBounds.contains(event.position)) {
            // Action button clicked
            if (onAction_) {
                onAction_();
            }
            dismiss();
            return;
        }
    }

    // Dismiss on click if no action or outside action button
    dismiss();
}

bool ToastNotification::keyPressed(const juce::KeyPress& key) {
    if (key.isKeyCode(juce::KeyPress::escapeKey)) {
        dismiss();
        return true;
    }
    else if (key.isKeyCode(juce::KeyPress::returnKey) && onAction_) {
        if (onAction_) {
            onAction_();
        }
        dismiss();
        return true;
    }
    return false;
}

void ToastNotification::drawSkia(SkCanvas* canvas) {
    // Basic Skia rendering for toast
    // In a real implementation, we would use Skia primitives here
    // for better performance and glassmorphism.
    // For now, we'll let juce::Graphics handled it via paint(),
    // but we need this implementation to make the class non-abstract.
}

void ToastNotification::show() {
    isActive_.store(true);
    animateIn();
}

void ToastNotification::dismiss() {
    if (isActive_.exchange(false)) {
        animateOut();
    }
}

bool ToastNotification::isActive() const {
    return isActive_.load();
}

void ToastNotification::animateIn() {
    animationProgress_ = 0.0f;
    animateToTarget();
}

void ToastNotification::animateOut() {
    animationProgress_ = 1.0f;
    animateToTarget();
}

void ToastNotification::updateAnimation(float progress) {
    animationProgress_ = progress;
    currentBounds = currentBounds.withTrimmedTop(10 * (1.0f - progress));
    repaint();
}

void ToastNotification::setPosition(ToastPosition position) {
    position_ = position;
    resized(); // Recalculate position
}

void ToastNotification::setMargin(int margin) {
    margin_ = margin;
    resized(); // Recalculate position
}

void ToastNotification::setDuration(int durationMs) {
    durationMs_ = durationMs;
    if (isActive_ && durationMs_ > 0) {
        dismissTimer_.startTimer(durationMs_);
    }
}

void ToastNotification::setAction(const Callback& callback, const juce::String& text) {
    onAction_ = callback;
    actionText_ = text;
    resized(); // Update layout
    repaint();
}

void ToastNotification::setDismissible(bool dismissible) {
    if (!dismissible && isActive_) {
        dismissTimer_.stopTimer();
    }
}

ToastNotification::ToastConfig ToastNotification::getConfig() const {
    ToastConfig config;

    switch (type_) {
        case ToastType::Info:
            config.background = ZenithDesignSystem::getColor(ZenithDesignSystem::Colors::Primary).withAlpha(0.9f);
            config.border = ZenithDesignSystem::getColor(ZenithDesignSystem::Colors::Primary);
            config.text = juce::Colours::white;
            config.actionText = juce::Colours::white;
            config.icon = "ℹ";
            config.shadowBlur = 10;
            config.shadowColour = ZenithDesignSystem::getColor(ZenithDesignSystem::Colors::Primary);
            break;

        case ToastType::Warning:
            config.background = ZenithDesignSystem::getColor(ZenithDesignSystem::Colors::Warning).withAlpha(0.9f);
            config.border = ZenithDesignSystem::getColor(ZenithDesignSystem::Colors::Warning);
            config.text = juce::Colours::black;
            config.actionText = juce::Colours::black;
            config.icon = "⚠";
            config.shadowBlur = 10;
            config.shadowColour = ZenithDesignSystem::getColor(ZenithDesignSystem::Colors::Warning);
            break;

        case ToastType::Error:
            config.background = ZenithDesignSystem::getColor(ZenithDesignSystem::Colors::Error).withAlpha(0.9f);
            config.border = ZenithDesignSystem::getColor(ZenithDesignSystem::Colors::Error);
            config.text = juce::Colours::white;
            config.actionText = juce::Colours::white;
            config.icon = "✕";
            config.shadowBlur = 15;
            config.shadowColour = ZenithDesignSystem::getColor(ZenithDesignSystem::Colors::Error);
            break;

        case ToastType::Success:
            config.background = ZenithDesignSystem::getColor(ZenithDesignSystem::Colors::Success).withAlpha(0.9f);
            config.border = ZenithDesignSystem::getColor(ZenithDesignSystem::Colors::Success);
            config.text = juce::Colours::white;
            config.actionText = juce::Colours::white;
            config.icon = "✓";
            config.shadowBlur = 10;
            config.shadowColour = ZenithDesignSystem::getColor(ZenithDesignSystem::Colors::Success);
            break;

        case ToastType::Critical:
            config.background = juce::Colours::darkred.withAlpha(0.95f);
            config.border = juce::Colours::red;
            config.text = juce::Colours::white;
            config.actionText = juce::Colours::white;
            config.icon = "⚡";
            config.shadowBlur = 20;
            config.shadowColour = juce::Colours::red;
            break;
    }

    return config;
}

juce::Rectangle<int> ToastNotification::calculateTargetPosition() const {
    return getDefaultBounds(position_);
}

void ToastNotification::timerCallback() {
    dismiss();
}

void ToastNotification::animateToTarget() {
    // Simple slide animation - in a real implementation, you'd use
    // the animation system or a custom animator
    updateAnimation(animationProgress_);
}

ToastNotificationManager& ToastNotificationManager::getInstance() {
    static ToastNotificationManager instance;
    return instance;
}

ToastNotificationManager::ToastNotificationManager()
    : maxVisibleToasts_(3)
    , defaultPosition_(ToastPosition::BottomRight)
    , animationDurationMs_(300)
    , toastMargin_(10) {

    setName("ToastNotificationManager");
    createParentIfNeeded();

    // Start timer for processing queue
    startTimer(100); // Check queue every 100ms
}

ToastNotificationManager::~ToastNotificationManager() {
    clearAll();
}

void ToastNotificationManager::showToast(const juce::String& message,
                                       const Callback& onAction,
                                       ToastType type,
                                       const juce::String& actionText,
                                       int duration) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto toast = std::make_unique<ToastNotification>(message, type, onAction, actionText, duration);
    toast->setPosition(defaultPosition_);

    ToastQueueItem item;
    item.toast = std::move(toast);
    item.timestamp = juce::Time::getCurrentTime();
    item.isProcessing = false;

    toastQueue_.push_back(std::move(item));

    ZENITH_LOG_INFO("Toast queued: " + message.toStdString());
}

void ToastNotificationManager::showToast(const UIError& error, const Callback& onAction) {
    juce::String message = error.message;

    if (error.details.isNotEmpty()) {
        message += ": " + error.details;
    }

    showToast(message, onAction,
              convertSeverityToToastType(error.severity),
              error.actionText.isNotEmpty() ? error.actionText : "OK",
              error.severity == ErrorSeverity::Fatal ? 0 : 5000); // No auto-dismiss for fatal
}

void ToastNotificationManager::clearAll() {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto* toast : activeToasts_) {
        if (toast->isActive()) {
            toast->dismiss();
        }
    }

    toastQueue_.clear();
    activeToasts_.clear();

    positionStacks_.clear();
}

void ToastNotificationManager::dismissToast(ToastNotification* toast) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = std::find(activeToasts_.begin(), activeToasts_.end(), toast);
    if (it != activeToasts_.end()) {
        removeToast(toast);
    }
}

void ToastNotificationManager::setMaxVisibleToasts(int max) {
    std::lock_guard<std::mutex> lock(mutex_);
    maxVisibleToasts_ = juce::jmax(1, max);

    // Trim active toasts if needed
    while (activeToasts_.size() > maxVisibleToasts_) {
        auto* toast = activeToasts_.back();
        if (toast->isActive()) {
            toast->dismiss();
        }
    }
}

void ToastNotificationManager::setDefaultPosition(ToastPosition position) {
    std::lock_guard<std::mutex> lock(mutex_);
    defaultPosition_ = position;
}

void ToastNotificationManager::setAnimationDuration(int durationMs) {
    animationDurationMs_ = juce::jmax(0, durationMs);
}

void ToastNotificationManager::setToastMargin(int margin) {
    std::lock_guard<std::mutex> lock(mutex_);
    toastMargin_ = juce::jmax(0, margin);
}

bool ToastNotificationManager::hasActiveToasts() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return !activeToasts_.empty();
}

int ToastNotificationManager::getActiveToastCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return activeToasts_.size();
}

void ToastNotificationManager::paint(juce::Graphics& g) {
    // Background overlay when toasts are active
    if (hasActiveToasts()) {
        g.setColour(juce::Colours::black.withAlpha(0.1f));
        g.fillAll();
    }
}

void ToastNotificationManager::resized() {
    positionActiveToasts();
}

void ToastNotificationManager::timerCallback() {
    std::lock_guard<std::mutex> lock(mutex_);

    // Process queue if we have space
    if (activeToasts_.size() < maxVisibleToasts_) {
        processNextToast();
    }

    // Clean up inactive toasts
    activeToasts_.erase(
        std::remove_if(activeToasts_.begin(), activeToasts_.end(),
            [](ToastNotification* toast) {
                return !toast->isActive();
            }),
        activeToasts_.end());

    // Reposition active toasts
    positionActiveToasts();
}

void ToastNotificationManager::processNextToast() {
    if (toastQueue_.empty()) {
        return;
    }

    // Find next unprocessed toast
    for (auto& item : toastQueue_) {
        if (!item.isProcessing) {
            item.isProcessing = true;
            auto* toast = item.toast.get();

            // Add to active list
            activeToasts_.push_back(toast);

            // Show toast
            toast->show();

            ZENITH_LOG_INFO("Toast shown: " + toast->getMessage().toStdString());
            break;
        }
    }
}

void ToastNotificationManager::positionActiveToasts() {
    for (size_t i = 0; i < activeToasts_.size(); i++) {
        auto* toast = activeToasts_[i];
        auto bounds = calculateStackedPosition(defaultPosition_, (int)i);
        toast->setBounds(bounds);
    }
}

void ToastNotificationManager::removeToast(ToastNotification* toast) {
    auto it = std::find(activeToasts_.begin(), activeToasts_.end(), toast);
    if (it != activeToasts_.end()) {
        activeToasts_.erase(it);

        // Clean up position stacks
        for (auto& stackPair : positionStacks_) {
            auto& stack = stackPair.second;
            stack.positions.erase(
                std::remove(stack.positions.begin(), stack.positions.end(), toast->getBounds()),
                stack.positions.end());
        }
    }
}

void ToastNotificationManager::createParentIfNeeded() {
    if (!parent_.getComponent()) {
        // Find or create a parent component to host the manager
        // This is a simplified version - in practice you'd want to attach to the main window
        auto* mainComp = juce::TopLevelWindow::getActiveTopLevelWindow();
        if (mainComp) {
            parent_ = mainComp;
        } else {
            // Create a dummy parent if needed
            parent_ = new juce::Component();
            parent_->addToDesktop(juce::ComponentPeer::windowIsTemporary);
        }
    }
}

juce::Rectangle<int> ToastNotificationManager::getDefaultBounds(ToastPosition position) const {
    auto* parent = parent_.getComponent();
    if (!parent) {
        return juce::Rectangle<int>(100, 100, 300, 50);
    }

    auto screenBounds = parent->getScreenBounds();
    auto toastWidth = 300;
    auto toastHeight = 60;
    int margin = toastMargin_;

    switch (position) {
        case ToastPosition::TopLeft:
            return juce::Rectangle<int>(screenBounds.getX() + margin,
                                      screenBounds.getY() + margin,
                                      toastWidth, toastHeight);

        case ToastPosition::TopRight:
            return juce::Rectangle<int>(screenBounds.getRight() - toastWidth - margin,
                                      screenBounds.getY() + margin,
                                      toastWidth, toastHeight);

        case ToastPosition::BottomLeft:
            return juce::Rectangle<int>(screenBounds.getX() + margin,
                                      screenBounds.getBottom() - toastHeight - margin,
                                      toastWidth, toastHeight);

        case ToastPosition::BottomRight:
            return juce::Rectangle<int>(screenBounds.getRight() - toastWidth - margin,
                                      screenBounds.getBottom() - toastHeight - margin,
                                      toastWidth, toastHeight);

        case ToastPosition::TopCenter:
            return juce::Rectangle<int>(screenBounds.getCentreX() - toastWidth / 2,
                                      screenBounds.getY() + margin,
                                      toastWidth, toastHeight);

        case ToastPosition::BottomCenter:
            return juce::Rectangle<int>(screenBounds.getCentreX() - toastWidth / 2,
                                      screenBounds.getBottom() - toastHeight - margin,
                                      toastWidth, toastHeight);

        case ToastPosition::Center:
            return juce::Rectangle<int>(screenBounds.getCentreX() - toastWidth / 2,
                                      screenBounds.getCentreY() - toastHeight / 2,
                                      toastWidth, toastHeight);
    }

    return juce::Rectangle<int>(0, 0, toastWidth, toastHeight);
}

juce::Rectangle<int> ToastNotificationManager::calculateStackedPosition(ToastPosition position, int index) const {
    auto baseBounds = getDefaultBounds(position);
    int offset = (index * (60 + toastMargin_)); // 60px height + margin

    switch (position) {
        case ToastPosition::TopLeft:
        case ToastPosition::TopCenter:
        case ToastPosition::TopRight:
            return baseBounds.withY(baseBounds.getY() + offset);

        case ToastPosition::BottomLeft:
        case ToastPosition::BottomCenter:
        case ToastPosition::BottomRight:
            return baseBounds.withY(baseBounds.getY() - offset);

        case ToastPosition::Center:
            // Center toasts stack vertically in the middle
            return baseBounds.withY(baseBounds.getY() + (index - activeToasts_.size() / 2) * 70);
    }

    return baseBounds;
}

ToastType ToastNotificationManager::convertSeverityToToastType(ErrorSeverity severity) {
    switch (severity) {
        case ErrorSeverity::Fatal:
            return ToastType::Critical;
        case ErrorSeverity::Critical:
            return ToastType::Error;
        case ErrorSeverity::Error:
            return ToastType::Error;
        case ErrorSeverity::Warning:
            return ToastType::Warning;
        case ErrorSeverity::Info:
            return ToastType::Info;
        case ErrorSeverity::Debug:
            return ToastType::Info;
        default:
            return ToastType::Info;
    }
}

} // namespace zenith::UI