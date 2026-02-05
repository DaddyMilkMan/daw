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

 * @file UnifiedButton.h
 * @brief Unified button component
 *
 * This is the new button component that replaces both SkiaButton and JUCE Button,
 * providing a consistent interface across the application.
 */


#include "../UnifiedComponent.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace zenith {

/**
 * @class UnifiedButton
 * @brief Unified button component
 *
 * This button component provides a consistent interface across the application,
 * with customizable appearance, behavior, and theming.
 */
class UnifiedButton : public UnifiedComponent {
public:
    /**
     * @class Builder
     * @brief Builder pattern for creating buttons
     */
    class Builder {
    public:
        Builder& withText(const juce::String& text);
        Builder& withSize(int width, int height);
        Builder& withPosition(int x, int y);
        Builder& withEnabled(bool enabled);
        Builder& withVisible(bool visible);
        Builder& withClickHandler(std::function<void()> handler);
        Builder& withToggleMode(bool toggle);
        Builder& withToggleState(bool state);
        Builder& withButtonTextColour(juce::Colour colour);
        Builder& withButtonBackgroundColour(juce::Colour colour);
        Builder& withButtonBorderColour(juce::Colour colour);
        Builder& withButtonCornerRadius(float radius);

        std::unique_ptr<UnifiedButton> build();

    private:
        juce::String text_;
        juce::Rectangle<int> bounds_{0, 0, 100, 30};
        bool enabled_{true};
        bool visible_{true};
        bool toggleMode_{false};
        bool toggleState_{false};
        std::function<void()> clickHandler_;
        juce::Colour buttonTextColour_{juce::Colours::white};
        juce::Colour buttonBackgroundColour_{juce::Colours::grey};
        juce::Colour buttonBorderColour_{juce::Colours::black};
        float buttonCornerRadius_{5.0f};
    };

    UnifiedButton();
    explicit UnifiedButton(const Builder& builder);
    ~UnifiedButton() override;

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
    // Button Specific
    //==========================================================================

    /**
     * @brief Set the button text
     * @param text The button text
     */
    void setText(const juce::String& text);

    /**
     * @brief Get the button text
     * @return The button text
     */
    const juce::String& getText() const { return text_; }

    /**
     * @brief Set the button as toggle mode
     * @param toggle Whether the button is in toggle mode
     */
    void setToggleMode(bool toggle);

    /**
     * @brief Check if the button is in toggle mode
     * @return True if the button is in toggle mode
     */
    bool isToggleMode() const { return toggleMode_; }

    /**
     * @brief Set the toggle state
     * @param state The toggle state
     */
    void setToggleState(bool state);

    /**
     * @brief Get the toggle state
     * @return The toggle state
     */
    bool getToggleState() const { return toggleState_; }

    /**
     * @brief Check if the button is pressed
     * @return True if the button is pressed
     */
    bool isPressed() const { return pressed_; }

    /**
     * @brief Set the button's text color
     * @param colour The text color
     */
    void setButtonTextColour(juce::Colour colour);

    /**
     * @brief Get the button's text color
     * @return The text color
     */
    juce::Colour getButtonTextColour() const { return buttonTextColour_; }

    /**
     * @brief Set the button's background color
     * @param colour The background color
     */
    void setButtonBackgroundColour(juce::Colour colour);

    /**
     * @brief Get the button's background color
     * @return The background color
     */
    juce::Colour getButtonBackgroundColour() const { return buttonBackgroundColour_; }

    /**
     * @brief Set the button's border color
     * @param colour The border color
     */
    void setButtonBorderColour(juce::Colour colour);

    /**
     * @brief Get the button's border color
     * @return The border color
     */
    juce::Colour getButtonBorderColour() const { return buttonBorderColour_; }

    /**
     * @brief Set the button's corner radius
     * @param radius The corner radius
     */
    void setButtonCornerRadius(float radius);

    /**
     * @brief Get the button's corner radius
     * @return The corner radius
     */
    float getButtonCornerRadius() const { return buttonCornerRadius_; }

    //==========================================================================
    // Event Handlers
    //==========================================================================

    /**
     * @brief Set the click handler
     * @param handler Function to call when clicked
     */
    void setClickHandler(std::function<void()> handler) override;

    /**
     * @brief Set the change handler (for toggle buttons)
     * @param handler Function to call when state changes
     */
    void setChangeHandler(std::function<void()> handler);

    //==========================================================================
    // State Queries
    //==========================================================================

    /**
     * @brief Check if the button is hovered
     * @return True if the button is hovered
     */
    bool isHovered() const { return hovered_; }

    /**
     * @brief Check if the button is focused
     * @return True if the button is focused
     */
    bool isFocused() const { return hasFocus(); }

protected:
    void onInitialize() override;
    void onUpdate(float deltaTime) override;
    void onRender(juce::Graphics& graphics) override;
    void onClick() override;
    void onChange() override;

private:
    //==========================================================================
    // Private Members
    //==========================================================================

    juce::String text_;
    bool toggleMode_;
    bool toggleState_;
    bool pressed_;
    bool hovered_;
    bool focused_;

    juce::Colour buttonTextColour_;
    juce::Colour buttonBackgroundColour_;
    juce::Colour buttonBorderColour_;
    float buttonCornerRadius_;

    std::function<void()> changeHandler_;

    //==========================================================================
    // Private Methods
    //==========================================================================

    void updateButtonState();
    juce::Rectangle<float> getButtonBounds() const;
    juce::Path getButtonShape() const;

    // Animation state
    struct AnimationState {
        float scale{1.0f};
        float alpha{1.0f};
        float hoverProgress{0.0f};
        float pressProgress{0.0f};
    };

    AnimationState animation_;

    // JUCE Callbacks
    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void focusGained(FocusChangeType cause) override;
    void focusLost(FocusChangeType cause) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UnifiedButton)
};

} // namespace zenith