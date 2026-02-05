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

/*
    ==============================================================================
    Original file header:
*/

 * @file UnifiedSlider.h
 * @brief Unified slider component
 *
 * This is the new slider component that replaces both SkiaSlider and JUCE Slider,
 * providing a consistent interface across the application.
 */


#include "../UnifiedComponent.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace zenith {

/**
 * @class UnifiedSlider
 * @brief Unified slider component
 *
 * This slider component provides a consistent interface across the application,
 * with customizable appearance, behavior, and theming.
 */
class UnifiedSlider : public UnifiedComponent {
public:
    /**
     * @class Builder
     * @brief Builder pattern for creating sliders
     */
    class Builder {
    public:
        Builder& withRange(double min, double max, double interval = 0.0);
        Builder& withValue(double value);
        Builder& withSize(int width, int height);
        Builder& withPosition(int x, int y);
        Builder& withEnabled(bool enabled);
        Builder& withVisible(bool visible);
        Builder& withChangeHandler(std::function<void(double)> handler);
        Builder& withDragEndHandler(std::function<void(double)> handler);
        Builder& withHorizontal(bool horizontal);
        Builder& withTextBoxEnabled(bool enabled);
        Builder& withTextBoxStyle(juce::Slider::TextBoxStyle style);
        Builder& withSliderColour(juce::Colour colour);
        Builder& withTrackColour(juce::Colour colour);
        Builder& withThumbColour(juce::Colour colour);
        Builder& withTextBoxColour(juce::Colour colour);

        std::unique_ptr<UnifiedSlider> build();

    private:
        double min_{0.0};
        double max_{1.0};
        double interval_{0.0};
        double value_{0.5};
        juce::Rectangle<int> bounds_{0, 0, 200, 30};
        bool enabled_{true};
        bool visible_{true};
        bool horizontal_{true};
        bool textBoxEnabled_{true};
        juce::Slider::TextBoxStyle textBoxStyle_{juce::Slider::TextBoxRight};
        juce::Colour sliderColour_{juce::Colours::grey};
        juce::Colour trackColour_{juce::Colours::lightgrey};
        juce::Colour thumbColour_{juce::Colours::black};
        juce::Colour textBoxColour_{juce::Colours::black};
        std::function<void(double)> changeHandler_;
        std::function<void(double)> dragEndHandler_;
    };

    UnifiedSlider();
    explicit UnifiedSlider(const Builder& builder);
    ~UnifiedSlider() override;

    //==========================================================================
    // UnifiedComponent Overrides
    //==========================================================================

    void initialize() override;
    void update(float deltaTime) override;
    void render(juce::Graphics& graphics) override;
    bool keyPressed(const juce::KeyPress& key) override;

    juce::Rectangle<int> getPreferredSize() const override;
    juce::Rectangle<int> getMinimumSize() const override;

    //==========================================================================
    // Slider Specific
    //==========================================================================

    /**
     * @brief Set the slider range
     * @param min Minimum value
     * @param max Maximum value
     * @param interval Interval between values (0.0 for continuous)
     */
    void setRange(double min, double max, double interval = 0.0);

    /**
     * @brief Get the slider range
     * @param min Minimum value (output parameter)
     * @param max Maximum value (output parameter)
     * @param interval Interval between values (output parameter)
     */
    void getRange(double& min, double& max, double& interval) const;

    /**
     * @brief Set the slider value
     * @param value The new value
     * @param sendNotification Whether to send change notifications
     */
    void setValue(double value, juce::NotificationType sendNotification = juce::sendNotification);

    /**
     * @brief Get the slider value
     * @return The current value
     */
    double getValue() const { return value_; }

    /**
     * @brief Set the slider orientation
     * @param horizontal Whether the slider is horizontal
     */
    void setHorizontal(bool horizontal);

    /**
     * @brief Check if the slider is horizontal
     * @return True if the slider is horizontal
     */
    bool isHorizontal() const { return horizontal_; }

    /**
     * @brief Set the text box enabled state
     * @param enabled Whether the text box is enabled
     */
    void setTextBoxEnabled(bool enabled);

    /**
     * @brief Check if the text box is enabled
     * @return True if the text box is enabled
     */
    bool isTextBoxEnabled() const { return textBoxEnabled_; }

    /**
     * @brief Set the text box style
     * @param style The text box style
     */
    void setTextBoxStyle(juce::Slider::TextBoxStyle style);

    /**
     * @brief Get the text box style
     * @return The text box style
     */
    juce::Slider::TextBoxStyle getTextBoxStyle() const { return textBoxStyle_; }

    /**
     * @brief Set the slider color
     * @param colour The slider color
     */
    void setSliderColour(juce::Colour colour);

    /**
     * @brief Get the slider color
     * @return The slider color
     */
    juce::Colour getSliderColour() const { return sliderColour_; }

    /**
     * @brief Set the track color
     * @param colour The track color
     */
    void setTrackColour(juce::Colour colour);

    /**
     * @brief Get the track color
     * @return The track color
     */
    juce::Colour getTrackColour() const { return trackColour_; }

    /**
     * @brief Set the thumb color
     * @param colour The thumb color
     */
    void setThumbColour(juce::Colour colour);

    /**
     * @brief Get the thumb color
     * @return The thumb color
     */
    juce::Colour getThumbColour() const { return thumbColour_; }

    /**
     * @brief Set the text box color
     * @param colour The text box color
     */
    void setTextBoxColour(juce::Colour colour);

    /**
     * @brief Get the text box color
     * @return The text box color
     */
    juce::Colour getTextBoxColour() const { return textBoxColour_; }

    //==========================================================================
    // Event Handlers
    //==========================================================================

    /**
     * @brief Set the change handler
     * @param handler Function to call when value changes
     */
    void setChangeHandler(std::function<void(double)> handler);

    /**
     * @brief Set the drag end handler
     * @param handler Function to call when drag ends
     */
    void setDragEndHandler(std::function<void(double)> handler);

    //==========================================================================
    // State Queries
    //==========================================================================

    /**
     * @brief Check if the slider is being dragged
     * @return True if the slider is being dragged
     */
    bool isDragging() const { return dragging_; }

    /**
     * @brief Check if the slider is hovered
     * @return True if the slider is hovered
     */
    bool isHovered() const { return hovered_; }

    /**
     * @brief Check if the slider is focused
     * @return True if the slider is focused
     */
    bool isFocused() const { return hasFocus(); }

    /**
     * @brief Get the slider's precision
     * @return The precision (number of decimal places)
     */
    int getPrecision() const;

protected:
    void onInitialize() override;
    void onUpdate(float deltaTime) override;
    void onRender(juce::Graphics& graphics) override;
    void onChange() override;

private:
    //==========================================================================
    // Private Members
    //==========================================================================

    double min_;
    double max_;
    double interval_;
    double value_;
    double lastValue_;
    bool horizontal_;
    bool textBoxEnabled_;
    juce::Slider::TextBoxStyle textBoxStyle_;
    bool dragging_;
    bool hovered_;
    bool focused_;

    juce::Colour sliderColour_;
    juce::Colour trackColour_;
    juce::Colour thumbColour_;
    juce::Colour textBoxColour_;

    std::function<void(double)> changeHandler_;
    std::function<void(double)> dragEndHandler_;

    //==========================================================================
    // Private Methods
    //==========================================================================

    void updateValue(double newValue);
    double snappedValue(double value) const;
    juce::Rectangle<float> getTrackBounds() const;
    juce::Rectangle<float> getThumbBounds() const;
    juce::Rectangle<float> getTextBoxBounds() const;
    float getValueNormalized() const;
    double denormalizedValue(float normalized) const;

    // Animation state
    struct AnimationState {
        float thumbPosition{0.5f};
        float trackProgress{0.0f};
        float hoverAlpha{0.0f};
        float dragScale{1.0f};
    };

    AnimationState animation_;

    // Drag state
    struct DragState {
        bool active{false};
        juce::Point<float> startPos;
        double startValue;
        double sensitivity{1.0};
    };

    DragState drag_;

    // JUCE Callbacks
    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;
    void focusGained(FocusChangeType cause) override;
    void focusLost(FocusChangeType cause) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UnifiedSlider)
};

} // namespace zenith