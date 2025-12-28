/**
 * @file ArrangerClipManager.h
 * @brief Clip creation, deletion, selection, and view management for ArrangerComponent
 * 
 * This module handles all clip-related operations including:
 * - Building and maintaining clip view data structures
 * - Selection state management
 * - Clip creation, deletion, and duplication
 */
#pragma once

#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_data_structures/juce_data_structures.h>

namespace zenith {

// Forward declarations
class ArrangerComponent;
class ArrangerGridUtils;
class ProjectState;

//==============================================================================
/**
 * @struct MidiNoteBlob
 * @brief Lightweight representation of a MIDI note for clip thumbnail rendering
 */
struct MidiNoteBlob {
    int pitch;              ///< MIDI note number (0-127)
    double startBeats;      ///< Start position in beats (relative to clip start)
    double lengthBeats;     ///< Duration in beats
};

//==============================================================================
/**
 * @struct ClipView
 * @brief Visual representation of a clip in the arranger
 * 
 * Contains all data needed to render and interact with a clip,
 * including cached content for efficient drawing.
 */
struct ClipView {
    juce::String clipId;              ///< Unique identifier
    juce::String trackId;             ///< Parent track ID
    int trackIndex = 0;               ///< Cached track index for O(1) lookups
    double startBeats;                ///< Start position in beats
    double lengthBeats;               ///< Duration in beats
    double fadeInBeats = 0.0;         ///< Fade in length in beats
    double fadeOutBeats = 0.0;        ///< Fade out length in beats
    bool isMidi;                      ///< True for MIDI, false for audio
    bool isSelected;                  ///< Selection state
    juce::Rectangle<float> bounds;    ///< Screen bounds (updated by recomputeClipBounds)

    // Cached content for rendering
    juce::String audioFilePath;              ///< For audio clips: source file path
    std::vector<MidiNoteBlob> noteBlobs;     ///< For MIDI clips: note data

    /**
     * @brief Check if a point is in the left resize zone
     * @param p Point to test
     * @return True if within 5 pixels of left edge
     */
    bool isInLeftResizeZone(juce::Point<float> p) const {
        return p.x >= bounds.getX() && p.x <= bounds.getX() + 5.0f;
    }

    /**
     * @brief Check if a point is in the right resize zone
     * @param p Point to test
     * @return True if within 5 pixels of right edge
     */
    bool isInRightResizeZone(juce::Point<float> p) const {
        return p.x >= bounds.getRight() - 5.0f && p.x <= bounds.getRight();
    }
};

//==============================================================================
/**
 * @class ArrangerClipManager
 * @brief Manages clip lifecycle and selection in the arranger view
 * 
 * Responsibilities:
 * - Rebuilding clip views from project state
 * - Computing clip screen bounds based on current zoom/scroll
 * - Managing selection state
 * - Creating, deleting, and duplicating clips
 */
class ArrangerClipManager {
public:
    /**
     * @brief Construct clip manager for an arranger component
     * @param owner Reference to the owning ArrangerComponent
     * @param projectState Reference to the project state
     * @param gridUtils Reference to grid utilities for coordinate conversion
     */
    ArrangerClipManager(ArrangerComponent& owner, ProjectState& projectState, ArrangerGridUtils& gridUtils);

    //==========================================================================
    // Clip View Management
    //==========================================================================
    
    /**
     * @brief Rebuild all clip views from project state
     * 
     * Iterates over all tracks and clips in the project, creating ClipView
     * entries for each. Also updates the MiniMap and rebuilds track components.
     */
    void rebuildClipViews();
    
    /**
     * @brief Recompute screen bounds for all clip views
     * 
     * Called when zoom or scroll position changes. Updates the bounds
     * rectangle for each clip based on current view state.
     */
    void recomputeClipBounds();
    
    /**
     * @brief Rebuild track components to match project state
     */
    void rebuildTrackComponents();
    
    /**
     * @brief Find a clip view by its ID
     * @param clipId The clip's unique identifier
     * @return Pointer to ClipView, or nullptr if not found
     */
    ClipView* findClipView(const juce::String& clipId);
    
    /**
     * @brief Find the topmost clip at a given screen point
     * @param point Screen coordinates to test
     * @return Pointer to ClipView, or nullptr if no clip at that point
     */
    ClipView* findClipAtPoint(juce::Point<float> point);
    
    /**
     * @brief Process clips for a single track (helper for rebuildClipViews)
     * @param track ValueTree node for the track
     * @param trackIndex Zero-based track index
     */
    void processTrackClips(const juce::ValueTree& track, int trackIndex);

    //==========================================================================
    // Selection Management
    //==========================================================================
    
    /**
     * @brief Clear all clip selections
     */
    void clearSelection();
    
    /**
     * @brief Select or toggle a clip's selection
     * @param clipId The clip to select
     * @param addToSelection If true, add to existing selection; if false, replace selection
     */
    void selectClip(const juce::String& clipId, bool addToSelection);
    
    /**
     * @brief Select all clips that intersect a rectangle
     * @param rect Screen rectangle to test
     */
    void selectClipsInRect(juce::Rectangle<float> rect);
    
    /**
     * @brief Check if a clip is currently selected
     * @param clipId The clip to check
     * @return True if selected
     */
    bool isClipSelected(const juce::String& clipId) const;

    //==========================================================================
    // Clip Operations
    //==========================================================================
    
    /**
     * @brief Create a new empty clip at a screen position
     * @param point Screen coordinates where to create the clip
     */
    void createClipAtPoint(juce::Point<float> point);
    
    /**
     * @brief Delete all selected clips
     */
    void deleteSelectedClips();
    
    /**
     * @brief Duplicate all selected clips
     * 
     * Creates copies placed immediately after the originals.
     */
    void duplicateSelectedClips();

    /**
     * @brief Split all selected clips at the current playhead position
     */
    void splitSelectedClipsAtPlayhead();

    /**
     * @brief Consolidate selected clips on a track into a single audio clip
     * 
     * Renders the selected time range of the track to a new audio file and 
     * replaces the selection with a single clip referencing that file.
     */
    void consolidateSelectedClips();

    /**
     * @brief Render selected MIDI clips to audio (Bounce in Place)
     */
    void renderSelectedClipsToAudio();

    /**
     * @brief Detect tempo from the selected audio clip
     */
    void detectTempoForSelectedClip();

    //==========================================================================
    // Accessors
    //==========================================================================
    
    /** @brief Get the array of clip views */
    juce::Array<ClipView>& getClipViews() { return clipViews_; }
    const juce::Array<ClipView>& getClipViews() const { return clipViews_; }
    
    /** @brief Get the array of selected clip IDs */
    juce::StringArray& getSelectedClipIds() { return selectedClipIds_; }
    const juce::StringArray& getSelectedClipIds() const { return selectedClipIds_; }
    
    /**
     * @brief Get the screen bounds of a clip by its ID
     * @param clipId The clip's unique identifier
     * @return Screen rectangle, or empty rectangle if not found
     */
    juce::Rectangle<float> getClipBounds(const juce::String& clipId) const {
        for (const auto& clip : clipViews_) {
            if (clip.clipId == clipId) {
                return clip.bounds;
            }
        }
        return {};
    }

private:
    ArrangerComponent& owner_;         ///< Owning component
    ProjectState& projectState_;       ///< Project state reference
    ArrangerGridUtils& gridUtils_;     ///< Grid utilities reference
    
    juce::Array<ClipView> clipViews_;           ///< All clip views
    juce::StringArray selectedClipIds_;         ///< IDs of selected clips
};

} // namespace zenith
