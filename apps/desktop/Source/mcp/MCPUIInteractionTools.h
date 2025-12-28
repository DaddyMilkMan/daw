/*
  ==============================================================================

    MCPUIInteractionTools.h
    Created: 2025-12-23
    Author:  Zenith DAW Team

    UI Interaction tools for MCP - allowing AI to control the DAW interface.
    Provides mouse and keyboard simulation capabilities.

  ==============================================================================
*/

#pragma once

#include "MCPToolSchemas.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <typeinfo>

namespace zenith {
namespace mcp {
namespace ui {

//==============================================================================
// Schemas
//==============================================================================

inline juce::var clickSchema() {
  auto *props = new juce::DynamicObject();
  zenith::mcp::schemas::setArrProp(
      props, "x",
      zenith::mcp::schemas::makeNumberProperty(
          "X coordinate (relative to window)", true));
  zenith::mcp::schemas::setArrProp(
      props, "y",
      zenith::mcp::schemas::makeNumberProperty(
          "Y coordinate (relative to window)", true));
  zenith::mcp::schemas::setArrProp(
      props, "modifiers",
      zenith::mcp::schemas::makeStringProperty(
          "Optional modifiers (shift, ctrl, alt, command)"));
  return zenith::mcp::schemas::buildToolSchema(
      "click", "Simulate a mouse click at specific coordinates", props,
      {"x", "y"});
}

inline juce::var clickComponentSchema() {
  auto *props = new juce::DynamicObject();
  zenith::mcp::schemas::setArrProp(
      props, "componentId",
      zenith::mcp::schemas::makeStringProperty("ID of the component to click"));
  return zenith::mcp::schemas::buildToolSchema(
      "click_component", "Find and click a component by its ID", props,
      {"componentId"});
}

inline juce::var typeTextSchema() {
  auto *props = new juce::DynamicObject();
  zenith::mcp::schemas::setArrProp(
      props, "text", zenith::mcp::schemas::makeStringProperty("Text to type"));
  return zenith::mcp::schemas::buildToolSchema(
      "type_text", "Type text into the currently focused component", props,
      {"text"});
}

//==============================================================================
// Implementation
//==============================================================================

/**
 * @brief Find component by ID (recursive)
 */
inline juce::Component *findComponentById(juce::Component *parent,
                                          const juce::String &id) {
  if (parent->getComponentID() == id)
    return parent;

  for (auto *child : parent->getChildren()) {
    if (auto *found = findComponentById(child, id))
      return found;
  }
  return nullptr;
}

/**
 * @brief helper to parse modifiers
 */
inline juce::ModifierKeys parseModifiers(const juce::String &mods) {
  juce::ModifierKeys keys;
  if (mods.containsIgnoreCase("shift"))
    keys = keys.withFlags(juce::ModifierKeys::shiftModifier);
  if (mods.containsIgnoreCase("ctrl"))
    keys = keys.withFlags(juce::ModifierKeys::ctrlModifier);
  if (mods.containsIgnoreCase("alt"))
    keys = keys.withFlags(juce::ModifierKeys::altModifier);
  if (mods.containsIgnoreCase("command") || mods.containsIgnoreCase("meta"))
    keys = keys.withFlags(juce::ModifierKeys::commandModifier);
  return keys;
}

/**
 * @brief Execute a click directly on a component at a specific position
 * Must be called on Message Thread!
 */
inline juce::var executeClick(juce::Component *mainWindow, int x, int y,
                              const juce::String &modifiers) {
  if (!mainWindow) {
    auto *error = new juce::DynamicObject();
    zenith::mcp::schemas::setStringProp(error, "error",
                                        "Main window not available");
    juce::var v;
    v = error;
    return v;
  }

  // Find component under mouse
  juce::Component *target = mainWindow->getComponentAt(x, y);
  if (!target)
    target = mainWindow;

  // Convert to local coordinates
  juce::Point<int> localPointInt =
      target->getLocalPoint(mainWindow, juce::Point<int>(x, y));
  juce::Point<float> localPoint = localPointInt.toFloat();

  // Create mouse event
  juce::MouseEvent event(juce::Desktop::getInstance().getMainMouseSource(),
                         localPoint, parseModifiers(modifiers), 1.0f, 0.0f,
                         0.0f, 0.0f, 0.0f, target, target,
                         juce::Time::getCurrentTime(), localPoint,
                         juce::Time::getCurrentTime(), 1, false);

  // Simulate click
  target->mouseDown(event);
  target->mouseUp(event);

  if (juce::Button *button = dynamic_cast<juce::Button *>(target)) {
    button->triggerClick();
  }

  auto *result = new juce::DynamicObject();
  zenith::mcp::schemas::setStringProp(result, "status", "clicked");

  juce::String targetId = target->getComponentID();
  if (targetId.isEmpty()) {
    targetId = typeid(*target).name();
  }

  zenith::mcp::schemas::setStringProp(result, "target", targetId);

  juce::var v;
  v = result;
  return v;
}

/**
 * @brief Execute click on component by ID
 */
inline juce::var executeClickComponent(juce::Component *mainWindow,
                                       const juce::String &componentId) {
  if (!mainWindow) {
    auto *error = new juce::DynamicObject();
    zenith::mcp::schemas::setStringProp(error, "error",
                                        "Main window not available");
    juce::var v;
    v = error;
    return v;
  }

  juce::Component *target = findComponentById(mainWindow, componentId);

  if (!target) {
    auto *error = new juce::DynamicObject();
    zenith::mcp::schemas::setStringProp(error, "error",
                                        "Component not found: " + componentId);
    juce::var v;
    v = error;
    return v;
  }

  // Click center of component
  juce::Rectangle<int> bounds = target->getScreenBounds();
  juce::Point<int> center = bounds.getCentre();
  juce::Point<int> windowPoint = mainWindow->getLocalPoint(nullptr, center);

  return executeClick(mainWindow, windowPoint.getX(), windowPoint.getY(), "");
}

/**
 * @brief Execute typing text
 */
inline juce::var executeTypeText(juce::Component *mainWindow,
                                 const juce::String &text) {
  if (!mainWindow) {
    auto *error = new juce::DynamicObject();
    zenith::mcp::schemas::setStringProp(error, "error",
                                        "Main window not available");
    juce::var v;
    v = error;
    return v;
  }

  juce::Component *focus = juce::Component::getCurrentlyFocusedComponent();
  if (!focus) {
    auto *error = new juce::DynamicObject();
    zenith::mcp::schemas::setStringProp(error, "error",
                                        "No component has focus");
    juce::var v;
    v = error;
    return v;
  }

  for (int i = 0; i < text.length(); ++i) {
    juce::juce_wchar character = text[i];
    focus->keyPressed(juce::KeyPress(character));
  }

  auto *result = new juce::DynamicObject();
  zenith::mcp::schemas::setStringProp(result, "status", "typed");
  zenith::mcp::schemas::setStringProp(result, "target",
                                      focus->getComponentID());
  juce::var v;
  v = result;
  return v;
}

} // namespace ui
} // namespace mcp
} // namespace zenith
