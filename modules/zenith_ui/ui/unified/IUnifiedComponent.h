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


/**
 * File: IUnifiedComponent.h
 * Brief: Unified UI component interface
 *
 * This interface provides a common base for all UI components,
 * abstracting the rendering backend (Skia) from component logic.
 */


#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <memory>

namespace zenith {

/**
 * @class IUnifiedComponent
 // Brief: Base interface for all unified UI components
 *
 * This interface defines the contract for all UI components,
 * enabling consistent behavior across different types of UI elements.
 */
class IUnifiedComponent : public juce::Component {
public:
    virtual ~IUnifiedComponent() = default;

    //==========================================================================
    // Component Lifecycle
    //==========================================================================

    /**
     // Brief: Initialize the component
     */
    virtual void initialize() = 0;

    /**
     // Brief: Update component state
     * @param deltaTime Time since last update in seconds
     */
    virtual void update(float deltaTime) = 0;

    /**
     // Brief: Render the component
     * @param graphics Rendering context
     */
    virtual void render(juce::Graphics& graphics) = 0;

    /**
     // Brief: Handle keyboard input
     * @param key The key that was pressed
     * @return True if the key was handled
     */
    virtual bool keyPressed(const juce::KeyPress& key) override = 0;

    //==========================================================================
    // State Management
    //==========================================================================

    /**
     // Brief: Set the component's enabled state
     * @param enabled Whether the component is enabled
     */
    virtual void setEnabled(bool enabled) = 0;

    /**
     // Brief: Set the component's visibility
     * @param visible Whether the component is visible
     */
    virtual void setVisible(bool visible) = 0;

    /**
     // Brief: Check if the component is enabled
     * @return True if the component is enabled
     */
    virtual bool isEnabled() const = 0;

    /**
     // Brief: Check if the component is visible
     * @return True if the component is visible
     */
    virtual bool isVisible() const = 0;

    //==========================================================================
    // Styling
    //==========================================================================

    /**
     // Brief: Apply theme to the component
     */
    virtual void applyTheme() = 0;

    /**
     // Brief: Refresh the component's appearance
     */
    virtual void refresh() = 0;

    //==========================================================================
    // Interaction
    //==========================================================================

    /**
     // Brief: Set the click handler
     * @param handler Function to call when clicked
     */
    virtual void setClickHandler(std::function<void()> handler) = 0;

    /**
     // Brief: Set the change handler
     * @param handler Function to call when value changes
     */
    virtual void setChangeHandler(std::function<void()> handler) = 0;

    /**
     // Brief: Set the focus handler
     * @param handler Function to call when focus changes
     */
    virtual void setFocusHandler(std::function<void(bool)> handler) = 0;

    //==========================================================================
    // Layout
    //==========================================================================

    /**
     // Brief: Set the component's size
     * @param width Width in pixels
     * @param height Height in pixels
     */
    virtual void setSize(int width, int height) = 0;

    /**
     // Brief: Set the component's position
     * @param x X position in pixels
     * @param y Y position in pixels
     */
    virtual void setPosition(int x, int y) = 0;

    /**
     // Brief: Get the preferred size
     * @return Preferred size
     */
    virtual juce::Rectangle<int> getPreferredSize() const = 0;

    /**
     // Brief: Get the minimum size
     * @return Minimum size
     */
    virtual juce::Rectangle<int> getMinimumSize() const = 0;

    /**
     // Brief: Get the maximum size
     * @return Maximum size
     */
    virtual juce::Rectangle<int> getMaximumSize() const = 0;

    //==========================================================================
    // Event Listeners
    //==========================================================================

    /**
     // Brief: Add a listener for component events
     * @param listener The listener to add
     */
    virtual void addListener(juce::ComponentListener* listener) = 0;

    /**
     // Brief: Remove a listener for component events
     * @param listener The listener to remove
     */
    virtual void removeListener(juce::ComponentListener* listener) = 0;
};

/**
 * @class IUnifiedContainer
 // Brief: Interface for container components
 */
class IUnifiedContainer : public IUnifiedComponent {
public:
    virtual ~IUnifiedContainer() = default;

    /**
     // Brief: Add a child component
     * @param child The child component to add
     */
    virtual void addChildComponent(std::shared_ptr<IUnifiedComponent> child) = 0;

    /**
     // Brief: Remove a child component
     * @param child The child component to remove
     */
    virtual void removeChildComponent(std::shared_ptr<IUnifiedComponent> child) = 0;

    /**
     // Brief: Remove all child components
     */
    virtual void removeAllChildren() = 0;

    /**
     // Brief: Get the number of child components
     * @return Number of child components
     */
    virtual int getNumChildComponents() const = 0;

    /**
     // Brief: Get a child component by index
     * @param index Index of the child component
     * @return The child component, or nullptr if not found
     */
    virtual std::shared_ptr<IUnifiedComponent> getChildComponent(int index) const = 0;

    /**
     // Brief: Layout the child components
     */
    virtual void layoutChildren() = 0;
};

} // namespace zenith