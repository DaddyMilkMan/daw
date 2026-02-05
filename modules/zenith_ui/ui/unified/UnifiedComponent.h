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

 * @file UnifiedComponent.h
 * @brief Unified UI component implementation
 *
 * This is the concrete implementation of the unified UI component,
 * using Skia for rendering but abstracting the details from component logic.
 */


#include "IUnifiedComponent.h"
#include "../../framework/SkiaComponent.h"
#include "../../design-system/ZenithTheme.h"
#include <memory>
#include <functional>

namespace zenith {

/**
 * @class UnifiedComponent
 * @brief Base implementation for all unified UI components
 *
 * This class provides a common base for all UI components, handling
 * common functionality like theming, event handling, and layout.
 */
class UnifiedComponent : public IUnifiedComponent {
public:
    /**
     * @class Builder
     * @brief Builder pattern for creating components
     */
    class Builder {
    public:
        Builder& withId(const juce::String& id);
        Builder& withTheme(Theme* theme);
        Builder& withSize(int width, int height);
        Builder& withPosition(int x, int y);
        Builder& withEnabled(bool enabled);
        Builder& withVisible(bool visible);
        Builder& withClickHandler(std::function<void()> handler);
        Builder& withChangeHandler(std::function<void()> handler);
        Builder& withFocusHandler(std::function<void(bool)> handler);

        std::unique_ptr<UnifiedComponent> build();

    private:
        juce::String id_;
        Theme* theme_{nullptr};
        juce::Rectangle<int> bounds_{0, 0, 100, 100};
        bool enabled_{true};
        bool visible_{true};
        std::function<void()> clickHandler_;
        std::function<void()> changeHandler_;
        std::function<void(bool)> focusHandler_;
    };

    UnifiedComponent();
    explicit UnifiedComponent(const Builder& builder);
    ~UnifiedComponent() override;

    //==========================================================================
    // IUnifiedComponent Implementation
    //==========================================================================

    void initialize() override;
    void update(float deltaTime) override;
    void render(juce::Graphics& graphics) override;
    bool keyPressed(const juce::KeyPress& key) override;

    void setEnabled(bool enabled) override;
    void setVisible(bool visible) override;
    bool isEnabled() const override;
    bool isVisible() const override;

    void applyTheme() override;
    void refresh() override;

    void setClickHandler(std::function<void()> handler) override;
    void setChangeHandler(std::function<void()> handler) override;
    void setFocusHandler(std::function<void(bool)> handler) override;

    void setSize(int width, int height) override;
    void setPosition(int x, int y) override;
    juce::Rectangle<int> getPreferredSize() const override;
    juce::Rectangle<int> getMinimumSize() const override;
    juce::Rectangle<int> getMaximumSize() const override;

    void addListener(juce::ComponentListener* listener) override;
    void removeListener(juce::ComponentListener* listener) override;

    //==========================================================================
    // UnifiedComponent Specific
    //==========================================================================

    /**
     * @brief Get the component's ID
     * @return The component ID
     */
    const juce::String& getId() const { return id_; }

    /**
     * @brief Set the component's ID
     * @param id The component ID
     */
    void setId(const juce::String& id);

    /**
     * @brief Get the current theme
     * @return Current theme
     */
    Theme* getTheme() const { return theme_; }

    /**
     * @brief Set the current theme
     * @param theme The theme to use
     */
    void setTheme(Theme* theme);

    /**
     * @brief Get the bounds rectangle
     * @return The bounds rectangle
     */
    const juce::Rectangle<int>& getBounds() const { return bounds_; }

    /**
     * @brief Set the bounds rectangle
     * @param bounds The bounds rectangle
     */
    void setBounds(const juce::Rectangle<int>& bounds);

    /**
     * @brief Check if the component has focus
     * @return True if the component has focus
     */
    bool hasFocus() const;

    /**
     * @brief Request focus for the component
     * @param downwards Whether to move focus downwards
     */
    void grabFocus(bool downwards = true);

    /**
     * @brief Release focus
     */
    void releaseFocus();

    //==========================================================================
    // Event Handling
    //==========================================================================

    /**
     * @brief Handle mouse down event
     * @param event The mouse event
     */
    virtual void mouseDown(const juce::MouseEvent& event);

    /**
     * @brief Handle mouse up event
     * @param event The mouse event
     */
    virtual void mouseUp(const juce::MouseEvent& event);

    /**
     * @brief Handle mouse drag event
     * @param event The mouse event
     */
    virtual void mouseDrag(const juce::MouseEvent& event);

    /**
     * @brief Handle mouse enter event
     * @param event The mouse event
     */
    virtual void mouseEnter(const juce::MouseEvent& event);

    /**
     * @brief Handle mouse exit event
     * @param event The mouse event
     */
    virtual void mouseExit(const juce::MouseEvent& event);

    /**
     * @brief Handle mouse move event
     * @param event The mouse event
     */
    virtual void mouseMove(const juce::MouseEvent& event);

    /**
     * @brief Handle mouse wheel event
     * @param event The mouse event
     */
    virtual void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel);

    /**
     * @brief Handle mouse double click event
     * @param event The mouse event
     */
    virtual void mouseDoubleClick(const juce::MouseEvent& event);

    //==========================================================================
    // Animation Support
    //==========================================================================

    /**
     * @brief Start an animation
     * @param duration Animation duration in milliseconds
     * @param easing Easing function
     * @param callback Callback when animation completes
     */
    void animate(int duration, std::function<float(float)> easing, std::function<void()> callback = nullptr);

    /**
     * @brief Stop all animations
     */
    void stopAnimations();

    /**
     * @brief Check if animations are running
     * @return True if animations are running
     */
    bool isAnimating() const;

protected:
    //==========================================================================
    // Protected Virtual Methods (for subclasses to override)
    //==========================================================================

    /**
     * @brief Called when the component is initialized
     */
    virtual void onInitialize();

    /**
     * @brief Called when the component is updated
     * @param deltaTime Time since last update in seconds
     */
    virtual void onUpdate(float deltaTime);

    /**
     * @brief Called when the component is rendered
     * @param graphics Rendering context
     */
    virtual void onRender(juce::Graphics& graphics);

    /**
     * @brief Called when the theme is applied
     */
    virtual void onThemeApplied();

    /**
     * @brief Called when the component is clicked
     */
    virtual void onClick();

    /**
     * @brief Called when the component's value changes
     */
    virtual void onChange();

    /**
     * @brief Called when the component gains or loses focus
     * @param focused Whether the component has focus
     */
    virtual void onFocusChanged(bool focused);

    /**
     * @brief Called when the component's bounds change
     * @param newBounds The new bounds
     */
    virtual void onBoundsChanged(const juce::Rectangle<int>& newBounds);

private:
    //==========================================================================
    // Private Members
    //==========================================================================

    juce::String id_;
    Theme* theme_;
    juce::Rectangle<int> bounds_;
    bool enabled_;
    bool visible_;

    std::function<void()> clickHandler_;
    std::function<void()> changeHandler_;
    std::function<void(bool)> focusHandler_;

    juce::Component::SafePointer<UnifiedComponent> safeThis_;
    juce::Array<juce::Component::SafePointer<juce::ComponentListener>> listeners_;

    // Animation state
    struct AnimationState {
        bool running{false};
        double startTime{0.0};
        int duration{0};
        std::function<float(float)> easing;
        std::function<void()> callback;
    };

    std::vector<AnimationState> animations_;

    //==========================================================================
    // Private Methods
    //==========================================================================

    void updateAnimations(float deltaTime);
    void completeAnimation(size_t index);
    void notifyListeners();
    juce::Rectangle<int> calculateBounds(int width, int height) const;

    // JUCE Callbacks
    void paint(juce::Graphics& g) override;
    void resized() override;
    void visibilityChanged() override;
    void enablementChanged() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;
    void focusGained(FocusChangeType cause) override;
    void focusLost(FocusChangeType cause) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UnifiedComponent)
};

} // namespace zenith