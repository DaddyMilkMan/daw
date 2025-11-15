/**
 * @file ArrangerComponent.cpp
 * @brief Arranger component implementation
 */

#include "../include/ArrangerComponent.h"
#include <cmath>

//==============================================================================
ArrangerComponent::ArrangerComponent(ProjectState& projectState, Engine& engine)
    : projectState_(projectState),
      engine_(engine)
{
    setWantsKeyboardFocus(true);

    // Listen to tempo map and markers for undo/redo support
    auto& state = projectState_.getState();
    state.addListener(this);

    DBG("ArrangerComponent: Constructor");
}

ArrangerComponent::~ArrangerComponent()
{
    auto& state = projectState_.getState();
    state.removeListener(this);

    DBG("ArrangerComponent: Destructor");
}

//==============================================================================
// Component Interface
//==============================================================================

void ArrangerComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    g.fillAll(juce::Colour(0xff1e1e1e));

    // Split into sections
    auto tempoLaneArea = bounds.removeFromTop(TEMPO_LANE_HEIGHT);
    auto markerLaneArea = bounds.removeFromTop(MARKER_LANE_HEIGHT);
    auto timeRulerArea = bounds.removeFromTop(TIME_RULER_HEIGHT);

    // Paint each section
    paintTempoLane(g, tempoLaneArea);
    paintMarkerLane(g, markerLaneArea);
    paintTimeRuler(g, timeRulerArea);

    // Paint main arranger area (placeholder for now)
    g.setColour(juce::Colour(0xff2d2d2d));
    g.fillRect(bounds);
}

void ArrangerComponent::resized()
{
    // Layout is done in paint() via removeFromTop()
}

//==============================================================================
// Coordinate Conversion
//==============================================================================

float ArrangerComponent::beatsToX(double beats) const
{
    return static_cast<float>((beats - viewOffsetBeats_) * pixelsPerBeat_);
}

double ArrangerComponent::xToBeats(float x) const
{
    return viewOffsetBeats_ + (x / pixelsPerBeat_);
}

float ArrangerComponent::bpmToY(double bpm) const
{
    // Map BPM to y within tempo lane (inverted: low BPM at bottom)
    double normalized = (bpm - MIN_TEMPO) / (MAX_TEMPO - MIN_TEMPO);
    normalized = juce::jlimit(0.0, 1.0, normalized);
    return static_cast<float>(TEMPO_LANE_HEIGHT * (1.0 - normalized));
}

double ArrangerComponent::yToBpm(float y) const
{
    // Inverse of bpmToY
    double normalized = 1.0 - (y / TEMPO_LANE_HEIGHT);
    normalized = juce::jlimit(0.0, 1.0, normalized);
    return MIN_TEMPO + normalized * (MAX_TEMPO - MIN_TEMPO);
}

double ArrangerComponent::snapBeats(double beats) const
{
    // Snap to 1/4 beat grid
    const double gridSize = 0.25;
    return std::round(beats / gridSize) * gridSize;
}

//==============================================================================
// Tempo Lane Rendering
//==============================================================================

void ArrangerComponent::paintTempoLane(juce::Graphics& g, juce::Rectangle<int> area)
{
    // Background
    g.setColour(juce::Colour(0xff252525));
    g.fillRect(area);

    // Border
    g.setColour(juce::Colour(0xff3a3a3a));
    g.drawRect(area, 1);

    // Label
    g.setColour(juce::Colours::lightgrey);
    g.setFont(12.0f);
    g.drawText("TEMPO", area.reduced(4), juce::Justification::topLeft);

    // Clip to area for rendering
    g.saveState();
    g.reduceClipRegion(area);

    // Paint tempo curve and points
    paintTempoCurve(g, area);
    paintTempoPoints(g, area);

    g.restoreState();
}

void ArrangerComponent::paintTempoCurve(juce::Graphics& g, juce::Rectangle<int> area)
{
    auto tempoPoints = projectState_.getTempoPoints();

    if (tempoPoints.size() < 2)
        return;

    // Draw tempo curve as connected line segments
    juce::Path path;
    bool firstPoint = true;

    for (const auto& pt : tempoPoints)
    {
        float x = beatsToX(pt.timeBeats);
        float y = area.getY() + bpmToY(pt.bpm);

        if (firstPoint)
        {
            path.startNewSubPath(x, y);
            firstPoint = false;
        }
        else
        {
            path.lineTo(x, y);
        }
    }

    // Extend to right edge
    if (tempoPoints.size() > 0)
    {
        const auto& lastPt = tempoPoints.getReference(tempoPoints.size() - 1);
        float lastY = area.getY() + bpmToY(lastPt.bpm);
        path.lineTo(static_cast<float>(getWidth()), lastY);
    }

    g.setColour(juce::Colour(0xff4a9eff));
    g.strokePath(path, juce::PathStrokeType(2.0f));
}

void ArrangerComponent::paintTempoPoints(juce::Graphics& g, juce::Rectangle<int> area)
{
    auto tempoPoints = projectState_.getTempoPoints();

    for (const auto& pt : tempoPoints)
    {
        float x = beatsToX(pt.timeBeats);
        float y = area.getY() + bpmToY(pt.bpm);

        // Only draw if visible
        if (x < -10.0f || x > getWidth() + 10.0f)
            continue;

        // Draw handle
        bool isSelected = (pt.id == selectedTempoPointId_);
        g.setColour(isSelected ? juce::Colours::yellow : juce::Colour(0xff4a9eff));
        g.fillEllipse(x - 5.0f, y - 5.0f, 10.0f, 10.0f);

        // Draw outline
        g.setColour(juce::Colours::white.withAlpha(0.8f));
        g.drawEllipse(x - 5.0f, y - 5.0f, 10.0f, 10.0f, 1.5f);

        // Draw BPM label
        g.setFont(10.0f);
        g.setColour(juce::Colours::white);
        juce::String label = juce::String(pt.bpm, 1) + " BPM";
        g.drawText(label, juce::Rectangle<float>(x - 30.0f, y + 8.0f, 60.0f, 14.0f),
                   juce::Justification::centred);
    }
}

//==============================================================================
// Marker Lane Rendering
//==============================================================================

void ArrangerComponent::paintMarkerLane(juce::Graphics& g, juce::Rectangle<int> area)
{
    // Background
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillRect(area);

    // Border
    g.setColour(juce::Colour(0xff3a3a3a));
    g.drawRect(area, 1);

    // Label
    g.setColour(juce::Colours::lightgrey);
    g.setFont(11.0f);
    g.drawText("MARKERS", area.reduced(4), juce::Justification::topLeft);

    // Clip to area
    g.saveState();
    g.reduceClipRegion(area);

    paintMarkers(g, area);

    g.restoreState();
}

void ArrangerComponent::paintMarkers(juce::Graphics& g, juce::Rectangle<int> area)
{
    auto markers = projectState_.getMarkers();

    for (const auto& marker : markers)
    {
        float x = beatsToX(marker.timeBeats);

        // Only draw if visible
        if (x < -100.0f || x > getWidth() + 100.0f)
            continue;

        // Parse color or use default
        juce::Colour markerColor = juce::Colours::orange;
        if (marker.color.isNotEmpty())
        {
            markerColor = juce::Colour::fromString(marker.color);
        }

        bool isSelected = (marker.id == selectedMarkerId_);

        // Draw vertical line
        g.setColour(markerColor.withAlpha(0.7f));
        g.drawLine(x, static_cast<float>(area.getY()),
                   x, static_cast<float>(area.getBottom()),
                   2.0f);

        // Draw marker flag/label
        juce::Rectangle<float> labelRect(x + 3.0f, static_cast<float>(area.getY() + 2),
                                          100.0f, static_cast<float>(area.getHeight() - 4));

        // Background for label
        if (isSelected)
            g.setColour(markerColor.brighter(0.3f).withAlpha(0.9f));
        else
            g.setColour(markerColor.withAlpha(0.8f));

        auto labelBounds = g.getCurrentFont().getStringWidth(marker.name) + 8.0f;
        labelRect = labelRect.withWidth(labelBounds);
        g.fillRoundedRectangle(labelRect, 3.0f);

        // Label text
        g.setColour(juce::Colours::white);
        g.setFont(11.0f);
        g.drawText(marker.name, labelRect, juce::Justification::centred);
    }
}

//==============================================================================
// Time Ruler Rendering
//==============================================================================

void ArrangerComponent::paintTimeRuler(juce::Graphics& g, juce::Rectangle<int> area)
{
    // Background
    g.setColour(juce::Colour(0xff202020));
    g.fillRect(area);

    // Border
    g.setColour(juce::Colour(0xff3a3a3a));
    g.drawRect(area, 1);

    // Draw beat markers
    g.setColour(juce::Colours::grey);
    g.setFont(10.0f);

    double startBeat = std::floor(viewOffsetBeats_);
    double endBeat = std::ceil(viewOffsetBeats_ + getWidth() / pixelsPerBeat_);

    for (double beat = startBeat; beat <= endBeat; beat += 1.0)
    {
        float x = beatsToX(beat);

        // Only draw if visible
        if (x < 0.0f || x > getWidth())
            continue;

        // Draw tick
        g.drawLine(x, static_cast<float>(area.getY()),
                   x, static_cast<float>(area.getY() + 5), 1.0f);

        // Draw beat number (every 4 beats = 1 bar in 4/4)
        if (static_cast<int>(beat) % 4 == 0)
        {
            int bar = static_cast<int>(beat / 4) + 1;
            g.drawText(juce::String(bar), juce::Rectangle<float>(x - 10.0f,
                       static_cast<float>(area.getY() + 5), 20.0f,
                       static_cast<float>(area.getHeight() - 5)),
                       juce::Justification::centred);
        }
    }
}

//==============================================================================
// Mouse Interaction
//==============================================================================

void ArrangerComponent::mouseDown(const juce::MouseEvent& e)
{
    float y = static_cast<float>(e.y);

    if (isInTempoLane(y))
    {
        // Check if clicking on existing tempo point
        juce::String tempoPointId = findTempoPointNear(static_cast<float>(e.x), y);

        if (tempoPointId.isNotEmpty())
        {
            // Start dragging tempo point
            dragMode_ = DragMode::TempoPoint;
            draggedItemId_ = tempoPointId;
            selectedTempoPointId_ = tempoPointId;
            selectedMarkerId_ = {};

            auto tempoPoints = projectState_.getTempoPoints();
            for (const auto& pt : tempoPoints)
            {
                if (pt.id == tempoPointId)
                {
                    dragStartBeats_ = pt.timeBeats;
                    dragStartBpm_ = pt.bpm;
                    break;
                }
            }

            dragStartPos_ = e.position;
            repaint();
        }
        else
        {
            // Clear selection
            selectedTempoPointId_ = {};
            selectedMarkerId_ = {};
            repaint();
        }
    }
    else if (isInMarkerLane(y))
    {
        // Check if clicking on existing marker
        juce::String markerId = findMarkerNear(static_cast<float>(e.x), y);

        if (markerId.isNotEmpty())
        {
            // Start dragging marker
            dragMode_ = DragMode::Marker;
            draggedItemId_ = markerId;
            selectedMarkerId_ = markerId;
            selectedTempoPointId_ = {};

            auto markers = projectState_.getMarkers();
            for (const auto& m : markers)
            {
                if (m.id == markerId)
                {
                    dragStartBeats_ = m.timeBeats;
                    break;
                }
            }

            dragStartPos_ = e.position;
            repaint();
        }
        else
        {
            // Clear selection
            selectedTempoPointId_ = {};
            selectedMarkerId_ = {};
            repaint();
        }
    }
}

void ArrangerComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (dragMode_ == DragMode::TempoPoint && draggedItemId_.isNotEmpty())
    {
        // Calculate new position
        float dx = e.position.x - dragStartPos_.x;
        float dy = e.position.y - dragStartPos_.y;

        double newBeats = dragStartBeats_ + (dx / pixelsPerBeat_);
        newBeats = snapBeats(juce::jmax(0.0, newBeats));

        double newBpm = dragStartBpm_ + ((dy / TEMPO_LANE_HEIGHT) * (MIN_TEMPO - MAX_TEMPO));
        newBpm = juce::jlimit(MIN_TEMPO, MAX_TEMPO, newBpm);

        // Get current tempo point to preserve time signature
        auto tempoPoints = projectState_.getTempoPoints();
        int timeSigNum = 4, timeSigDen = 4;
        for (const auto& pt : tempoPoints)
        {
            if (pt.id == draggedItemId_)
            {
                timeSigNum = pt.timeSigNum;
                timeSigDen = pt.timeSigDen;
                break;
            }
        }

        // Update (will trigger valueTreePropertyChanged and repaint)
        projectState_.moveTempoPoint(draggedItemId_, newBeats, newBpm,
                                     timeSigNum, timeSigDen,
                                     "Tempo: Move point");
    }
    else if (dragMode_ == DragMode::Marker && draggedItemId_.isNotEmpty())
    {
        // Calculate new position
        float dx = e.position.x - dragStartPos_.x;
        double newBeats = dragStartBeats_ + (dx / pixelsPerBeat_);
        newBeats = snapBeats(juce::jmax(0.0, newBeats));

        // Update (will trigger valueTreeChildOrderChanged and repaint)
        projectState_.moveMarker(draggedItemId_, newBeats, "Markers: Move marker");
    }
}

void ArrangerComponent::mouseUp(const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);

    // Reset drag state
    dragMode_ = DragMode::None;
    draggedItemId_ = {};
}

void ArrangerComponent::mouseDoubleClick(const juce::MouseEvent& e)
{
    float y = static_cast<float>(e.y);

    if (isInTempoLane(y))
    {
        // Add tempo point
        double beats = xToBeats(static_cast<float>(e.x));
        beats = snapBeats(juce::jmax(0.0, beats));

        double bpm = yToBpm(y);
        bpm = juce::jlimit(MIN_TEMPO, MAX_TEMPO, bpm);

        projectState_.addTempoPoint(beats, bpm, 4, 4, "Tempo: Add point");
        engine_.rebuildTempoMap();

        DBG("ArrangerComponent: Added tempo point at " + juce::String(beats) +
            " beats, " + juce::String(bpm, 1) + " BPM");
    }
    else if (isInMarkerLane(y))
    {
        // Check if double-clicking existing marker for rename
        juce::String markerId = findMarkerNear(static_cast<float>(e.x), y);

        if (markerId.isNotEmpty())
        {
            // Rename existing marker
            showMarkerRenameDialog(markerId);
        }
        else
        {
            // Add new marker
            double beats = xToBeats(static_cast<float>(e.x));
            beats = snapBeats(juce::jmax(0.0, beats));

            // Generate default name
            auto markers = projectState_.getMarkers();
            juce::String name = "Marker " + juce::String(markers.size() + 1);

            projectState_.addMarker(beats, name, "", "Markers: Add marker");

            DBG("ArrangerComponent: Added marker '" + name + "' at " +
                juce::String(beats) + " beats");
        }
    }
}

bool ArrangerComponent::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey)
    {
        // Delete selected item
        if (selectedTempoPointId_.isNotEmpty())
        {
            bool deleted = projectState_.deleteTempoPoint(selectedTempoPointId_,
                                                          "Tempo: Delete point");
            if (deleted)
            {
                selectedTempoPointId_ = {};
                engine_.rebuildTempoMap();
                repaint();
                return true;
            }
        }
        else if (selectedMarkerId_.isNotEmpty())
        {
            bool deleted = projectState_.deleteMarker(selectedMarkerId_,
                                                      "Markers: Delete marker");
            if (deleted)
            {
                selectedMarkerId_ = {};
                repaint();
                return true;
            }
        }
    }

    return false;
}

//==============================================================================
// Interaction Helpers
//==============================================================================

juce::String ArrangerComponent::findTempoPointNear(float x, float y, float tolerance)
{
    auto tempoPoints = projectState_.getTempoPoints();

    for (const auto& pt : tempoPoints)
    {
        float ptX = beatsToX(pt.timeBeats);
        float ptY = bpmToY(pt.bpm);

        float dx = x - ptX;
        float dy = y - ptY;
        float distSq = dx * dx + dy * dy;

        if (distSq < tolerance * tolerance)
            return pt.id;
    }

    return {};
}

juce::String ArrangerComponent::findMarkerNear(float x, float y, float tolerance)
{
    auto markers = projectState_.getMarkers();

    for (const auto& m : markers)
    {
        float markerX = beatsToX(m.timeBeats);

        if (std::abs(x - markerX) < tolerance)
            return m.id;
    }

    juce::ignoreUnused(y);
    return {};
}

bool ArrangerComponent::isInTempoLane(float y) const
{
    return y < TEMPO_LANE_HEIGHT;
}

bool ArrangerComponent::isInMarkerLane(float y) const
{
    return y >= TEMPO_LANE_HEIGHT && y < (TEMPO_LANE_HEIGHT + MARKER_LANE_HEIGHT);
}

void ArrangerComponent::showMarkerRenameDialog(const juce::String& markerId)
{
    auto markers = projectState_.getMarkers();
    juce::String currentName;

    for (const auto& m : markers)
    {
        if (m.id == markerId)
        {
            currentName = m.name;
            break;
        }
    }

    if (currentName.isEmpty())
        return;

    // Show input dialog
    juce::AlertWindow window("Rename Marker",
                             "Enter new name for marker:",
                             juce::AlertWindow::QuestionIcon);

    window.addTextEditor("name", currentName, "Marker name:");
    window.addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
    window.addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    if (window.runModalLoop() == 1)
    {
        juce::String newName = window.getTextEditorContents("name").trim();

        if (newName.isNotEmpty() && newName != currentName)
        {
            projectState_.renameMarker(markerId, newName, "Markers: Rename marker");
            repaint();
        }
    }
}

//==============================================================================
// ValueTree::Listener Implementation
//==============================================================================

void ArrangerComponent::valueTreePropertyChanged(juce::ValueTree& tree,
                                                  const juce::Identifier& property)
{
    juce::ignoreUnused(tree, property);

    // Repaint when tempo points or markers change
    repaint();

    // Rebuild tempo map if tempo-related change
    if (tree.hasType(ProjectState::ID_TEMPO_POINT))
    {
        engine_.rebuildTempoMap();
    }
}

void ArrangerComponent::valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child)
{
    juce::ignoreUnused(parent, child);

    // Repaint when tempo points or markers are added
    repaint();

    // Rebuild tempo map if tempo point added
    if (child.hasType(ProjectState::ID_TEMPO_POINT))
    {
        engine_.rebuildTempoMap();
    }
}

void ArrangerComponent::valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index)
{
    juce::ignoreUnused(parent, child, index);

    // Repaint when tempo points or markers are removed
    repaint();

    // Rebuild tempo map if tempo point removed
    if (child.hasType(ProjectState::ID_TEMPO_POINT))
    {
        engine_.rebuildTempoMap();
    }
}

void ArrangerComponent::valueTreeChildOrderChanged(juce::ValueTree& parent, int oldIndex, int newIndex)
{
    juce::ignoreUnused(parent, oldIndex, newIndex);

    // Repaint when order changes (e.g., marker moved)
    repaint();
}
