/**
 * @file MarkerLaneComponent.h
 * @brief UI component for displaying and editing markers
 *
 * Phase 15: Tempo Map & Global Markers MVP
 *
 * Displays markers as labeled flags along a timeline.
 * Allows adding, moving, renaming, and deleting markers via mouse interaction.
 */

#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_events/juce_events.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_data_structures/juce_data_structures.h>
#include "ProjectState.h"
#include "../framework/SkiaComponent.h"
#include <core/SkCanvas.h>

//==============================================================================
namespace zenith {

//==============================================================================
/**
 * @class MarkerLaneComponent
 * @brief UI lane for marker editing
 *
 * User interactions:
 * - Double-click to add marker (auto-named "Marker 1", "Marker 2", etc.)
 * - Drag to move marker horizontally
 * - Double-click label to rename (via AlertWindow)
 * - Select marker + Delete/Backspace to remove
 */
class MarkerLaneComponent : public SkiaComponent,
                            private juce::ValueTree::Listener
{
public:
    //==========================================================================
    /**
     * @brief Constructor
     * @param projectState Reference to project state
     */
    explicit MarkerLaneComponent(ProjectState& projectState);

    /**
     * @brief Destructor
     */
    ~MarkerLaneComponent() override;

    //==========================================================================
    // Component interface
    //==========================================================================

    void drawSkia(SkCanvas* canvas) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;

    bool keyPressed(const juce::KeyPress& key) override;

private:
    //==========================================================================
    // ValueTree::Listener
    //==========================================================================

    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override;
    void valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index) [[maybe_unused]] override;
    void valueTreeChildOrderChanged(juce::ValueTree& parent, int oldIndex, int newIndex) [[maybe_unused]] override;
    void valueTreeParentChanged(juce::ValueTree& tree) override;

    //==========================================================================
    // Helper Methods
    //==========================================================================

    /**
     * @brief Convert X coordinate to beats
     */
    double xToBeats(float x) const;

    /**
     * @brief Convert beats to X coordinate
     */
    float beatsToX(double beats) const;

    /**
     * @brief Find marker at position (returns ID or empty string)
     */
    juce::String findMarkerAt(float x, float y) const;

    /**
     * @brief Draw a marker
     */
    void drawMarker(SkCanvas* canvas, double timeBeats, const juce::String& name, 
                   const juce::String& colorHex, bool selected) const;

    /**
     * @brief Draw all markers
     */
    void drawMarkers(SkCanvas* canvas) const;

    /**
     * @brief Track mouse movement for hover effects
     */
    void mouseMove(const juce::MouseEvent& event) override;

    /**
     * @brief Clear hover state when mouse exits
     */
    void mouseExit(const juce::MouseEvent& event) override;

    /**
     * @brief Generate next marker name ("Marker 1", "Marker 2", etc.)
     */
    juce::String generateMarkerName() const;

    /**
     * @brief Show rename dialog for marker
     */
    void showRenameDialog(const juce::String& markerId);

    //==========================================================================
    // Member Variables
    //==========================================================================

    ProjectState& projectState;

    // View settings (MVP: fixed range, no zoom/scroll)
    double viewStartBeats = 0.0;
    double viewEndBeats = 64.0;  // 16 bars at 4/4

    // Interaction state
    juce::String selectedMarkerId;
    juce::String hoveredMarkerId;
    bool isDraggingMarker = false;
    float dragStartX = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MarkerLaneComponent)
};

} // namespace zenith

