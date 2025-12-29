/*
  ==============================================================================

    SessionViewComponent.h
    Created: 2025-12-12
    Author:  Zenith DAW Team
    Refactored: 2025-12-29 (Skia Integration)

    High-performance Session View using SkiaGridComponent.

  ==============================================================================
*/

#pragma once

#include "../framework/SkiaComponent.h"
#include "../../engine/Engine.h"
#include "../../engine/ProjectState.h"
#include "../session/SkiaGridComponent.h"
#include <juce_core/juce_core.h>

namespace zenith {

//==============================================================================
/**
    @class SessionViewComponent
    @brief Controller for the Session View, delegating rendering to SkiaGridComponent
*/
class SessionViewComponent : public SkiaComponent,
                             public juce::ValueTree::Listener,
                             public juce::DragAndDropTarget
{
public:
    SessionViewComponent(Engine& engine, ProjectState& state);
    ~SessionViewComponent() override;

    //==========================================================================
    // Component Overrides
    //==========================================================================
    void resized() override;
    void paint(juce::Graphics& g) override;
    void drawSkia(SkCanvas* canvas) override;
    
    //==========================================================================
    // ValueTree::Listener
    //==========================================================================
    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override;
    void valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index) override;
    void valueTreeChildOrderChanged(juce::ValueTree& parent, int oldIndex, int newIndex) override;

    //==========================================================================
    // DragAndDropTarget
    //==========================================================================
    bool isInterestedInDragSource(const juce::DragAndDropTarget::SourceDetails& details) override;
    void itemDragEnter(const juce::DragAndDropTarget::SourceDetails& details) override;
    void itemDragMove(const juce::DragAndDropTarget::SourceDetails& details) override;
    void itemDragExit(const juce::DragAndDropTarget::SourceDetails& details) override;
    void itemDropped(const juce::DragAndDropTarget::SourceDetails& details) override;
    
    //==========================================================================
    // Internal Types (Made public for access in member definitions if needed)
    //==========================================================================
    struct TrackHeader {
        juce::String trackId;
        juce::String name;
        juce::Colour trackColor;
        bool isArmed = false;
        bool isSoloed = false;
        bool isMuted = false;
        juce::Rectangle<float> bounds;
        juce::Rectangle<float> armButtonBounds;
        juce::Rectangle<float> soloButtonBounds;
        juce::Rectangle<float> muteButtonBounds;
    };

    struct MidiNoteInfo {
        int pitch;
        float startPos;
        float length;
    };

    struct ClipSlot {
        juce::String clipId;
        juce::String trackId;
        juce::String name;
        juce::Colour clipColor;
        bool hasClip = false;
        bool isPlaying = false;
        bool isQueued = false;
        bool isMidi = false;
        juce::Rectangle<float> bounds;
        std::vector<float> waveformPeaks;
        std::vector<MidiNoteInfo> midiNotes;
    };

    struct SceneRow {
        int sceneIndex;
        juce::String name;
        juce::Rectangle<float> launchButtonBounds;
    };

    enum class HoverState {
        None,
        ClipSlot,
        ClipPlayButton,
        ClipStopButton,
        TrackArm,
        TrackSolo,
        TrackMute,
        SceneLaunch
    };

private:
    // Private members start here
    //==========================================================================
    // Logic Helpers
    //==========================================================================
    void rebuildGrid();
    void syncClipData();
    SessionClipCell createCellFromClip(const juce::ValueTree& clipTree, int trackIdx, int sceneIdx);
    void buildWaveformPreview(SessionClipCell& slot, const juce::String& audioFilePath);
    void buildMidiPreview(SessionClipCell& slot, const juce::ValueTree& clipTree);

    //==========================================================================
    // Members
    //==========================================================================
    Engine& engine_;
    ProjectState& projectState_;
    
    std::unique_ptr<SkiaGridComponent> gridComponent_;
    int numScenes_ = 8;
    static constexpr float BUTTON_SIZE = 24.0f;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SessionViewComponent)
};

} // namespace zenith
