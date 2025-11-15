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
const juce::Identifier ProjectState::ID_POINTS("POINTS");
const juce::Identifier ProjectState::ID_POINT("POINT");

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

const juce::Identifier ProjectState::PROP_PARAM_ID("paramId");
const juce::Identifier ProjectState::PROP_TIME_BEATS("timeBeats");
const juce::Identifier ProjectState::PROP_VALUE("value");

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
// Automation Management
//==============================================================================

juce::ValueTree ProjectState::getOrCreateAutomationEnvelope(const juce::String& trackId, const juce::String& paramId)
{
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
    }

    // Find existing envelope for this parameter
    for (auto envelope : automationNode)
    {
        if (envelope.getType() == ID_ENVELOPE && envelope[PROP_PARAM_ID].toString() == paramId)
            return envelope;
    }

    // Create new envelope
    juce::ValueTree envelope(ID_ENVELOPE);
    envelope.setProperty(PROP_PARAM_ID, paramId, nullptr);

    // Create POINTS container
    envelope.appendChild(juce::ValueTree(ID_POINTS), nullptr);

    automationNode.appendChild(envelope, &undoManager);

    DBG("ProjectState: Created automation envelope for " + trackId + ":" + paramId);
    return envelope;
}

juce::ValueTree ProjectState::getAutomationEnvelope(const juce::String& trackId, const juce::String& paramId) const
{
    auto track = findTrack(trackId);
    if (!track.isValid())
        return {};

    auto automationNode = track.getChildWithName(ID_AUTOMATION);
    if (!automationNode.isValid())
        return {};

    for (auto envelope : automationNode)
    {
        if (envelope.getType() == ID_ENVELOPE && envelope[PROP_PARAM_ID].toString() == paramId)
            return envelope;
    }

    return {};
}

bool ProjectState::hasAutomation(const juce::String& trackId, const juce::String& paramId) const
{
    auto envelope = getAutomationEnvelope(trackId, paramId);
    if (!envelope.isValid())
        return false;

    auto pointsNode = envelope.getChildWithName(ID_POINTS);
    return pointsNode.isValid() && pointsNode.getNumChildren() > 0;
}

juce::String ProjectState::addAutomationPoint(const juce::String& trackId, const juce::String& paramId,
                                               double timeBeats, double value, const juce::String& actionName)
{
    auto envelope = getOrCreateAutomationEnvelope(trackId, paramId);
    if (!envelope.isValid())
    {
        DBG("ProjectState: Failed to get/create envelope");
        return {};
    }

    auto pointsNode = envelope.getChildWithName(ID_POINTS);
    if (!pointsNode.isValid())
    {
        DBG("ProjectState: POINTS node not found");
        return {};
    }

    // Generate unique ID for the point
    auto pointId = generateUniqueId("point");

    // Create point
    juce::ValueTree point(ID_POINT);
    point.setProperty(PROP_ID, pointId, nullptr);
    point.setProperty(PROP_TIME_BEATS, timeBeats, nullptr);
    point.setProperty(PROP_VALUE, value, nullptr);

    // Begin undo transaction
    undoManager.beginNewTransaction(actionName);

    // Add point to envelope (sorted by time)
    int insertIndex = 0;
    for (int i = 0; i < pointsNode.getNumChildren(); ++i)
    {
        auto existingPoint = pointsNode.getChild(i);
        double existingTime = existingPoint[PROP_TIME_BEATS];
        if (existingTime > timeBeats)
            break;
        insertIndex = i + 1;
    }

    pointsNode.addChild(point, insertIndex, &undoManager);

    DBG("ProjectState: Added automation point " + pointId + " at " + juce::String(timeBeats) + " beats");
    return pointId;
}

bool ProjectState::moveAutomationPoint(const juce::String& trackId, const juce::String& paramId,
                                       const juce::String& pointId, double newTimeBeats, double newValue,
                                       const juce::String& actionName)
{
    auto envelope = getAutomationEnvelope(trackId, paramId);
    if (!envelope.isValid())
        return false;

    auto point = findAutomationPoint(envelope, pointId);
    if (!point.isValid())
        return false;

    undoManager.beginNewTransaction(actionName);
    point.setProperty(PROP_TIME_BEATS, newTimeBeats, &undoManager);
    point.setProperty(PROP_VALUE, newValue, &undoManager);

    // Re-sort points by time if necessary
    auto pointsNode = envelope.getChildWithName(ID_POINTS);
    if (pointsNode.isValid())
    {
        // Remove and re-insert to maintain sorted order
        int currentIndex = pointsNode.indexOf(point);
        int newIndex = 0;

        for (int i = 0; i < pointsNode.getNumChildren(); ++i)
        {
            if (i == currentIndex)
                continue;
            auto otherPoint = pointsNode.getChild(i);
            double otherTime = otherPoint[PROP_TIME_BEATS];
            if (otherTime > newTimeBeats)
                break;
            newIndex++;
        }

        if (newIndex != currentIndex)
        {
            pointsNode.removeChild(point, &undoManager);
            pointsNode.addChild(point, newIndex, &undoManager);
        }
    }

    DBG("ProjectState: Moved automation point " + pointId);
    return true;
}

bool ProjectState::deleteAutomationPoint(const juce::String& trackId, const juce::String& paramId,
                                         const juce::String& pointId, const juce::String& actionName)
{
    auto envelope = getAutomationEnvelope(trackId, paramId);
    if (!envelope.isValid())
        return false;

    auto pointsNode = envelope.getChildWithName(ID_POINTS);
    if (!pointsNode.isValid())
        return false;

    auto point = findAutomationPoint(envelope, pointId);
    if (!point.isValid())
        return false;

    undoManager.beginNewTransaction(actionName);
    pointsNode.removeChild(point, &undoManager);

    DBG("ProjectState: Deleted automation point " + pointId);
    return true;
}

void ProjectState::clearAutomation(const juce::String& trackId, const juce::String& paramId,
                                   const juce::String& actionName)
{
    auto envelope = getAutomationEnvelope(trackId, paramId);
    if (!envelope.isValid())
        return;

    auto pointsNode = envelope.getChildWithName(ID_POINTS);
    if (!pointsNode.isValid())
        return;

    undoManager.beginNewTransaction(actionName);
    pointsNode.removeAllChildren(&undoManager);

    DBG("ProjectState: Cleared automation for " + trackId + ":" + paramId);
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

juce::ValueTree ProjectState::findTrack(const juce::String& trackId) const
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

juce::ValueTree ProjectState::findAutomationPoint(const juce::ValueTree& envelope, const juce::String& pointId) const
{
    if (!envelope.isValid())
        return {};

    auto pointsNode = envelope.getChildWithName(ID_POINTS);
    if (!pointsNode.isValid())
        return {};

    for (auto point : pointsNode)
    {
        if (point[PROP_ID].toString() == pointId)
            return point;
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
