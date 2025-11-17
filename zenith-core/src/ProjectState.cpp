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
const juce::Identifier ProjectState::ID_NOTES("NOTES");
const juce::Identifier ProjectState::ID_NOTE("NOTE");

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

// MIDI Note properties
const juce::Identifier ProjectState::PROP_START_BEATS("startBeats");
const juce::Identifier ProjectState::PROP_LENGTH_BEATS("lengthBeats");
const juce::Identifier ProjectState::PROP_PITCH("pitch");
const juce::Identifier ProjectState::PROP_VELOCITY("velocity");

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

juce::ValueTree ProjectState::findClip(const juce::String& trackId, const juce::String& clipId)
{
    auto track = findTrack(trackId);
    if (!track.isValid())
        return {};

    auto clipsNode = track.getChildWithName(ID_CLIPS);
    if (!clipsNode.isValid())
        return {};

    for (auto clip : clipsNode)
    {
        if (clip[PROP_ID].toString() == clipId)
            return clip;
    }

    return {};
}

//==============================================================================
// MIDI Note Management
//==============================================================================

juce::String ProjectState::addNote(const juce::String& trackId, const juce::String& clipId,
                                    double startBeats, double lengthBeats, int pitch, int velocity,
                                    const juce::String& actionName)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    jassert(startBeats >= 0.0);
    jassert(lengthBeats > 0.0);

    // Clamp values
    pitch = juce::jlimit(0, 127, pitch);
    velocity = juce::jlimit(0, 127, velocity);

    auto clip = findClip(trackId, clipId);
    if (!clip.isValid())
    {
        DBG("ProjectState: Clip not found: " + clipId);
        return {};
    }

    // Get or create NOTES node
    auto notesNode = clip.getChildWithName(ID_NOTES);
    if (!notesNode.isValid())
    {
        notesNode = juce::ValueTree(ID_NOTES);
        clip.appendChild(notesNode, &undoManager);
        DBG("ProjectState: Created NOTES node for clip " + clipId);
    }

    // Generate unique note ID
    auto noteId = generateUniqueId("note");

    // Create note
    juce::ValueTree note(ID_NOTE);
    note.setProperty(PROP_ID, noteId, nullptr);
    note.setProperty(PROP_START_BEATS, startBeats, nullptr);
    note.setProperty(PROP_LENGTH_BEATS, lengthBeats, nullptr);
    note.setProperty(PROP_PITCH, pitch, nullptr);
    note.setProperty(PROP_VELOCITY, velocity, nullptr);

    // Add to notes node
    notesNode.appendChild(note, &undoManager);

    DBG("ProjectState: Added note " + noteId + " at " + juce::String(startBeats) + " beats, pitch " + juce::String(pitch));
    return noteId;
}

bool ProjectState::moveNote(const juce::String& trackId, const juce::String& clipId,
                             const juce::String& noteId, double newStartBeats, double newLengthBeats,
                             int newPitch, int newVelocity, const juce::String& actionName)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    jassert(newStartBeats >= 0.0);
    jassert(newLengthBeats > 0.0);

    // Clamp values
    newPitch = juce::jlimit(0, 127, newPitch);
    newVelocity = juce::jlimit(0, 127, newVelocity);

    auto clip = findClip(trackId, clipId);
    if (!clip.isValid())
        return false;

    auto notesNode = clip.getChildWithName(ID_NOTES);
    if (!notesNode.isValid())
        return false;

    // Find note
    for (int i = 0; i < notesNode.getNumChildren(); ++i)
    {
        auto note = notesNode.getChild(i);
        if (note[PROP_ID].toString() == noteId)
        {
            // Update properties
            note.setProperty(PROP_START_BEATS, newStartBeats, &undoManager);
            note.setProperty(PROP_LENGTH_BEATS, newLengthBeats, &undoManager);
            note.setProperty(PROP_PITCH, newPitch, &undoManager);
            note.setProperty(PROP_VELOCITY, newVelocity, &undoManager);

            DBG("ProjectState: Moved note " + noteId);
            return true;
        }
    }

    return false;
}

bool ProjectState::removeNote(const juce::String& trackId, const juce::String& clipId,
                               const juce::String& noteId, const juce::String& actionName)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    auto clip = findClip(trackId, clipId);
    if (!clip.isValid())
        return false;

    auto notesNode = clip.getChildWithName(ID_NOTES);
    if (!notesNode.isValid())
        return false;

    // Find and remove note
    for (int i = 0; i < notesNode.getNumChildren(); ++i)
    {
        auto note = notesNode.getChild(i);
        if (note[PROP_ID].toString() == noteId)
        {
            notesNode.removeChild(i, &undoManager);
            DBG("ProjectState: Removed note " + noteId);
            return true;
        }
    }

    return false;
}

juce::ValueTree ProjectState::getNotesForClip(const juce::String& trackId, const juce::String& clipId) const
{
    auto clip = const_cast<ProjectState*>(this)->findClip(trackId, clipId);
    if (!clip.isValid())
        return {};

    return clip.getChildWithName(ID_NOTES);
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
