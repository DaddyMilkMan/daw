/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "ArrangementComponent.h"


 * @file ArrangementComponent.cpp
 * @brief Arrangement view with automation lanes - Skia rendering implementation



//==============================================================================
// Constructor & Destructor
//==============================================================================

ArrangementComponent::ArrangementComponent(ProjectState& ps, Engine& eng)
    : projectState(ps), engine(eng) {
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    setWantsKeyboardFocus(true);

    projectState.getState().addListener(this);

    rebuildTrackUIState();

    startTimerHz(60);

    textFont_ = SkFont();
    textFont_.setSize(12.0f);
}

ArrangementComponent::~ArrangementComponent() {
    stopTimer();
    projectState.getState().removeListener(this);
}

//==============================================================================
// SkiaComponent Interface
//==============================================================================

void ArrangementComponent::drawSkia(SkCanvas* canvas) {
    if (!canvas) return;

    SkRect bounds = SkRect::MakeWH(static_cast<float>(getWidth()),
                                    static_cast<float>(getHeight()));

    SkPaint bgPaint;
    bgPaint.setColor(design::colors::BG_01);
    bgPaint.setAntiAlias(true);
    canvas->drawRect(bounds, bgPaint);

    SkRect headerArea = SkRect::MakeXYWH(0, 0, 
        static_cast<float>(TRACK_HEADER_WIDTH), bounds.height());
    SkRect timelineArea = SkRect::MakeXYWH(
        static_cast<float>(TRACK_HEADER_WIDTH), 0,
        bounds.width() - static_cast<float>(TRACK_HEADER_WIDTH), bounds.height());

    drawTrackHeaders(canvas, headerArea);
    drawTimeline(canvas, timelineArea);

    drawChildren(canvas);
}

void ArrangementComponent::resized() {
    SkiaComponent::resized();
    rebuildTrackUIState();
}

//==============================================================================
// Timer Callback
//==============================================================================

void ArrangementComponent::timerCallback() {
    juce::int64 playheadSamples = engine.getPlayheadSamples();
    double sampleRate = engine.getSampleRate();
    double tempo = projectState.getTempo();
    double newPlayhead = 0.0;
    if (sampleRate > 0.0 && tempo > 0.0) {
        double seconds = static_cast<double>(playheadSamples) / sampleRate;
        newPlayhead = seconds * (tempo / 60.0);
    }
    if (std::abs(newPlayhead - playheadBeats_) > 0.001) {
        playheadBeats_ = newPlayhead;
        markDirty();
    }
}

//==============================================================================
// ValueTree::Listener Interface
//==============================================================================

void ArrangementComponent::valueTreePropertyChanged(
    juce::ValueTree& tree, const juce::Identifier& property) {
    juce::ignoreUnused(tree, property);
    rebuildTrackUIState();
    markDirty();
}

void ArrangementComponent::valueTreeChildAdded(
    juce::ValueTree& parent, juce::ValueTree& child) {
    juce::ignoreUnused(parent, child);
    rebuildTrackUIState();
    markDirty();
}

void ArrangementComponent::valueTreeChildRemoved(
    juce::ValueTree& parent, juce::ValueTree& child, int index) {
    juce::ignoreUnused(parent, child, index);
    rebuildTrackUIState();
    markDirty();
}

void ArrangementComponent::valueTreeChildOrderChanged(
    juce::ValueTree& parent, int oldIndex, int newIndex) {
    juce::ignoreUnused(parent, oldIndex, newIndex);
    rebuildTrackUIState();
    markDirty();
}

void ArrangementComponent::valueTreeParentChanged(juce::ValueTree& tree) {
    juce::ignoreUnused(tree);
}

//==============================================================================
// Mouse Interaction
//==============================================================================

void ArrangementComponent::mouseDown(const juce::MouseEvent& event) {
    juce::Point<int> pos = event.getPosition();

    for (int i = 0; i < static_cast<int>(trackUIStates.size()); ++i) {
        if (isPointInAutomationLane(pos, i)) {
            juce::String paramId, pointId;
            juce::String trackId = findAutomationPointAtPosition(pos, i, paramId, pointId);

            if (pointId.isNotEmpty()) {
                auto points = getAutomationPoints(trackId, paramId);
                for (const auto& pt : points) {
                    if (pt.pointId == pointId) {
                        startDraggingPoint(trackId, paramId, pointId, pt.timeBeats, pt.value);
                        return;
                    }
                }
            } else if (!trackUIStates[i].visibleAutomationParam.isEmpty()) {
                juce::Rectangle<int> laneArea = getAutomationLaneArea(i);
                double timeBeats = pixelsToBeats(pos.x - TRACK_HEADER_WIDTH);
                double value = pixelsToAutomationValue(pos.y, laneArea,
                    trackUIStates[i].visibleAutomationParam);
                addAutomationPoint(trackUIStates[i].trackId,
                    trackUIStates[i].visibleAutomationParam, timeBeats, value);
            }
            return;
        }
    }
}

void ArrangementComponent::mouseDrag(const juce::MouseEvent& event) {
    if (!selectedPoint.isDragging) return;

    int trackIndex = -1;
    for (int i = 0; i < static_cast<int>(trackUIStates.size()); ++i) {
        if (trackUIStates[i].trackId == selectedPoint.trackId) {
            trackIndex = i;
            break;
        }
    }

    if (trackIndex < 0) return;

    juce::Rectangle<int> laneArea = getAutomationLaneArea(trackIndex);
    double newTimeBeats = snapToGrid(pixelsToBeats(event.x - TRACK_HEADER_WIDTH));
    double newValue = pixelsToAutomationValue(event.y, laneArea, selectedPoint.paramId);

    updateDraggingPoint(newTimeBeats, newValue);
    markDirty();
}

void ArrangementComponent::mouseUp(const juce::MouseEvent& event) {
    juce::ignoreUnused(event);
    if (selectedPoint.isDragging) {
        finishDraggingPoint();
    }
}

bool ArrangementComponent::keyPressed(const juce::KeyPress& key) {
    if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey) {
        deleteSelectedPoint();
        return true;
    }
    return false;
}

//==============================================================================
// Skia Drawing Helpers
//==============================================================================

void ArrangementComponent::drawTrackHeaders(SkCanvas* canvas, SkRect area) {
    SkPaint headerBgPaint;
    headerBgPaint.setColor(design::colors::BG_02);
    headerBgPaint.setAntiAlias(true);
    canvas->drawRect(area, headerBgPaint);

    SkPaint borderPaint;
    borderPaint.setColor(design::colors::BORDER_DEFAULT);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.0f);
    borderPaint.setAntiAlias(true);
    canvas->drawLine(area.fRight, area.fTop, area.fRight, area.fBottom, borderPaint);

    SkPaint textPaint;
    textPaint.setColor(design::colors::TEXT_PRIMARY);
    textPaint.setAntiAlias(true);

    auto tracksNode = projectState.getState().getChildWithName(ProjectState::ID_TRACKS);
    int numTracks = tracksNode.isValid() ? tracksNode.getNumChildren() : 0;

    for (int i = 0; i < numTracks; ++i) {
        juce::Rectangle<int> headerArea = getTrackHeaderArea(i);
        float y = static_cast<float>(headerArea.getY());
        float trackH = static_cast<float>(TRACK_HEIGHT);

        SkRect trackHeaderRect = SkRect::MakeXYWH(0, y, area.width(), trackH);

        SkPaint trackBgPaint;
        trackBgPaint.setColor((i % 2 == 0) ? design::colors::BG_02 : design::colors::BG_03);
        trackBgPaint.setAntiAlias(true);
        canvas->drawRect(trackHeaderRect, trackBgPaint);

        auto track = tracksNode.getChild(i);
        juce::String name = track.getProperty(ProjectState::PROP_NAME, "Track " + juce::String(i + 1));

        canvas->drawSimpleText(name.toRawUTF8(), name.length(), SkTextEncoding::kUTF8,
            10.0f, y + 20.0f, textFont_, textPaint);

        SkPaint separatorPaint;
        separatorPaint.setColor(design::colors::BORDER_SUBTLE);
        separatorPaint.setStrokeWidth(1.0f);
        canvas->drawLine(0, y + trackH, area.width(), y + trackH, separatorPaint);

        if (i < static_cast<int>(trackUIStates.size()) &&
            !trackUIStates[i].visibleAutomationParam.isEmpty()) {

            SkPaint automationLabelPaint;
            automationLabelPaint.setColor(getAutomationColor(trackUIStates[i].visibleAutomationParam));
            automationLabelPaint.setAntiAlias(true);

            juce::String paramLabel = trackUIStates[i].visibleAutomationParam.substring(0, 1).toUpperCase();
            canvas->drawSimpleText(paramLabel.toRawUTF8(), paramLabel.length(), SkTextEncoding::kUTF8,
                area.width() - 20.0f, y + 40.0f, textFont_, automationLabelPaint);
        }
    }
}

void ArrangementComponent::drawTimeline(SkCanvas* canvas, SkRect area) {
    SkPaint timelineBgPaint;
    timelineBgPaint.setColor(design::colors::BG_01);
    timelineBgPaint.setAntiAlias(true);
    canvas->drawRect(area, timelineBgPaint);

    drawGridLines(canvas, area);

    auto tracksNode = projectState.getState().getChildWithName(ProjectState::ID_TRACKS);
    int numTracks = tracksNode.isValid() ? tracksNode.getNumChildren() : 0;

    for (int i = 0; i < numTracks; ++i) {
        juce::Rectangle<int> trackArea = getTrackArea(i);
        SkRect skTrackArea = SkRect::MakeXYWH(
            static_cast<float>(trackArea.getX()),
            static_cast<float>(trackArea.getY()),
            static_cast<float>(trackArea.getWidth()),
            static_cast<float>(trackArea.getHeight()));
        drawTrack(canvas, i, skTrackArea);
    }

    drawPlayhead(canvas, area);
}

void ArrangementComponent::drawTrack(SkCanvas* canvas, int trackIndex, SkRect area) {
    SkPaint trackBgPaint;
    trackBgPaint.setColor((trackIndex % 2 == 0) ? design::colors::BG_01 : design::colors::BG_02);
    trackBgPaint.setAntiAlias(true);
    canvas->drawRect(area, trackBgPaint);

    if (trackIndex < static_cast<int>(trackUIStates.size()) &&
        !trackUIStates[trackIndex].visibleAutomationParam.isEmpty()) {

        juce::Rectangle<int> laneArea = getAutomationLaneArea(trackIndex);
        SkRect skLaneArea = SkRect::MakeXYWH(
            static_cast<float>(laneArea.getX()),
            static_cast<float>(laneArea.getY()),
            static_cast<float>(laneArea.getWidth()),
            static_cast<float>(laneArea.getHeight()));

        drawAutomationLane(canvas, trackUIStates[trackIndex].trackId,
            trackUIStates[trackIndex].visibleAutomationParam, skLaneArea);
    }

    SkPaint separatorPaint;
    separatorPaint.setColor(design::colors::BORDER_SUBTLE);
    separatorPaint.setStrokeWidth(1.0f);
    canvas->drawLine(area.fLeft, area.fBottom, area.fRight, area.fBottom, separatorPaint);
}

void ArrangementComponent::drawAutomationLane(SkCanvas* canvas, const juce::String& trackId,
                                               const juce::String& paramId, SkRect area) {
    SkPaint laneBgPaint;
    laneBgPaint.setColor(design::withAlpha(design::colors::BG_00, 0.5f));
    laneBgPaint.setAntiAlias(true);
    canvas->drawRect(area, laneBgPaint);

    auto points = getAutomationPoints(trackId, paramId);
    if (points.isEmpty()) return;

    SkColor lineColor = getAutomationColor(paramId);

    SkPath curvePath;
    bool first = true;

    juce::Rectangle<int> laneAreaInt(
        static_cast<int>(area.fLeft), static_cast<int>(area.fTop),
        static_cast<int>(area.width()), static_cast<int>(area.height()));

    for (const auto& pt : points) {
        float x = static_cast<float>(beatsToPixels(pt.timeBeats));
        float y = static_cast<float>(automationValueToPixels(pt.value, laneAreaInt, paramId));

        if (first) {
            curvePath.moveTo(x, y);
            first = false;
        } else {
            curvePath.lineTo(x, y);
        }
    }

    SkPaint curvePaint;
    curvePaint.setColor(lineColor);
    curvePaint.setStyle(SkPaint::kStroke_Style);
    curvePaint.setStrokeWidth(2.0f);
    curvePaint.setAntiAlias(true);
    canvas->drawPath(curvePath, curvePaint);

    SkPaint pointPaint;
    pointPaint.setColor(lineColor);
    pointPaint.setAntiAlias(true);

    SkPaint selectedPointPaint;
    selectedPointPaint.setColor(design::colors::TEXT_PRIMARY);
    selectedPointPaint.setAntiAlias(true);

    for (const auto& pt : points) {
        float x = static_cast<float>(beatsToPixels(pt.timeBeats));
        float y = static_cast<float>(automationValueToPixels(pt.value, laneAreaInt, paramId));

        bool isSelected = (selectedPoint.pointId == pt.pointId);
        float radius = isSelected ? 6.0f : 4.0f;

        canvas->drawCircle(x, y, radius, isSelected ? selectedPointPaint : pointPaint);
    }
}

void ArrangementComponent::drawGridLines(SkCanvas* canvas, SkRect area) {
    SkPaint gridPaint;
    gridPaint.setColor(design::colors::BORDER_SUBTLE);
    gridPaint.setStrokeWidth(1.0f);
    gridPaint.setAntiAlias(true);

    SkPaint beatGridPaint;
    beatGridPaint.setColor(design::colors::BORDER_DEFAULT);
    beatGridPaint.setStrokeWidth(1.0f);
    beatGridPaint.setAntiAlias(true);

    double startBeat = viewOffsetBeats;
    double endBeat = viewOffsetBeats + (area.width() / pixelsPerBeat);

    for (double beat = std::floor(startBeat); beat <= endBeat; beat += 1.0) {
        float x = static_cast<float>(beatsToPixels(beat));
        bool isMeasure = (static_cast<int>(beat) % 4 == 0);
        canvas->drawLine(x, area.fTop, x, area.fBottom, isMeasure ? beatGridPaint : gridPaint);
    }
}

void ArrangementComponent::drawPlayhead(SkCanvas* canvas, SkRect area) {
    float x = static_cast<float>(beatsToPixels(playheadBeats_));

    if (x < area.fLeft || x > area.fRight) return;

    SkPaint playheadPaint;
    playheadPaint.setColor(design::colors::PLAYHEAD);
    playheadPaint.setStrokeWidth(2.0f);
    playheadPaint.setAntiAlias(true);
    canvas->drawLine(x, area.fTop, x, area.fBottom, playheadPaint);

    SkPath trianglePath;
    trianglePath.moveTo(x - 6.0f, area.fTop);
    trianglePath.lineTo(x + 6.0f, area.fTop);
    trianglePath.lineTo(x, area.fTop + 10.0f);
    trianglePath.close();

    SkPaint trianglePaint;
    trianglePaint.setColor(design::colors::PLAYHEAD);
    trianglePaint.setAntiAlias(true);
    canvas->drawPath(trianglePath, trianglePaint);
}

//==============================================================================
// Coordinate Mapping
//==============================================================================

double ArrangementComponent::pixelsToBeats(int pixels) const {
    return viewOffsetBeats + (static_cast<double>(pixels) / pixelsPerBeat);
}

int ArrangementComponent::beatsToPixels(double beats) const {
    return TRACK_HEADER_WIDTH + static_cast<int>((beats - viewOffsetBeats) * pixelsPerBeat);
}

double ArrangementComponent::pixelsToAutomationValue(int y, juce::Rectangle<int> laneArea,
                                                      const juce::String& paramId) const {
    double normalized = 1.0 - (static_cast<double>(y - laneArea.getY()) / laneArea.getHeight());
    normalized = juce::jlimit(0.0, 1.0, normalized);

    if (paramId == "volume") {
        return normalized;
    } else if (paramId == "pan") {
        return normalized * 2.0 - 1.0;
    } else if (paramId == "mute") {
        return normalized > 0.5 ? 1.0 : 0.0;
    }
    return normalized;
}

int ArrangementComponent::automationValueToPixels(double value, juce::Rectangle<int> laneArea,
                                                   const juce::String& paramId) const {
    double normalized = 0.0;

    if (paramId == "volume") {
        normalized = value;
    } else if (paramId == "pan") {
        normalized = (value + 1.0) / 2.0;
    } else if (paramId == "mute") {
        normalized = value > 0.5 ? 1.0 : 0.0;
    } else {
        normalized = value;
    }

    normalized = juce::jlimit(0.0, 1.0, normalized);
    return laneArea.getY() + static_cast<int>((1.0 - normalized) * laneArea.getHeight());
}

//==============================================================================
// Hit Testing
//==============================================================================

bool ArrangementComponent::isPointInAutomationLane(juce::Point<int> pos, int trackIndex) const {
    juce::Rectangle<int> laneArea = getAutomationLaneArea(trackIndex);
    return laneArea.contains(pos);
}

juce::String ArrangementComponent::findAutomationPointAtPosition(
    juce::Point<int> pos, int trackIndex, juce::String& outParamId, juce::String& outPointId) {

    if (trackIndex < 0 || trackIndex >= static_cast<int>(trackUIStates.size())) {
        return {};
    }

    const auto& uiState = trackUIStates[trackIndex];
    if (uiState.visibleAutomationParam.isEmpty()) {
        return {};
    }

    juce::Rectangle<int> laneArea = getAutomationLaneArea(trackIndex);
    auto points = getAutomationPoints(uiState.trackId, uiState.visibleAutomationParam);

    for (const auto& pt : points) {
        int px = beatsToPixels(pt.timeBeats);
        int py = automationValueToPixels(pt.value, laneArea, uiState.visibleAutomationParam);

        if (std::abs(pos.x - px) < 8 && std::abs(pos.y - py) < 8) {
            outParamId = uiState.visibleAutomationParam;
            outPointId = pt.pointId;
            return uiState.trackId;
        }
    }

    return {};
}

juce::Rectangle<int> ArrangementComponent::getTrackArea(int trackIndex) const {
    int y = trackIndex * TRACK_HEIGHT;
    return juce::Rectangle<int>(TRACK_HEADER_WIDTH, y, getWidth() - TRACK_HEADER_WIDTH, TRACK_HEIGHT);
}

juce::Rectangle<int> ArrangementComponent::getAutomationLaneArea(int trackIndex) const {
    juce::Rectangle<int> trackArea = getTrackArea(trackIndex);
    int laneHeight = (TRACK_HEIGHT * AUTOMATION_LANE_HEIGHT_RATIO) / 100;
    return juce::Rectangle<int>(trackArea.getX(), trackArea.getBottom() - laneHeight,
                                 trackArea.getWidth(), laneHeight);
}

juce::Rectangle<int> ArrangementComponent::getTrackHeaderArea(int trackIndex) const {
    int y = trackIndex * TRACK_HEIGHT;
    return juce::Rectangle<int>(0, y, TRACK_HEADER_WIDTH, TRACK_HEIGHT);
}

//==============================================================================
// Automation Editing
//==============================================================================

void ArrangementComponent::addAutomationPoint(const juce::String& trackId,
                                               const juce::String& paramId,
                                               double timeBeats, double value) {
    projectState.addAutomationPoint(trackId, paramId, timeBeats, value, "Add automation point");
    markDirty();
}

void ArrangementComponent::startDraggingPoint(const juce::String& trackId,
                                               const juce::String& paramId,
                                               const juce::String& pointId,
                                               double timeBeats, double value) {
    selectedPoint.trackId = trackId;
    selectedPoint.paramId = paramId;
    selectedPoint.pointId = pointId;
    selectedPoint.originalTimeBeats = timeBeats;
    selectedPoint.originalValue = value;
    selectedPoint.isDragging = true;
    markDirty();
}

void ArrangementComponent::updateDraggingPoint(double newTimeBeats, double newValue) {
    if (!selectedPoint.isValid()) return;
    projectState.moveAutomationPoint(selectedPoint.trackId, selectedPoint.paramId,
                                      selectedPoint.pointId, newTimeBeats, newValue,
                                      "Move automation point");
}

void ArrangementComponent::finishDraggingPoint() {
    selectedPoint.isDragging = false;
}

void ArrangementComponent::deleteSelectedPoint() {
    if (!selectedPoint.isValid()) return;
    projectState.deleteAutomationPoint(selectedPoint.trackId, selectedPoint.paramId,
                                        selectedPoint.pointId, "Delete automation point");
    selectedPoint.clear();
    markDirty();
}

//==============================================================================
// Track UI State Management
//==============================================================================

void ArrangementComponent::rebuildTrackUIState() {
    auto tracksNode = projectState.getState().getChildWithName(ProjectState::ID_TRACKS);
    int numTracks = tracksNode.isValid() ? tracksNode.getNumChildren() : 0;

    if (numTracks != lastTrackCount) {
        trackUIStates.clear();
        trackUIStates.resize(numTracks);

        for (int i = 0; i < numTracks; ++i) {
            auto track = tracksNode.getChild(i);
            trackUIStates[i].trackId = track.getProperty(ProjectState::PROP_ID).toString();
            trackUIStates[i].trackIndex = i;
            trackUIStates[i].visibleAutomationParam = "volume";
        }

        lastTrackCount = numTracks;
    }
}

void ArrangementComponent::setVisibleAutomationParam(int trackIndex, const juce::String& paramId) {
    if (trackIndex >= 0 && trackIndex < static_cast<int>(trackUIStates.size())) {
        trackUIStates[trackIndex].visibleAutomationParam = paramId;
        markDirty();
    }
}

juce::String ArrangementComponent::getVisibleAutomationParam(int trackIndex) const {
    if (trackIndex >= 0 && trackIndex < static_cast<int>(trackUIStates.size())) {
        return trackUIStates[trackIndex].visibleAutomationParam;
    }
    return {};
}

//==============================================================================
// Helpers
//==============================================================================

juce::Array<AutomationPointView> ArrangementComponent::getAutomationPoints(
    const juce::String& trackId, const juce::String& paramId) const {

    juce::Array<AutomationPointView> result;

    auto automationEnvelope = projectState.getAutomationEnvelope(trackId, paramId);
    if (!automationEnvelope.isValid()) return result;

    for (int i = 0; i < automationEnvelope.getNumChildren(); ++i) {
        auto pointNode = automationEnvelope.getChild(i);
        AutomationPointView view;
        view.pointId = pointNode.getProperty(ProjectState::PROP_ID).toString();
        view.timeBeats = static_cast<double>(pointNode.getProperty(ProjectState::PROP_TIME_BEATS, 0.0));
        view.value = static_cast<double>(pointNode.getProperty(ProjectState::PROP_VALUE, 0.0));
        result.add(view);
    }

    return result;
}

SkColor ArrangementComponent::getAutomationColor(const juce::String& paramId) const {
    if (paramId == "volume") {
        return design::colors::CYAN;
    } else if (paramId == "pan") {
        return design::colors::MAGENTA;
    } else if (paramId == "mute") {
        return design::colors::AMBER;
    }
    return design::colors::AUTOMATION;
}

double ArrangementComponent::snapToGrid(double beats) const {
    double gridSize = 1.0 / GRID_SNAP_BEATS;
    return std::round(beats / gridSize) * gridSize;
}

} // namespace zenith
