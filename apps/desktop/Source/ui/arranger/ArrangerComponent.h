/**
 * @file ArrangerComponent.h
 * @brief Timeline/Arranger view component for Zenith DAW
 * 
 * The ArrangerComponent is the main timeline view that displays tracks and clips.
 * It delegates to specialized helper classes for different concerns:
 * - ArrangerGridUtils: Coordinate conversion and waveform caching
 * - ArrangerClipManager: Clip lifecycle and selection
 * - ArrangerInputHandler: Mouse and keyboard input
 * - ArrangerRenderer: Skia drawing (when ZENITH_USE_SKIA is defined)
 */
#pragma once

#include "SkiaComponent.h"
#include "Engine.h"
#include "ProjectState.h"
#include "MiniMapComponent.h"
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <core/SkCanvas.h>
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

#include "MacroToolbar.h"

// Forward declaration for browser drag
namespace zenith {
class BrowserDragData;
}

namespace zenith {

// Forward declarations for helper classes
class ArrangerGridUtils;
class ArrangerClipManager;
class ArrangerInputHandler;
class ArrangerTrackComponent;
struct ClipView;

#ifdef ZENITH_USE_SKIA
class ArrangerRenderer;
#endif

//==============================================================================
/**
 * @brief Grid resolution options for snapping
 */
enum class GridResolution {
    Bar_1 = 0, ///< 4 beats (in 4/4)
    Beat_1,    ///< 1 beat (quarter note)
    Beat_1_2,  ///< 1/2 beat (eighth note)
    Beat_1_4,  ///< 1/4 beat (sixteenth note)
    Beat_1_8,  ///< 1/8 beat (thirty-second)
    Beat_1_3,  ///< 1/3 beat (triplet eighth)
    Beat_1_6,  ///< 1/6 beat (triplet sixteenth)
    Off        ///< No snap
};

/**
 * @brief Convert grid resolution to beat value
 * @param res Grid resolution enum value
 * @return Beat value (e.g., 4.0 for Bar_1, 1.0 for Beat_1)
 */
inline double gridResolutionToBeats(GridResolution res) {
    switch (res) {
    case GridResolution::Bar_1:
        return 4.0;
    case GridResolution::Beat_1:
        return 1.0;
    case GridResolution::Beat_1_2:
        return 0.5;
    case GridResolution::Beat_1_4:
        return 0.25;
    case GridResolution::Beat_1_8:
        return 0.125;
    case GridResolution::Beat_1_3:
        return 1.0 / 3.0;
    case GridResolution::Beat_1_6:
        return 1.0 / 6.0;
    case GridResolution::Off:
        return 0.0;
    default:
        return 1.0;
    }
}

//==============================================================================
/**
 * @class ArrangerComponent
 * @brief Main timeline/arranger view for the DAW
 * 
 * Displays tracks and clips in a horizontal timeline. Supports:
 * - Clip selection, movement, and resizing
 * - Zoom and scroll navigation
 * - Drag-and-drop from browser
 * - Keyboard shortcuts for editing
 * - Premium glassmorphic Skia rendering
 */
class ArrangerComponent : public SkiaComponent,
                          public juce::ValueTree::Listener,
                          public juce::DragAndDropTarget {
public:
    /**
     * @brief Construct arranger component
     * @param engine Reference to the audio engine
     * @param ps Reference to the project state
     */
    ArrangerComponent(Engine& engine, ProjectState& ps);
    
    ~ArrangerComponent() override;

    //==========================================================================
    // Component Interface
    //==========================================================================
    
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e,
                        const juce::MouseWheelDetails& wheel) override;

    //==========================================================================
    // Callbacks
    //==========================================================================
    
    /** @brief Callback when a clip is double-clicked (for opening editor) */
    std::function<void(const juce::String& trackId, const juce::String& clipId)>
        onClipDoubleClicked;

    //==========================================================================
    // ValueTree::Listener Interface
    //==========================================================================
    
    void valueTreePropertyChanged(juce::ValueTree& tree,
                                  const juce::Identifier& property) override;
    void valueTreeChildAdded(juce::ValueTree& parent,
                             juce::ValueTree& child) override;
    void valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child,
                               int index) override;
    void valueTreeChildOrderChanged(juce::ValueTree& parent, int oldIndex,
                                    int newIndex) override;

    //==========================================================================
    // Skia Rendering
    //==========================================================================
    
    void drawSkia(SkCanvas* canvas) override;

    //==========================================================================
    // Tooltip
    //==========================================================================
    
    juce::String getTooltip();

    //==========================================================================
    // DragAndDropTarget Interface
    //==========================================================================
    
    bool isInterestedInDragSource(
        const juce::DragAndDropTarget::SourceDetails& details) override;
    void itemDropped(const juce::DragAndDropTarget::SourceDetails& details) override;
    void itemDragEnter(const juce::DragAndDropTarget::SourceDetails& details) override;
    void itemDragExit(const juce::DragAndDropTarget::SourceDetails& details) override;
    void itemDragMove(const juce::DragAndDropTarget::SourceDetails& details) override;

    //==========================================================================
    // Grid Resolution Control
    //==========================================================================
    
    void setGridResolution(GridResolution res);
    GridResolution getGridResolution() const { return gridResolution_; }

    //==========================================================================
    // Timer Interface
    //==========================================================================
    
    void timerCallback() override;

private:
    // Allow helper classes to access private members
    friend class ArrangerGridUtils;
    friend class ArrangerClipManager;
    friend class ArrangerInputHandler;
#ifdef ZENITH_USE_SKIA
    friend class ArrangerRenderer;
#endif

    //==========================================================================
    // Core References
    //==========================================================================
    
    Engine& engine_;
    zenith::ProjectState& projectState;

    //==========================================================================
    // Helper Module Objects
    //==========================================================================
    
    std::unique_ptr<ArrangerGridUtils> gridUtils_;
    std::unique_ptr<ArrangerClipManager> clipManager_;
    std::unique_ptr<ArrangerInputHandler> inputHandler_;
#ifdef ZENITH_USE_SKIA
    std::unique_ptr<ArrangerRenderer> renderer_;
#endif

    //==========================================================================
    // Child Components
    //==========================================================================
    
    MiniMapComponent miniMap;
    std::unique_ptr<MacroToolbar> macroToolbar;
    std::unique_ptr<ArrangerTrackComponent> sectionTrack;
    std::vector<std::unique_ptr<ArrangerTrackComponent>> trackComponents;

    //==========================================================================
    // View State
    //==========================================================================
    
    double pixelsPerBeat = 50.0;
    double viewStartBeats = 0.0;
    int firstVisibleTrackIndex = 0;

    double gridSnapBeats = 1.0;
    GridResolution gridResolution_ = GridResolution::Beat_1;

    //==========================================================================
    // Playhead State
    //==========================================================================
    
    double playheadBeats_ = 0.0;
    bool isPlaying_ = false;
    bool followPlayhead_ = true;

    //==========================================================================
    // Loop Region State
    //==========================================================================
    
    bool loopEnabled_ = false;
    double loopStartBeats_ = 0.0;
    double loopEndBeats_ = 8.0;

    //==========================================================================
    // Drop Zone State
    //==========================================================================
    
    bool isDropTargetActive_ = false;
    int dropTargetTrackIndex_ = -1;
    double dropTargetBeats_ = 0.0;

    //==========================================================================
    // Private Methods
    //==========================================================================
    
    bool keyPressed(const juce::KeyPress& key) override;
    void updatePlayheadFromEngine();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrangerComponent)
};

} // namespace zenith
