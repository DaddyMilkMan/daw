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

#pragma once

// TempoLaneComponent.h


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

/**
 * @class TempoLaneComponent
 * @brief UI lane for tempo map editing
 *
 * User interactions:
 * - Double-click to add tempo point
 * - Drag to move tempo point (horizontal = time, vertical = BPM)
 * - Select point + Delete/Backspace to remove
 */
class TempoLaneComponent : public SkiaComponent,
                           private juce::ValueTree::Listener
{
public:
    //==========================================================================
    /**
     * @brief Constructor
     * @param projectState Reference to project state
     */
    explicit TempoLaneComponent(ProjectState& projectState);

    /**
     * @brief Destructor
     */
    ~TempoLaneComponent() override;

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
    void valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index) override;
    void valueTreeChildOrderChanged(juce::ValueTree& parent, int oldIndex, int newIndex) override;
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
     * @brief Convert Y coordinate to BPM
     */
    double yToBpm(float y) const;

    /**
     * @brief Convert BPM to Y coordinate
     */
    float bpmToY(double bpm) const;

    /**
     * @brief Find tempo point at position (returns ID or empty string)
     */
    juce::String findPointAt(float x, float y) const;

    /**
     * @brief Draw a tempo point
     */
    void drawTempoPoint(SkCanvas* canvas, double timeBeats, double bpm, bool selected) const;

    /**
     * @brief Draw grid lines for BPM
     */
    void drawGrid(SkCanvas* canvas) const;

    /**
     * @brief Draw tempo curve connecting points
     */
    void drawTempoCurve(SkCanvas* canvas) const;

    /**
     * @brief Draw all tempo points
     */
    void drawTempoPoints(SkCanvas* canvas) const;

    /**
     * @brief Track mouse movement for hover effects
     */
    void mouseMove(const juce::MouseEvent& event) override;

    /**
     * @brief Clear hover state when mouse exits
     */
    void mouseExit(const juce::MouseEvent& event) override;


    //==========================================================================
    // Member Variables
    //==========================================================================

    ProjectState& projectState;

    // View settings (MVP: fixed range, no zoom/scroll)
    double viewStartBeats = 0.0;
    double viewEndBeats = 64.0;  // 16 bars at 4/4
    double minBpm = 40.0;
    double maxBpm = 240.0;

    // Interaction state
    juce::String selectedPointId;
    juce::String hoveredPointId;
    bool isDraggingPoint = false;
    float dragStartX = 0.0f;
    float dragStartY = 0.0f;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TempoLaneComponent)
};

} // namespace zenith

