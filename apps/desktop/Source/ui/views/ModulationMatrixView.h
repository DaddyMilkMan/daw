/*
  ==============================================================================

    ModulationMatrixView.h
    Created: 2025-12-09
    Author:  Zenith DAW

    A+ Grade Implementation: Node-based Modulation Graph specific definitions.

  ==============================================================================
*/

#pragma once

#include "../../dsp/GlobalLFO.h"
#include "../../engine/MacroControl.h"
#include "../skia/SkiaComponent.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace zenith {

class Engine;

//==============================================================================
/**
 * @brief Node-based Modulation Matrix Visualization
 *
 * Uses Skia to render a node graph where:
 * - Left side: Source Nodes (LFOs, Macros)
 * - Right side: Destination Nodes (Parameters)
 * - Connections: Animated Bezier curves with particle flow
 */
class ModulationMatrixView : public SkiaComponent {
public:
  ModulationMatrixView();
  ~ModulationMatrixView() override;

  void setEngine(Engine *engine);

  // Skia Rendering Override
  void drawSkia(SkCanvas *canvas) override;

  // Interactions
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseDrag(const juce::MouseEvent &e) override;
  void mouseUp(const juce::MouseEvent &e) override;
  void mouseMove(const juce::MouseEvent &e) override;

  void timerCallback() override;

  // Refresh graph from Engine state
  void refreshNodes();

private:
  Engine *engine_ = nullptr;
  double time_ = 0.0;

  enum class NodeType { Source, Destination };

  struct Node {
    juce::String id;
    juce::String name;
    NodeType type;
    SkColor color;
    juce::Rectangle<float> bounds;
    juce::Point<float> portLocation;
    bool isHovered = false;
  };

  struct Connection {
    juce::String sourceId;
    juce::String destId;
    float amount = 1.0f;
  };

  std::vector<Node> sourceNodes_;
  std::vector<Node> destNodes_;
  std::vector<Connection> connections_;

  // Drag State
  bool isDragging_ = false;
  juce::Point<float> dragStartPos_;
  juce::Point<float> dragEndPos_;
  juce::String dragSourceId_;

  void layoutNodes();
  void drawNode(SkCanvas *canvas, const Node &node);
  void drawConnection(SkCanvas *canvas, const Connection &conn);

  Node *findNode(const juce::String &id);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationMatrixView)
};

} // namespace zenith
