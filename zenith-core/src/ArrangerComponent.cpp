/**
 * @file ArrangerComponent.cpp
 * @brief Timeline/Arranger view implementation
 */

#include "../include/ArrangerComponent.h"

//==============================================================================
// Constructor / Destructor
//==============================================================================

ArrangerComponent::ArrangerComponent(ProjectState& ps)
    : projectState(ps)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    setWantsKeyboardFocus(true);

    // Listen to ProjectState changes
    projectState.getState().addListener(this);

    // Initial clip view build
    rebuildClipViews();

    DBG("ArrangerComponent: Created");
}

ArrangerComponent::~ArrangerComponent()
{
    projectState.getState().removeListener(this);
    DBG("ArrangerComponent: Destroyed");
}

//==============================================================================
// ValueTree::Listener interface
//==============================================================================

void ArrangerComponent::valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property)
{
    juce::ignoreUnused(tree, property);
    // Clip property changed (start, length, etc.)
    rebuildClipViews();
    repaint();
}

void ArrangerComponent::valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child)
{
    juce::ignoreUnused(parent, child);
    // Track or clip added
    rebuildClipViews();
    repaint();
}

void ArrangerComponent::valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index)
{
    juce::ignoreUnused(parent, child, index);
    // Track or clip removed
    rebuildClipViews();
    repaint();
}

void ArrangerComponent::valueTreeChildOrderChanged(juce::ValueTree& parent, int oldIndex, int newIndex)
{
    juce::ignoreUnused(parent, oldIndex, newIndex);
    rebuildClipViews();
    repaint();
}

//==============================================================================
// Clip view management
//==============================================================================

void ArrangerComponent::rebuildClipViews()
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    clipViews.clear();

    auto tracksNode = projectState.getState().getChildWithName(ProjectState::ID_TRACKS);
    if (!tracksNode.isValid())
        return;

    int trackIndex = 0;
    for (auto track : tracksNode)
    {
        auto trackId = track[ProjectState::PROP_ID].toString();
        auto clipsNode = track.getChildWithName(ProjectState::ID_CLIPS);

        if (clipsNode.isValid())
        {
            for (auto clip : clipsNode)
            {
                ClipView view;
                view.clipId = clip[ProjectState::PROP_ID].toString();
                view.trackId = trackId;
                view.startBeats = clip[ProjectState::PROP_START];
                view.lengthBeats = clip[ProjectState::PROP_LENGTH];

                auto clipType = clip[ProjectState::PROP_TYPE].toString();
                view.isMidi = (clipType == "midi");

                view.isSelected = selectedClipIds.contains(view.clipId);

                clipViews.add(view);
            }
        }

        trackIndex++;
    }

    recomputeClipBounds();
}

void ArrangerComponent::recomputeClipBounds()
{
    for (auto& clipView : clipViews)
    {
        // Find track index for this clip
        int trackIndex = 0;
        auto tracksNode = projectState.getState().getChildWithName(ProjectState::ID_TRACKS);
        if (tracksNode.isValid())
        {
            for (auto track : tracksNode)
            {
                if (track[ProjectState::PROP_ID].toString() == clipView.trackId)
                    break;
                trackIndex++;
            }
        }

        float x = beatsToX(clipView.startBeats);
        float y = trackIndexToY(trackIndex);
        float width = static_cast<float>(clipView.lengthBeats * pixelsPerBeat);
        float height = static_cast<float>(trackHeight - 4); // 2px margin top/bottom

        clipView.bounds = juce::Rectangle<float>(x, y + 2.0f, width, height);
    }
}

ClipView* ArrangerComponent::findClipView(const juce::String& clipId)
{
    for (auto& clipView : clipViews)
    {
        if (clipView.clipId == clipId)
            return &clipView;
    }
    return nullptr;
}

ClipView* ArrangerComponent::findClipAtPoint(juce::Point<float> point)
{
    // Search in reverse order so topmost clips are hit first
    for (int i = clipViews.size() - 1; i >= 0; --i)
    {
        if (clipViews.getReference(i).bounds.contains(point))
            return &clipViews.getReference(i);
    }
    return nullptr;
}

//==============================================================================
// Coordinate conversion
//==============================================================================

float ArrangerComponent::beatsToX(double beats) const
{
    return static_cast<float>((beats - viewStartBeats) * pixelsPerBeat);
}

double ArrangerComponent::xToBeats(float x) const
{
    return viewStartBeats + (x / pixelsPerBeat);
}

float ArrangerComponent::trackIndexToY(int trackIndex) const
{
    return rulerHeight + (trackIndex - firstVisibleTrackIndex) * trackHeight;
}

int ArrangerComponent::yToTrackIndex(float y) const
{
    if (y < rulerHeight)
        return -1;

    return firstVisibleTrackIndex + static_cast<int>((y - rulerHeight) / trackHeight);
}

double ArrangerComponent::snapToGrid(double beats) const
{
    return std::round(beats / gridSnapBeats) * gridSnapBeats;
}

//==============================================================================
// Selection management
//==============================================================================

void ArrangerComponent::clearSelection()
{
    selectedClipIds.clear();
    for (auto& clipView : clipViews)
        clipView.isSelected = false;
    repaint();
}

void ArrangerComponent::selectClip(const juce::String& clipId, bool addToSelection)
{
    if (!addToSelection)
        clearSelection();

    if (selectedClipIds.contains(clipId))
    {
        // Toggle off if adding to selection
        if (addToSelection)
        {
            selectedClipIds.removeValue(clipId);
            if (auto* view = findClipView(clipId))
                view->isSelected = false;
        }
    }
    else
    {
        selectedClipIds.add(clipId);
        if (auto* view = findClipView(clipId))
            view->isSelected = true;
    }

    repaint();
}

void ArrangerComponent::selectClipsInRect(juce::Rectangle<float> rect)
{
    for (auto& clipView : clipViews)
    {
        if (rect.intersects(clipView.bounds))
        {
            clipView.isSelected = true;
            selectedClipIds.add(clipView.clipId);
        }
    }
    repaint();
}

bool ArrangerComponent::isClipSelected(const juce::String& clipId) const
{
    return selectedClipIds.contains(clipId);
}

//==============================================================================
// Clip operations
//==============================================================================

void ArrangerComponent::createClipAtPoint(juce::Point<float> point)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    int trackIndex = yToTrackIndex(point.y);
    if (trackIndex < 0)
        return;

    // Get track at index
    auto tracksNode = projectState.getState().getChildWithName(ProjectState::ID_TRACKS);
    if (!tracksNode.isValid() || trackIndex >= tracksNode.getNumChildren())
        return;

    auto track = tracksNode.getChild(trackIndex);
    auto trackId = track[ProjectState::PROP_ID].toString();

    // Calculate clip position
    double startBeats = snapToGrid(xToBeats(point.x));
    double lengthBeats = 4.0; // Default 4 beats (1 bar in 4/4)

    // Create clip via ProjectState
    projectState.createEmptyClip(trackId, startBeats, lengthBeats, true, "Clip", "Create clip");

    DBG("ArrangerComponent: Created clip at " + juce::String(startBeats) + " beats on track " + trackId);
}

void ArrangerComponent::deleteSelectedClips()
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    if (selectedClipIds.isEmpty())
        return;

    // Begin single undo transaction for all deletes
    projectState.getUndoManager().beginNewTransaction("Delete clips");

    // Delete all selected clips
    for (const auto& clipId : selectedClipIds)
    {
        auto [track, clip] = projectState.findClip(clipId);
        if (track.isValid() && clip.isValid())
        {
            juce::String trackId = track.getProperty(ProjectState::PROP_ID).toString();
            projectState.deleteClip(trackId, clipId, "Delete clips");
        }
    }

    clearSelection();

    DBG("ArrangerComponent: Deleted " + juce::String(selectedClipIds.size()) + " clips");
}

void ArrangerComponent::duplicateSelectedClips()
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    if (selectedClipIds.isEmpty())
        return;

    // Begin single undo transaction
    projectState.getUndoManager().beginNewTransaction("Duplicate clips");

    juce::Array<juce::String> newClipIds;

    // Duplicate each selected clip
    for (const auto& clipId : selectedClipIds)
    {
        auto [track, clip] = projectState.findClip(clipId);
        if (!clip.isValid())
            continue;

        auto trackId = track[ProjectState::PROP_ID].toString();
        double startBeats = clip[ProjectState::PROP_START];
        double lengthBeats = clip[ProjectState::PROP_LENGTH];
        bool isMidi = (clip[ProjectState::PROP_TYPE].toString() == "midi");
        auto name = clip[ProjectState::PROP_NAME].toString();

        // Place duplicate after original
        double newStart = startBeats + lengthBeats;

        auto newClipId = projectState.createEmptyClip(trackId, newStart, lengthBeats, isMidi, name + " copy", "Duplicate clips");
        newClipIds.add(newClipId);
    }

    // Select the new clips
    clearSelection();
    for (const auto& newId : newClipIds)
        selectedClipIds.add(newId);

    rebuildClipViews();

    DBG("ArrangerComponent: Duplicated " + juce::String(newClipIds.size()) + " clips");
}

//==============================================================================
// Component interface - Painting
//==============================================================================

void ArrangerComponent::paint(juce::Graphics& g)
{
    paintBackground(g);
    paintTracks(g);
    paintClips(g);
    paintTimeRuler(g);
    paintMarquee(g);
}

void ArrangerComponent::paintBackground(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e1e));
}

void ArrangerComponent::paintTimeRuler(juce::Graphics& g)
{
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillRect(0, 0, getWidth(), rulerHeight);

    g.setColour(juce::Colours::white);
    g.setFont(12.0f);

    // Draw beat markers
    double startBeat = std::floor(viewStartBeats);
    double endBeat = viewStartBeats + (getWidth() / pixelsPerBeat);

    for (double beat = startBeat; beat <= endBeat; beat += 1.0)
    {
        float x = beatsToX(beat);
        if (x < 0 || x > getWidth())
            continue;

        // Draw tick
        g.setColour(gridLineColour);
        g.drawLine(x, rulerHeight - 8.0f, x, static_cast<float>(rulerHeight), 1.0f);

        // Draw beat number
        g.setColour(juce::Colours::lightgrey);
        g.drawText(juce::String(static_cast<int>(beat + 1)),
                   static_cast<int>(x - 15), 2, 30, rulerHeight - 10,
                   juce::Justification::centred, false);
    }
}

void ArrangerComponent::paintTracks(juce::Graphics& g)
{
    auto tracksNode = projectState.getState().getChildWithName(ProjectState::ID_TRACKS);
    if (!tracksNode.isValid())
        return;

    int numTracks = tracksNode.getNumChildren();

    for (int i = firstVisibleTrackIndex; i < numTracks; ++i)
    {
        float y = trackIndexToY(i);
        if (y > getHeight())
            break;

        // Alternate track colors
        g.setColour(i % 2 == 0 ? trackLaneColour : trackLaneColour.darker(0.1f));
        g.fillRect(0.0f, y, static_cast<float>(getWidth()), static_cast<float>(trackHeight));

        // Track divider
        g.setColour(trackDividerColour);
        g.drawLine(0.0f, y, static_cast<float>(getWidth()), y, 1.0f);

        // Draw vertical grid lines
        double startBeat = std::floor(viewStartBeats);
        double endBeat = viewStartBeats + (getWidth() / pixelsPerBeat);

        g.setColour(gridLineColour.withAlpha(0.3f));
        for (double beat = startBeat; beat <= endBeat; beat += 1.0)
        {
            float x = beatsToX(beat);
            if (x >= 0 && x <= getWidth())
                g.drawLine(x, y, x, y + trackHeight, 1.0f);
        }

        // Track name
        auto track = tracksNode.getChild(i);
        auto trackName = track[ProjectState::PROP_NAME].toString();

        g.setColour(juce::Colours::white.withAlpha(0.7f));
        g.setFont(14.0f);
        g.drawText(trackName, 10, static_cast<int>(y + 5), 200, 20, juce::Justification::centredLeft, false);
    }
}

void ArrangerComponent::paintClips(juce::Graphics& g)
{
    for (const auto& clipView : clipViews)
    {
        // Clip color
        juce::Colour clipColour = clipView.isMidi ? midiClipColour : audioClipColour;

        if (clipView.isSelected)
        {
            // Draw selection border
            g.setColour(selectedClipColour);
            g.drawRect(clipView.bounds, 2.0f);
            clipColour = clipColour.brighter(0.2f);
        }

        // Draw clip fill
        g.setColour(clipColour);
        g.fillRect(clipView.bounds.reduced(1.0f));

        // Draw clip name
        g.setColour(juce::Colours::black.withAlpha(0.8f));
        g.setFont(12.0f);

        auto textBounds = clipView.bounds.reduced(4.0f, 2.0f);
        if (textBounds.getWidth() > 20.0f)
        {
            // Get clip name from ProjectState
            auto [track, clip] = projectState.findClip(clipView.clipId);
            if (clip.isValid())
            {
                auto clipName = clip[ProjectState::PROP_NAME].toString();
                g.drawText(clipName, textBounds.toNearestInt(), juce::Justification::centredLeft, true);
            }
        }
    }
}

void ArrangerComponent::paintMarquee(juce::Graphics& g)
{
    if (currentDragMode == DragMode::Marquee && !marqueeRect.isEmpty())
    {
        g.setColour(juce::Colours::white.withAlpha(0.1f));
        g.fillRect(marqueeRect);

        g.setColour(juce::Colours::white.withAlpha(0.5f));
        g.drawRect(marqueeRect, 1.0f);
    }
}

void ArrangerComponent::resized()
{
    recomputeClipBounds();
}

//==============================================================================
// Component interface - Mouse handling
//==============================================================================

void ArrangerComponent::mouseDown(const juce::MouseEvent& e)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    grabKeyboardFocus();

    dragStartPoint = e.position;
    currentDragMode = DragMode::None;

    auto* clip = findClipAtPoint(e.position);

    if (clip != nullptr)
    {
        // Check for resize zones
        if (clip->isInLeftResizeZone(e.position))
        {
            currentDragMode = DragMode::ResizeClipLeft;
            resizingClipId = clip->clipId;
            resizeOriginalStart = clip->startBeats;
            resizeOriginalLength = clip->lengthBeats;
            DBG("ArrangerComponent: Start resize left");
        }
        else if (clip->isInRightResizeZone(e.position))
        {
            currentDragMode = DragMode::ResizeClipRight;
            resizingClipId = clip->clipId;
            resizeOriginalStart = clip->startBeats;
            resizeOriginalLength = clip->lengthBeats;
            DBG("ArrangerComponent: Start resize right");
        }
        else
        {
            // Move mode
            currentDragMode = DragMode::MoveClips;

            bool isCtrlOrCmd = e.mods.isCommandDown();

            // Handle selection
            if (!clip->isSelected)
            {
                selectClip(clip->clipId, isCtrlOrCmd);
            }
            else if (isCtrlOrCmd)
            {
                // Ctrl-click on selected clip = deselect
                selectClip(clip->clipId, true);
            }

            // Cache original positions for all selected clips
            clipDragStates.clear();
            for (const auto& clipId : selectedClipIds)
            {
                if (auto* view = findClipView(clipId))
                {
                    // Find track index
                    int trackIndex = 0;
                    auto tracksNode = projectState.getState().getChildWithName(ProjectState::ID_TRACKS);
                    if (tracksNode.isValid())
                    {
                        for (auto track : tracksNode)
                        {
                            if (track[ProjectState::PROP_ID].toString() == view->trackId)
                                break;
                            trackIndex++;
                        }
                    }

                    ClipDragState state;
                    state.clipId = clipId;
                    state.originalStartBeats = view->startBeats;
                    state.originalTrackIndex = trackIndex;
                    clipDragStates.add(state);
                }
            }

            DBG("ArrangerComponent: Start move " + juce::String(clipDragStates.size()) + " clips");
        }
    }
    else
    {
        // Clicked empty area
        bool isShift = e.mods.isShiftDown();

        if (isShift)
        {
            // Start marquee selection
            currentDragMode = DragMode::Marquee;
            marqueeRect = juce::Rectangle<float>(e.position, e.position);
        }
        else
        {
            // Clear selection
            clearSelection();
        }
    }
}

void ArrangerComponent::mouseDrag(const juce::MouseEvent& e)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    if (currentDragMode == DragMode::MoveClips)
    {
        // Calculate delta
        double deltaBeats = xToBeats(e.position.x) - xToBeats(dragStartPoint.x);
        int deltaTrackIndex = yToTrackIndex(e.position.y) - yToTrackIndex(dragStartPoint.y);

        // Update clip view positions for visual feedback
        for (const auto& dragState : clipDragStates)
        {
            if (auto* view = findClipView(dragState.clipId))
            {
                double newStart = dragState.originalStartBeats + deltaBeats;
                int newTrackIndex = dragState.originalTrackIndex + deltaTrackIndex;

                // Clamp
                newStart = juce::jmax(0.0, newStart);
                newTrackIndex = juce::jmax(0, newTrackIndex);

                // Update visual position
                view->startBeats = newStart;

                // Update track (if changed)
                auto tracksNode = projectState.getState().getChildWithName(ProjectState::ID_TRACKS);
                if (tracksNode.isValid() && newTrackIndex < tracksNode.getNumChildren())
                {
                    auto newTrack = tracksNode.getChild(newTrackIndex);
                    view->trackId = newTrack[ProjectState::PROP_ID].toString();
                }
            }
        }

        recomputeClipBounds();
        repaint();
    }
    else if (currentDragMode == DragMode::ResizeClipLeft)
    {
        if (auto* view = findClipView(resizingClipId))
        {
            double newStart = xToBeats(e.position.x);
            double originalEnd = resizeOriginalStart + resizeOriginalLength;
            double newLength = originalEnd - newStart;

            // Enforce minimum length
            if (newLength < 0.25)
            {
                newStart = originalEnd - 0.25;
                newLength = 0.25;
            }

            view->startBeats = newStart;
            view->lengthBeats = newLength;

            recomputeClipBounds();
            repaint();
        }
    }
    else if (currentDragMode == DragMode::ResizeClipRight)
    {
        if (auto* view = findClipView(resizingClipId))
        {
            double newEnd = xToBeats(e.position.x);
            double newLength = newEnd - resizeOriginalStart;

            // Enforce minimum length
            newLength = juce::jmax(0.25, newLength);

            view->lengthBeats = newLength;

            recomputeClipBounds();
            repaint();
        }
    }
    else if (currentDragMode == DragMode::Marquee)
    {
        marqueeRect = juce::Rectangle<float>(dragStartPoint, e.position);
        repaint();
    }
}

void ArrangerComponent::mouseUp(const juce::MouseEvent& e)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    juce::ignoreUnused(e);

    if (currentDragMode == DragMode::MoveClips)
    {
        // Commit move to ProjectState
        if (!clipDragStates.isEmpty())
        {
            projectState.getUndoManager().beginNewTransaction("Move clips");

            for (const auto& dragState : clipDragStates)
            {
                if (auto* view = findClipView(dragState.clipId))
                {
                    double snappedStart = snapToGrid(view->startBeats);
                    projectState.moveClip(dragState.clipId, view->trackId, snappedStart, "Move clips");
                }
            }

            DBG("ArrangerComponent: Committed move for " + juce::String(clipDragStates.size()) + " clips");
        }

        clipDragStates.clear();
    }
    else if (currentDragMode == DragMode::ResizeClipLeft || currentDragMode == DragMode::ResizeClipRight)
    {
        // Commit resize to ProjectState
        if (auto* view = findClipView(resizingClipId))
        {
            double snappedStart = snapToGrid(view->startBeats);
            double snappedLength = snapToGrid(view->lengthBeats);

            projectState.setClipRange(resizingClipId, snappedStart, snappedLength, "Resize clip");

            DBG("ArrangerComponent: Committed resize for clip " + resizingClipId);
        }

        resizingClipId.clear();
    }
    else if (currentDragMode == DragMode::Marquee)
    {
        // Select clips in marquee
        selectClipsInRect(marqueeRect);
        marqueeRect = juce::Rectangle<float>();
    }

    currentDragMode = DragMode::None;
    repaint();
}

void ArrangerComponent::mouseMove(const juce::MouseEvent& e)
{
    // Update cursor based on hover position
    auto* clip = findClipAtPoint(e.position);

    if (clip != nullptr)
    {
        if (clip->isInLeftResizeZone(e.position) || clip->isInRightResizeZone(e.position))
            setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
        else
            setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    }
    else
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }
}

void ArrangerComponent::mouseDoubleClick(const juce::MouseEvent& e)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    auto* clip = findClipAtPoint(e.position);

    if (clip == nullptr)
    {
        // Double-clicked empty area - create clip
        createClipAtPoint(e.position);
    }
    // else: could open piano roll in future
}

void ArrangerComponent::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    bool isShift = e.mods.isShiftDown();
    bool isCtrlOrCmd = e.mods.isCommandDown();

    if (isCtrlOrCmd)
    {
        // Zoom horizontal
        double zoomFactor = 1.0 + (wheel.deltaY * 0.5);
        double oldPixelsPerBeat = pixelsPerBeat;
        pixelsPerBeat *= zoomFactor;
        pixelsPerBeat = juce::jlimit(10.0, 200.0, pixelsPerBeat);

        // Zoom around mouse position
        double beatsAtMouse = xToBeats(e.position.x);
        double pixelsAtMouse = e.position.x;
        viewStartBeats = beatsAtMouse - (pixelsAtMouse / pixelsPerBeat);
        viewStartBeats = juce::jmax(0.0, viewStartBeats);

        recomputeClipBounds();
        repaint();
    }
    else if (isShift)
    {
        // Scroll horizontal
        viewStartBeats -= wheel.deltaY * 2.0;
        viewStartBeats = juce::jmax(0.0, viewStartBeats);

        recomputeClipBounds();
        repaint();
    }
    else
    {
        // Scroll vertical
        firstVisibleTrackIndex -= static_cast<int>(wheel.deltaY * 2.0);

        auto tracksNode = projectState.getState().getChildWithName(ProjectState::ID_TRACKS);
        int maxTrackIndex = tracksNode.isValid() ? tracksNode.getNumChildren() - 1 : 0;
        firstVisibleTrackIndex = juce::jlimit(0, maxTrackIndex, firstVisibleTrackIndex);

        recomputeClipBounds();
        repaint();
    }
}

//==============================================================================
// Component interface - Keyboard handling
//==============================================================================

bool ArrangerComponent::keyPressed(const juce::KeyPress& key)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    // Delete / Backspace
    if (key.isKeyCode(juce::KeyPress::deleteKey) || key.isKeyCode(juce::KeyPress::backspaceKey))
    {
        deleteSelectedClips();
        return true;
    }

    // Ctrl/Cmd+D - Duplicate
    if (key.getModifiers().isCommandDown() && key.getKeyCode() == 'D')
    {
        duplicateSelectedClips();
        return true;
    }

    // Ctrl/Cmd+Z - Undo
    if (key.getModifiers().isCommandDown() && key.getKeyCode() == 'Z')
    {
        projectState.undo();
        return true;
    }

    // Ctrl/Cmd+Shift+Z or Ctrl/Cmd+Y - Redo
    if ((key.getModifiers().isCommandDown() && key.getModifiers().isShiftDown() && key.getKeyCode() == 'Z') ||
        (key.getModifiers().isCommandDown() && key.getKeyCode() == 'Y'))
    {
        projectState.redo();
        return true;
    }

    // + key - Zoom in
    if (key.getKeyCode() == '+' || key.getKeyCode() == '=')
    {
        pixelsPerBeat *= 1.2;
        pixelsPerBeat = juce::jmin(200.0, pixelsPerBeat);
        recomputeClipBounds();
        repaint();
        return true;
    }

    // - key - Zoom out
    if (key.getKeyCode() == '-')
    {
        pixelsPerBeat /= 1.2;
        pixelsPerBeat = juce::jmax(10.0, pixelsPerBeat);
        recomputeClipBounds();
        repaint();
        return true;
    }

    // Escape - Clear selection
    if (key.isKeyCode(juce::KeyPress::escapeKey))
    {
        clearSelection();
        return true;
    }

    return false;
}
