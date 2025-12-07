#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_events/juce_events.h>
#include "../ProjectState.h"
#include "../../Source/ui/skia/SkiaComponent.h"
#include "../Engine.h"

#include <core/SkCanvas.h>

// Forward declaration for browser drag
namespace zenith { class BrowserDragData; }


namespace zenith {

//==============================================================================
// Grid resolution options for snapping
enum class GridResolution {
    Bar_1 = 0,      // 4 beats (in 4/4)
    Beat_1,         // 1 beat (quarter note)
    Beat_1_2,       // 1/2 beat (eighth note)
    Beat_1_4,       // 1/4 beat (sixteenth note)
    Beat_1_8,       // 1/8 beat (thirty-second)
    Beat_1_3,       // 1/3 beat (triplet eighth)
    Beat_1_6,       // 1/6 beat (triplet sixteenth)
    Off             // No snap
};

// Convert grid resolution to beat value
inline double gridResolutionToBeats(GridResolution res) {
    switch (res) {
        case GridResolution::Bar_1:    return 4.0;
        case GridResolution::Beat_1:   return 1.0;
        case GridResolution::Beat_1_2: return 0.5;
        case GridResolution::Beat_1_4: return 0.25;
        case GridResolution::Beat_1_8: return 0.125;
        case GridResolution::Beat_1_3: return 1.0 / 3.0;
        case GridResolution::Beat_1_6: return 1.0 / 6.0;
        case GridResolution::Off:      return 0.0;
        default:                       return 1.0;
    }
}

class ArrangerComponent : public SkiaComponent,
                          public juce::ValueTree::Listener,
                          public juce::DragAndDropTarget
{
public:
    ArrangerComponent(Engine& engine, ProjectState& ps);
    ~ArrangerComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

    std::function<void(const juce::String& trackId, const juce::String& clipId)> onClipDoubleClicked;

    // ValueTree::Listener
    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override;
    void valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index) override;
    void valueTreeChildOrderChanged(juce::ValueTree& parent, int oldIndex, int newIndex) override;

    void drawSkia(SkCanvas* canvas) override;

    juce::String getTooltip();
    
    // DragAndDropTarget interface
    bool isInterestedInDragSource(const juce::DragAndDropTarget::SourceDetails& details) override;
    void itemDropped(const juce::DragAndDropTarget::SourceDetails& details) override;
    void itemDragEnter(const juce::DragAndDropTarget::SourceDetails& details) override;
    void itemDragExit(const juce::DragAndDropTarget::SourceDetails& details) override;
    void itemDragMove(const juce::DragAndDropTarget::SourceDetails& details) override;

    // Grid resolution control
    void setGridResolution(GridResolution res);
    GridResolution getGridResolution() const { return gridResolution_; }

    // Timer callback for playhead updates
    void timerCallback() override;

private:
    Engine& engine_;
    zenith::ProjectState& projectState;

    struct ClipView {
        juce::String clipId;
        juce::String trackId;
        double startBeats;
        double lengthBeats;
        bool isMidi;
        bool isSelected;
        juce::Rectangle<float> bounds;

        bool isInLeftResizeZone(juce::Point<float> p) const {
            return p.x >= bounds.getX() && p.x <= bounds.getX() + 5.0f;
        }

        bool isInRightResizeZone(juce::Point<float> p) const {
            return p.x >= bounds.getRight() - 5.0f && p.x <= bounds.getRight();
        }
    };

    bool keyPressed(const juce::KeyPress& key) override;

    juce::Array<ClipView> clipViews;
    juce::StringArray selectedClipIds;

    // View state
    double pixelsPerBeat = 50.0;
    double viewStartBeats = 0.0;
    int firstVisibleTrackIndex = 0;
    float trackHeight = 60.0f;
    float rulerHeight = 30.0f;
    double gridSnapBeats = 1.0;
    GridResolution gridResolution_ = GridResolution::Beat_1;

    // Playhead state (updated from Engine via timer)
    double playheadBeats_ = 0.0;
    bool isPlaying_ = false;
    bool followPlayhead_ = true;  // Auto-scroll to follow playhead

    // Loop region state
    bool loopEnabled_ = false;
    double loopStartBeats_ = 0.0;
    double loopEndBeats_ = 8.0;

    // Drag state
    enum class DragMode {
        None,
        MoveClips,
        ResizeClipLeft,
        ResizeClipRight,
        Marquee
    };
    DragMode currentDragMode = DragMode::None;
    juce::Point<float> dragStartPoint;
    
    struct ClipDragState {
        juce::String clipId;
        double originalStartBeats;
        int originalTrackIndex;
    };
    juce::Array<ClipDragState> clipDragStates;
    
    juce::String resizingClipId;
    double resizeOriginalStart = 0.0;
    double resizeOriginalLength = 0.0;
    
    juce::Rectangle<float> marqueeRect;
    
    // Drop zone state (for browser drag-and-drop)
    bool isDropTargetActive_ = false;
    int dropTargetTrackIndex_ = -1;
    double dropTargetBeats_ = 0.0;

    // Methods
    void rebuildClipViews();
    void recomputeClipBounds();
    ClipView* findClipView(const juce::String& clipId);
    ClipView* findClipAtPoint(juce::Point<float> point);
    
    float beatsToX(double beats) const;
    double xToBeats(float x) const;
    float trackIndexToY(int trackIndex) const;
    int yToTrackIndex(float y) const;
    double snapToGrid(double beats) const;

    void clearSelection();
    void selectClip(const juce::String& clipId, bool addToSelection);
    void selectClipsInRect(juce::Rectangle<float> rect);
    bool isClipSelected(const juce::String& clipId) const;

    void createClipAtPoint(juce::Point<float> point);
    void deleteSelectedClips();
    void duplicateSelectedClips();

    // Utility
    void updatePlayheadFromEngine();
    double samplesToBeats(juce::int64 samples) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrangerComponent)
};

} // namespace zenith
