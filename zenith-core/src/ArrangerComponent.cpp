/**
 * @file ArrangerComponent.cpp
 * @brief Arranger implementation
 */

#include "../include/ArrangerComponent.h"

//==============================================================================
ArrangerComponent::ArrangerComponent(ProjectState& state)
    : projectState(state)
{
    // Listen to ProjectState root for track/clip changes
    projectState.getState().addListener(this);

    refreshClipsFromProjectState();
}

ArrangerComponent::~ArrangerComponent()
{
    projectState.getState().removeListener(this);
}

//==============================================================================
// Data Management
//==============================================================================

void ArrangerComponent::refreshClipsFromProjectState()
{
    clipViews.clear();

    auto tracksNode = projectState.getState().getChildWithName(ProjectState::ID_TRACKS);
    if (!tracksNode.isValid())
        return;

    int trackIndex = 0;
    for (auto track : tracksNode)
    {
        if (!track.hasType(ProjectState::ID_TRACK))
            continue;

        juce::String trackId = track[ProjectState::PROP_ID].toString();

        auto clipsNode = track.getChildWithName(ProjectState::ID_CLIPS);
        if (!clipsNode.isValid())
        {
            ++trackIndex;
            continue;
        }

        for (auto clipTree : clipsNode)
        {
            if (!clipTree.hasType(ProjectState::ID_CLIP))
                continue;

            ClipView clip;
            clip.clipId = clipTree[ProjectState::PROP_ID].toString();
            clip.trackId = trackId;
            clip.name = clipTree.getProperty(ProjectState::PROP_NAME, "Untitled Clip").toString();
            clip.type = clipTree.getProperty(ProjectState::PROP_TYPE, "midi").toString();
            clip.startBeats = clipTree.getProperty(ProjectState::PROP_START, 0.0);
            clip.lengthBeats = clipTree.getProperty(ProjectState::PROP_LENGTH, 4.0);
            clip.trackIndex = trackIndex;

            clipViews.push_back(clip);
        }

        ++trackIndex;
    }

    updateClipRectangles();
    repaint();
}

void ArrangerComponent::updateClipRectangles()
{
    for (auto& clip : clipViews)
    {
        float x = beatsToPixels(clip.startBeats);
        float y = trackToPixels(clip.trackIndex);
        float width = beatsToPixels(clip.lengthBeats);
        float height = static_cast<float>(trackHeight - 4);

        clip.bounds = juce::Rectangle<float>(x, y + 2, width, height);
    }
}

//==============================================================================
// Coordinate Conversion
//==============================================================================

double ArrangerComponent::pixelsToBeats(float x) const
{
    return (x + scrollOffsetX) / pixelsPerBeat;
}

float ArrangerComponent::beatsToPixels(double beats) const
{
    return static_cast<float>(beats * pixelsPerBeat - scrollOffsetX);
}

int ArrangerComponent::pixelsToTrack(float y) const
{
    return static_cast<int>((y + scrollOffsetY) / trackHeight);
}

float ArrangerComponent::trackToPixels(int trackIndex) const
{
    return trackIndex * trackHeight - scrollOffsetY;
}

//==============================================================================
// Interaction
//==============================================================================

ArrangerComponent::ClipView* ArrangerComponent::findClipAtPosition(float x, float y)
{
    for (auto& clip : clipViews)
    {
        if (clip.bounds.contains(x, y))
            return &clip;
    }
    return nullptr;
}

void ArrangerComponent::mouseDown(const juce::MouseEvent& e)
{
    auto* clip = findClipAtPosition(static_cast<float>(e.x), static_cast<float>(e.y));

    if (clip != nullptr)
    {
        DBG("Clicked clip: " + clip->clipId + " (" + clip->type + ")");
    }
}

void ArrangerComponent::mouseDoubleClick(const juce::MouseEvent& e)
{
    auto* clip = findClipAtPosition(static_cast<float>(e.x), static_cast<float>(e.y));

    if (clip != nullptr && clip->type == "midi")
    {
        openPianoRollForClip(*clip);
    }
}

void ArrangerComponent::openPianoRollForClip(const ClipView& clip)
{
    DBG("Opening Piano Roll for clip: " + clip.clipId);

    // Create PianoRollComponent if needed
    if (pianoRoll == nullptr)
    {
        pianoRoll = std::make_unique<PianoRollComponent>(projectState);
    }

    // Set clip context
    MidiClipContext context;
    context.clipId = clip.clipId;
    context.trackId = clip.trackId;
    context.clipStartBeats = clip.startBeats;
    context.clipLengthBeats = clip.lengthBeats;
    context.clipName = clip.name;

    pianoRoll->setClipContext(context);

    // Create/show modal window
    if (pianoRollWindow == nullptr)
    {
        pianoRollWindow = std::make_unique<juce::DocumentWindow>(
            "Piano Roll",
            juce::Colours::darkgrey,
            juce::DocumentWindow::allButtons);

        pianoRollWindow->setContentNonOwned(pianoRoll.get(), true);
        pianoRollWindow->setResizable(true, false);
        pianoRollWindow->setUsingNativeTitleBar(true);
        pianoRollWindow->centreWithSize(1000, 600);
        pianoRollWindow->setVisible(true);
    }
    else
    {
        pianoRollWindow->setName("Piano Roll - " + clip.name);
        pianoRollWindow->toFront(true);
    }
}

//==============================================================================
// ValueTree Listener (auto-refresh when clips change)
//==============================================================================

void ArrangerComponent::valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child)
{
    if (parent.hasType(ProjectState::ID_TRACKS) || parent.hasType(ProjectState::ID_CLIPS))
    {
        refreshClipsFromProjectState();
    }
}

void ArrangerComponent::valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index)
{
    if (parent.hasType(ProjectState::ID_TRACKS) || parent.hasType(ProjectState::ID_CLIPS))
    {
        refreshClipsFromProjectState();
    }
}

void ArrangerComponent::valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property)
{
    if (tree.hasType(ProjectState::ID_CLIP))
    {
        refreshClipsFromProjectState();
    }
}

//==============================================================================
// Rendering
//==============================================================================

void ArrangerComponent::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(juce::Colour(0xff1e1e1e));

    auto bounds = getLocalBounds();

    // Draw track backgrounds (alternating colors)
    int numTracks = projectState.getNumTracks();
    for (int i = 0; i < numTracks; ++i)
    {
        float y = trackToPixels(i);
        juce::Colour trackColor = (i % 2 == 0) ? juce::Colour(0xff2a2a2a) : juce::Colour(0xff252525);

        g.setColour(trackColor);
        g.fillRect(0.0f, y, static_cast<float>(bounds.getWidth()), static_cast<float>(trackHeight));
    }

    // Draw beat grid lines
    g.setColour(juce::Colour(0xff303030));
    for (double beat = 0.0; beat < 64.0; beat += 1.0)  // Show 64 beats
    {
        float x = beatsToPixels(beat);
        g.drawVerticalLine(static_cast<int>(x), 0.0f, static_cast<float>(bounds.getHeight()));
    }

    // Draw clips
    for (const auto& clip : clipViews)
    {
        if (clip.type == "midi")
        {
            g.setColour(juce::Colours::lightblue);
        }
        else
        {
            g.setColour(juce::Colours::lightgreen);
        }

        g.fillRect(clip.bounds);

        // Clip border
        g.setColour(juce::Colours::white.withAlpha(0.3f));
        g.drawRect(clip.bounds, 1.0f);

        // Clip name
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(12.0f));
        g.drawText(clip.name,
                   clip.bounds.reduced(4.0f),
                   juce::Justification::centredLeft,
                   true);
    }

    // Instructions
    if (clipViews.empty())
    {
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(16.0f));
        g.drawText("No clips yet. Use Wingman to create MIDI clips.",
                   bounds,
                   juce::Justification::centred);
    }
    else
    {
        g.setColour(juce::Colours::lightgrey);
        g.setFont(juce::Font(14.0f));
        g.drawText("Double-click MIDI clip to open Piano Roll",
                   bounds.removeFromBottom(30).reduced(10, 5),
                   juce::Justification::centredLeft);
    }
}

void ArrangerComponent::resized()
{
    updateClipRectangles();
}
