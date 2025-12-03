#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_events/juce_events.h>
#include "ProjectState.h"
#include "../../Source/ui/skia/SkiaComponent.h"

#include <core/SkCanvas.h>

class ArrangerComponent : public zenith::SkiaComponent,
                          public juce::ValueTree::Listener
{
public:
    ArrangerComponent(ProjectState& ps);
    ~ArrangerComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

    // ValueTree::Listener
    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override;
    void valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index) override;
    void valueTreeChildOrderChanged(juce::ValueTree& parent, int oldIndex, int newIndex) override;

    void drawSkia(SkCanvas* canvas) override;

    juce::String getTooltip();

private:
    ProjectState& projectState;

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

    void paintBackground(juce::Graphics& g);
    void paintTimeRuler(juce::Graphics& g);
    void paintTracks(juce::Graphics& g);
    void paintClips(juce::Graphics& g);
    void paintMarquee(juce::Graphics& g);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrangerComponent)
};
