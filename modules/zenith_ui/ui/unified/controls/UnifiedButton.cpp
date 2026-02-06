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
#include "UnifiedButton.h"
#include "../Theme.h"

namespace zenith {

UnifiedButton::Builder& UnifiedButton::Builder::withText(const juce::String& text) {
    text_ = text;
    return *this;
}

UnifiedButton::Builder& UnifiedButton::Builder::withSize(int width, int height) {
    bounds_.setWidth(width);
    bounds_.setHeight(height);
    return *this;
}

UnifiedButton::Builder& UnifiedButton::Builder::withPosition(int x, int y) {
    bounds_.setX(x);
    bounds_.setY(y);
    return *this;
}

UnifiedButton::Builder& UnifiedButton::Builder::withEnabled(bool enabled) {
    enabled_ = enabled;
    return *this;
}

UnifiedButton::Builder& UnifiedButton::Builder::withVisible(bool visible) {
    visible_ = visible;
    return *this;
}

UnifiedButton::Builder& UnifiedButton::Builder::withClickHandler(std::function<void()> handler) {
    clickHandler_ = handler;
    return *this;
}

UnifiedButton::Builder& UnifiedButton::Builder::withToggleMode(bool toggle) {
    toggleMode_ = toggle;
    return *this;
}

UnifiedButton::Builder& UnifiedButton::Builder::withToggleState(bool state) {
    toggleState_ = state;
    return *this;
}

UnifiedButton::Builder& UnifiedButton::Builder::withButtonTextColour(juce::Colour colour) {
    buttonTextColour_ = colour;
    return *this;
}

UnifiedButton::Builder& UnifiedButton::Builder::withButtonBackgroundColour(juce::Colour colour) {
    buttonBackgroundColour_ = colour;
    return *this;
}

UnifiedButton::Builder& UnifiedButton::Builder::withButtonBorderColour(juce::Colour colour) {
    buttonBorderColour_ = colour;
    return *this;
}

UnifiedButton::Builder& UnifiedButton::Builder::withButtonCornerRadius(float radius) {
    buttonCornerRadius_ = radius;
    return *this;
}

std::unique_ptr<UnifiedButton> UnifiedButton::Builder::build() {
    return std::make_unique<UnifiedButton>(*this);
}

UnifiedButton::UnifiedButton() {
    DBG("UnifiedButton: Constructor");
    initialize();
}

UnifiedButton::UnifiedButton(const Builder& builder) : UnifiedComponent() {
    setBounds(builder.bounds_);
    setEnabled(builder.enabled_);
    setVisible(builder.visible_);
    text_ = builder.text_;
    toggleMode_ = builder.toggleMode_;
    toggleState_ = builder.toggleState_;
    buttonTextColour_ = builder.buttonTextColour_;
    buttonBackgroundColour_ = builder.buttonBackgroundColour_;
    buttonBorderColour_ = builder.buttonBorderColour_;
    buttonCornerRadius_ = builder.buttonCornerRadius_;

    setClickHandler(builder.clickHandler_);
    initialize();
}

UnifiedButton::~UnifiedButton() {
    DBG("UnifiedButton: Destructor");
}

void UnifiedButton::initialize() {
    DBG("UnifiedButton: Initializing - " + text_);

    // Initialize state
    pressed_ = false;
    hovered_ = false;
    focused_ = false;

    // Set up animations
    animation_.scale = 1.0f;
    animation_.alpha = 1.0f;
    animation_.hoverProgress = 0.0f;
    animation_.pressProgress = 0.0f;

    onInitialize();
}

void UnifiedButton::update(float deltaTime) {
    UnifiedComponent::update(deltaTime);

    // Update animations
    if (hovered_) {
        animation_.hoverProgress = juce::jmin(1.0f, animation_.hoverProgress + deltaTime * 5.0f);
    } else {
        animation_.hoverProgress = juce::jmax(0.0f, animation_.hoverProgress - deltaTime * 5.0f);
    }

    if (pressed_) {
        animation_.pressProgress = juce::jmin(1.0f, animation_.pressProgress + deltaTime * 10.0f);
    } else {
        animation_.pressProgress = juce::jmax(0.0f, animation_.pressProgress - deltaTime * 10.0f);
    }

    onUpdate(deltaTime);
}

void UnifiedButton::render(juce::Graphics& graphics) {
    // Save graphics state
    juce::Graphics::ScopedSaveState state(graphics);

    // Apply transformations
    auto bounds = getButtonBounds();
    graphics.addTransform(juce::AffineTransform::scale(
        animation_.scale,
        animation_.scale,
        bounds.getCentreX(),
        bounds.getCentreY()
    ));

    // Apply alpha
    graphics.setOpacity(animation_.alpha);

    // Draw button background
    graphics.setColour(buttonBackgroundColour_);
    auto buttonPath = getButtonShape();
    graphics.fillPath(buttonPath);

    // Draw button border
    if (buttonBorderColour_.getAlpha() > 0) {
        graphics.setColour(buttonBorderColour_);
        graphics.strokePath(buttonPath, juce::PathStrokeType(1.0f));
    }

    // Draw button text
    if (!text_.isEmpty()) {
        graphics.setColour(buttonTextColour_);

        // Calculate text position
        auto textBounds = buttonPath.getBounds().toNearestInt();
        auto font = juce::Font(16.0f * 0.8f);

        // Centre the text
        auto textArea = textBounds.reduced(10, 5);
        graphics.setFont(font);
        graphics.drawFittedText(text_, textArea, juce::Justification::centred, 1);
    }

    onRender(graphics);
}

bool UnifiedButton::keyPressed(const juce::KeyPress& key) {
    // Handle space and return keys
    // Handle space and return keys
    if (key.isKeyCode(juce::KeyPress::spaceKey) || key.isKeyCode(juce::KeyPress::returnKey)) {
        if (toggleMode_) {
            toggleState_ = !toggleState_;
        }
        if (clickHandler_) clickHandler_();
        repaint();
        return true;
    }

    return UnifiedComponent::keyPressed(key);
}

juce::Rectangle<int> UnifiedButton::getPreferredSize() const {
    if (!text_.isEmpty()) {
        auto font = juce::Font(16.0f * 0.8f);
        auto textWidth = (int)font.getStringWidth(text_) + 20; // Add padding
        auto textHeight = (int)font.getHeight() + 10; // Add padding
        return juce::Rectangle<int>(0, 0, juce::jmax(bounds_.getWidth(), textWidth), juce::jmax(bounds_.getHeight(), textHeight));
    }
    return bounds_;
}

juce::Rectangle<int> UnifiedButton::getMinimumSize() const {
    return juce::Rectangle<int>(30, 20); // Minimum size
}

void UnifiedButton::setText(const juce::String& text) {
    if (text_ != text) {
        text_ = text;
        repaint();
    }
}

void UnifiedButton::setToggleMode(bool toggle) {
    if (toggleMode_ != toggle) {
        toggleMode_ = toggle;
        setToggleState(false); // Reset toggle state
    }
}

void UnifiedButton::setToggleState(bool state) {
    if (toggleMode_ && toggleState_ != state) {
        toggleState_ = state;
        onChange();
    }
}

void UnifiedButton::setButtonTextColour(juce::Colour colour) {
    if (buttonTextColour_ != colour) {
        buttonTextColour_ = colour;
        repaint();
    }
}

void UnifiedButton::setButtonBackgroundColour(juce::Colour colour) {
    if (buttonBackgroundColour_ != colour) {
        buttonBackgroundColour_ = colour;
        repaint();
    }
}

void UnifiedButton::setButtonBorderColour(juce::Colour colour) {
    if (buttonBorderColour_ != colour) {
        buttonBorderColour_ = colour;
        repaint();
    }
}

void UnifiedButton::setButtonCornerRadius(float radius) {
    buttonCornerRadius_ = juce::jmax(0.0f, radius);
    repaint();
}

void UnifiedButton::setClickHandler(std::function<void()> handler) {
    UnifiedComponent::setClickHandler(handler);
}

void UnifiedButton::setChangeHandler(std::function<void()> handler) {
    changeHandler_ = handler;
}

void UnifiedButton::onInitialize() {
    // Initialize animations
    animate(200, [](float t) { return t * t * (3.0f - 2.0f * t); }); // Smooth step
}

void UnifiedButton::onUpdate(float deltaTime) {
    // Update button animations
    float targetScale = 1.0f + animation_.pressProgress * 0.05f; // Press scale
    animation_.scale += (targetScale - animation_.scale) * deltaTime * 10.0f;

    // Handle keyboard navigation
    if (hasFocus()) {
        focused_ = true;
    } else {
        focused_ = false;
    }
}

void UnifiedButton::onRender(juce::Graphics& graphics) {
    // Default rendering is handled in the main render method
    // This can be overridden for custom effects
}

void UnifiedButton::onClick() {
    if (toggleMode_) {
        toggleState_ = !toggleState_;
        onChange();
    }

    UnifiedComponent::onClick();
}

void UnifiedButton::onChange() {
    if (changeHandler_) {
        changeHandler_();
    }

    // Notify listeners
    listeners_.call(&juce::ComponentListener::componentMovedOrResized, *this, false, false);
}

void UnifiedButton::updateButtonState() {
    // Update button state based on current settings
    if (toggleMode_) {
        setToggleState(toggleState_);
    }
}

juce::Rectangle<float> UnifiedButton::getButtonBounds() const {
    return getLocalBounds().toFloat();
}

juce::Path UnifiedButton::getButtonShape() const {
    auto bounds = getButtonBounds();
    juce::Path path;

    // Create rounded rectangle
    if (buttonCornerRadius_ > 0) {
        path.addRoundedRectangle(bounds.reduced(2.0f), buttonCornerRadius_);
    } else {
        path.addRectangle(bounds.reduced(2.0f));
    }

    return path;
}

// JUCE Callbacks
void UnifiedButton::mouseEnter(const juce::MouseEvent& event) {
    UnifiedComponent::mouseEnter(event);
    hovered_ = true;
}

void UnifiedButton::mouseExit(const juce::MouseEvent& event) {
    UnifiedComponent::mouseExit(event);
    hovered_ = false;
}

void UnifiedButton::mouseDown(const juce::MouseEvent& event) {
    if (!isEnabled()) return;

    pressed_ = true;
    animate(100, [](float t) { return t * t * (3.0f - 2.0f * t); });
}

void UnifiedButton::mouseUp(const juce::MouseEvent& event) {
    if (!isEnabled()) return;

    pressed_ = false;
    animate(100, [](float t) { return t * t * (3.0f - 2.0f * t); });
}

void UnifiedButton::mouseDrag(const juce::MouseEvent& event) {
    UnifiedComponent::mouseDrag(event);
    pressed_ = getLocalBounds().toFloat().contains(event.position);
}

void UnifiedButton::focusGained(FocusChangeType cause) {
    UnifiedComponent::focusGained(cause);
    focused_ = true;
    repaint();
}

void UnifiedButton::focusLost(FocusChangeType cause) {
    UnifiedComponent::focusLost(cause);
    focused_ = false;
    repaint();
}

} // namespace zenith