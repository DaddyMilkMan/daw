/**
 * @file ArrangerComponent.h
 * @brief Minimal timeline arranger UI (v0.1)
 *
 * Responsibilities:
 * - Visualize tracks and clips from ProjectModel
 * - Draw playhead
 * - Handle mouse interaction:
 *   - Click to set playhead
 *   - Drag clips horizontally to change startSample
 *
 * v0.1 limitations:
 * - No zoom/scroll
 * - No resizing clips
 * - No adding/removing tracks from UI
 * - No snapping/grid
 * - No drag between tracks
 * - No automation, MIDI, tempo
 *
 * MESSAGE THREAD only
 */

#pragma once

#include <JuceHeader.h>
#include "../../include/editor/ProjectEditorState.h"

namespace zenith {

/**
 * @brief Minimal arranger view for v0.1
 *
 * Visual timeline showing tracks (horizontal rows) and clips (rectangles).
 * Simple click-to-set-playhead and drag-to-move-clip interaction.
 */
class ArrangerComponent : public juce::Component
{
public:
    /**
     * @brief Construct arranger
     * @param editorState Reference to editor state (model + playback)
     */
    explicit ArrangerComponent(ProjectEditorState& editorState);
    ~ArrangerComponent() override;

    //==========================================================================
    // Component Interface
    //==========================================================================

    void paint(juce::Graphics& g) override;
    void resized() override;

    //==========================================================================
    // Mouse Interaction
    //==========================================================================

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

private:
    //==========================================================================
    // Layout Configuration (v0.1 constants)
    //==========================================================================

    static constexpr float kPixelsPerSecond = 100.0f;  ///< Fixed zoom level
    static constexpr int kTrackHeight = 48;            ///< Height of each track row
    static constexpr int kTrackHeaderWidth = 120;      ///< Width of left track header
    static constexpr int kTimelinePadding = 4;         ///< Vertical padding within track

    //==========================================================================
    // Drag State
    //==========================================================================

    /**
     * @brief State for clip drag operation
     */
    struct DragState
    {
        bool active = false;             ///< true if drag in progress
        int trackIndex = -1;             ///< Track being dragged
        int clipIndex = -1;              ///< Clip being dragged
        int64_t originalStart = 0;       ///< Original startSample before drag
        int dragStartX = 0;              ///< Mouse X at drag start
    } drag_;

    //==========================================================================
    // Internal Helpers
    //==========================================================================

    /**
     * @brief Get project sample rate (for time conversions)
     * @return Sample rate in Hz
     */
    double getProjectSampleRate_() const;

    /**
     * @brief Convert timeline position to X coordinate
     * @param samples Absolute timeline position (samples)
     * @return X coordinate (pixels)
     */
    float timeToX_(int64_t samples) const;

    /**
     * @brief Convert X coordinate to timeline position
     * @param x X coordinate (pixels)
     * @return Absolute timeline position (samples)
     */
    int64_t xToSamples_(int x) const;

    /**
     * @brief Draw all tracks and clips
     * @param g Graphics context
     */
    void drawTracks_(juce::Graphics& g);

    /**
     * @brief Draw playhead line
     * @param g Graphics context
     */
    void drawPlayhead_(juce::Graphics& g);

    /**
     * @brief Hit test for clip at mouse position
     * @param x Mouse X coordinate
     * @param y Mouse Y coordinate
     * @return Pair of {trackIndex, clipIndex}, or {-1, -1} if no hit
     */
    std::pair<int, int> hitTestClip_(int x, int y) const;

    //==========================================================================
    // Member Variables
    //==========================================================================

    ProjectEditorState& editor_;  ///< Reference to editor state

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrangerComponent)
};

} // namespace zenith
