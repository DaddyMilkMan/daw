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
#include "UnifiedComponent.h"
#include "../Theme.h"

namespace zenith {
UnifiedComponent::Builder& UnifiedComponent::Builder::withId(const juce::String& id) {
    id_ = id;
    return *this;
}

UnifiedComponent::Builder& UnifiedComponent::Builder::withTheme(Theme* theme) {
    theme_ = theme;
    return *this;
}

UnifiedComponent::Builder& UnifiedComponent::Builder::withSize(int width, int height) {
    bounds_.setWidth(width);
    bounds_.setHeight(height);
    return *this;
}

UnifiedComponent::Builder& UnifiedComponent::Builder::withPosition(int x, int y) {
    bounds_.setX(x);
    bounds_.setY(y);
    return *this;
}

UnifiedComponent::Builder& UnifiedComponent::Builder::withEnabled(bool enabled) {
    enabled_ = enabled;
    return *this;
}

UnifiedComponent::Builder& UnifiedComponent::Builder::withVisible(bool visible) {
    visible_ = visible;
    return *this;
}

UnifiedComponent::Builder& UnifiedComponent::Builder::withClickHandler(std::function<void()> handler) {
    clickHandler_ = handler;
    return *this;
}

UnifiedComponent::Builder& UnifiedComponent::Builder::withChangeHandler(std::function<void()> handler) {
    changeHandler_ = handler;
    return *this;
}

UnifiedComponent::Builder& UnifiedComponent::Builder::withFocusHandler(std::function<void(bool)> handler) {
    focusHandler_ = handler;
    return *this;
}

std::unique_ptr<UnifiedComponent> UnifiedComponent::Builder::build() {
    return std::make_unique<UnifiedComponent>(*this);
}

UnifiedComponent::UnifiedComponent() {
    DBG("UnifiedComponent: Constructor");
    initialize();
}

UnifiedComponent::UnifiedComponent(const Builder& builder) {
    id_ = builder.id_;
    theme_ = builder.theme_;
    bounds_ = builder.bounds_;
    enabled_ = builder.enabled_;
    visible_ = builder.visible_;
    clickHandler_ = builder.clickHandler_;
    changeHandler_ = builder.changeHandler_;
    focusHandler_ = builder.focusHandler_;

    initialize();
}

UnifiedComponent::~UnifiedComponent() {
    DBG("UnifiedComponent: Destructor - " + id_);
    stopAnimations();
}

void UnifiedComponent::initialize() {
    DBG("UnifiedComponent: Initializing - " + id_);

    // Set up the component
    setBounds(bounds_);
    setEnabled(enabled_);
    setVisible(visible_);

    // Call virtual initialization
    onInitialize();

    DBG("UnifiedComponent: Initialization complete - " + id_);
}

void UnifiedComponent::update(float deltaTime) {
    // Update animations
    updateAnimations(deltaTime);

    // Call virtual update method
    onUpdate(deltaTime);
}

void UnifiedComponent::render(juce::Graphics& graphics) {
    // Save the current state
    juce::Graphics::ScopedSaveState graphicsState(graphics);

    // Apply theme
    if (theme_) {
        applyTheme();
    }

    // Call virtual render method
    onRender(graphics);
}

bool UnifiedComponent::keyPressed(const juce::KeyPress& key) {
    // Handle focus navigation
    if (key.isKeyCode(juce::KeyPress::tabKey)) {
        // Use JUCE's built-in tab navigation
        if (key.getModifiers().isShiftDown()) {
            juce::Component::moveKeyboardFocusToSibling(false);
        } else {
            juce::Component::moveKeyboardFocusToSibling(true);
        }
        return true;
    }

    // Let subclasses handle the key
    return false;
}

void UnifiedComponent::setEnabled(bool enabled) {
    if (enabled_ != enabled) {
        enabled_ = enabled;
        setInterceptsMouseClicks(enabled, enabled);
        enablementChanged();
        notifyListeners();
    }
}

void UnifiedComponent::setVisible(bool visible) {
    if (visible_ != visible) {
        visible_ = visible;
        visibilityChanged();
        notifyListeners();
    }
}

bool UnifiedComponent::isEnabled() const {
    return enabled_;
}

bool UnifiedComponent::isVisible() const {
    return visible_;
}

void UnifiedComponent::applyTheme() {
    // Apply base theme settings
    // Note: ThemeManager::Theme doesn't have getColor - this is a stub implementation
    // Real theming should be done through ZenithTheme colors
    
    // Call virtual theme method
    onThemeApplied();
}

void UnifiedComponent::refresh() {
    repaint();
}

void UnifiedComponent::setClickHandler(std::function<void()> handler) {
    clickHandler_ = handler;
}

void UnifiedComponent::setChangeHandler(std::function<void()> handler) {
    changeHandler_ = handler;
}

void UnifiedComponent::setFocusHandler(std::function<void(bool)> handler) {
    focusHandler_ = handler;
}

void UnifiedComponent::setSize(int width, int height) {
    auto newBounds = calculateBounds(width, height);
    setBounds(newBounds);
}

void UnifiedComponent::setPosition(int x, int y) {
    auto newBounds = bounds_.withPosition(x, y);
    setBounds(newBounds);
}

juce::Rectangle<int> UnifiedComponent::getPreferredSize() const {
    return bounds_;
}

juce::Rectangle<int> UnifiedComponent::getMinimumSize() const {
    return bounds_.withWidth(juce::jmax(1, bounds_.getWidth()))
                 .withHeight(juce::jmax(1, bounds_.getHeight()));
}

juce::Rectangle<int> UnifiedComponent::getMaximumSize() const {
    return bounds_.withWidth(juce::jmax(bounds_.getWidth(), 10000))
                 .withHeight(juce::jmax(bounds_.getHeight(), 10000));
}

void UnifiedComponent::addListener(juce::ComponentListener* listener) {
    if (listener) {
        listeners_.add(listener);
        juce::Component::addComponentListener(listener);
    }
}

void UnifiedComponent::removeListener(juce::ComponentListener* listener) {
    listeners_.remove(listener);
    juce::Component::removeComponentListener(listener);
}

void UnifiedComponent::setId(const juce::String& id) {
    id_ = id;
}

void UnifiedComponent::setTheme(Theme* theme) {
    if (theme_ != theme) {
        theme_ = theme;
        applyTheme();
    }
}

void UnifiedComponent::setBounds(const juce::Rectangle<int>& bounds) {
    if (bounds_ != bounds) {
        auto oldBounds = bounds_;
        bounds_ = bounds;
        setTopLeftPosition(bounds_.getPosition());
        setSize(bounds_.getWidth(), bounds_.getHeight());
        onBoundsChanged(bounds_);
        notifyListeners();
    }
}

bool UnifiedComponent::hasFocus() const {
    return hasKeyboardFocus(true);
}

void UnifiedComponent::grabFocus(bool downwards) {
    juce::ignoreUnused(downwards);
    grabKeyboardFocus();
}

void UnifiedComponent::releaseFocus() {
    // Release focus by moving it to parent
    if (hasKeyboardFocus(true)) {
        if (auto* parent = getParentComponent()) {
            parent->grabKeyboardFocus();
        }
    }
}

void UnifiedComponent::mouseDown(const juce::MouseEvent& event) {
    DBG("UnifiedComponent: Mouse down - " + id_);

    // Check if enabled
    if (!enabled_) {
        return;
    }

    // Check bounds and trigger click
    if (bounds_.contains(event.position.toInt())) {
        onClick();
    }
    
    repaint();
}

void UnifiedComponent::mouseUp(const juce::MouseEvent& event) {
    juce::ignoreUnused(event);
    repaint();
}

void UnifiedComponent::mouseDrag(const juce::MouseEvent& event) {
    juce::ignoreUnused(event);
    repaint();
}

void UnifiedComponent::mouseEnter(const juce::MouseEvent& event) {
    juce::ignoreUnused(event);
    repaint();
}

void UnifiedComponent::mouseExit(const juce::MouseEvent& event) {
    juce::ignoreUnused(event);
    repaint();
}

void UnifiedComponent::mouseMove(const juce::MouseEvent& event) {
    juce::ignoreUnused(event);
    repaint();
}

void UnifiedComponent::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) {
    juce::ignoreUnused(event, wheel);
    repaint();
}

void UnifiedComponent::mouseDoubleClick(const juce::MouseEvent& event) {
    juce::ignoreUnused(event);
    repaint();
}

void UnifiedComponent::animate(int duration, std::function<float(float)> easing, std::function<void()> callback) {
    AnimationState anim;
    anim.running = true;
    anim.startTime = juce::Time::getMillisecondCounterHiRes() / 1000.0;
    anim.duration = duration;
    anim.easing = easing;
    anim.callback = callback;

    animations_.push_back(anim);
}

void UnifiedComponent::stopAnimations() {
    animations_.clear();
}

bool UnifiedComponent::isAnimating() const {
    return !animations_.empty();
}

void UnifiedComponent::onInitialize() {
    // Default implementation does nothing
}

void UnifiedComponent::onUpdate(float deltaTime) {
    // Default implementation does nothing
}

void UnifiedComponent::onRender(juce::Graphics& graphics) {
    // Default implementation clears the background
    graphics.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void UnifiedComponent::onThemeApplied() {
    // Default implementation applies theme to children
    for (auto* child : getChildren()) {
        if (auto unifiedChild = dynamic_cast<UnifiedComponent*>(child)) {
            unifiedChild->applyTheme();
        }
    }
}

void UnifiedComponent::onClick() {
    if (clickHandler_) {
        clickHandler_();
    }
}

void UnifiedComponent::onChange() {
    if (changeHandler_) {
        changeHandler_();
    }
}

void UnifiedComponent::onFocusChanged(bool focused) {
    if (focusHandler_) {
        focusHandler_(focused);
    }
}

void UnifiedComponent::onBoundsChanged(const juce::Rectangle<int>& newBounds) {
    // Default implementation does nothing
}

void UnifiedComponent::updateAnimations(float deltaTime) {
    auto currentTime = juce::Time::getMillisecondCounterHiRes() / 1000.0;

    for (size_t i = 0; i < animations_.size();) {
        auto& anim = animations_[i];

        if (anim.running) {
            float progress = static_cast<float>((currentTime - anim.startTime) / (anim.duration / 1000.0));

            if (progress >= 1.0f) {
                progress = 1.0f;
                anim.running = false;
                completeAnimation(i);

                if (anim.callback) {
                    anim.callback();
                }

                // Remove completed animation
                animations_.erase(animations_.begin() + i);
            } else {
                // Update animation
                float easedValue = anim.easing(progress);
                onUpdateAnimation(easedValue);
                i++;
            }
        } else {
            i++;
        }
    }
}

void UnifiedComponent::completeAnimation(size_t index) {
    // Default implementation does nothing
}

void UnifiedComponent::notifyListeners() {
    listeners_.call([this](juce::ComponentListener& listener) {
        listener.componentMovedOrResized(*this, false, false);
    });
}

juce::Rectangle<int> UnifiedComponent::calculateBounds(int width, int height) const {
    return juce::Rectangle<int>(0, 0, width, height);
}

// JUCE Callbacks
void UnifiedComponent::paint(juce::Graphics& g) {
    render(g);
}

void UnifiedComponent::resized() {
    bounds_ = getLocalBounds();
    onBoundsChanged(bounds_);

    // Layout children
    for (auto* child : getChildren()) {
        if (auto unifiedChild = dynamic_cast<UnifiedComponent*>(child)) {
            unifiedChild->setBounds(unifiedChild->getBounds());
        }
    }
}

void UnifiedComponent::visibilityChanged() {
    if (visible_ != isVisible()) {
        visible_ = isVisible();
    }
}

void UnifiedComponent::enablementChanged() {
    if (enabled_ != isEnabled()) {
        enabled_ = isEnabled();
    }
}

void UnifiedComponent::focusGained(FocusChangeType cause) {
    onFocusChanged(true);
}

void UnifiedComponent::focusLost(FocusChangeType cause) {
    onFocusChanged(false);
}

void UnifiedComponent::onUpdateAnimation(float value) {
    // Default implementation does nothing
}

} // namespace zenith