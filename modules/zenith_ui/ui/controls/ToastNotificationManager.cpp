/*
  ToastNotificationManager.cpp

  Minimal, buildable toast notification implementation.

  The previous implementation mixed non-existent JUCE APIs and had a header
  include-cycle with UIErrorHandler. This version keeps the public surface
  area from ToastNotificationManager.h but implements a simpler, correct JUCE
  component-based toast system.
*/

#include "ToastNotificationManager.h"
#include "../framework/UIErrorHandler.h" // UIError + ErrorSeverity
#include <algorithm>

namespace zenith::UI {

//==============================================================================
ToastNotification::ToastNotification(const juce::String& message,
                                     ToastType type,
                                     const Callback& onAction,
                                     const juce::String& actionText,
                                     int duration)
    : message_(message),
      type_(type),
      onAction_(onAction),
      actionText_(actionText),
      durationMs_(duration),
      isActive_(false),
      position_(ToastPosition::BottomRight),
      margin_(10),
      isHovered_(false),
      animationProgress_(0.0f) {
  setWantsKeyboardFocus(true);
  setInterceptsMouseClicks(true, true);
}

void ToastNotification::paint(juce::Graphics& g) {
  const auto cfg = getConfig();
  const auto bounds = getLocalBounds().toFloat();

  g.setColour(cfg.background);
  g.fillRoundedRectangle(bounds, 8.0f);

  g.setColour(cfg.border);
  g.drawRoundedRectangle(bounds, 8.0f, 1.0f);

  auto content = getLocalBounds().reduced(12);
  g.setColour(cfg.text);
  g.setFont(g.getCurrentFont().withHeight(14.0f));
  g.drawFittedText(message_, content, juce::Justification::topLeft, 3);

  if (onAction_ && actionText_.isNotEmpty()) {
    auto btn = content.removeFromBottom(26);
    btn = btn.removeFromRight(90).reduced(0, 2);

    g.setColour(isHovered_ ? cfg.text : cfg.border);
    g.fillRoundedRectangle(btn.toFloat(), 6.0f);

    g.setColour(isHovered_ ? cfg.background : cfg.text);
    g.setFont(g.getCurrentFont().withHeight(12.0f));
    g.drawFittedText(actionText_, btn, juce::Justification::centred, 1);
  }
}

void ToastNotification::resized() {
  // Layout is calculated in paint using the current bounds.
}

void ToastNotification::mouseEnter(const juce::MouseEvent&) {
  isHovered_ = true;
  repaint();
}

void ToastNotification::mouseExit(const juce::MouseEvent&) {
  isHovered_ = false;
  repaint();
}

void ToastNotification::mouseDown(const juce::MouseEvent& e) {
  if (onAction_ && actionText_.isNotEmpty()) {
    auto content = getLocalBounds().reduced(12);
    auto btn = content.removeFromBottom(26);
    btn = btn.removeFromRight(90).reduced(0, 2);
    if (btn.contains(e.getPosition())) {
      onAction_();
      dismiss();
      return;
    }
  }

  dismiss();
}

bool ToastNotification::keyPressed(const juce::KeyPress& key) {
  if (key == juce::KeyPress::escapeKey) {
    dismiss();
    return true;
  }
  return false;
}

void ToastNotification::drawSkia(SkCanvas*) {
  // Intentionally empty: this component uses JUCE paint(). Skia integration
  // can be added later without breaking the build.
}

void ToastNotification::show() {
  isActive_.store(true);
  setVisible(true);

  if (durationMs_ > 0) {
    startTimer(durationMs_);
  }
}

void ToastNotification::dismiss() {
  stopTimer();
  isActive_.store(false);
  setVisible(false);
}

bool ToastNotification::isActive() const { return isActive_.load(); }

void ToastNotification::animateIn() { show(); }
void ToastNotification::animateOut() { dismiss(); }

void ToastNotification::updateAnimation(float progress) {
  animationProgress_ = progress;
  repaint();
}

void ToastNotification::setPosition(ToastPosition position) { position_ = position; }
void ToastNotification::setMargin(int margin) { margin_ = margin; }

void ToastNotification::setDuration(int durationMs) {
  durationMs_ = durationMs;
  if (isActive() && durationMs_ > 0) {
    startTimer(durationMs_);
  }
}

void ToastNotification::setAction(const Callback& callback, const juce::String& text) {
  onAction_ = callback;
  actionText_ = text;
  repaint();
}

void ToastNotification::setDismissible(bool) {
  // Current implementation always allows dismissal.
}

ToastType ToastNotification::getType() const { return type_; }
juce::String ToastNotification::getMessage() const { return message_; }
int ToastNotification::getDuration() const { return durationMs_; }

ToastNotification::ToastConfig ToastNotification::getConfig() const {
  ToastConfig cfg{};

  // If the design system is available, prefer it; otherwise use JUCE defaults.
  auto primary = juce::Colours::dodgerblue;
  auto warning = juce::Colours::yellow;
  auto error = juce::Colours::red;
  auto success = juce::Colours::green;

  switch (type_) {
  case ToastType::Info:
    cfg.background = primary.withAlpha(0.92f);
    cfg.border = primary;
    cfg.text = juce::Colours::white;
    break;
  case ToastType::Warning:
    cfg.background = warning.withAlpha(0.92f);
    cfg.border = warning.darker(0.2f);
    cfg.text = juce::Colours::black;
    break;
  case ToastType::Error:
    cfg.background = error.withAlpha(0.92f);
    cfg.border = error.darker(0.2f);
    cfg.text = juce::Colours::white;
    break;
  case ToastType::Success:
    cfg.background = success.withAlpha(0.92f);
    cfg.border = success.darker(0.2f);
    cfg.text = juce::Colours::white;
    break;
  case ToastType::Critical:
    cfg.background = error.darker(0.2f).withAlpha(0.95f);
    cfg.border = error;
    cfg.text = juce::Colours::white;
    break;
  }

  cfg.actionText = cfg.text;
  cfg.shadowBlur = 0;
  cfg.shadowColour = juce::Colours::black;
  return cfg;
}

juce::Rectangle<int> ToastNotification::calculateTargetPosition() const {
  // The manager sets bounds; keep any previous target if present.
  return targetBounds_.isEmpty() ? getBounds() : targetBounds_;
}

void ToastNotification::timerCallback() {
  // Auto-dismiss after duration.
  dismiss();

  if (auto* m = dynamic_cast<ToastNotificationManager*>(getParentComponent())) {
    m->dismissToast(this);
  }
}

void ToastNotification::animateToTarget() {
  // No-op in this minimal implementation.
}

//==============================================================================
ToastNotificationManager& ToastNotificationManager::getInstance() {
  static ToastNotificationManager instance;
  return instance;
}

ToastNotificationManager::ToastNotificationManager()
    : maxVisibleToasts_(3),
      defaultPosition_(ToastPosition::BottomRight),
      animationDurationMs_(180),
      toastMargin_(10) {
  setName("ToastNotificationManager");
  setInterceptsMouseClicks(false, false);
  startTimerHz(30);
}

ToastNotificationManager::~ToastNotificationManager() { clearAll(); }

void ToastNotificationManager::createParentIfNeeded() {
  // If the manager isn't attached anywhere, put it on the desktop so toasts can
  // actually appear. This is "best effort"; callers can embed it themselves.
  if (getParentComponent() != nullptr || isOnDesktop()) {
    return;
  }

  addToDesktop(juce::ComponentPeer::windowIsTemporary);
  setAlwaysOnTop(true);

  if (auto* display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()) {
    setBounds(display->userArea);
  } else {
    setBounds(0, 0, 800, 600);
  }

  setVisible(true);
}

void ToastNotificationManager::showToast(const juce::String& message,
                                        const Callback& onAction,
                                        ToastType type,
                                        const juce::String& actionText,
                                        int duration) {
  std::lock_guard<std::mutex> lock(mutex_);

  createParentIfNeeded();

  // If we're at capacity, drop the oldest.
  while (static_cast<int>(activeToasts_.size()) >= maxVisibleToasts_) {
    dismissToast(activeToasts_.front());
  }

  auto toast = std::make_unique<ToastNotification>(message, type, onAction, actionText, duration);
  toast->setPosition(defaultPosition_);
  toast->setMargin(toastMargin_);

  addAndMakeVisible(toast.get());
  toast->show();

  ToastQueueItem item;
  item.toast = std::move(toast);
  item.timestamp = juce::Time::getCurrentTime();
  item.isProcessing = true;

  auto* raw = item.toast.get();
  toastQueue_.push_back(std::move(item));
  activeToasts_.push_back(raw);

  positionActiveToasts();
}

void ToastNotificationManager::showToast(const UIError& error, const Callback& onAction) {
  ToastType type = ToastType::Info;
  switch (error.severity) {
  case ErrorSeverity::Fatal:
  case ErrorSeverity::Critical:
    type = ToastType::Critical;
    break;
  case ErrorSeverity::Error:
    type = ToastType::Error;
    break;
  case ErrorSeverity::Warning:
    type = ToastType::Warning;
    break;
  case ErrorSeverity::Info:
  case ErrorSeverity::Debug:
    type = ToastType::Info;
    break;
  }

  showToast(juce::String(error.message), onAction, type, "OK", error.severity == ErrorSeverity::Fatal ? 0 : 5000);
}

void ToastNotificationManager::clearAll() {
  std::lock_guard<std::mutex> lock(mutex_);

  for (auto* toast : activeToasts_) {
    if (toast) {
      removeChildComponent(toast);
    }
  }

  activeToasts_.clear();
  toastQueue_.clear();

  repaint();
}

void ToastNotificationManager::dismissToast(ToastNotification* toast) {
  if (!toast) return;
  removeToast(toast);
}

void ToastNotificationManager::setMaxVisibleToasts(int max) {
  std::lock_guard<std::mutex> lock(mutex_);
  maxVisibleToasts_ = std::max(1, max);
}

void ToastNotificationManager::setDefaultPosition(ToastPosition position) {
  std::lock_guard<std::mutex> lock(mutex_);
  defaultPosition_ = position;
  positionActiveToasts();
}

void ToastNotificationManager::setAnimationDuration(int durationMs) {
  std::lock_guard<std::mutex> lock(mutex_);
  animationDurationMs_ = std::max(0, durationMs);
}

void ToastNotificationManager::setToastMargin(int margin) {
  std::lock_guard<std::mutex> lock(mutex_);
  toastMargin_ = std::max(0, margin);
  positionActiveToasts();
}

bool ToastNotificationManager::hasActiveToasts() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return !activeToasts_.empty();
}

int ToastNotificationManager::getActiveToastCount() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return static_cast<int>(activeToasts_.size());
}

void ToastNotificationManager::paint(juce::Graphics&) {
  // Manager itself is transparent; toasts paint themselves.
}

void ToastNotificationManager::resized() { positionActiveToasts(); }

void ToastNotificationManager::timerCallback() {
  std::lock_guard<std::mutex> lock(mutex_);

  // Reap inactive toasts.
  for (auto it = activeToasts_.begin(); it != activeToasts_.end();) {
    auto* toast = *it;
    if (!toast || !toast->isActive()) {
      if (toast) removeChildComponent(toast);
      it = activeToasts_.erase(it);
    } else {
      ++it;
    }
  }

  // Remove any queue items that no longer have an active toast.
  toastQueue_.erase(
      std::remove_if(toastQueue_.begin(), toastQueue_.end(),
                     [](const ToastQueueItem& item) { return item.toast == nullptr || !item.toast->isActive(); }),
      toastQueue_.end());

  positionActiveToasts();
}

//------------------------------------------------------------------------------
// Internals (kept for API compatibility; simplified behavior)
//------------------------------------------------------------------------------
void ToastNotificationManager::showToastInternal(const ToastNotification::Callback& onAction,
                                                ToastType type,
                                                const juce::String& actionText,
                                                int duration) {
  showToast("Toast", onAction, type, actionText, duration);
}

void ToastNotificationManager::processNextToast() {
  // Queueing is not implemented in the minimal version.
}

void ToastNotificationManager::positionActiveToasts() {
  if (activeToasts_.empty()) return;

  const auto* display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay();
  const auto area = display ? display->userArea : getLocalBounds();

  const int width = 360;
  const int baseHeight = 80;

  for (int i = 0; i < static_cast<int>(activeToasts_.size()); ++i) {
    auto* toast = activeToasts_[i];
    if (!toast) continue;

    const int height = baseHeight;
    auto b = calculateStackedPosition(defaultPosition_, i).withSizeKeepingCentre(width, height);
    toast->setBounds(b);
  }
}

void ToastNotificationManager::removeToast(ToastNotification* toast) {
  if (!toast) return;

  toast->dismiss();
  removeChildComponent(toast);

  activeToasts_.erase(std::remove(activeToasts_.begin(), activeToasts_.end(), toast), activeToasts_.end());
  toastQueue_.erase(
      std::remove_if(toastQueue_.begin(), toastQueue_.end(),
                     [toast](const ToastQueueItem& item) { return item.toast.get() == toast; }),
      toastQueue_.end());

  positionActiveToasts();
}

juce::Rectangle<int> ToastNotificationManager::getDefaultBounds(ToastPosition position) const {
  const auto* display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay();
  const auto area = display ? display->userArea : getLocalBounds();

  constexpr int width = 360;
  constexpr int height = 80;
  constexpr int pad = 16;

  switch (position) {
  case ToastPosition::TopLeft:
    return {area.getX() + pad, area.getY() + pad, width, height};
  case ToastPosition::TopRight:
    return {area.getRight() - pad - width, area.getY() + pad, width, height};
  case ToastPosition::BottomLeft:
    return {area.getX() + pad, area.getBottom() - pad - height, width, height};
  case ToastPosition::BottomRight:
    return {area.getRight() - pad - width, area.getBottom() - pad - height, width, height};
  case ToastPosition::TopCenter:
    return {area.getCentreX() - width / 2, area.getY() + pad, width, height};
  case ToastPosition::BottomCenter:
    return {area.getCentreX() - width / 2, area.getBottom() - pad - height, width, height};
  case ToastPosition::Center:
    return {area.getCentreX() - width / 2, area.getCentreY() - height / 2, width, height};
  }

  return {area.getRight() - pad - width, area.getBottom() - pad - height, width, height};
}

juce::Rectangle<int> ToastNotificationManager::calculateStackedPosition(ToastPosition position, int index) const {
  auto base = getDefaultBounds(position);

  // Stack upward for bottom positions, downward for top positions.
  const int dy = (base.getHeight() + toastMargin_) * index;
  switch (position) {
  case ToastPosition::BottomLeft:
  case ToastPosition::BottomRight:
  case ToastPosition::BottomCenter:
    return base.translated(0, -dy);
  default:
    return base.translated(0, dy);
  }
}

//------------------------------------------------------------------------------
// PositionStack methods (not used in minimal version; provided for linkage)
//------------------------------------------------------------------------------
int ToastNotificationManager::PositionStack::getAvailablePosition() { return static_cast<int>(positions.size()); }
void ToastNotificationManager::PositionStack::pushPosition(const juce::Rectangle<int>& pos) { positions.push_back(pos); }
void ToastNotificationManager::PositionStack::clear() { positions.clear(); }

} // namespace zenith::UI
