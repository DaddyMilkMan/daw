/*
  ==============================================================================

    LayoutManager.h
    Created: 2025-12-12
    Author:  Zenith DAW Team

    Central layout management system for Zenith DAW.
    - Save/Load layout presets to JSON
    - Manage panel arrangements
    - Persist layouts on app close/restore on open
    - Support for tab groups, collapsible panels, and drag-to-resize

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <map>
#include <memory>
#include <vector>

namespace zenith {

// Forward declarations
class ResizablePanelContainer;
class SkiaComponent;

namespace layout {

//==============================================================================
// Panel Configuration
//==============================================================================

/**
 * @brief Configuration for a single panel
 */
struct PanelConfig {
  juce::String id;           // Unique panel identifier
  juce::String name;         // Display name
  juce::String type;         // Component type to create
  float initialSize = 0.0f;  // Initial size (0 = auto)
  float minSize = 100.0f;    // Minimum size in pixels
  float maxSize = 0.0f;      // Maximum size (0 = unlimited)
  float flex = 1.0f;         // Flex grow/shrink factor
  bool isCollapsed = false;  // Whether panel is collapsed
  bool isCollapsible = true; // Whether panel can be collapsed
  bool isVisible = true;     // Whether panel is visible
  int tabGroupIndex = -1;    // Tab group (-1 = not in tab group)
  juce::StringArray tabIds;  // IDs of tabs in this tab group
  juce::String activeTabId;  // Currently active tab ID
  juce::var customData;      // Custom panel-specific data

  // Serialization
  juce::var toVar() const;
  static PanelConfig fromVar(const juce::var &v);
};

/**
 * @brief Configuration for a layout divider
 */
struct DividerConfig {
  juce::String id;
  float position = 0.5f;     // Position as ratio (0.0-1.0)
  bool isHorizontal = false; // true = horizontal divider, false = vertical
  float minPositionRatio = 0.1f;
  float maxPositionRatio = 0.9f;

  juce::var toVar() const;
  static DividerConfig fromVar(const juce::var &v);
};

/**
 * @brief Complete layout configuration
 */
struct LayoutConfig {
  juce::String id;
  juce::String name;
  juce::String description;
  juce::Array<PanelConfig> panels;
  juce::Array<DividerConfig> dividers;
  juce::var rootLayout; // Nested structure defining layout hierarchy
  juce::Time lastModified;

  juce::var toVar() const;
  static LayoutConfig fromVar(const juce::var &v);
  juce::String toJSON() const;
  static LayoutConfig fromJSON(const juce::String &json);
};

//==============================================================================
// Layout Manager
//==============================================================================

/**
 * @brief Central layout management system
 *
 * Manages:
 * - Layout presets (Production, Editing, Mixing, etc.)
 * - Save/Load to JSON files
 * - Layout persistence on app close
 * - Panel registration and creation
 */
class LayoutManager : public juce::ChangeBroadcaster {
public:
  //============================================================================
  // Singleton
  //============================================================================
  static LayoutManager &getInstance();

  //============================================================================
  // Panel Registration
  //============================================================================

  /**
   * @brief Register a panel type with factory function
   */
  using PanelFactory = std::function<std::unique_ptr<juce::Component>()>;
  void registerPanelType(const juce::String &typeId,
                         const juce::String &displayName, PanelFactory factory);

  /**
   * @brief Create a panel component by type
   */
  std::unique_ptr<juce::Component> createPanel(const juce::String &typeId);

  /**
   * @brief Get all registered panel types
   */
  juce::StringArray getRegisteredPanelTypes() const;
  juce::String getPanelDisplayName(const juce::String &typeId) const;

  //============================================================================
  // Layout Presets
  //============================================================================

  /**
   * @brief Get built-in layout presets
   */
  LayoutConfig getPreset(const juce::String &presetName) const;
  juce::StringArray getAvailablePresets() const;

  /**
   * @brief Load/Apply a layout
   */
  void applyLayout(const LayoutConfig &config,
                   ResizablePanelContainer *container);
  void applyPreset(const juce::String &presetName,
                   ResizablePanelContainer *container);

  /**
   * @brief Save current layout from container
   */
  LayoutConfig captureCurrentLayout(const ResizablePanelContainer *container,
                                    const juce::String &name = "Custom") const;

  //============================================================================
  // Layout Persistence
  //============================================================================

  /**
   * @brief Save layout to JSON file
   */
  bool saveLayoutToFile(const LayoutConfig &config, const juce::File &file);
  LayoutConfig loadLayoutFromFile(const juce::File &file) const;

  /**
   * @brief Get layouts directory
   */
  juce::File getLayoutsDirectory() const;

  /**
   * @brief List saved custom layouts
   */
  juce::Array<juce::File> getSavedLayouts() const;

  /**
   * @brief Auto-save/restore last layout
   */
  void saveLastLayout(const ResizablePanelContainer *container);
  LayoutConfig loadLastLayout() const;
  bool hasLastLayout() const;

  //============================================================================
  // Animation Settings
  //============================================================================

  struct AnimationSettings {
    bool enabled = true;
    int resizeDurationMs = 150;
    int collapseDurationMs = 200;
    int tabSwitchDurationMs = 100;
    float springStiffness = 300.0f;
    float springDamping = 20.0f;
  };

  void setAnimationSettings(const AnimationSettings &settings);
  const AnimationSettings &getAnimationSettings() const {
    return animationSettings_;
  }

  //============================================================================
  // Edit Mode (for visual layout debugging)
  //============================================================================

  void setEditModeEnabled(bool enabled);
  bool isEditModeEnabled() const { return editModeEnabled_; }

  //============================================================================
  // Semantic Layout Morphing
  //============================================================================

  enum class AgentContext { None, SampleHunter, Wingman, PresetEvolver };
  
  /**
   * @brief Apply a semantic layout morph based on active AI agent.
   * Expands relevant panels and dims others to focus attention.
   */
  void applySemanticMorph(AgentContext context);
  
  /**
   * @brief Enable/disable layout morphing (user preference).
   */
  void setMorphingEnabled(bool enabled) { morphingEnabled_ = enabled; }
  bool isMorphingEnabled() const { return morphingEnabled_; }

private:
  LayoutManager();
  ~LayoutManager() = default;

  // Panel factories
  struct PanelTypeInfo {
    juce::String displayName;
    PanelFactory factory;
  };
  std::map<juce::String, PanelTypeInfo> panelTypes_;

  // Settings
  AnimationSettings animationSettings_;
  bool editModeEnabled_ = false;
  bool morphingEnabled_ = false; // Disabled by default per user request

  // Built-in presets
  void initializeBuiltInPresets();
  std::map<juce::String, LayoutConfig> builtInPresets_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LayoutManager)
};

} // namespace layout
} // namespace zenith
