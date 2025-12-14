/*
  ==============================================================================

    ModulationMatrixView.h
    Created: 2025-12-09
    Enhanced: 2025-12-12
    Author:  Zenith DAW

    Animated Modulation Routing Visualization.
    Features:
    - Bezier curves for modulation connections
    - Animated particle flow along routes
    - Neon glow on active connections
    - Real-time visualization of modulation values
    - Interactive drag-to-connect interface

  ==============================================================================
*/

#pragma once

#include "../../dsp/GlobalLFO.h"
#include "../../engine/MacroControl.h"
#include "../ZenithTheme.h"
#include "../skia/SkiaComponent.h"
#include "../skia/ZenithDesignSystem.h"
#include <JuceHeader.h>
#include <core/SkPath.h>
#include <core/SkPathMeasure.h>
#include <random>

namespace zenith {

class Engine;

//==============================================================================
/**
 * @brief Represents a modulation source node in the visualization
 */
struct ModulationSourceNode {
  juce::String id;
  juce::String displayName;

  enum class Type {
    LFO,
    Envelope,
    Macro,
    Velocity,
    Aftertouch,
    ModWheel,
    KeyTracking
  };

  Type type = Type::LFO;
  SkColor color = 0xFF00F0FF;
  juce::Point<float> position;
  float radius = 24.0f;

  // Real-time modulation value (-1 to 1)
  float currentValue = 0.0f;

  bool isHovered = false;
  bool isSelected = false;
};

//==============================================================================
/**
 * @brief Represents a modulation destination node
 */
struct ModulationDestNode {
  juce::String id;
  juce::String displayName;

  enum class Type {
    FilterCutoff,
    FilterResonance,
    OscPitch,
    OscDetune,
    OscMix,
    AmpGain,
    FXParameter,
    Pan,
    CustomParam
  };

  Type type = Type::FilterCutoff;
  SkColor color = 0xFFFF9944;
  juce::Point<float> position;
  float radius = 24.0f;

  // Current parameter value (0 to 1)
  float currentValue = 0.5f;

  bool isHovered = false;
  bool isSelected = false;

  // For plugin params
  juce::String trackId;
  int pluginIndex = -1;
  int paramIndex = -1;
};

//==============================================================================
/**
 * @brief Represents a modulation routing connection with animation data
 */
struct ModulationConnection {
  juce::String sourceId;
  juce::String destId;

  float amount = 0.0f; // -1 to 1 (bipolar)

  // Bezier control points (calculated dynamically)
  SkPath path;
  float pathLength = 0.0f;

  // Animation state
  float animationPhase = 0.0f;

  // Particles for animated flow
  struct Particle {
    float position = 0.0f; // 0-1 along path
    float speed = 0.02f;
    float size = 4.0f;
    float alpha = 1.0f;
  };
  std::vector<Particle> particles;

  bool isHovered = false;
  bool isSelected = false;

  // Color interpolation based on amount
  SkColor getColor() const {
    if (amount > 0) {
      // Positive: Orange/warm
      uint8_t intensity = static_cast<uint8_t>(std::abs(amount) * 255);
      return SkColorSetARGB(255, 255, 150 + (intensity / 3),
                            100 - intensity / 4);
    } else {
      // Negative: Blue/cool
      uint8_t intensity = static_cast<uint8_t>(std::abs(amount) * 255);
      return SkColorSetARGB(255, 100 - intensity / 4, 150 + (intensity / 3),
                            255);
    }
  }

  float getThickness() const {
    return 2.0f + std::abs(amount) * 4.0f; // 2-6px based on depth
  }
};

//==============================================================================
/**
 * @brief The Animated Modulation Matrix View
 *
 * A visual node-based modulation routing interface featuring:
 * - Source nodes on the left (LFOs, Envelopes, Macros)
 * - Destination nodes on the right (Parameters)
 * - Animated bezier connections between them
 * - Particle flow visualization
 * - Neon glow effects synchronized to modulation values
 */
class ModulationMatrixView : public SkiaComponent {
public:
  ModulationMatrixView();
  ~ModulationMatrixView() override;

  void setEngine(Engine *engine);

  // SkiaComponent overrides
  void drawSkia(SkCanvas *canvas) override;
  void timerCallback() override;

  // Mouse interaction
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseUp(const juce::MouseEvent &e) override;
  void mouseDrag(const juce::MouseEvent &e) override;
  void mouseMove(const juce::MouseEvent &e) override;
  void mouseDoubleClick(const juce::MouseEvent &e) override;
  void mouseWheelMove(const juce::MouseEvent &e,
                      const juce::MouseWheelDetails &wheel) override;

  // Refresh the matrix when routing changes
  void refreshMatrix();

  // AI Vision Support
  std::vector<AIElementInfo> getInspectableElements() override;

private:
  Engine *engine_ = nullptr;
  double time_ = 0.0;

  // Source and Destination Nodes
  std::vector<ModulationSourceNode> sourceNodes_;
  std::vector<ModulationDestNode> destNodes_;

  // Connections
  std::vector<ModulationConnection> connections_;

  // Interaction state
  ModulationSourceNode *hoveredSource_ = nullptr;
  ModulationDestNode *hoveredDest_ = nullptr;
  ModulationConnection *hoveredConnection_ = nullptr;
  ModulationConnection *selectedConnection_ = nullptr;

  // Drag-to-connect state
  bool isDraggingConnection_ = false;
  juce::String dragSourceId_;
  juce::Point<float> dragCurrentPos_;

  // Panning and zooming
  juce::Point<float> viewOffset_ = {0, 0};
  float zoomLevel_ = 1.0f;
  bool isPanning_ = false;
  juce::Point<float> lastPanPos_;

  // Grid configuration
  juce::Rectangle<float> gridBounds_;
  SkColor gridColor_ = 0x10FFFFFF;
  float gridSpacing_ = 40.0f;

  // Animation
  float globalAnimTime_ = 0.0f;
  std::mt19937 rng_;

  // Layout constants
  static constexpr float kNodeRadius = 28.0f;
  static constexpr float kNodeSpacingY = 70.0f;
  static constexpr float kSourceColumnX = 80.0f;
  static constexpr float kDestColumnX = 600.0f;
  static constexpr float kHeaderHeight = 50.0f;

  // Colors
  SkColor bgColor_ = design::colors::BG_DARKEST;
  SkColor sourceGlowColor_ = design::colors::CYAN;
  SkColor destGlowColor_ = design::colors::MAGENTA;

  // Build node lists
  void buildSourceNodes();
  void buildDestNodes();
  void buildConnections();

  // Layout
  void layoutNodes();
  void updateConnectionPaths();

  // Drawing helpers
  void drawBackground(SkCanvas *canvas);
  void drawGrid(SkCanvas *canvas);
  void drawSourceNodes(SkCanvas *canvas);
  void drawDestNodes(SkCanvas *canvas);
  void drawConnections(SkCanvas *canvas);
  void drawDragPreview(SkCanvas *canvas);
  void drawAmountEditor(SkCanvas *canvas);
  void drawNodeGlow(SkCanvas *canvas, const juce::Point<float> &pos,
                    float radius, SkColor color, float intensity);
  void drawNode(SkCanvas *canvas, const juce::Point<float> &pos, float radius,
                SkColor color, const juce::String &label, bool isSource,
                bool isHovered, bool isSelected, float value);
  void drawConnection(SkCanvas *canvas, ModulationConnection &conn);
  void drawParticles(SkCanvas *canvas, ModulationConnection &conn);
  void drawBipolarIndicator(SkCanvas *canvas, const ModulationConnection &conn);

  // Hit testing
  ModulationSourceNode *hitTestSource(const juce::Point<float> &pos);
  ModulationDestNode *hitTestDest(const juce::Point<float> &pos);
  ModulationConnection *hitTestConnection(const juce::Point<float> &pos);

  // Connection management
  void createConnection(const juce::String &sourceId,
                        const juce::String &destId);
  void deleteConnection(ModulationConnection *conn);
  void updateConnectionAmount(ModulationConnection *conn, float delta);

  // Animation updates
  void updateParticles(float deltaTime);
  void updateModulationValues();

  // Coordinate transforms
  juce::Point<float> screenToWorld(const juce::Point<float> &screen) const;
  juce::Point<float> worldToScreen(const juce::Point<float> &world) const;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationMatrixView)
};

} // namespace zenith
