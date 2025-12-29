/*
  ==============================================================================

    SessionViewComponent.cpp
    Created: 2025-12-12
    Author:  Zenith DAW Team
    Refactored: 2025-12-29 (Skia Integration)

  ==============================================================================
*/

#include "SessionViewComponent.h"
#include "../design-system/ZenithDesignSystem.h"

namespace zenith {

//==============================================================================
// Construction
//==============================================================================
SessionViewComponent::SessionViewComponent(Engine& engine, ProjectState& state)
    : engine_(engine), projectState_(state)
{
    setOpaque(true);
    
    // Create the high-performance grid renderer
    gridComponent_ = std::make_unique<SkiaGridComponent>(engine, state);
    addAndMakeVisible(gridComponent_.get());
    
    // Listen to state changes
    projectState_.getState().addListener(this);
    
    // Initial build
    rebuildGrid();
}

SessionViewComponent::~SessionViewComponent()
{
    projectState_.getState().removeListener(this);
}

//==============================================================================
// Component Overrides
//==============================================================================
void SessionViewComponent::resized()
{
    if (gridComponent_)
    {
        gridComponent_->setBounds(getLocalBounds());
    }
}

void SessionViewComponent::paint(juce::Graphics& g)
{
    // SkiaComponent handles paint redirection to drawSkia
    SkiaComponent::paint(g);
}

void SessionViewComponent::drawSkia(SkCanvas* canvas)
{
    if (gridComponent_)
    {
        drawChildren(canvas);
    }
}

//==============================================================================
// Logic Helpers
//==============================================================================
void SessionViewComponent::rebuildGrid()
{
    if (!gridComponent_) return;
    
    auto tracksTree = projectState_.getState().getChildWithName(ProjectState::ID_TRACKS);
    int numTracks = tracksTree.getNumChildren();
    
    gridComponent_->setGridSize(numTracks, numScenes_);
    
    syncClipData();
}

void SessionViewComponent::syncClipData()
{
    if (!gridComponent_) return;
    
    auto tracksTree = projectState_.getState().getChildWithName(ProjectState::ID_TRACKS);
    
    for (int t = 0; t < tracksTree.getNumChildren(); ++t)
    {
        auto trackTree = tracksTree.getChild(t);
        auto clipsTree = trackTree.getChildWithName(ProjectState::ID_CLIPS);
        
        for (int c = 0; c < clipsTree.getNumChildren(); ++c)
        {
            auto clipTree = clipsTree.getChild(c);
            
            double start = clipTree.getProperty(ProjectState::PROP_START);
            int sceneIdx = static_cast<int>(start / 4.0); // Simple quantization for now
            
            if (sceneIdx >= 0 && sceneIdx < numScenes_)
            {
                SessionClipCell cell = createCellFromClip(clipTree, t, sceneIdx);
                gridComponent_->setClipData(t, sceneIdx, cell);
            }
        }
    }
}

SessionClipCell SessionViewComponent::createCellFromClip(const juce::ValueTree& clipTree, int trackIdx, int sceneIdx)
{
    SessionClipCell cell;
    cell.trackIndex = trackIdx;
    cell.sceneIndex = sceneIdx;
    cell.clipId = clipTree.getProperty(ProjectState::PROP_ID).toString();
    cell.hasClip = true;
    
    // Properties
    cell.isAudio = (clipTree.getProperty(ProjectState::PROP_TYPE).toString() != "midi");
    // FIXED: Use PROP_VOLUME instead of PROP_GAIN
    cell.gainDb = clipTree.getProperty(ProjectState::PROP_VOLUME, 0.0f);
    
    // Name
    juce::String name = clipTree.getProperty(ProjectState::PROP_NAME);
    
    // Visuals (generate dummy waveform for now if not available)
    if (cell.isAudio) {
        // Mock waveform
        for (int i = 0; i < 20; ++i) {
            cell.waveformPeaks.push_back(juce::Random::getSystemRandom().nextFloat());
        }
    }
    
    return cell;
}

//==============================================================================
// ValueTree::Listener
//==============================================================================
void SessionViewComponent::valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property)
{
    // FIXED: Use ID_CLIP instead of ID_CIP
    if (tree.hasType(ProjectState::ID_CLIP)) 
    {
        rebuildGrid(); 
    }
    else if (tree.hasType(ProjectState::ID_TRACK))
    {
         rebuildGrid();
    }
}

void SessionViewComponent::valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child)
{
    rebuildGrid();
}

void SessionViewComponent::valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index)
{
    rebuildGrid();
}

void SessionViewComponent::valueTreeChildOrderChanged(juce::ValueTree& parent, int oldIndex, int newIndex)
{
    rebuildGrid();
}

//==============================================================================
// DragAndDropTarget
//==============================================================================
bool SessionViewComponent::isInterestedInDragSource(const juce::DragAndDropTarget::SourceDetails& details)
{
    return details.description.toString().startsWith("clip:") ||
           details.description.toString().startsWith("sample:");
}

void SessionViewComponent::itemDragEnter(const juce::DragAndDropTarget::SourceDetails& details)
{
}

void SessionViewComponent::itemDragMove(const juce::DragAndDropTarget::SourceDetails& details)
{
}

void SessionViewComponent::itemDragExit(const juce::DragAndDropTarget::SourceDetails& details)
{
}

void SessionViewComponent::itemDropped(const juce::DragAndDropTarget::SourceDetails& details)
{
    DBG("SessionView drop: " << details.description.toString());
}

} // namespace zenith
