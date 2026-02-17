/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#pragma once

#include "SkiaRotaryKnob.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <array>

extern "C++" {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-warning-option"
#pragma clang diagnostic ignored "-Wattributes"
#include <core/SkCanvas.h>
#include <core/SkColor.h>
#include <core/SkPath.h>
#include <core/SkRect.h>
#include <effects/SkGradientShader.h>
#pragma clang diagnostic pop
}

namespace zenith {

//==============================================================================
// Modulation Source Info
//==============================================================================

struct ModulationSource {
    juce::String id;
    juce::String name;
    SkColor color;
    float currentValue = 0.0f;      // -1.0 to 1.0 (bipolar)
    bool bipolar = true;

    // Visualization
    float smoothedValue = 0.0f;
    float displayedValue = 0.0f;
    bool isActive = false;
};

//==============================================================================
// Modulation Routing
//==============================================================================

struct ModulationRouting {
    ModulationSource* source = nullptr;
    float amount = 0.0f;            // Depth of modulation
    float minimum = -1.0f;          // Minimum output value
    float maximum = 1.0f;          // Maximum output value
    bool isActive = false;
    bool isInverted = false;

    // Visualization
    float currentOutput = 0.0f;     // Current modulated value

    // Curve type
    int curveType = 0;              // 0=linear, 1=exp, etc.
};

//==============================================================================
// Smart Knob with Modulation Visualization
//==============================================================================

class SkiaSmartKnob : public SkiaRotaryKnob {
public:
    SkiaSmartKnob();
    ~SkiaSmartKnob() override = default;

    void drawSkia(SkCanvas* canvas) override;
    std::vector<AIElementInfo> getInspectableElements() override;

    // Modulation routing
    void addModulationSource(ModulationSource* source);
    void removeModulationSource(const juce::String& sourceId);
    void clearModulationSources();

    void setModulationRouting(const ModulationRouting& routing);
    void removeModulationRouting(int index);
    void clearModulationRoutings();

    // Get total modulated value (base + all modulations)
    float getModulatedValue() const;

    // Visualization
    void setShowModulationRing(bool show) { showModulationRing_ = show; markDirty(); }
    void setShowValueTooltip(bool show) { showValueTooltip_ = show; markDirty(); }
    void setShowMiniDisplay(bool show) { showMiniDisplay_ = show; markDirty(); }

    // Style
    void setModulationRingWidth(float width) { modulationRingWidth_ = width; }
    void setModulationOpacity(float opacity) { modulationOpacity_ = opacity; }

protected:
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;

private:
    // Modulation state
    std::vector<ModulationSource*> modulationSources_;
    std::vector<ModulationRouting> modulationRoutings_;

    // Visualization
    bool showModulationRing_ = true;
    bool showValueTooltip_ = true;
    bool showMiniDisplay_ = false;
    float modulationRingWidth_ = 2.0f;
    float modulationOpacity_ = 0.7f;

    // Interaction
    int hoveredRouting_ = -1;
    float dragStartValue_ = 0.0f;

    // Animation
    float valueAnimationTarget_ = 0.0f;
    float valueAnimationCurrent_ = 0.0f;

    // Drawing helpers
    void drawModulationRing(SkCanvas* canvas, const SkRect& bounds, float modulatedValue);
    void drawModulationRings(SkCanvas* canvas, const SkRect& bounds);
    void drawValueTooltip(SkCanvas* canvas, const SkRect& bounds);
    void drawMiniDisplay(SkCanvas* canvas, const SkRect& bounds);

    float calculateTotalModulation() const;
    SkColor getModulationColor() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaSmartKnob)
};

//==============================================================================
// Modulation Ring Types
//==============================================================================

enum class ModulationRingStyle {
    Solid,              // Solid filled ring
    Segmented,          // Segmented LED-style ring
    Dotted,             // Dotted ring
    Waveform,           // Waveform visualization
    Bars,               // Vertical bar indicators
    Concentric          // Multiple concentric rings
};

//==============================================================================
// Advanced Modulation Display
//==============================================================================

class SkiaModulationDisplay : public SkiaComponent {
public:
    SkiaModulationDisplay();
    ~SkiaModulationDisplay() override = default;

    void drawSkia(SkCanvas* canvas) override;

    // Sources to display
    void setSources(const std::vector<ModulationSource*>& sources);
    void addSource(ModulationSource* source);

    // Routing to display
    void setRoutings(const std::vector<ModulationRouting>& routings);
    void addRouting(const ModulationRouting& routing);

    // Layout
    void setLayoutStyle(ModulationRingStyle style) { layoutStyle_ = style; }
    void setMaxRoutingsToShow(int max) { maxRoutings_ = max; }

    // Display options
    void setShowSourceNames(bool show) { showSourceNames_ = show; }
    void setShowAmountValues(bool show) { showAmountValues_ = show; }
    void setAnimationSpeed(float speed) { animationSpeed_ = speed; }

protected:
    void timerCallback() override;

private:
    std::vector<ModulationSource*> sources_;
    std::vector<ModulationRouting> routings_;

    ModulationRingStyle layoutStyle_ = ModulationRingStyle::Segmented;
    int maxRoutings_ = 8;
    bool showSourceNames_ = true;
    bool showAmountValues_ = true;
    float animationSpeed_ = 1.0f;

    float animationPhase_ = 0.0f;

    // Layout cache
    std::vector<SkRect> sourceRects_;

    void drawSolidRing(SkCanvas* canvas, const SkRect& bounds,
                     const std::vector<float>& amounts);
    void drawSegmentedRing(SkCanvas* canvas, const SkRect& bounds,
                         const std::vector<float>& amounts);
    void drawDottedRing(SkCanvas* canvas, const SkRect& bounds,
                       const std::vector<float>& amounts);
    void drawWaveformRing(SkCanvas* canvas, const SkRect& bounds,
                         const std::vector<float>& waveform);
    void drawBars(SkCanvas* canvas, const SkRect& bounds,
                 const std::vector<float>& amounts);
    void drawConcentricRings(SkCanvas* canvas, const SkRect& bounds,
                            const std::vector<float>& amounts);

    void drawSourceIndicators(SkCanvas* canvas, const SkRect& bounds);
    void drawAmountLabels(SkCanvas* canvas, const SkRect& bounds);

    SkColor getSourceColor(int index) const;
};

//==============================================================================
// Parameter with Smart Modulation
//==============================================================================

struct SmartParameter {
    juce::String id;
    juce::String name;
    float value = 0.5f;
    float minValue = 0.0f;
    float maxValue = 1.0f;
    float defaultValue = 0.5f;

    // Modulation
    std::vector<ModulationRouting> routings;

    // Display
    juce::String format;             // Value format string
    int decimalPlaces = 2;
    juce::String unit;

    // UI state
    bool isModulated = false;
    bool isLocked = false;
    bool isHidden = false;
    bool isAutomated = false;
    bool hasMidiAssignment = false;

    // Computed value (base + all mods)
    float modulatedValue = 0.5f;

    // Update modulated value
    void updateModulatedValue();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SmartParameter)
};

//==============================================================================
// Modulation Matrix Component (Enhanced)
//==============================================================================

class SkiaModulationMatrix : public SkiaComponent {
public:
    SkiaModulationMatrix();
    ~SkiaModulationMatrix() override = default;

    void drawSkia(SkCanvas* canvas) override;

    // Data access
    void setSources(const std::vector<ModulationSource>& sources);
    void setDestinations(const std::vector<juce::String>& destinations);

    // Routing
    void addRouting(const ModulationRouting& routing);
    void removeRouting(int index);
    void clearAllRoutings();

    // Display
    void setCompactMode(bool compact) { compactMode_ = compact; markDirty(); }
    void setShowCurveEditor(bool show) { showCurveEditor_ = show; markDirty(); }
    void setExpandedMode(bool expanded) { expandedMode_ = expanded; markDirty(); }

    // Interaction
    int getHoveredSource() const { return hoveredSource_; }
    int getHoveredDestination() const { return hoveredDestination_; }
    int getHoveredRouting() const { return hoveredRouting_; }

    // Callbacks
    using RoutingChangedCallback = std::function<void(const ModulationRouting&)>;
    void setRoutingChangedCallback(RoutingChangedCallback callback) {
        routingChangedCallback_ = std::move(callback);
    }

protected:
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

private:
    // Data
    std::vector<ModulationSource> sources_;
    std::vector<juce::String> destinations_;
    std::vector<ModulationRouting> routings_;

    // Display
    bool compactMode_ = false;
    bool showCurveEditor_ = false;
    bool expandedMode_ = false;

    // Layout
    SkRect sourcesRect_;
    SkRect destinationsRect_;
    SkRect matrixRect_;
    SkRect curveEditorRect_;

    // Interaction
    int hoveredSource_ = -1;
    int hoveredDestination_ = -1;
    int hoveredRouting_ = -1;
    int draggedRouting_ = -1;
    bool isCreatingRouting_ = false;
    int routingStartSource_ = -1;

    // Temp routing (during drag creation)
    ModulationRouting tempRouting_;

    // Animation
    float newRoutingAnimation_ = 0.0f;

    // Callback
    RoutingChangedCallback routingChangedCallback_;

    // Drawing helpers
    void drawSourcesPanel(SkCanvas* canvas, const SkRect& bounds);
    void drawDestinationsPanel(SkCanvas* canvas, const SkRect& bounds);
    void drawMatrix(SkCanvas* canvas, const SkRect& bounds);
    void drawRoutingLine(SkCanvas* canvas, const SkRect& bounds, const ModulationRouting& routing,
                     const SkPoint& sourcePos, const SkPoint& destPos);
    void drawRoutingDot(SkCanvas* canvas, const SkPoint& pos, SkColor color, bool isActive);
    void drawCurveEditor(SkCanvas* canvas, const SkRect& bounds);
    void drawTempLine(SkCanvas* canvas, const SkRect& bounds, int sourceIndex);

    // Layout helpers
    SkPoint getSourcePosition(int index, const SkRect& bounds) const;
    SkPoint getDestinationPosition(int index, const SkRect& bounds) const;
    int getSourceAtPosition(const SkPoint& pos, const SkRect& bounds) const;
    int getDestinationAtPosition(const SkPoint& pos, const SkRect& bounds) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaModulationMatrix)
};

//==============================================================================
// Context Menu System
//==============================================================================

class SkiaContextMenu : public SkiaComponent {
public:
    struct MenuItem {
        juce::String id;
        juce::String text;
        juce::String shortcutText;

        bool isEnabled = true;
        bool isSeparator = false;
        bool isChecked = false;
        bool isSubmenu = false;

        std::vector<MenuItem> subItems;
        std::function<void()> action;

        int iconType = 0;             // 0=none, 1=check, 2=lock, 3=star, etc.
        SkColor iconColor = SkColorSetRGB(150, 150, 150);

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MenuItem)
    };

    SkiaContextMenu();
    ~SkiaContextMenu() override = default;

    void drawSkia(SkCanvas* canvas) override;

    // Menu management
    void setItems(const std::vector<MenuItem>& items);
    void clearItems();
    void addItem(const MenuItem& item);
    void addSeparator();
    void addSubmenu(const juce::String& text, const std::vector<MenuItem>& items);

    // Display
    void showAt(const juce::Point<int>& position);
    void dismiss();

    // Quick creation helpers
    void createStandardMenu(bool canUndo, bool canRedo, bool canLearn);
    void createKnobContextMenu();
    void createFaderContextMenu();
    void createWaveformContextMenu();

    // Callback
    using MenuItemClickedCallback = std::function<void(const juce::String&)>;
    void setMenuItemClickedCallback(MenuItemClickedCallback callback) {
        itemClickedCallback_ = std::move(callback);
    }

    bool isVisible() const { return isVisible_; }

protected:
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;

private:
    std::vector<MenuItem> menuItems_;
    int hoveredIndex_ = -1;
    bool isVisible_ = false;

    // Layout
    float itemHeight_ = 28.0f;
    SkRect menuRect_;
    juce::Point<int> anchorPosition_;

    // Submenu handling
    SkiaContextMenu* activeSubmenu_ = nullptr;
    int submenuParentIndex_ = -1;
    std::unique_ptr<SkiaContextMenu> submenu_;

    MenuItemClickedCallback itemClickedCallback_;

    // Drawing
    void drawMenuBackground(SkCanvas* canvas, const SkRect& bounds);
    void drawMenuItem(SkCanvas* canvas, const SkRect& bounds, const MenuItem& item,
                    int index, bool isHovered, bool hasSubmenu);
    void drawSeparator(SkCanvas* canvas, const SkRect& bounds, int index);
    void drawSubmenuArrow(SkCanvas* canvas, const SkRect& bounds, bool isOpen);
    void drawCheckbox(SkCanvas* canvas, const SkRect& bounds, bool isChecked);
    void drawIcon(SkCanvas* canvas, const SkRect& bounds, int iconType, SkColor color);

    // Helper
    int getMenuItemAt(float y) const;
    void dismissSubmenus();
};

//==============================================================================
// Global Undo Manager
//==============================================================================

class GlobalUndoManager {
public:
    struct UndoableAction {
        juce::String description;
        juce::String actionType;  // "parameter_change", "preset_load", etc.

        // Before state
        juce::StringArray affectedParameters;
        std::vector<float> previousValues;

        // After state
        std::vector<float> newValues;

        // Timestamp
        juce::int64 timestamp;

        // Action function
        std::function<bool()> undoFunc;
        std::function<bool()> redoFunc;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UndoableAction)
    };

    GlobalUndoManager();
    ~GlobalUndoManager() = default;

    // State management
    void beginNewAction(const juce::String& description, const juce::String& type);
    void addParameterChange(const juce::String& paramId, float newValue, float oldValue);
    void endAction();

    // Undo/Redo
    void undo();
    void redo();
    bool canUndo() const { return historyPosition_ > 0; }
    bool canRedo() const { return historyPosition_ < history_.size(); }

    // Current action
    bool isInAction() const { return currentAction_ != nullptr; }
    void discardCurrentAction();

    // History
    size_t getHistorySize() const { return history_.size(); }
    const UndoableAction& getHistoryEntry(size_t index) const;

    // Clear
    void clear();
    void setMaxHistorySize(size_t maxSize) { maxHistorySize_ = maxSize; }

    // Grouping
    void beginGroup(const juce::String& name);
    void endGroup();

    // Callbacks
    using StateChangedCallback = std::function<void()>;
    void setStateChangedCallback(StateChangedCallback callback) {
        stateChangedCallback_ = std::move(callback);
    }

    // Preset state snapshots
    void saveState(const juce::String& presetId, const SynthStateSnapshot& state);

private:
    std::vector<std::unique_ptr<UndoableAction>> history_;
    size_t historyPosition_ = 0;
    size_t maxHistorySize_ = 1000;

    std::unique_ptr<UndoableAction> currentAction_;
    juce::StringArray openGroups_;

    StateChangedCallback stateChangedCallback_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GlobalUndoManager)
};

//==============================================================================
// Right-Click Handler (for all components)
//==============================================================================

class RightClickHandler {
public:
    using ContextMenuBuilder = std::function<void(SkiaContextMenu*)>;

    static void attachTo(SkiaComponent* component);
    static void detachFrom(SkiaComponent* component);

    // Default menu builders
    static void buildKnobMenu(SkiaContextMenu* menu, SkiaRotaryKnob* knob,
                             SynthParameter* param, GlobalUndoManager* undo);
    static void buildFaderMenu(SkiaContextMenu* menu, SkiaLinearFader* fader,
                             SynthParameter* param, GlobalUndoManager* undo);
    static void buildWaveformMenu(SkiaContextMenu* menu, SkiaWaveformDisplay* display);
    static void buildEnvelopMenu(SkiaContextMenu* menu, SkiaEnvelopeEditor* editor);
    static void buildGeneralMenu(SkiaContextMenu* menu);

    static ContextMenuBuilder getDefaultMenuBuilder() {
        return [](SkiaContextMenu* menu) { buildGeneralMenu(menu); };
    }

private:
    struct ComponentAttachment {
        SkiaComponent* component;
        ContextMenuBuilder builder;
        bool hasCustomBuilder = false;
        juce::Component::SafePointer<SkiaContextMenu> activeMenu;
    };

    static juce::HashMap<SkiaComponent*, ComponentAttachment> attachments_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RightClickHandler)
};

} // namespace zenith
