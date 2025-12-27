/**
 * @file ArrangerInputHandler.h
 * @brief Mouse and keyboard input handling for ArrangerComponent
 * 
 * This module handles all user input for the arranger view including:
 * - Mouse events (click, drag, wheel, double-click)
 * - Keyboard shortcuts
 * - Cursor management
 * - Tooltip generation
 */
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <map>

namespace zenith {

// Forward declarations
class ArrangerComponent;
class ArrangerClipManager;
class ArrangerGridUtils;
class ProjectState;
struct ClipView;

//==============================================================================
/**
 * @brief Drag mode enumeration for tracking current interaction state
 */
enum class DragMode {
    None,            ///< No drag in progress
    MoveClips,       ///< Dragging selected clips
    ResizeClipLeft,  ///< Resizing clip from left edge
    ResizeClipRight, ///< Resizing clip from right edge
    Marquee          ///< Marquee selection in progress
};

//==============================================================================
/**
 * @brief Edit mode enumeration for clip movement behavior
 */
enum class EditMode {
    Overwrite, ///< Default: Move clips freely, overlapping if needed
    Insert,    ///< Push content to the right to make room (Splicing)
    Ripple     ///< Push subsequent content by the exact same delta (Ripple Edit)
};

//==============================================================================
/**
 * @brief State for tracking clip positions during drag operations
 */
struct ClipDragState {
    juce::String clipId;          ///< Clip being dragged
    double originalStartBeats;    ///< Start position before drag began
    int originalTrackIndex;       ///< Track index before drag began
};

//==============================================================================
/**
 * @class ArrangerInputHandler
 * @brief Handles mouse and keyboard input for the arranger view
 * 
 * Responsibilities:
 * - Processing mouse events for clip selection, movement, and resizing
 * - Handling keyboard shortcuts (delete, duplicate, undo/redo, zoom)
 * - Managing drag state and edit modes
 * - Updating cursor based on hover position
 * - Generating tooltips for clips
 */
class ArrangerInputHandler {
public:
    /**
     * @brief Construct input handler for an arranger component
     * @param owner Reference to the owning ArrangerComponent
     * @param projectState Reference to the project state
     * @param clipManager Reference to the clip manager
     * @param gridUtils Reference to the grid utilities
     */
    ArrangerInputHandler(ArrangerComponent& owner, ProjectState& projectState,
                         ArrangerClipManager& clipManager, ArrangerGridUtils& gridUtils);

    //==========================================================================
    // Mouse Event Handlers
    //==========================================================================
    
    /**
     * @brief Handle mouse button press
     * @param e Mouse event details
     */
    void mouseDown(const juce::MouseEvent& e);
    
    /**
     * @brief Handle mouse drag during button hold
     * @param e Mouse event details
     */
    void mouseDrag(const juce::MouseEvent& e);
    
    /**
     * @brief Handle mouse button release
     * @param e Mouse event details
     */
    void mouseUp(const juce::MouseEvent& e);
    
    /**
     * @brief Handle mouse movement (for cursor updates)
     * @param e Mouse event details
     */
    void mouseMove(const juce::MouseEvent& e);
    
    /**
     * @brief Handle double-click (create clip or open editor)
     * @param e Mouse event details
     */
    void mouseDoubleClick(const juce::MouseEvent& e);
    
    /**
     * @brief Handle mouse wheel for zoom/scroll
     * @param e Mouse event details
     * @param wheel Wheel movement details
     */
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel);

    //==========================================================================
    // Keyboard Event Handlers
    //==========================================================================
    
    /**
     * @brief Handle keyboard input
     * @param key Key press details
     * @return True if key was handled
     */
    bool keyPressed(const juce::KeyPress& key);

    //==========================================================================
    // Tooltip
    //==========================================================================
    
    /**
     * @brief Get tooltip text for current mouse position
     * @return Tooltip string, or empty if no tooltip needed
     */
    juce::String getTooltip();

    /**
     * @brief Trigger AI stem separation for a clip
     * @param clipId ID of the clip to separate
     */
    void ripAudioToStems(const juce::String& clipId);


    //==========================================================================
    // State Accessors
    //==========================================================================
    
    /** @brief Get current drag mode */
    DragMode getCurrentDragMode() const { return currentDragMode_; }
    
    /** @brief Get current edit mode */
    EditMode getCurrentEditMode() const { return currentEditMode_; }
    
    /** @brief Get marquee selection rectangle */
    juce::Rectangle<float> getMarqueeRect() const { return marqueeRect_; }
    
    /** @brief Get insertion guide X position (-1 if not visible) */
    float getInsertionGuideX() const { return insertionGuideX_; }

private:
    ArrangerComponent& owner_;           ///< Owning component
    ProjectState& projectState_;         ///< Project state reference
    ArrangerClipManager& clipManager_;   ///< Clip manager reference
    ArrangerGridUtils& gridUtils_;       ///< Grid utilities reference

    // Drag state
    DragMode currentDragMode_ = DragMode::None;
    EditMode currentEditMode_ = EditMode::Overwrite;
    juce::Point<float> dragStartPoint_;
    juce::Array<ClipDragState> clipDragStates_;
    
    // Resize state
    juce::String resizingClipId_;
    double resizeOriginalStart_ = 0.0;
    double resizeOriginalLength_ = 0.0;
    
    // Marquee state
    juce::Rectangle<float> marqueeRect_;
    
    // Edit mode state
    float insertionGuideX_ = -1.0f;
    std::map<juce::String, double> initialClipStarts_;
    
    // Optimization state
    double lastDragDeltaBeats_ = -99999.0;
    int lastDragDeltaTrack_ = -99999;

    //==========================================================================
    // Helper Methods
    //==========================================================================
    
    /**
     * @brief Handle click on track header area
     * @param e Mouse event
     * @param trackIndex Index of clicked track
     */
    void handleTrackHeaderClick(const juce::MouseEvent& e, int trackIndex);
    
    /**
     * @brief Handle clip move during drag operation
     * @param e Mouse event
     */
    void handleClipMoveDrag(const juce::MouseEvent& e);
    
    /**
     * @brief Handle clip resize during drag operation
     * @param e Mouse event
     * @param isLeftEdge True for left edge, false for right edge
     */
    void handleClipResizeDrag(const juce::MouseEvent& e, bool isLeftEdge);
    
    /**
     * @brief Commit clip movement to project state
     */
    void commitClipMove();
    
    /**
     * @brief Commit clip resize to project state
     */
    void commitClipResize();
};

} // namespace zenith
