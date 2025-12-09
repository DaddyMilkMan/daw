/*
  ==============================================================================

    SessionViewComponent.h
    Created: 2025-12-08
    Author:  Zenith DAW

    Non-linear clip launcher grid (Session View).
    Columns = Tracks, Rows = Scenes.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "skia/SkiaComponent.h"
#include "../engine/ProjectState.h"
#include "../engine/Engine.h"

namespace zenith {

class SessionViewComponent : public SkiaComponent,
                             public juce::ValueTree::Listener
{
public:
    SessionViewComponent(ProjectState& state, Engine& engine);
    ~SessionViewComponent() override;

    void drawSkia(SkCanvas* canvas) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    // ValueTree::Listener
    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override;
    void valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index) override;

private:
    ProjectState& projectState_;
    Engine& engine_;

    // Layout constants
    static constexpr float COLUMN_WIDTH = 100.0f;
    static constexpr float ROW_HEIGHT = 60.0f;
    static constexpr float SCENE_HEADER_WIDTH = 60.0f;
    static constexpr float GAP = 4.0f;

    // State
    int numScenes_ = 8; // Default number of rows
    
    // Helper to get clip at specific slot
    juce::ValueTree getClipAt(int trackIndex, int sceneIndex);
    
    // Interactions
    void triggerClip(int trackIndex, int sceneIndex);
    void triggerScene(int sceneIndex);
    void stopTrack(int trackIndex);

    // Drawing helpers
    void drawGrid(SkCanvas* canvas);
    void drawSlot(SkCanvas* canvas, const SkRect& rect, int trackIndex, int sceneIndex);
    void drawSceneHeader(SkCanvas* canvas, const SkRect& rect, int sceneIndex);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SessionViewComponent)
};

} // namespace zenith
