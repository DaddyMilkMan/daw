/*
  ==============================================================================

    MCPVisionTools.h
    Created: 2025-12-23
    Author:  Zenith DAW Team

    Vision tools for MCP - allowing AI to "see" the DAW interface.
    Provides screenshot capture and component inspection capabilities.

  ==============================================================================
*/

#pragma once

#include "MCPToolSchemas.h"
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <typeinfo>

namespace zenith {
namespace mcp {
namespace vision {

//==============================================================================
// Schemas
//==============================================================================

inline juce::var screenshotSchema() {
  auto *props = new juce::DynamicObject();
  // Optional: region (x, y, w, h)
  zenith::mcp::schemas::setArrProp(props, "region",
                                   zenith::mcp::schemas::makeStringProperty(
                                       "Optional region 'x,y,w,h' to capture"));
  zenith::mcp::schemas::setArrProp(
      props, "scale",
      zenith::mcp::schemas::makeNumberProperty("Scale factor (default 1.0)"));
  return zenith::mcp::schemas::buildToolSchema(
      "screenshot",
      "Capture a screenshot of the DAW window as a base64 PNG image", props);
}

inline juce::var getUITreeSchema() {
  auto *props = new juce::DynamicObject();
  zenith::mcp::schemas::setArrProp(
      props, "depth",
      zenith::mcp::schemas::makeNumberProperty(
          "Maximum depth to traverse (default 5)", true));
  return zenith::mcp::schemas::buildToolSchema(
      "get_ui_tree",
      "Get the UI component hierarchy with bounds and visibility state", props);
}

//==============================================================================
// Implementation
//==============================================================================

/**
 * @brief Traverse component hierarchy and build JSON tree
 */
inline juce::var buildComponentTree(juce::Component *comp, int depth,
                                    int maxDepth) {
  auto *node = new juce::DynamicObject();

  // Basic properties
  zenith::mcp::schemas::setStringProp(node, "class", typeid(*comp).name());
  zenith::mcp::schemas::setStringProp(node, "name", comp->getName());
  zenith::mcp::schemas::setStringProp(node, "id", comp->getComponentID());

  // Bounds (screen coordinates)
  auto bounds = comp->getScreenBounds();
  zenith::mcp::schemas::setIntProp(node, "x", bounds.getX());
  zenith::mcp::schemas::setIntProp(node, "y", bounds.getY());
  zenith::mcp::schemas::setIntProp(node, "w", bounds.getWidth());
  zenith::mcp::schemas::setIntProp(node, "h", bounds.getHeight());

  // State
  zenith::mcp::schemas::setBoolProp(node, "visible", comp->isVisible());
  zenith::mcp::schemas::setBoolProp(node, "enabled", comp->isEnabled());

  // Children
  if (depth < maxDepth && comp->getNumChildComponents() > 0) {
    juce::var children;
    for (auto *child : comp->getChildren()) {
      children.append(buildComponentTree(child, depth + 1, maxDepth));
    }
    zenith::mcp::schemas::setArrProp(node, "children", children);
  }

  return juce::var(node);
}

/**
 * @brief Execution handler for screenshot
 * Must be called on the Message Thread!
 */
inline juce::var executeScreenshot(juce::Component *mainWindow,
                                   const juce::var &args) {
  if (!mainWindow) {
    auto *error = new juce::DynamicObject();
    zenith::mcp::schemas::setStringProp(error, "error",
                                        "Main window not available");
    return juce::var(error);
  }

  // Parse arguments
  float scale = args.hasProperty("scale") ? (float)args["scale"] : 1.0f;
  if (scale <= 0.0f)
    scale = 1.0f;
  if (scale > 2.0f)
    scale = 2.0f; // Limit max scale

  juce::Rectangle<int> bounds = mainWindow->getLocalBounds();

  // Parse region if provided
  if (args.hasProperty("region")) {
    juce::String regionStr = args["region"].toString();
    juce::StringArray parts;
    parts.addTokens(regionStr, ",", "");

    if (parts.size() == 4) {
      int x = parts[0].getIntValue();
      int y = parts[1].getIntValue();
      int w = parts[2].getIntValue();
      int h = parts[3].getIntValue();

      juce::Rectangle<int> region(x, y, w, h);
      bounds = bounds.getIntersection(region);
    }
  }

  // Capture screenshot
  juce::Image snapshot =
      mainWindow->createComponentSnapshot(bounds, false, scale);

  if (snapshot.isNull()) {
    auto *error = new juce::DynamicObject();
    zenith::mcp::schemas::setStringProp(error, "error",
                                        "Failed to capture screenshot");
    return juce::var(error);
  }

  // Convert to PNG
  juce::MemoryOutputStream stream;
  juce::PNGImageFormat pngFormat;
  pngFormat.writeImageToStream(snapshot, stream);

  // Convert to Base64
  juce::String base64 =
      juce::Base64::toBase64(stream.getData(), stream.getDataSize());

  auto *result = new juce::DynamicObject();
  zenith::mcp::schemas::setStringProp(result, "image_data", base64);
  zenith::mcp::schemas::setStringProp(result, "format", "png");
  zenith::mcp::schemas::setStringProp(result, "encoding", "base64");
  zenith::mcp::schemas::setIntProp(result, "width", snapshot.getWidth());
  zenith::mcp::schemas::setIntProp(result, "height", snapshot.getHeight());

  return juce::var(result);
}

} // namespace vision
} // namespace mcp
} // namespace zenith
