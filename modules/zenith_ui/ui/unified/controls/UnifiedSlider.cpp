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
#include "UnifiedSlider.h"
#include "../Theme.h"

namespace zenith {
    
UnifiedSlider::Builder& UnifiedSlider::Builder::withRange(double min, double max, double interval) {
    min_ = min;
    max_ = max;
    interval_ = interval;
    return *this;
}

UnifiedSlider::Builder& UnifiedSlider::Builder::withValue(double value) {
    value_ = value;
    return *this;
}

UnifiedSlider::Builder& UnifiedSlider::Builder::withSize(int width, int height) {
    bounds_.setWidth(width);
    bounds_.setHeight(height);
    return *this;
}

UnifiedSlider::Builder& UnifiedSlider::Builder::withPosition(int x, int y) {
    bounds_.setX(x);
    bounds_.setY(y);
    return *this;
}

UnifiedSlider::Builder& UnifiedSlider::Builder::withEnabled(bool enabled) {
    enabled_ = enabled;
    return *this;
}

UnifiedSlider::Builder& UnifiedSlider::Builder::withVisible(bool visible) {
    visible_ = visible;
    return *this;
}

UnifiedSlider::Builder& UnifiedSlider::Builder::withChangeHandler(std::function<void(double)> handler) {
    changeHandler_ = handler;
    return *this;
}

UnifiedSlider::Builder& UnifiedSlider::Builder::withDragEndHandler(std::function<void(double)> handler) {
    dragEndHandler_ = handler;
    return *this;
}

UnifiedSlider::Builder& UnifiedSlider::Builder::withHorizontal(bool horizontal) {
    horizontal_ = horizontal;
    return *this;
}

UnifiedSlider::Builder& UnifiedSlider::Builder::withTextBoxEnabled(bool enabled) {
    textBoxEnabled_ = enabled;
    return *this;
}

UnifiedSlider::Builder& UnifiedSlider::Builder::withTextBoxStyle(juce::Slider::TextEntryBoxPosition style) {
    textBoxStyle_ = style;
    return *this;
}

UnifiedSlider::Builder& UnifiedSlider::Builder::withSliderColour(juce::Colour colour) {
    sliderColour_ = colour;
    return *this;
}

UnifiedSlider::Builder& UnifiedSlider::Builder::withTrackColour(juce::Colour colour) {
    trackColour_ = colour;
    return *this;
}

UnifiedSlider::Builder& UnifiedSlider::Builder::withThumbColour(juce::Colour colour) {
    thumbColour_ = colour;
    return *this;
}

UnifiedSlider::Builder& UnifiedSlider::Builder::withTextBoxColour(juce::Colour colour) {
    textBoxColour_ = colour;
    return *this;
}

std::unique_ptr<UnifiedSlider> UnifiedSlider::Builder::build() {
    return std::make_unique<UnifiedSlider>(*this);
}

UnifiedSlider::UnifiedSlider() {
    DBG("UnifiedSlider: Constructor");
    initialize();
}

UnifiedSlider::UnifiedSlider(const Builder& builder) : UnifiedComponent() {
    setBounds(builder.bounds_);
    setEnabled(builder.enabled_);
    setVisible(builder.visible_);
    min_ = builder.min_;
    max_ = builder.max_;
    interval_ = builder.interval_;
    value_ = builder.value_;
    horizontal_ = builder.horizontal_;
    textBoxEnabled_ = builder.textBoxEnabled_;
    textBoxStyle_ = builder.textBoxStyle_;
    sliderColour_ = builder.sliderColour_;
    trackColour_ = builder.trackColour_;
    thumbColour_ = builder.thumbColour_;
    textBoxColour_ = builder.textBoxColour_;
    changeHandler_ = builder.changeHandler_;
    dragEndHandler_ = builder.dragEndHandler_;

    initialize();
}

UnifiedSlider::~UnifiedSlider() {
    DBG("UnifiedSlider: Destructor");
}

void UnifiedSlider::initialize() {
    DBG("UnifiedSlider: Initializing");

    // Initialize state
    dragging_ = false;
    hovered_ = false;
    focused_ = false;
    lastValue_ = value_;

    // Initialize animations
    animation_.thumbPosition = getValueNormalized();
    animation_.trackProgress = 0.0f;
    animation_.hoverAlpha = 0.0f;
    animation_.dragScale = 1.0f;

    onInitialize();
}

void UnifiedSlider::update(float deltaTime) {
    UnifiedComponent::update(deltaTime);

    // Update animations
    if (hovered_) {
        animation_.hoverAlpha = juce::jmin(1.0f, animation_.hoverAlpha + deltaTime * 5.0f);
    } else {
        animation_.hoverAlpha = juce::jmax(0.0f, animation_.hoverAlpha - deltaTime * 5.0f);
    }

    if (dragging_) {
        animation_.dragScale = juce::jmin(1.2f, animation_.dragScale + deltaTime * 15.0f);
    } else {
        animation_.dragScale = juce::jmax(1.0f, animation_.dragScale - deltaTime * 15.0f);
    }

    // Smooth thumb position animation
    float targetPosition = getValueNormalized();
    animation_.thumbPosition += (targetPosition - animation_.thumbPosition) * deltaTime * 10.0f;

    onUpdate(deltaTime);
}

void UnifiedSlider::render(juce::Graphics& graphics) {
    // Save graphics state
    juce::Graphics::ScopedSaveState state(graphics);

    // Render track
    auto trackBounds = getTrackBounds();
    graphics.setColour(trackColour_);

    if (horizontal_) {
        graphics.fillRect(trackBounds);
    } else {
        graphics.fillRect(trackBounds);
    }

    // Render filled portion
    float fillProgress = getValueNormalized();
    auto thumbBounds = getThumbBounds();

    if (horizontal_) {
        auto filledBounds = trackBounds.withWidth(trackBounds.getWidth() * fillProgress);
        graphics.setColour(sliderColour_);
        graphics.fillRect(filledBounds);
    } else {
        auto filledBounds = trackBounds.withHeight(trackBounds.getHeight() * (1.0f - fillProgress));
        graphics.setColour(sliderColour_);
        graphics.fillRect(filledBounds);
    }

    // Render thumb
    auto thumbPos = thumbBounds.getCentre();
    float thumbSize = juce::jmin(thumbBounds.getWidth(), thumbBounds.getHeight()) * animation_.dragScale;

    graphics.setColour(thumbColour_);
    graphics.fillEllipse(thumbPos.x - thumbSize/2, thumbPos.y - thumbSize/2, thumbSize, thumbSize);

    // Render hover effect
    if (animation_.hoverAlpha > 0.0f) {
        graphics.setColour(thumbColour_.withAlpha(animation_.hoverAlpha * 0.3f));
        graphics.drawEllipse(thumbPos.x - thumbSize/2 - 2, thumbPos.y - thumbSize/2 - 2, thumbSize + 4, thumbSize + 4, 2.0f);
    }

    // Render text box if enabled
    if (textBoxEnabled_) {
        auto textBoxBounds = getTextBoxBounds();
        graphics.setColour(textBoxColour_);

        // Draw text box background
        graphics.fillRect(textBoxBounds);

        // Draw text
        auto valueText = juce::String(value_, getPrecision());
        graphics.drawText(valueText, textBoxBounds, juce::Justification::centred);
    }

    onRender(graphics);
}

bool UnifiedSlider::keyPressed(const juce::KeyPress& key) {
    if (!isEnabled()) return false;

    // Handle arrow keys
    if (key.isKeyCode(juce::KeyPress::leftKey) || key.isKeyCode(juce::KeyPress::rightKey) ||
        key.isKeyCode(juce::KeyPress::upKey) || key.isKeyCode(juce::KeyPress::downKey)) {

        double delta = 0.0;
        if (key.isKeyCode(juce::KeyPress::leftKey) || key.isKeyCode(juce::KeyPress::downKey)) {
            delta = -0.01 * (max_ - min_);
        } else {
            delta = 0.01 * (max_ - min_);
        }

        double newValue = snappedValue(value_ + delta);
        setValue(newValue);
        return true;
    }

    // Handle page up/down
    if (key.isKeyCode(juce::KeyPress::pageUpKey)) {
        double newValue = snappedValue(value_ + (max_ - min_) * 0.1);
        setValue(newValue);
        return true;
    } else if (key.isKeyCode(juce::KeyPress::pageDownKey)) {
        double newValue = snappedValue(value_ - (max_ - min_) * 0.1);
        setValue(newValue);
        return true;
    }

    return UnifiedComponent::keyPressed(key);
}

juce::Rectangle<int> UnifiedSlider::getPreferredSize() const {
    if (horizontal_) {
        return juce::Rectangle<int>(bounds_.getWidth(), 40);
    } else {
        return juce::Rectangle<int>(40, bounds_.getHeight());
    }
}

juce::Rectangle<int> UnifiedSlider::getMinimumSize() const {
    if (horizontal_) {
        return juce::Rectangle<int>(100, 30);
    } else {
        return juce::Rectangle<int>(30, 100);
    }
}

void UnifiedSlider::setRange(double min, double max, double interval) {
    if (min < max) {
        min_ = min;
        max_ = max;
        interval_ = interval;

        // Clamp current value to new range
        value_ = juce::jlimit(min_, max_, value_);
        lastValue_ = value_;

        repaint();
    }
}

void UnifiedSlider::getRange(double& min, double& max, double& interval) const {
    min = min_;
    max = max_;
    interval = interval_;
}

void UnifiedSlider::setValue(double newValue, juce::NotificationType sendNotification) {
    double oldValue = value_;
    value_ = snappedValue(newValue);
    lastValue_ = value_;

    if (oldValue != value_) {
        onChange();

        if (sendNotification != juce::dontSendNotification) {
            // Send change notification
        }
    }
}

void UnifiedSlider::setHorizontal(bool horizontal) {
    if (horizontal_ != horizontal) {
        horizontal_ = horizontal;
        repaint();
    }
}

void UnifiedSlider::setTextBoxEnabled(bool enabled) {
    if (textBoxEnabled_ != enabled) {
        textBoxEnabled_ = enabled;
        repaint();
    }
}

void UnifiedSlider::setTextBoxStyle(juce::Slider::TextEntryBoxPosition style) {
    if (textBoxStyle_ != style) {
        textBoxStyle_ = style;
        repaint();
    }
}

void UnifiedSlider::setSliderColour(juce::Colour colour) {
    if (sliderColour_ != colour) {
        sliderColour_ = colour;
        repaint();
    }
}

void UnifiedSlider::setTrackColour(juce::Colour colour) {
    if (trackColour_ != colour) {
        trackColour_ = colour;
        repaint();
    }
}

void UnifiedSlider::setThumbColour(juce::Colour colour) {
    if (thumbColour_ != colour) {
        thumbColour_ = colour;
        repaint();
    }
}

void UnifiedSlider::setTextBoxColour(juce::Colour colour) {
    if (textBoxColour_ != colour) {
        textBoxColour_ = colour;
        repaint();
    }
}

void UnifiedSlider::setChangeHandler(std::function<void(double)> handler) {
    changeHandler_ = handler;
}

void UnifiedSlider::setDragEndHandler(std::function<void(double)> handler) {
    dragEndHandler_ = handler;
}

void UnifiedSlider::onInitialize() {
    // Initialize with default values
}

void UnifiedSlider::onUpdate(float deltaTime) {
    // Update slider animations
    if (hasFocus()) {
        focused_ = true;
    } else {
        focused_ = false;
    }
}

void UnifiedSlider::onRender(juce::Graphics& graphics) {
    // Default rendering is handled in the main render method
}

void UnifiedSlider::onChange() {
    if (changeHandler_) {
        changeHandler_(value_);
    }

    // Notify listeners
    listeners_.call(&juce::ComponentListener::componentMovedOrResized, *this, false, false);
}

void UnifiedSlider::updateValue(double newValue) {
    setValue(newValue);
}

double UnifiedSlider::snappedValue(double value) const {
    if (interval_ > 0.0) {
        return juce::roundToInt(value / interval_) * interval_;
    }
    return value;
}

juce::Rectangle<float> UnifiedSlider::getTrackBounds() const {
    auto bounds = getLocalBounds().toFloat().reduced(10, 10);

    if (horizontal_) {
        return bounds.withHeight(8);
    } else {
        return bounds.withWidth(8);
    }
}

juce::Rectangle<float> UnifiedSlider::getThumbBounds() const {
    auto trackBounds = getTrackBounds();
    float thumbSize = 16.0f;
    float thumbPosition = animation_.thumbPosition;

    if (horizontal_) {
        float x = trackBounds.getX() + thumbPosition * trackBounds.getWidth() - thumbSize/2;
        return juce::Rectangle<float>(x, trackBounds.getCentreY() - thumbSize/2, thumbSize, thumbSize);
    } else {
        float y = trackBounds.getY() + (1.0f - thumbPosition) * trackBounds.getHeight() - thumbSize/2;
        return juce::Rectangle<float>(trackBounds.getCentreX() - thumbSize/2, y, thumbSize, thumbSize);
    }
}

juce::Rectangle<float> UnifiedSlider::getTextBoxBounds() const {
    auto bounds = getLocalBounds().toFloat();

    if (horizontal_) {
        return bounds.withRight(bounds.getRight() - 60).reduced(5, 2);
    } else {
        return bounds.withBottom(bounds.getBottom() - 60).reduced(2, 5);
    }
}

float UnifiedSlider::getValueNormalized() const {
    if (max_ > min_) {
        return static_cast<float>((value_ - min_) / (max_ - min_));
    }
    return 0.5f;
}

double UnifiedSlider::denormalizedValue(float normalized) const {
    return min_ + normalized * (max_ - min_);
}

int UnifiedSlider::getPrecision() const {
    if (interval_ > 0.0) {
        int decimals = 0;
        double temp = interval_;
        while (temp < 1.0 && temp > 0.0) {
            temp *= 10.0;
            decimals++;
        }
        return decimals;
    }
    return 2; // Default to 2 decimal places
}

// JUCE Callbacks
void UnifiedSlider::mouseEnter(const juce::MouseEvent& event) {
    UnifiedComponent::mouseEnter(event);
    hovered_ = true;
}

void UnifiedSlider::mouseExit(const juce::MouseEvent& event) {
    UnifiedComponent::mouseExit(event);
    hovered_ = false;
}

void UnifiedSlider::mouseDown(const juce::MouseEvent& event) {
    if (!isEnabled()) return;

    drag_.active = true;
    drag_.startPos = event.position;
    drag_.startValue = value_;
    drag_.sensitivity = horizontal_ ? 200.0 : -200.0; // Sensitivity for vertical sliders

    dragging_ = true;
    grabFocus();
}

void UnifiedSlider::mouseUp(const juce::MouseEvent& event) {
    if (!isEnabled()) return;

    drag_.active = false;
    dragging_ = false;

    if (dragEndHandler_) {
        dragEndHandler_(value_);
    }
}

void UnifiedSlider::mouseDrag(const juce::MouseEvent& event) {
    if (!isEnabled() || !drag_.active) return;

    auto delta = horizontal_ ? event.position.x - drag_.startPos.x : drag_.startPos.y - event.position.y;
    double range = max_ - min_;
    double deltaValue = (delta / drag_.sensitivity) * range;

    double newValue = snappedValue(drag_.startValue + deltaValue);
    setValue(newValue);
}

void UnifiedSlider::mouseMove(const juce::MouseEvent& event) {
    UnifiedComponent::mouseMove(event);
    hovered_ = getLocalBounds().toFloat().contains(event.position);
}

void UnifiedSlider::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) {
    if (!isEnabled()) return;

    double delta = wheel.deltaY > 0 ? 0.01 : -0.01;
    double range = max_ - min_;
    double newValue = snappedValue(value_ + delta * range);
    setValue(newValue);
}

void UnifiedSlider::focusGained(FocusChangeType cause) {
    UnifiedComponent::focusGained(cause);
    focused_ = true;
    repaint();
}

void UnifiedSlider::focusLost(FocusChangeType cause) {
    UnifiedComponent::focusLost(cause);
    focused_ = false;
    repaint();
}

} // namespace zenith