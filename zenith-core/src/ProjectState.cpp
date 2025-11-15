/**
 * @file ProjectState.cpp
 * @brief Project state implementation
 */

#include "../include/ProjectState.h"

#include <functional>

namespace
{
//==============================================================================
int extractNumericSuffix(const juce::String& identifier)
{
    auto trimmed = identifier.trim();

    if (trimmed.isEmpty())
        return -1;

    const int lastUnderscore = trimmed.lastIndexOfChar('_');

    if (lastUnderscore < 0)
        return -1;

    auto numericPart = trimmed.substring(lastUnderscore + 1).trim();

    if (numericPart.isEmpty() || !numericPart.containsOnly("0123456789"))
        return -1;

    return numericPart.getIntValue();
}
} // namespace

//==============================================================================
// Static identifier definitions
//==============================================================================

const juce::Identifier ProjectState::ID_PROJECT("PROJECT");
const juce::Identifier ProjectState::ID_TRACKS("TRACKS");
const juce::Identifier ProjectState::ID_TRACK("TRACK");
const juce::Identifier ProjectState::ID_CLIPS("CLIPS");
const juce::Identifier ProjectState::ID_CLIP("CLIP");
const juce::Identifier ProjectState::ID_MIXER("MIXER");
const juce::Identifier ProjectState::ID_AUTOMATION("AUTOMATION");
const juce::Identifier ProjectState::ID_ENVELOPE("ENVELOPE");
const juce::Identifier ProjectState::ID_POINT("POINT");

// Phase 15: Tempo Map + Markers
const juce::Identifier ProjectState::ID_TEMPO_MAP("TEMPO_MAP");
const juce::Identifier ProjectState::ID_TEMPO_POINT("TEMPO_POINT");
const juce::Identifier ProjectState::ID_MARKERS("MARKERS");
const juce::Identifier ProjectState::ID_MARKER("MARKER");

const juce::Identifier ProjectState::PROP_NAME("name");
const juce::Identifier ProjectState::PROP_TEMPO("tempo");
const juce::Identifier ProjectState::PROP_TIME_SIG_NUM("timeSignatureNumerator");
const juce::Identifier ProjectState::PROP_TIME_SIG_DEN("timeSignatureDenominator");
const juce::Identifier ProjectState::PROP_SAMPLE_RATE("sampleRate");

const juce::Identifier ProjectState::PROP_ID("id");
const juce::Identifier ProjectState::PROP_TYPE("type");
const juce::Identifier ProjectState::PROP_VOLUME("volume");
const juce::Identifier ProjectState::PROP_PAN("pan");
const juce::Identifier ProjectState::PROP_MUTE("mute");
const juce::Identifier ProjectState::PROP_SOLO("solo");

const juce::Identifier ProjectState::PROP_START("start");
const juce::Identifier ProjectState::PROP_LENGTH("length");

// Phase 13: Automation properties
const juce::Identifier ProjectState::PROP_PARAM("param");
const juce::Identifier ProjectState::PROP_TIME_BEATS("timeBeats");
const juce::Identifier ProjectState::PROP_VALUE("value");

// Phase 15: Tempo Map + Markers properties
const juce::Identifier ProjectState::PROP_BPM("bpm");
const juce::Identifier ProjectState::PROP_COLOR("color");

//==============================================================================
ProjectState::ProjectState()
{
    DBG("ProjectState: Constructor");
    newProject();
}

ProjectState::~ProjectState()
{
    DBG("ProjectState: Destructor");
}

//==============================================================================
// Project Management
//==============================================================================

void ProjectState::newProject()
{
    DBG("ProjectState: Creating new project");

    // Clear undo history
    undoManager.clearUndoHistory();

    // Create default state
    createDefaultState();

    // Reset ID counter for a fresh project
    idCounter.store(0);

    DBG("ProjectState: New project created");
}

bool ProjectState::loadFromFile(const juce::File& file)
{
    DBG("ProjectState: Loading from " + file.getFullPathName());

    if (!file.existsAsFile())
    {
        DBG("ProjectState: File does not exist");
        return false;
    }

    // Parse XML
    auto xml = juce::parseXML(file);

    if (xml == nullptr)
    {
        DBG("ProjectState: Failed to parse XML");
        return false;
    }

    // Create ValueTree from XML
    auto newState = juce::ValueTree::fromXml(*xml);

    if (!newState.isValid() || newState.getType() != ID_PROJECT)
    {
        DBG("ProjectState: Invalid project file");
        return false;
    }

    // Replace current state
    state = newState;

    // Ensure future IDs do not clash with those loaded from disk
    rebuildIdCounter();

    // Clear undo history (fresh start)
    undoManager.clearUndoHistory();

    DBG("ProjectState: Loaded successfully");
    return true;
}

bool ProjectState::saveToFile(const juce::File& file)
{
    DBG("ProjectState: Saving to " + file.getFullPathName());

    // Convert ValueTree to XML
    auto xml = state.createXml();

    if (xml == nullptr)
    {
        DBG("ProjectState: Failed to create XML");
        return false;
    }

    // Save to file
    if (!xml->writeTo(file))
    {
        DBG("ProjectState: Failed to write file");
        return false;
    }

    DBG("ProjectState: Saved successfully");
    return true;
}

//==============================================================================
// Project Properties
//==============================================================================

juce::String ProjectState::getProjectName() const
{
    return state[PROP_NAME].toString();
}

void ProjectState::setProjectName(const juce::String& name)
{
    state.setProperty(PROP_NAME, name, &undoManager);
}

double ProjectState::getTempo() const
{
    return state[PROP_TEMPO];
}

void ProjectState::setTempo(double tempo)
{
    tempo = juce::jlimit(20.0, 999.0, tempo);
    state.setProperty(PROP_TEMPO, tempo, &undoManager);
}

int ProjectState::getTimeSignatureNumerator() const
{
    return state[PROP_TIME_SIG_NUM];
}

int ProjectState::getTimeSignatureDenominator() const
{
    return state[PROP_TIME_SIG_DEN];
}

void ProjectState::setTimeSignature(int numerator, int denominator)
{
    state.setProperty(PROP_TIME_SIG_NUM, numerator, &undoManager);
    state.setProperty(PROP_TIME_SIG_DEN, denominator, &undoManager);
}

//==============================================================================
// Track Management
//==============================================================================

juce::String ProjectState::addTrack(const juce::String& name, const juce::String& type)
{
    auto tracksNode = state.getChildWithName(ID_TRACKS);

    if (!tracksNode.isValid())
    {
        // Create TRACKS node if it doesn't exist
        tracksNode = juce::ValueTree(ID_TRACKS);
        state.appendChild(tracksNode, &undoManager);
    }

    // Generate unique ID
    auto trackId = generateUniqueId("track");

    // Create track
    juce::ValueTree track(ID_TRACK);
    track.setProperty(PROP_ID, trackId, nullptr);
    track.setProperty(PROP_NAME, name, nullptr);
    track.setProperty(PROP_TYPE, type, nullptr);
    track.setProperty(PROP_VOLUME, 0.8, nullptr);
    track.setProperty(PROP_PAN, 0.0, nullptr);
    track.setProperty(PROP_MUTE, false, nullptr);
    track.setProperty(PROP_SOLO, false, nullptr);

    // Create empty CLIPS node
    track.appendChild(juce::ValueTree(ID_CLIPS), nullptr);

    // Add to project
    tracksNode.appendChild(track, &undoManager);

    DBG("ProjectState: Added track '" + name + "' with ID " + trackId);

    return trackId;
}

void ProjectState::removeTrack(const juce::String& trackId)
{
    auto track = findTrack(trackId);

    if (track.isValid())
    {
        auto tracksNode = state.getChildWithName(ID_TRACKS);
        tracksNode.removeChild(track, &undoManager);

        DBG("ProjectState: Removed track " + trackId);
    }
}

int ProjectState::getNumTracks() const
{
    auto tracksNode = state.getChildWithName(ID_TRACKS);

    if (tracksNode.isValid())
        return tracksNode.getNumChildren();

    return 0;
}

//==============================================================================
// Undo/Redo
//==============================================================================

void ProjectState::undo()
{
    if (undoManager.canUndo())
    {
        undoManager.undo();
        DBG("ProjectState: Undo");
    }
}

void ProjectState::redo()
{
    if (undoManager.canRedo())
    {
        undoManager.redo();
        DBG("ProjectState: Redo");
    }
}

//==============================================================================
// Helper Methods
//==============================================================================

void ProjectState::createDefaultState()
{
    // Create root PROJECT node
    state = juce::ValueTree(ID_PROJECT);

    // Set default properties
    state.setProperty(PROP_NAME, "Untitled Project", nullptr);
    state.setProperty(PROP_TEMPO, 120.0, nullptr);
    state.setProperty(PROP_TIME_SIG_NUM, 4, nullptr);
    state.setProperty(PROP_TIME_SIG_DEN, 4, nullptr);
    state.setProperty(PROP_SAMPLE_RATE, 44100.0, nullptr);

    // Create TRACKS node
    state.appendChild(juce::ValueTree(ID_TRACKS), nullptr);

    // Create MIXER node
    juce::ValueTree mixer(ID_MIXER);
    mixer.setProperty(PROP_VOLUME, 0.8, nullptr);
    state.appendChild(mixer, nullptr);

    // Phase 15: Create TEMPO_MAP node with initial tempo point at beat 0
    juce::ValueTree tempoMap(ID_TEMPO_MAP);
    juce::ValueTree initialTempoPoint(ID_TEMPO_POINT);
    initialTempoPoint.setProperty(PROP_ID, "tempo_0", nullptr);
    initialTempoPoint.setProperty(PROP_TIME_BEATS, 0.0, nullptr);
    initialTempoPoint.setProperty(PROP_BPM, 120.0, nullptr);
    initialTempoPoint.setProperty(PROP_TIME_SIG_NUM, 4, nullptr);
    initialTempoPoint.setProperty(PROP_TIME_SIG_DEN, 4, nullptr);
    tempoMap.appendChild(initialTempoPoint, nullptr);
    state.appendChild(tempoMap, nullptr);

    // Phase 15: Create MARKERS node
    state.appendChild(juce::ValueTree(ID_MARKERS), nullptr);

    DBG("ProjectState: Default state created");
}

juce::String ProjectState::generateUniqueId(const juce::String& prefix)
{
    int id = idCounter.fetch_add(1);
    return prefix + "_" + juce::String(id);
}

juce::ValueTree ProjectState::findTrack(const juce::String& trackId)
{
    auto tracksNode = state.getChildWithName(ID_TRACKS);

    if (!tracksNode.isValid())
        return {};

    for (auto track : tracksNode)
    {
        if (track[PROP_ID].toString() == trackId)
            return track;
    }

    return {};
}

void ProjectState::rebuildIdCounter()
{
    int highestId = -1;

    std::function<void(const juce::ValueTree&)> scanTree = [&](const juce::ValueTree& node)
    {
        if (!node.isValid())
            return;

        if (node.hasProperty(PROP_ID))
        {
            const int suffix = extractNumericSuffix(node[PROP_ID].toString());

            if (suffix > highestId)
                highestId = suffix;
        }

        for (int i = 0; i < node.getNumChildren(); ++i)
            scanTree(node.getChild(i));
    };

    scanTree(state);

    idCounter.store(highestId + 1);
}

//==============================================================================
// Phase 15: Tempo Map Management
//==============================================================================

juce::String ProjectState::addTempoPoint(double timeBeats, double bpm, int timeSigNum, int timeSigDen,
                                         const juce::String& actionName)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    jassert(timeBeats >= 0.0);

    // Clamp BPM to reasonable range
    bpm = juce::jlimit(40.0, 240.0, bpm);

    // Clamp time signature values
    timeSigNum = juce::jmax(1, timeSigNum);
    timeSigDen = juce::jmax(1, timeSigDen);

    // Get or create TEMPO_MAP node
    auto tempoMapNode = state.getChildWithName(ID_TEMPO_MAP);
    if (!tempoMapNode.isValid())
    {
        tempoMapNode = juce::ValueTree(ID_TEMPO_MAP);
        state.appendChild(tempoMapNode, &undoManager);
        DBG("ProjectState: Created TEMPO_MAP node");
    }

    // Generate unique ID
    auto pointId = generateUniqueId("tempo");

    // Create tempo point
    juce::ValueTree point(ID_TEMPO_POINT);
    point.setProperty(PROP_ID, pointId, nullptr);
    point.setProperty(PROP_TIME_BEATS, timeBeats, nullptr);
    point.setProperty(PROP_BPM, bpm, nullptr);
    point.setProperty(PROP_TIME_SIG_NUM, timeSigNum, nullptr);
    point.setProperty(PROP_TIME_SIG_DEN, timeSigDen, nullptr);

    // Insert in sorted order by time
    int insertIndex = 0;
    for (int i = 0; i < tempoMapNode.getNumChildren(); ++i)
    {
        auto existingPoint = tempoMapNode.getChild(i);
        double existingTime = existingPoint[PROP_TIME_BEATS];
        if (timeBeats >= existingTime)
            insertIndex = i + 1;
        else
            break;
    }

    tempoMapNode.addChild(point, insertIndex, &undoManager);

    DBG("ProjectState: Added tempo point " + pointId + " at " + juce::String(timeBeats) + " beats, " + juce::String(bpm) + " BPM");
    return pointId;
}

bool ProjectState::moveTempoPoint(const juce::String& pointId, double newTimeBeats, double newBpm,
                                   int newTimeSigNum, int newTimeSigDen, const juce::String& actionName)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    jassert(newTimeBeats >= 0.0);

    // Clamp values
    newBpm = juce::jlimit(40.0, 240.0, newBpm);
    newTimeSigNum = juce::jmax(1, newTimeSigNum);
    newTimeSigDen = juce::jmax(1, newTimeSigDen);

    auto tempoMapNode = state.getChildWithName(ID_TEMPO_MAP);
    if (!tempoMapNode.isValid())
        return false;

    // Find point
    for (int i = 0; i < tempoMapNode.getNumChildren(); ++i)
    {
        auto point = tempoMapNode.getChild(i);
        if (point[PROP_ID].toString() == pointId)
        {
            double oldTime = point[PROP_TIME_BEATS];

            // Cannot move the root tempo point (at beat 0) before 0.0
            // But we can change its BPM and time signature
            if (oldTime == 0.0 && newTimeBeats != 0.0)
            {
                DBG("ProjectState: Cannot move root tempo point away from beat 0");
                return false;
            }

            // Update properties
            point.setProperty(PROP_TIME_BEATS, newTimeBeats, &undoManager);
            point.setProperty(PROP_BPM, newBpm, &undoManager);
            point.setProperty(PROP_TIME_SIG_NUM, newTimeSigNum, &undoManager);
            point.setProperty(PROP_TIME_SIG_DEN, newTimeSigDen, &undoManager);

            // If time changed, re-sort
            if (newTimeBeats != oldTime)
            {
                tempoMapNode.removeChild(i, &undoManager);

                int insertIndex = 0;
                for (int j = 0; j < tempoMapNode.getNumChildren(); ++j)
                {
                    auto existingPoint = tempoMapNode.getChild(j);
                    double existingTime = existingPoint[PROP_TIME_BEATS];
                    if (newTimeBeats >= existingTime)
                        insertIndex = j + 1;
                    else
                        break;
                }

                tempoMapNode.addChild(point, insertIndex, &undoManager);
            }

            DBG("ProjectState: Moved tempo point " + pointId);
            return true;
        }
    }

    return false;
}

bool ProjectState::deleteTempoPoint(const juce::String& pointId, const juce::String& actionName)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    auto tempoMapNode = state.getChildWithName(ID_TEMPO_MAP);
    if (!tempoMapNode.isValid())
        return false;

    // Find and remove point
    for (int i = 0; i < tempoMapNode.getNumChildren(); ++i)
    {
        auto point = tempoMapNode.getChild(i);
        if (point[PROP_ID].toString() == pointId)
        {
            double timeBeats = point[PROP_TIME_BEATS];

            // Cannot delete the only tempo point at beat 0
            if (timeBeats == 0.0 && tempoMapNode.getNumChildren() == 1)
            {
                DBG("ProjectState: Cannot delete the only tempo point at beat 0");
                return false;
            }

            tempoMapNode.removeChild(i, &undoManager);
            DBG("ProjectState: Deleted tempo point " + pointId);
            return true;
        }
    }

    return false;
}

juce::Array<ProjectState::TempoPointSpec> ProjectState::getTempoPoints() const
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    juce::Array<TempoPointSpec> points;

    auto tempoMapNode = state.getChildWithName(ID_TEMPO_MAP);
    if (!tempoMapNode.isValid())
        return points;

    // Points are already sorted by time due to insertion logic
    for (auto point : tempoMapNode)
    {
        if (point.hasType(ID_TEMPO_POINT))
        {
            TempoPointSpec spec;
            spec.id = point[PROP_ID].toString();
            spec.timeBeats = point[PROP_TIME_BEATS];
            spec.bpm = point[PROP_BPM];
            spec.timeSigNum = point[PROP_TIME_SIG_NUM];
            spec.timeSigDen = point[PROP_TIME_SIG_DEN];
            points.add(spec);
        }
    }

    return points;
}

//==============================================================================
// Phase 15: Markers Management
//==============================================================================

juce::String ProjectState::addMarker(double timeBeats, const juce::String& name,
                                     const juce::String& color, const juce::String& actionName)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    jassert(timeBeats >= 0.0);

    // Get or create MARKERS node
    auto markersNode = state.getChildWithName(ID_MARKERS);
    if (!markersNode.isValid())
    {
        markersNode = juce::ValueTree(ID_MARKERS);
        state.appendChild(markersNode, &undoManager);
        DBG("ProjectState: Created MARKERS node");
    }

    // Generate unique ID
    auto markerId = generateUniqueId("marker");

    // Create marker
    juce::ValueTree marker(ID_MARKER);
    marker.setProperty(PROP_ID, markerId, nullptr);
    marker.setProperty(PROP_TIME_BEATS, timeBeats, nullptr);
    marker.setProperty(PROP_NAME, name, nullptr);
    marker.setProperty(PROP_COLOR, color, nullptr);

    // Insert in sorted order by time
    int insertIndex = 0;
    for (int i = 0; i < markersNode.getNumChildren(); ++i)
    {
        auto existingMarker = markersNode.getChild(i);
        double existingTime = existingMarker[PROP_TIME_BEATS];
        if (timeBeats >= existingTime)
            insertIndex = i + 1;
        else
            break;
    }

    markersNode.addChild(marker, insertIndex, &undoManager);

    DBG("ProjectState: Added marker '" + name + "' at " + juce::String(timeBeats) + " beats");
    return markerId;
}

bool ProjectState::moveMarker(const juce::String& markerId, double newTimeBeats, const juce::String& actionName)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    jassert(newTimeBeats >= 0.0);

    auto markersNode = state.getChildWithName(ID_MARKERS);
    if (!markersNode.isValid())
        return false;

    // Find marker
    for (int i = 0; i < markersNode.getNumChildren(); ++i)
    {
        auto marker = markersNode.getChild(i);
        if (marker[PROP_ID].toString() == markerId)
        {
            double oldTime = marker[PROP_TIME_BEATS];

            // Update time
            marker.setProperty(PROP_TIME_BEATS, newTimeBeats, &undoManager);

            // If time changed, re-sort
            if (newTimeBeats != oldTime)
            {
                markersNode.removeChild(i, &undoManager);

                int insertIndex = 0;
                for (int j = 0; j < markersNode.getNumChildren(); ++j)
                {
                    auto existingMarker = markersNode.getChild(j);
                    double existingTime = existingMarker[PROP_TIME_BEATS];
                    if (newTimeBeats >= existingTime)
                        insertIndex = j + 1;
                    else
                        break;
                }

                markersNode.addChild(marker, insertIndex, &undoManager);
            }

            DBG("ProjectState: Moved marker " + markerId);
            return true;
        }
    }

    return false;
}

bool ProjectState::renameMarker(const juce::String& markerId, const juce::String& newName, const juce::String& actionName)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    auto markersNode = state.getChildWithName(ID_MARKERS);
    if (!markersNode.isValid())
        return false;

    // Find and rename marker
    for (auto marker : markersNode)
    {
        if (marker[PROP_ID].toString() == markerId)
        {
            marker.setProperty(PROP_NAME, newName, &undoManager);
            DBG("ProjectState: Renamed marker " + markerId + " to '" + newName + "'");
            return true;
        }
    }

    return false;
}

bool ProjectState::recolorMarker(const juce::String& markerId, const juce::String& newColor, const juce::String& actionName)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    auto markersNode = state.getChildWithName(ID_MARKERS);
    if (!markersNode.isValid())
        return false;

    // Find and recolor marker
    for (auto marker : markersNode)
    {
        if (marker[PROP_ID].toString() == markerId)
        {
            marker.setProperty(PROP_COLOR, newColor, &undoManager);
            DBG("ProjectState: Recolored marker " + markerId);
            return true;
        }
    }

    return false;
}

bool ProjectState::deleteMarker(const juce::String& markerId, const juce::String& actionName)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    auto markersNode = state.getChildWithName(ID_MARKERS);
    if (!markersNode.isValid())
        return false;

    // Find and remove marker
    for (int i = 0; i < markersNode.getNumChildren(); ++i)
    {
        auto marker = markersNode.getChild(i);
        if (marker[PROP_ID].toString() == markerId)
        {
            markersNode.removeChild(i, &undoManager);
            DBG("ProjectState: Deleted marker " + markerId);
            return true;
        }
    }

    return false;
}

juce::Array<ProjectState::MarkerSpec> ProjectState::getMarkers() const
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    juce::Array<MarkerSpec> markers;

    auto markersNode = state.getChildWithName(ID_MARKERS);
    if (!markersNode.isValid())
        return markers;

    // Markers are already sorted by time due to insertion logic
    for (auto marker : markersNode)
    {
        if (marker.hasType(ID_MARKER))
        {
            MarkerSpec spec;
            spec.id = marker[PROP_ID].toString();
            spec.timeBeats = marker[PROP_TIME_BEATS];
            spec.name = marker[PROP_NAME].toString();
            spec.color = marker[PROP_COLOR].toString();
            markers.add(spec);
        }
    }

    return markers;
}

//==============================================================================
// Phase 13: Automation Management
//==============================================================================

juce::ValueTree ProjectState::getOrCreateAutomationEnvelope(const juce::String& trackId, const juce::String& paramId)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    jassert(paramId == "volume" || paramId == "pan" || paramId == "mute");

    auto track = findTrack(trackId);
    if (!track.isValid())
    {
        DBG("ProjectState: Track not found: " + trackId);
        return {};
    }

    // Get or create AUTOMATION node
    auto automationNode = track.getChildWithName(ID_AUTOMATION);
    if (!automationNode.isValid())
    {
        automationNode = juce::ValueTree(ID_AUTOMATION);
        track.appendChild(automationNode, &undoManager);
        DBG("ProjectState: Created AUTOMATION node for track " + trackId);
    }

    // Find envelope for this parameter
    for (auto envelope : automationNode)
    {
        if (envelope.hasType(ID_ENVELOPE) && envelope[PROP_PARAM].toString() == paramId)
            return envelope;
    }

    // Create new envelope
    juce::ValueTree envelope(ID_ENVELOPE);
    envelope.setProperty(PROP_PARAM, paramId, nullptr);
    automationNode.appendChild(envelope, &undoManager);

    DBG("ProjectState: Created envelope for " + trackId + "/" + paramId);
    return envelope;
}

juce::ValueTree ProjectState::getAutomationEnvelope(const juce::String& trackId, const juce::String& paramId) const
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    auto track = const_cast<ProjectState*>(this)->findTrack(trackId);
    if (!track.isValid())
        return {};

    auto automationNode = track.getChildWithName(ID_AUTOMATION);
    if (!automationNode.isValid())
        return {};

    // Find envelope for this parameter
    for (auto envelope : automationNode)
    {
        if (envelope.hasType(ID_ENVELOPE) && envelope[PROP_PARAM].toString() == paramId)
            return envelope;
    }

    return {};
}

bool ProjectState::hasAutomation(const juce::String& trackId, const juce::String& paramId) const
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    return getAutomationEnvelope(trackId, paramId).isValid();
}

juce::String ProjectState::addAutomationPoint(const juce::String& trackId, const juce::String& paramId,
                                               double timeBeats, double value, const juce::String& actionName)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    jassert(timeBeats >= 0.0);

    // Validate parameter ID and value range
    if (paramId == "volume")
        value = juce::jlimit(0.0, 1.0, value);
    else if (paramId == "pan")
        value = juce::jlimit(-1.0, 1.0, value);
    else if (paramId == "mute")
        value = value >= 0.5 ? 1.0 : 0.0;  // Binary
    else
    {
        DBG("ProjectState: Invalid parameter ID: " + paramId);
        return {};
    }

    auto envelope = getOrCreateAutomationEnvelope(trackId, paramId);
    if (!envelope.isValid())
    {
        DBG("ProjectState: Failed to get/create envelope");
        return {};
    }

    // Generate unique point ID
    auto pointId = generateUniqueId("point");

    // Create point
    juce::ValueTree point(ID_POINT);
    point.setProperty(PROP_ID, pointId, nullptr);
    point.setProperty(PROP_TIME_BEATS, timeBeats, nullptr);
    point.setProperty(PROP_VALUE, value, nullptr);

    // Insert in sorted order by time
    int insertIndex = 0;
    for (int i = 0; i < envelope.getNumChildren(); ++i)
    {
        auto existingPoint = envelope.getChild(i);
        double existingTime = existingPoint[PROP_TIME_BEATS];
        if (timeBeats >= existingTime)
            insertIndex = i + 1;
        else
            break;
    }

    envelope.addChild(point, insertIndex, &undoManager);

    DBG("ProjectState: Added automation point " + pointId + " at " + juce::String(timeBeats) + " beats");
    return pointId;
}

bool ProjectState::moveAutomationPoint(const juce::String& trackId, const juce::String& paramId,
                                        const juce::String& pointId, double newTimeBeats, double newValue,
                                        const juce::String& actionName)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    jassert(newTimeBeats >= 0.0);

    // Validate value range
    if (paramId == "volume")
        newValue = juce::jlimit(0.0, 1.0, newValue);
    else if (paramId == "pan")
        newValue = juce::jlimit(-1.0, 1.0, newValue);
    else if (paramId == "mute")
        newValue = newValue >= 0.5 ? 1.0 : 0.0;

    auto envelope = getAutomationEnvelope(trackId, paramId);
    if (!envelope.isValid())
        return false;

    // Find point
    for (int i = 0; i < envelope.getNumChildren(); ++i)
    {
        auto point = envelope.getChild(i);
        if (point[PROP_ID].toString() == pointId)
        {
            double oldTime = point[PROP_TIME_BEATS];

            // Update properties
            point.setProperty(PROP_TIME_BEATS, newTimeBeats, &undoManager);
            point.setProperty(PROP_VALUE, newValue, &undoManager);

            // If time changed, re-sort
            if (newTimeBeats != oldTime)
            {
                // Remove and re-insert in sorted position
                // Use undoManager for both operations to ensure proper undo/redo support
                envelope.removeChild(i, &undoManager);

                int insertIndex = 0;
                for (int j = 0; j < envelope.getNumChildren(); ++j)
                {
                    auto existingPoint = envelope.getChild(j);
                    double existingTime = existingPoint[PROP_TIME_BEATS];
                    if (newTimeBeats >= existingTime)
                        insertIndex = j + 1;
                    else
                        break;
                }

                envelope.addChild(point, insertIndex, &undoManager);
            }

            DBG("ProjectState: Moved automation point " + pointId);
            return true;
        }
    }

    return false;
}

bool ProjectState::deleteAutomationPoint(const juce::String& trackId, const juce::String& paramId,
                                          const juce::String& pointId, const juce::String& actionName)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    auto envelope = getAutomationEnvelope(trackId, paramId);
    if (!envelope.isValid())
        return false;

    // Find and remove point
    for (int i = 0; i < envelope.getNumChildren(); ++i)
    {
        auto point = envelope.getChild(i);
        if (point[PROP_ID].toString() == pointId)
        {
            envelope.removeChild(i, &undoManager);
            DBG("ProjectState: Deleted automation point " + pointId);
            return true;
        }
    }

    return false;
}

bool ProjectState::clearAutomation(const juce::String& trackId, const juce::String& paramId,
                                    const juce::String& actionName)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    auto track = findTrack(trackId);
    if (!track.isValid())
        return false;

    auto automationNode = track.getChildWithName(ID_AUTOMATION);
    if (!automationNode.isValid())
        return false;

    // Find and remove envelope
    for (int i = 0; i < automationNode.getNumChildren(); ++i)
    {
        auto envelope = automationNode.getChild(i);
        if (envelope.hasType(ID_ENVELOPE) && envelope[PROP_PARAM].toString() == paramId)
        {
            automationNode.removeChild(i, &undoManager);
            DBG("ProjectState: Cleared automation for " + trackId + "/" + paramId);
            return true;
        }
    }

    return false;
}
