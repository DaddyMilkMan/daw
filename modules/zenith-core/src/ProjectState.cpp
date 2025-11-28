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
const juce::Identifier ProjectState::PROP_ARMED("armed");

const juce::Identifier ProjectState::PROP_START("start");
const juce::Identifier ProjectState::PROP_LENGTH("length");
const juce::Identifier ProjectState::PROP_OFFSET("offset");
const juce::Identifier ProjectState::PROP_AUDIO_FILE("audioFile");

// Beat-based clip properties
const juce::Identifier ProjectState::PROP_START_BEATS("startBeats");
const juce::Identifier ProjectState::PROP_LENGTH_BEATS("lengthBeats");
const juce::Identifier ProjectState::PROP_LANE_INDEX("laneIndex");

// MIDI note properties
const juce::Identifier ProjectState::PROP_PITCH("pitch");
const juce::Identifier ProjectState::PROP_VELOCITY("velocity");

// Automation properties
const juce::Identifier ProjectState::PROP_PARAM("param");
const juce::Identifier ProjectState::PROP_PARAM_ID("paramId");
const juce::Identifier ProjectState::PROP_TIME_BEATS("timeBeats");
const juce::Identifier ProjectState::PROP_VALUE("value");

// Tempo/Marker properties
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
    track.setProperty(PROP_ARMED, false, nullptr);

    // Create empty CLIPS node
    track.appendChild(juce::ValueTree(ID_CLIPS), nullptr);

    // Add to project
    tracksNode.appendChild(track, &undoManager);

    DBG("ProjectState: Added track '" + name + "' with ID " + trackId);

    return trackId;
}

void ProjectState::removeTrack(const juce::String& trackId)
{
    auto track = findTrackInternal(trackId);

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

juce::ValueTree ProjectState::getTrack(const juce::String& trackId) const
{
    return const_cast<ProjectState*>(this)->findTrackInternal(trackId);
}

juce::ValueTree ProjectState::getTrackByIndex(int index)
{
    auto tracksNode = state.getChildWithName(ID_TRACKS);

    if (!tracksNode.isValid() || index < 0 || index >= tracksNode.getNumChildren())
        return {};

    return tracksNode.getChild(index);
}

//==============================================================================
// Track Mixer Properties (Getters)
//==============================================================================

float ProjectState::getTrackVolume(const juce::String& trackId) const
{
    auto track = const_cast<ProjectState*>(this)->findTrack(trackId);

    if (!track.isValid())
        return 1.0f;

    return track[PROP_VOLUME];
}

float ProjectState::getTrackPan(const juce::String& trackId) const
{
    auto track = const_cast<ProjectState*>(this)->findTrack(trackId);

    if (!track.isValid())
        return 0.0f;

    return track[PROP_PAN];
}

bool ProjectState::isTrackMuted(const juce::String& trackId) const
{
    auto track = const_cast<ProjectState*>(this)->findTrack(trackId);

    if (!track.isValid())
        return false;

    return track[PROP_MUTE];
}

bool ProjectState::isTrackSolo(const juce::String& trackId) const
{
    auto track = const_cast<ProjectState*>(this)->findTrack(trackId);

    if (!track.isValid())
        return false;

    return track[PROP_SOLO];
}

bool ProjectState::isTrackArmed(const juce::String& trackId) const
{
    auto track = const_cast<ProjectState*>(this)->findTrack(trackId);

    if (!track.isValid())
        return false;

    return track[PROP_ARMED];
}

juce::String ProjectState::getTrackName(const juce::String& trackId) const
{
    auto track = const_cast<ProjectState*>(this)->findTrack(trackId);

    if (!track.isValid())
        return {};

    return track[PROP_NAME].toString();
}

juce::String ProjectState::getTrackType(const juce::String& trackId) const
{
    auto track = const_cast<ProjectState*>(this)->findTrack(trackId);

    if (!track.isValid())
        return {};

    return track[PROP_TYPE].toString();
}

//==============================================================================
// Clip Management
//==============================================================================

juce::String ProjectState::addClip(const juce::String& trackId, double startBeats, double lengthBeats, const juce::String& actionName)
{
    auto track = findTrackInternal(trackId);
    if (!track.isValid())
    {
        DBG("ProjectState: Track not found: " + trackId);
        return {};
    }

    // Get or create CLIPS node
    auto clipsNode = track.getChildWithName(ID_CLIPS);
    if (!clipsNode.isValid())
    {
        clipsNode = juce::ValueTree(ID_CLIPS);
        track.appendChild(clipsNode, &undoManager);
    }

    // Generate unique clip ID
    auto clipId = generateUniqueId("clip");

    // Create clip
    juce::ValueTree clip(ID_CLIP);
    clip.setProperty(PROP_ID, clipId, nullptr);
    clip.setProperty(PROP_START, startBeats, nullptr);
    clip.setProperty(PROP_LENGTH, lengthBeats, nullptr);
    clip.setProperty(PROP_TYPE, "audio", nullptr);

    // Add to track
    clipsNode.appendChild(clip, &undoManager);

    DBG("ProjectState: Added clip " + clipId + " to track " + trackId);
    return clipId;
}

juce::String ProjectState::createEmptyClip(const juce::String& trackId,
                                            double startBeats,
                                            double lengthBeats,
                                            bool isMidi,
                                            const juce::String& name,
                                            const juce::String& actionName)
{
    auto track = findTrackInternal(trackId);

    if (!track.isValid())
    {
        DBG("ProjectState: Cannot create clip - track not found: " + trackId);
        return {};
    }

    // Get or create CLIPS node
    auto clipsNode = track.getChildWithName(ID_CLIPS);
    if (!clipsNode.isValid())
    {
        clipsNode = juce::ValueTree(ID_CLIPS);
        track.appendChild(clipsNode, nullptr);
    }

    // Generate unique clip ID
    auto clipId = generateUniqueId("clip");

    // Create clip ValueTree
    juce::ValueTree clip(ID_CLIP);
    clip.setProperty(PROP_ID, clipId, nullptr);
    clip.setProperty(PROP_NAME, name.isEmpty() ? "Clip" : name, nullptr);
    clip.setProperty(PROP_TYPE, isMidi ? "midi" : "audio", nullptr);
    clip.setProperty(PROP_START, startBeats, nullptr);
    clip.setProperty(PROP_LENGTH, lengthBeats, nullptr);

    // Add to track's clips with undo
    undoManager.beginNewTransaction(actionName);
    clipsNode.appendChild(clip, &undoManager);

    DBG("ProjectState: Created clip '" + name + "' with ID " + clipId + " on track " + trackId);

    return clipId;
}

bool ProjectState::removeClip(const juce::String& trackId, const juce::String& clipId, const juce::String& actionName)
{
    auto track = findTrackInternal(trackId);
    if (!track.isValid())
        return false;

    auto clipsNode = track.getChildWithName(ID_CLIPS);
    if (!clipsNode.isValid())
        return false;

    // Find and remove clip
    for (int i = 0; i < clipsNode.getNumChildren(); ++i)
    {
        auto clip = clipsNode.getChild(i);
        if (clip[PROP_ID].toString() == clipId)
        {
            clipsNode.removeChild(i, &undoManager);
            DBG("ProjectState: Removed clip " + clipId);
            return true;
        }
    }

    return false;
}

void ProjectState::moveClip(const juce::String& clipId,
                             const juce::String& newTrackId,
                             double newStartBeats,
                             const juce::String& actionName)
{
    auto [oldTrack, clip] = findClip(clipId);

    if (!clip.isValid())
    {
        DBG("ProjectState: Cannot move clip - clip not found: " + clipId);
        return;
    }

    undoManager.beginNewTransaction(actionName);

    // Update start position
    clip.setProperty(PROP_START, newStartBeats, &undoManager);

    // Check if moving to a different track
    auto oldTrackId = oldTrack[PROP_ID].toString();
    if (newTrackId != oldTrackId)
    {
        auto newTrack = findTrackInternal(newTrackId);
        if (!newTrack.isValid())
        {
            DBG("ProjectState: Cannot move clip - target track not found: " + newTrackId);
            return;
        }

        // Get or create CLIPS node on new track
        auto newClipsNode = newTrack.getChildWithName(ID_CLIPS);
        if (!newClipsNode.isValid())
        {
            newClipsNode = juce::ValueTree(ID_CLIPS);
            newTrack.appendChild(newClipsNode, &undoManager);
        }

        // Move clip to new track
        auto oldClipsNode = oldTrack.getChildWithName(ID_CLIPS);
        auto clipCopy = clip.createCopy();
        oldClipsNode.removeChild(clip, &undoManager);
        newClipsNode.appendChild(clipCopy, &undoManager);

        DBG("ProjectState: Moved clip " + clipId + " from track " + oldTrackId + " to " + newTrackId);
    }
    else
    {
        DBG("ProjectState: Moved clip " + clipId + " to " + juce::String(newStartBeats) + " beats");
    }
}

void ProjectState::setClipRange(const juce::String& clipId,
                                 double newStartBeats,
                                 double newLengthBeats,
                                 const juce::String& actionName)
{
    auto [track, clip] = findClip(clipId);

    if (!clip.isValid())
    {
        DBG("ProjectState: Cannot resize clip - clip not found: " + clipId);
        return;
    }

    // Enforce minimum length
    newLengthBeats = juce::jmax(0.25, newLengthBeats);

    undoManager.beginNewTransaction(actionName);
    clip.setProperty(PROP_START, newStartBeats, &undoManager);
    clip.setProperty(PROP_LENGTH, newLengthBeats, &undoManager);

    DBG("ProjectState: Resized clip " + clipId + " to start=" + juce::String(newStartBeats) + " length=" + juce::String(newLengthBeats));
}

void ProjectState::deleteClip(const juce::String& clipId,
                               const juce::String& actionName)
{
    auto [track, clip] = findClip(clipId);

    if (!clip.isValid())
    {
        DBG("ProjectState: Cannot delete clip - clip not found: " + clipId);
        return;
    }

    auto clipsNode = track.getChildWithName(ID_CLIPS);
    if (clipsNode.isValid())
    {
        undoManager.beginNewTransaction(actionName);
        clipsNode.removeChild(clip, &undoManager);

        DBG("ProjectState: Deleted clip " + clipId);
    }
}

bool ProjectState::setClipAudioFile(const juce::String& trackId, const juce::String& clipId, const juce::File& audioFile, const juce::String& actionName)
{
    auto clip = getClip(trackId, clipId);
    if (!clip.isValid())
    {
        DBG("ProjectState: Clip not found: " + trackId + "/" + clipId);
        return false;
    }

    // Store absolute path for now
    // TODO(zenith-core#1): Make relative to project file when project is saved
    clip.setProperty(PROP_AUDIO_FILE, audioFile.getFullPathName(), &undoManager);

    DBG("ProjectState: Set audio file for clip " + clipId + ": " + audioFile.getFileName());
    return true;
}

juce::String ProjectState::getClipAudioFile(const juce::String& trackId, const juce::String& clipId) const
{
    auto clip = getClip(trackId, clipId);
    if (!clip.isValid())
        return {};

    return clip[PROP_AUDIO_FILE].toString();
}

juce::ValueTree ProjectState::getClip(const juce::String& trackId, const juce::String& clipId) const
{
    auto track = const_cast<ProjectState*>(this)->findTrackInternal(trackId);
    if (!track.isValid())
        return {};

    auto clipsNode = track.getChildWithName(ID_CLIPS);
    if (!clipsNode.isValid())
        return {};

    // Find clip
    for (const auto& clip : clipsNode)
    {
        if (clip[PROP_ID].toString() == clipId)
            return clip;
    }

    return {};
}

std::pair<juce::ValueTree, juce::ValueTree> ProjectState::findClip(const juce::String& clipId)
{
    auto tracksNode = state.getChildWithName(ID_TRACKS);

    if (!tracksNode.isValid())
        return {{}, {}};

    // Search through all tracks
    for (const auto& track : tracksNode)
    {
        auto clipsNode = track.getChildWithName(ID_CLIPS);
        if (!clipsNode.isValid())
            continue;

        // Search clips in this track
        for (const auto& clip : clipsNode)
        {
            if (clip[PROP_ID].toString() == clipId)
                return {track, clip};
        }
    }

    return {{}, {}};
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
    for (const auto& envelope : automationNode)
    {
        if (envelope.getType() == ID_ENVELOPE && envelope[PROP_PARAM_ID].toString() == paramId)
            return envelope;
    }

    // Create new envelope
    juce::ValueTree envelope(ID_ENVELOPE);
    envelope.setProperty(PROP_PARAM_ID, paramId, nullptr);

    // Create POINTS container
    envelope.appendChild(juce::ValueTree(ID_POINT), nullptr);

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

    for (const auto& envelope : automationNode)
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

    auto pointsNode = envelope.getChildWithName(ID_POINT);
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

    auto pointsNode = envelope.getChildWithName(ID_POINT);
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
    auto pointsNode = envelope.getChildWithName(ID_POINT);
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

    auto pointsNode = envelope.getChildWithName(ID_POINT);
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

bool ProjectState::clearAutomation(const juce::String& trackId, const juce::String& paramId,
                                    const juce::String& actionName)
{
    auto envelope = getAutomationEnvelope(trackId, paramId);
    if (!envelope.isValid())
        return false;

    auto pointsNode = envelope.getChildWithName(ID_POINT);
    if (!pointsNode.isValid())
        return false;

    undoManager.beginNewTransaction(actionName);
    pointsNode.removeAllChildren(&undoManager);

    DBG("ProjectState: Cleared all automation points for " + trackId + " / " + paramId);
    return true;
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
// MIDI Note Management (Phase 8)
//==============================================================================

juce::Array<ProjectState::MidiNoteSpec> ProjectState::getMidiNotesForClip(const juce::String& clipId) const
{
    juce::Array<MidiNoteSpec> notes;

    auto [track, clip] = const_cast<ProjectState*>(this)->findClip(clipId);
    if (!clip.isValid())
        return notes;

    auto midiNotesNode = clip.getChildWithName(ID_NOTES);
    if (!midiNotesNode.isValid())
        return notes;

    for (const auto& noteTree : midiNotesNode)
    {
        if (!noteTree.hasType(ID_NOTE))
            continue;

        MidiNoteSpec note;
        note.id = noteTree[PROP_ID].toString();
        note.pitch = noteTree[PROP_PITCH];
        note.startBeats = noteTree[PROP_START_BEATS];
        note.lengthBeats = noteTree[PROP_LENGTH_BEATS];
        note.velocity = noteTree[PROP_VELOCITY];
        note.muted = noteTree.getProperty(PROP_MUTE, false);

        notes.add(note);
    }

    return notes;
}

juce::String ProjectState::addMidiNote(const juce::String& clipId, const MidiNoteSpec& note, const juce::String& actionName)
{
    auto [track, clip] = findClip(clipId);
    if (!clip.isValid())
    {
        DBG("ProjectState: Cannot add MIDI note - clip not found: " + clipId);
        return {};
    }

    // Get or create MIDI_NOTES container
    auto midiNotesNode = clip.getChildWithName(ID_NOTES);
    if (!midiNotesNode.isValid())
    {
        midiNotesNode = juce::ValueTree(ID_NOTES);
        clip.appendChild(midiNotesNode, &undoManager);
    }

    // Generate note ID if not provided
    juce::String noteId = note.id;
    if (noteId.isEmpty())
        noteId = generateUniqueId("note");

    // Validate note properties
    int pitch = juce::jlimit(0, 127, note.pitch);
    int velocity = juce::jlimit(0, 127, note.velocity);
    double startBeats = juce::jmax(0.0, note.startBeats);
    double lengthBeats = juce::jmax(0.0, note.lengthBeats);

    // Create note tree
    juce::ValueTree noteTree(ID_NOTE);
    noteTree.setProperty(PROP_ID, noteId, nullptr);
    noteTree.setProperty(PROP_PITCH, pitch, nullptr);
    noteTree.setProperty(PROP_START_BEATS, startBeats, nullptr);
    noteTree.setProperty(PROP_LENGTH_BEATS, lengthBeats, nullptr);
    noteTree.setProperty(PROP_VELOCITY, velocity, nullptr);
    if (note.muted)
        noteTree.setProperty(PROP_MUTE, true, nullptr);

    // Begin transaction
    undoManager.beginNewTransaction(actionName);
    midiNotesNode.appendChild(noteTree, &undoManager);

    DBG("ProjectState: Added MIDI note " + noteId + " to clip " + clipId);

    return noteId;
}

void ProjectState::removeMidiNote(const juce::String& clipId, const juce::String& noteId, const juce::String& actionName)
{
    auto noteTree = findMidiNote(clipId, noteId);
    if (!noteTree.isValid())
    {
        DBG("ProjectState: Cannot remove MIDI note - note not found: " + noteId);
        return;
    }

    auto parent = noteTree.getParent();
    if (parent.isValid())
    {
        undoManager.beginNewTransaction(actionName);
        parent.removeChild(noteTree, &undoManager);
        DBG("ProjectState: Removed MIDI note " + noteId + " from clip " + clipId);
    }
}

void ProjectState::moveMidiNote(const juce::String& clipId, const juce::String& noteId,
                                double newStartBeats, int newPitch, const juce::String& actionName)
{
    auto noteTree = findMidiNote(clipId, noteId);
    if (!noteTree.isValid())
    {
        DBG("ProjectState: Cannot move MIDI note - note not found: " + noteId);
        return;
    }

    // Validate new values
    int pitch = juce::jlimit(0, 127, newPitch);
    double startBeats = juce::jmax(0.0, newStartBeats);

    undoManager.beginNewTransaction(actionName);
    noteTree.setProperty(PROP_PITCH, pitch, &undoManager);
    noteTree.setProperty(PROP_START_BEATS, startBeats, &undoManager);

    DBG("ProjectState: Moved MIDI note " + noteId + " in clip " + clipId);
}

void ProjectState::quantizeClip(const juce::String& clipId, double gridBeats, const juce::String& actionName)
{
    auto notes = getMidiNotesForClip(clipId);
    if (notes.isEmpty())
        return;

    if (gridBeats <= 0.0)
    {
        DBG("ProjectState: Invalid grid size for quantization: " + juce::String(gridBeats));
        return;
    }

    undoManager.beginNewTransaction(actionName);

    for (const auto& note : notes)
    {
        // Quantize start time to nearest grid point
        double quantizedStart = std::round(note.startBeats / gridBeats) * gridBeats;
        quantizedStart = juce::jmax(0.0, quantizedStart);

        auto noteTree = findMidiNote(clipId, note.id);
        if (noteTree.isValid())
        {
            noteTree.setProperty(PROP_START_BEATS, quantizedStart, &undoManager);
        }
    }

    DBG("ProjectState: Quantized clip " + clipId + " to grid " + juce::String(gridBeats) + " beats");
}

void ProjectState::setMidiNoteVelocity(const juce::String& clipId, const juce::String& noteId,
                                        int newVelocity, const juce::String& actionName)
{
    auto noteTree = findMidiNote(clipId, noteId);
    if (!noteTree.isValid())
    {
        DBG("ProjectState: Cannot set velocity - note not found: " + noteId);
        return;
    }

    // Clamp velocity (1-127, never 0)
    int velocity = juce::jlimit(1, 127, newVelocity);

    undoManager.beginNewTransaction(actionName);
    noteTree.setProperty(PROP_VELOCITY, velocity, &undoManager);

    DBG("ProjectState: Set velocity for note " + noteId + " to " + juce::String(velocity));
}

void ProjectState::setMidiNoteLength(const juce::String& clipId, const juce::String& noteId,
                                      double newLengthBeats, const juce::String& actionName)
{
    auto noteTree = findMidiNote(clipId, noteId);
    if (!noteTree.isValid())
    {
        DBG("ProjectState: Cannot set length - note not found: " + noteId);
        return;
    }

    // Ensure positive length (minimum 0.01 beats)
    double lengthBeats = juce::jmax(0.01, newLengthBeats);

    undoManager.beginNewTransaction(actionName);
    noteTree.setProperty(PROP_LENGTH_BEATS, lengthBeats, &undoManager);

    DBG("ProjectState: Set length for note " + noteId + " to " + juce::String(lengthBeats) + " beats");
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

juce::ValueTree ProjectState::findTrackInternal(const juce::String& trackId)
{
    auto tracksNode = state.getChildWithName(ID_TRACKS);

    if (!tracksNode.isValid())
        return {};

    for (const auto& track : tracksNode)
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

    for (const auto& track : tracksNode)
    {
        if (track[PROP_ID].toString() == trackId)
            return track;
    }

    return {};
}

juce::ValueTree ProjectState::findMidiNote(const juce::String& clipId, const juce::String& noteId)
{
    auto [track, clip] = findClip(clipId);
    if (!clip.isValid())
        return {};

    auto midiNotesNode = clip.getChildWithName(ID_NOTES);
    if (!midiNotesNode.isValid())
        return {};

    for (const auto& noteTree : midiNotesNode)
    {
        if (noteTree.hasType(ID_NOTE) && noteTree[PROP_ID].toString() == noteId)
            return noteTree;
    }

    return {};
}

juce::ValueTree ProjectState::findAutomationPoint(const juce::ValueTree& envelope, const juce::String& pointId) const
{
    if (!envelope.isValid())
        return {};

    // Points are direct children of the envelope, not in a POINTS container
    for (const auto& point : envelope)
    {
        if (point.hasType(ID_POINT) && point[PROP_ID].toString() == pointId)
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

juce::ValueTree ProjectState::findClip(const juce::String& trackId, const juce::String& clipId)
{
    auto track = findTrack(trackId);
    if (!track.isValid())
        return {};

    auto clipsNode = track.getChildWithName(ID_CLIPS);
    if (!clipsNode.isValid())
        return {};

    for (const auto& clip : clipsNode)
    {
        if (clip.hasType(ID_CLIP) && clip[PROP_ID].toString() == clipId)
            return clip;
    }

    return {};
}

juce::ValueTree ProjectState::findNote(const juce::String& trackId, const juce::String& clipId, const juce::String& noteId)
{
    auto clip = findClip(trackId, clipId);
    if (!clip.isValid())
        return {};

    auto notesNode = clip.getChildWithName(ID_NOTES);
    if (!notesNode.isValid())
        return {};

    for (const auto& note : notesNode)
    {
        if (note.hasType(ID_NOTE) && note[PROP_ID].toString() == noteId)
            return note;
    }

    return {};
}

//==============================================================================
// U4.1: Clip Management (Beat-Based)
//==============================================================================

juce::String ProjectState::addClip(const juce::String& trackId,
                                    const juce::String& clipType,
                                    double startBeats,
                                    double lengthBeats,
                                    int laneIndex)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    // Validate parameters
    if (lengthBeats <= 0.0)
    {
        DBG("ProjectState: Invalid clip length: " + juce::String(lengthBeats));
        return {};
    }

    if (startBeats < 0.0)
        startBeats = 0.0;

    if (clipType != "audio" && clipType != "midi")
    {
        DBG("ProjectState: Invalid clip type: " + clipType);
        return {};
    }

    auto track = findTrack(trackId);
    if (!track.isValid())
    {
        DBG("ProjectState: Track not found: " + trackId);
        return {};
    }

    // Get or create CLIPS node
    auto clipsNode = track.getChildWithName(ID_CLIPS);
    if (!clipsNode.isValid())
    {
        clipsNode = juce::ValueTree(ID_CLIPS);
        track.appendChild(clipsNode, &undoManager);
        DBG("ProjectState: Created CLIPS node for track " + trackId);
    }

    // Generate unique clip ID
    auto clipId = generateUniqueId("clip");

    // Create clip ValueTree
    juce::ValueTree clip(ID_CLIP);
    clip.setProperty(PROP_ID, clipId, nullptr);
    clip.setProperty(PROP_TYPE, clipType, nullptr);
    clip.setProperty(PROP_START_BEATS, startBeats, nullptr);
    clip.setProperty(PROP_LENGTH_BEATS, lengthBeats, nullptr);
    clip.setProperty(PROP_LANE_INDEX, laneIndex, nullptr);

    // For MIDI clips, create empty NOTES node
    if (clipType == "midi")
    {
        clip.appendChild(juce::ValueTree(ID_NOTES), nullptr);
    }

    // Insert in sorted order by startBeats
    int insertIndex = 0;
    for (int i = 0; i < clipsNode.getNumChildren(); ++i)
    {
        auto existingClip = clipsNode.getChild(i);
        double existingStart = existingClip[PROP_START_BEATS];
        if (startBeats >= existingStart)
            insertIndex = i + 1;
        else
            break;
    }

    clipsNode.addChild(clip, insertIndex, &undoManager);

    DBG("ProjectState: Added " + clipType + " clip " + clipId + " at " + juce::String(startBeats) + " beats");
    return clipId;
}

bool ProjectState::removeClip(const juce::String& trackId, const juce::String& clipId)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    auto track = findTrack(trackId);
    if (!track.isValid())
        return false;

    auto clipsNode = track.getChildWithName(ID_CLIPS);
    if (!clipsNode.isValid())
        return false;

    for (int i = 0; i < clipsNode.getNumChildren(); ++i)
    {
        auto clip = clipsNode.getChild(i);
        if (clip.hasType(ID_CLIP) && clip[PROP_ID].toString() == clipId)
        {
            clipsNode.removeChild(i, &undoManager);
            DBG("ProjectState: Removed clip " + clipId);
            return true;
        }
    }

    return false;
}

bool ProjectState::moveClip(const juce::String& trackId,
                             const juce::String& clipId,
                             double newStartBeats)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    if (newStartBeats < 0.0)
        newStartBeats = 0.0;

    auto clip = findClip(trackId, clipId);
    if (!clip.isValid())
        return false;

    double oldStart = clip[PROP_START_BEATS];

    // Update start position
    clip.setProperty(PROP_START_BEATS, newStartBeats, &undoManager);

    // If position changed, re-sort
    if (newStartBeats != oldStart)
    {
        auto track = findTrack(trackId);
        auto clipsNode = track.getChildWithName(ID_CLIPS);

        // Find current index
        int currentIndex = -1;
        for (int i = 0; i < clipsNode.getNumChildren(); ++i)
        {
            if (clipsNode.getChild(i)[PROP_ID].toString() == clipId)
            {
                currentIndex = i;
                break;
            }
        }

        if (currentIndex >= 0)
        {
            // Remove from current position
            clipsNode.removeChild(currentIndex, &undoManager);

            // Find new sorted position
            int insertIndex = 0;
            for (int i = 0; i < clipsNode.getNumChildren(); ++i)
            {
                auto existingClip = clipsNode.getChild(i);
                double existingStart = existingClip[PROP_START_BEATS];
                if (newStartBeats >= existingStart)
                    insertIndex = i + 1;
                else
                    break;
            }

            // Re-insert at new position
            clipsNode.addChild(clip, insertIndex, &undoManager);
        }
    }

    DBG("ProjectState: Moved clip " + clipId + " to " + juce::String(newStartBeats) + " beats");
    return true;
}

bool ProjectState::resizeClip(const juce::String& trackId,
                               const juce::String& clipId,
                               double newLengthBeats)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    if (newLengthBeats <= 0.0)
    {
        DBG("ProjectState: Invalid clip length: " + juce::String(newLengthBeats));
        return false;
    }

    auto clip = findClip(trackId, clipId);
    if (!clip.isValid())
        return false;

    clip.setProperty(PROP_LENGTH_BEATS, newLengthBeats, &undoManager);

    DBG("ProjectState: Resized clip " + clipId + " to " + juce::String(newLengthBeats) + " beats");
    return true;
}

//==============================================================================
// U4.1: MIDI Note Management (Beat-Based)
//==============================================================================

juce::String ProjectState::addNote(const juce::String& trackId,
                                    const juce::String& clipId,
                                    double startBeats,
                                    double lengthBeats,
                                    int pitch,
                                    int velocity)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    // Validate parameters
    if (lengthBeats <= 0.0)
    {
        DBG("ProjectState: Invalid note length: " + juce::String(lengthBeats));
        return {};
    }

    if (startBeats < 0.0)
        startBeats = 0.0;

    pitch = juce::jlimit(0, 127, pitch);
    velocity = juce::jlimit(0, 127, velocity);

    auto clip = findClip(trackId, clipId);
    if (!clip.isValid())
    {
        DBG("ProjectState: Clip not found: " + clipId);
        return {};
    }

    // Verify it's a MIDI clip
    if (clip[PROP_TYPE].toString() != "midi")
    {
        DBG("ProjectState: Cannot add note to non-MIDI clip");
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

    // Create note ValueTree
    juce::ValueTree note(ID_NOTE);
    note.setProperty(PROP_ID, noteId, nullptr);
    note.setProperty(PROP_START_BEATS, startBeats, nullptr);
    note.setProperty(PROP_LENGTH_BEATS, lengthBeats, nullptr);
    note.setProperty(PROP_PITCH, pitch, nullptr);
    note.setProperty(PROP_VELOCITY, velocity, nullptr);

    // Insert in sorted order by startBeats, then by pitch
    int insertIndex = 0;
    for (int i = 0; i < notesNode.getNumChildren(); ++i)
    {
        auto existingNote = notesNode.getChild(i);
        double existingStart = existingNote[PROP_START_BEATS];
        int existingPitch = existingNote[PROP_PITCH];

        if (startBeats > existingStart || (startBeats == existingStart && pitch >= existingPitch))
            insertIndex = i + 1;
        else
            break;
    }

    notesNode.addChild(note, insertIndex, &undoManager);

    DBG("ProjectState: Added note " + noteId + " (pitch=" + juce::String(pitch) +
        ", start=" + juce::String(startBeats) + " beats)");
    return noteId;
}

bool ProjectState::removeNote(const juce::String& trackId,
                               const juce::String& clipId,
                               const juce::String& noteId)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    auto clip = findClip(trackId, clipId);
    if (!clip.isValid())
        return false;

    auto notesNode = clip.getChildWithName(ID_NOTES);
    if (!notesNode.isValid())
        return false;

    for (int i = 0; i < notesNode.getNumChildren(); ++i)
    {
        auto note = notesNode.getChild(i);
        if (note.hasType(ID_NOTE) && note[PROP_ID].toString() == noteId)
        {
            notesNode.removeChild(i, &undoManager);
            DBG("ProjectState: Removed note " + noteId);
            return true;
        }
    }

    return false;
}

bool ProjectState::moveNote(const juce::String& trackId,
                             const juce::String& clipId,
                             const juce::String& noteId,
                             double newStartBeats,
                             int newPitch)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    if (newStartBeats < 0.0)
        newStartBeats = 0.0;

    newPitch = juce::jlimit(0, 127, newPitch);

    auto note = findNote(trackId, clipId, noteId);
    if (!note.isValid())
        return false;

    double oldStart = note[PROP_START_BEATS];
    int oldPitch = note[PROP_PITCH];

    // Update properties
    note.setProperty(PROP_START_BEATS, newStartBeats, &undoManager);
    note.setProperty(PROP_PITCH, newPitch, &undoManager);

    // If position or pitch changed, re-sort
    if (newStartBeats != oldStart || newPitch != oldPitch)
    {
        auto clip = findClip(trackId, clipId);
        auto notesNode = clip.getChildWithName(ID_NOTES);

        // Find current index
        int currentIndex = -1;
        for (int i = 0; i < notesNode.getNumChildren(); ++i)
        {
            if (notesNode.getChild(i)[PROP_ID].toString() == noteId)
            {
                currentIndex = i;
                break;
            }
        }

        if (currentIndex >= 0)
        {
            // Remove from current position
            notesNode.removeChild(currentIndex, &undoManager);

            // Find new sorted position
            int insertIndex = 0;
            for (int i = 0; i < notesNode.getNumChildren(); ++i)
            {
                auto existingNote = notesNode.getChild(i);
                double existingStart = existingNote[PROP_START_BEATS];
                int existingPitch = existingNote[PROP_PITCH];

                if (newStartBeats > existingStart || (newStartBeats == existingStart && newPitch >= existingPitch))
                    insertIndex = i + 1;
                else
                    break;
            }

            // Re-insert at new position
            notesNode.addChild(note, insertIndex, &undoManager);
        }
    }

    DBG("ProjectState: Moved note " + noteId + " to " + juce::String(newStartBeats) +
        " beats, pitch " + juce::String(newPitch));
    return true;
}

bool ProjectState::resizeNote(const juce::String& trackId,
                               const juce::String& clipId,
                               const juce::String& noteId,
                               double newLengthBeats)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    if (newLengthBeats <= 0.0)
    {
        DBG("ProjectState: Invalid note length: " + juce::String(newLengthBeats));
        return false;
    }

    auto note = findNote(trackId, clipId, noteId);
    if (!note.isValid())
        return false;

    note.setProperty(PROP_LENGTH_BEATS, newLengthBeats, &undoManager);

    DBG("ProjectState: Resized note " + noteId + " to " + juce::String(newLengthBeats) + " beats");
    return true;
}

//==============================================================================
// U4.1: Debug Helpers
//==============================================================================

#if JUCE_DEBUG
void ProjectState::dumpClipStructureToLog() const
{
    DBG("========================================");
    DBG("ProjectState Clip & Note Structure Dump");
    DBG("========================================");

    auto tracksNode = state.getChildWithName(ProjectState::ID_TRACKS);
    if (!tracksNode.isValid())
    {
        DBG("No tracks found");
        return;
    }

    for (const auto& track : tracksNode)
    {
        if (!track.hasType(ProjectState::ID_TRACK))
            continue;

        juce::String trackId = track[ProjectState::PROP_ID].toString();
        juce::String trackName = track[ProjectState::PROP_NAME].toString();
        juce::String trackType = track[ProjectState::PROP_TYPE].toString();

        DBG("TRACK: " + trackId + " (" + trackName + ", type=" + trackType + ")");

        auto clipsNode = track.getChildWithName(ProjectState::ID_CLIPS);
        if (!clipsNode.isValid() || clipsNode.getNumChildren() == 0)
        {
            DBG("  (no clips)");
            continue;
        }

        for (const auto& clip : clipsNode)
        {
            if (!clip.hasType(ProjectState::ID_CLIP))
                continue;

            juce::String clipId = clip[ProjectState::PROP_ID].toString();
            juce::String clipType = clip[ProjectState::PROP_TYPE].toString();
            double startBeats = clip[ProjectState::PROP_START_BEATS];
            double lengthBeats = clip[ProjectState::PROP_LENGTH_BEATS];
            int laneIndex = clip[ProjectState::PROP_LANE_INDEX];

            DBG("  CLIP: " + clipId + " (type=" + clipType +
                ", start=" + juce::String(startBeats, 2) + " beats" +
                ", length=" + juce::String(lengthBeats, 2) + " beats" +
                ", lane=" + juce::String(laneIndex) + ")");

            // Show notes for MIDI clips
            if (clipType == "midi")
            {
                auto notesNode = clip.getChildWithName(ProjectState::ID_NOTES);
                if (!notesNode.isValid() || notesNode.getNumChildren() == 0)
                {
                    DBG("    (no notes)");
                    continue;
                }

                for (const auto& note : notesNode)
                {
                    if (!note.hasType(ProjectState::ID_NOTE))
                        continue;

                    juce::String noteId = note[ProjectState::PROP_ID].toString();
                    double noteStart = note[ProjectState::PROP_START_BEATS];
                    double noteLength = note[ProjectState::PROP_LENGTH_BEATS];
                    int pitch = note[ProjectState::PROP_PITCH];
                    int velocity = note[ProjectState::PROP_VELOCITY];

                    DBG("    NOTE: " + noteId +
                        " (start=" + juce::String(noteStart, 2) + " beats" +
                        ", length=" + juce::String(noteLength, 2) + " beats" +
                        ", pitch=" + juce::String(pitch) +
                        ", vel=" + juce::String(velocity) + ")");
                }
            }
        }
    }

    DBG("========================================");
}
#endif

//==============================================================================
// Track Property Management (Undoable)
//==============================================================================

void ProjectState::renameTrack(const juce::String& trackId, const juce::String& newName,
                                const juce::String& actionName)
{
    undoManager.beginNewTransaction(actionName);

    auto track = findTrack(trackId);
    if (track.isValid())
    {
        track.setProperty(PROP_NAME, newName, &undoManager);
        DBG("ProjectState: Renamed track " + trackId + " to '" + newName + "'");
    }
}

void ProjectState::setTrackVolume(const juce::String& trackId, float volumeLinear,
                                   const juce::String& actionName)
{
    undoManager.beginNewTransaction(actionName);

    auto track = findTrack(trackId);
    if (track.isValid())
    {
        track.setProperty(PROP_VOLUME, volumeLinear, &undoManager);
        DBG("ProjectState: Set track " + trackId + " volume to " + juce::String(volumeLinear));
    }
}

void ProjectState::setTrackPan(const juce::String& trackId, float pan,
                                const juce::String& actionName)
{
    undoManager.beginNewTransaction(actionName);

    auto track = findTrack(trackId);
    if (track.isValid())
    {
        track.setProperty(PROP_PAN, pan, &undoManager);
        DBG("ProjectState: Set track " + trackId + " pan to " + juce::String(pan));
    }
}

void ProjectState::setTrackMute(const juce::String& trackId, bool muted,
                                 const juce::String& actionName)
{
    undoManager.beginNewTransaction(actionName);

    auto track = findTrack(trackId);
    if (track.isValid())
    {
        track.setProperty(PROP_MUTE, muted, &undoManager);
        DBG("ProjectState: Set track " + trackId + " mute to " + (muted ? "true" : "false"));
    }
}

void ProjectState::setTrackSolo(const juce::String& trackId, bool soloed,
                                 const juce::String& actionName)
{
    undoManager.beginNewTransaction(actionName);

    auto track = findTrack(trackId);
    if (track.isValid())
    {
        track.setProperty(PROP_SOLO, soloed, &undoManager);
        DBG("ProjectState: Set track " + trackId + " solo to " + (soloed ? "true" : "false"));
    }
}

void ProjectState::setTrackArmed(const juce::String& trackId, bool armed,
                                  const juce::String& actionName)
{
    undoManager.beginNewTransaction(actionName);

    auto track = findTrack(trackId);
    if (track.isValid())
    {
        track.setProperty(PROP_ARMED, armed, &undoManager);
        DBG("ProjectState: Set track " + trackId + " armed to " + (armed ? "true" : "false"));
    }
}

//==============================================================================
// Clip Management (Undoable)
//==============================================================================

juce::String ProjectState::createClip(const juce::String& trackId, const juce::String& clipType,
                                       juce::int64 startSamples, juce::int64 lengthSamples,
                                       const juce::String& name,
                                       const juce::String& actionName)
{
    undoManager.beginNewTransaction(actionName);

    auto track = findTrack(trackId);
    if (!track.isValid())
    {
        DBG("ProjectState: Track not found: " + trackId);
        return {};
    }

    // Get or create CLIPS node
    auto clipsNode = track.getChildWithName(ID_CLIPS);
    if (!clipsNode.isValid())
    {
        clipsNode = juce::ValueTree(ID_CLIPS);
        track.appendChild(clipsNode, &undoManager);
    }

    // Generate unique clip ID
    auto clipId = generateUniqueId("clip");

    // Create clip ValueTree
    juce::ValueTree clip(ID_CLIP);
    clip.setProperty(PROP_ID, clipId, nullptr);
    clip.setProperty(PROP_NAME, name, nullptr);
    clip.setProperty(PROP_TYPE, clipType, nullptr);
    clip.setProperty(PROP_START, startSamples, nullptr);
    clip.setProperty(PROP_LENGTH, lengthSamples, nullptr);
    clip.setProperty(PROP_OFFSET, 0, nullptr);

    // Add to track
    clipsNode.appendChild(clip, &undoManager);

    DBG("ProjectState: Created clip '" + name + "' with ID " + clipId + " on track " + trackId);

    return clipId;
}

void ProjectState::deleteClip(const juce::String& trackId, const juce::String& clipId,
                               const juce::String& actionName)
{
    undoManager.beginNewTransaction(actionName);

    auto clip = getClip(trackId, clipId);
    if (clip.isValid())
    {
        auto clipsNode = clip.getParent();
        clipsNode.removeChild(clip, &undoManager);

        DBG("ProjectState: Deleted clip " + clipId + " from track " + trackId);
    }
}

void ProjectState::moveClip(const juce::String& trackId, const juce::String& clipId,
                             juce::int64 newStartSamples,
                             const juce::String& actionName)
{
    undoManager.beginNewTransaction(actionName);

    auto clip = getClip(trackId, clipId);
    if (clip.isValid())
    {
        clip.setProperty(PROP_START, newStartSamples, &undoManager);
        DBG("ProjectState: Moved clip " + clipId + " to " + juce::String(newStartSamples));
    }
}

std::pair<juce::String, juce::String> ProjectState::splitClip(const juce::String& trackId,
                                                                const juce::String& clipId,
                                                                juce::int64 splitSamples,
                                                                const juce::String& actionName)
{
    undoManager.beginNewTransaction(actionName);

    auto originalClip = getClip(trackId, clipId);
    if (!originalClip.isValid())
    {
        DBG("ProjectState: Clip not found: " + clipId);
        return {};
    }

    // Get original clip properties
    juce::String clipName = originalClip[PROP_NAME].toString();
    juce::String clipType = originalClip[PROP_TYPE].toString();
    juce::int64 clipStart = originalClip[PROP_START];
    juce::int64 clipLength = originalClip[PROP_LENGTH];
    juce::int64 clipOffset = originalClip[PROP_OFFSET];
    juce::int64 clipEnd = clipStart + clipLength;

    // Validate split position
    if (splitSamples <= clipStart || splitSamples >= clipEnd)
    {
        DBG("ProjectState: Invalid split position");
        return {};
    }

    // Calculate left and right clip properties
    juce::int64 leftLength = splitSamples - clipStart;
    juce::int64 rightLength = clipEnd - splitSamples;
    juce::int64 rightOffset = clipOffset + leftLength;

    // Create left clip
    juce::String leftClipId = createClip(trackId, clipType, clipStart, leftLength,
                                         clipName + " (L)", "");

    // Create right clip
    juce::String rightClipId = createClip(trackId, clipType, splitSamples, rightLength,
                                          clipName + " (R)", "");

    // Set offsets
    auto leftClip = getClip(trackId, leftClipId);
    auto rightClip = getClip(trackId, rightClipId);

    if (leftClip.isValid())
        leftClip.setProperty(PROP_OFFSET, clipOffset, &undoManager);

    if (rightClip.isValid())
        rightClip.setProperty(PROP_OFFSET, rightOffset, &undoManager);

    // Copy audio file property if present
    if (originalClip.hasProperty(PROP_AUDIO_FILE))
    {
        juce::String audioFile = originalClip[PROP_AUDIO_FILE].toString();
        if (leftClip.isValid())
            leftClip.setProperty(PROP_AUDIO_FILE, audioFile, &undoManager);
        if (rightClip.isValid())
            rightClip.setProperty(PROP_AUDIO_FILE, audioFile, &undoManager);
    }

    // Delete original clip
    deleteClip(trackId, clipId, "");

    DBG("ProjectState: Split clip " + clipId + " into " + leftClipId + " and " + rightClipId);

    return {leftClipId, rightClipId};
}

//==============================================================================
// MIDI Note Management
//==============================================================================

juce::ValueTree ProjectState::getOrCreateNotesContainer(const juce::String& clipId)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    auto [track, clip] = findClip(clipId);
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

    return notesNode;
}

juce::String ProjectState::addNote(const juce::String& clipId, double startBeats, double lengthBeats,
                                    int pitch, int velocity, const juce::String& actionName)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    jassert(startBeats >= 0.0);
    jassert(lengthBeats > 0.0);

    // Validate pitch and velocity
    pitch = juce::jlimit(0, 127, pitch);
    velocity = juce::jlimit(1, 127, velocity);  // 0 is note-off, so clamp to 1-127

    auto notesNode = getOrCreateNotesContainer(clipId);
    if (!notesNode.isValid())
    {
        DBG("ProjectState: Failed to get/create notes container");
        return {};
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

    // Insert in sorted order by start time, then pitch
    int insertIndex = 0;
    for (int i = 0; i < notesNode.getNumChildren(); ++i)
    {
        auto existingNote = notesNode.getChild(i);
        double existingStart = existingNote[PROP_START_BEATS];
        int existingPitch = existingNote[PROP_PITCH];

        if (startBeats > existingStart || (startBeats == existingStart && pitch >= existingPitch))
            insertIndex = i + 1;
        else
            break;
    }

    notesNode.addChild(note, insertIndex, &undoManager);

    DBG("ProjectState: Added note " + noteId + " at " + juce::String(startBeats) + " beats, pitch " + juce::String(pitch));
    return noteId;
}

bool ProjectState::moveNote(const juce::String& clipId, const juce::String& noteId,
                             double newStartBeats, double newLengthBeats,
                             int newPitch, int newVelocity, const juce::String& actionName)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    jassert(newStartBeats >= 0.0);
    jassert(newLengthBeats > 0.0);

    // Validate pitch and velocity
    newPitch = juce::jlimit(0, 127, newPitch);
    newVelocity = juce::jlimit(1, 127, newVelocity);

    auto notesNode = getNotes(clipId);
    if (!notesNode.isValid())
        return false;

    // Find note
    for (int i = 0; i < notesNode.getNumChildren(); ++i)
    {
        auto note = notesNode.getChild(i);
        if (note[PROP_ID].toString() == noteId)
        {
            double oldStart = note[PROP_START_BEATS];
            int oldPitch = note[PROP_PITCH];

            // Update properties
            note.setProperty(PROP_START_BEATS, newStartBeats, &undoManager);
            note.setProperty(PROP_LENGTH_BEATS, newLengthBeats, &undoManager);
            note.setProperty(PROP_PITCH, newPitch, &undoManager);
            note.setProperty(PROP_VELOCITY, newVelocity, &undoManager);

            // If time or pitch changed, re-sort
            if (newStartBeats != oldStart || newPitch != oldPitch)
            {
                // Remove and re-insert in sorted position
                notesNode.removeChild(i, &undoManager);

                int insertIndex = 0;
                for (int j = 0; j < notesNode.getNumChildren(); ++j)
                {
                    auto existingNote = notesNode.getChild(j);
                    double existingStart = existingNote[PROP_START_BEATS];
                    int existingPitch = existingNote[PROP_PITCH];

                    if (newStartBeats > existingStart || (newStartBeats == existingStart && newPitch >= existingPitch))
                        insertIndex = j + 1;
                    else
                        break;
                }

                notesNode.addChild(note, insertIndex, &undoManager);
            }

            DBG("ProjectState: Moved note " + noteId);
            return true;
        }
    }

    return false;
}

bool ProjectState::deleteNote(const juce::String& clipId, const juce::String& noteId,
                               const juce::String& actionName)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    auto notesNode = getNotes(clipId);
    if (!notesNode.isValid())
        return false;

    // Find and remove note
    for (int i = 0; i < notesNode.getNumChildren(); ++i)
    {
        auto note = notesNode.getChild(i);
        if (note[PROP_ID].toString() == noteId)
        {
            notesNode.removeChild(i, &undoManager);
            DBG("ProjectState: Deleted note " + noteId);
            return true;
        }
    }

    return false;
}

juce::ValueTree ProjectState::getNotes(const juce::String& clipId) const
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    auto [track, clip] = const_cast<ProjectState*>(this)->findClip(clipId);
    if (!clip.isValid())
        return {};

    return clip.getChildWithName(ID_NOTES);
}

//==============================================================================
// Tempo Map & Markers (Phase 15)
//==============================================================================

juce::String ProjectState::addTempoChange(double beatPosition, double bpm, const juce::String& actionName)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    auto tempoMap = state.getChildWithName(ID_TEMPO_MAP);
    if (!tempoMap.isValid())
    {
        tempoMap = juce::ValueTree(ID_TEMPO_MAP);
        state.addChild(tempoMap, -1, &undoManager);
    }

    // Create point
    juce::ValueTree point(ID_TEMPO_POINT);
    juce::String pointId = generateUniqueId("tempo_point");
    
    point.setProperty(PROP_ID, pointId, nullptr);
    point.setProperty(PROP_TIME_BEATS, beatPosition, nullptr);
    point.setProperty(PROP_BPM, bpm, nullptr);

    // Insert in sorted order
    int insertIndex = 0;
    for (int i = 0; i < tempoMap.getNumChildren(); ++i)
    {
        if ((double)tempoMap.getChild(i)[PROP_TIME_BEATS] > beatPosition)
        {
            insertIndex = i;
            break;
        }
        insertIndex = i + 1;
    }

    undoManager.beginNewTransaction(actionName);
    tempoMap.addChild(point, insertIndex, &undoManager);

    DBG("ProjectState: Added tempo change at " + juce::String(beatPosition) + " beats: " + juce::String(bpm) + " BPM");
    return pointId;
}

juce::ValueTree ProjectState::getTempoMap() const
{
    return state.getChildWithName(ID_TEMPO_MAP);
}

juce::String ProjectState::addMarker(double beatPosition, const juce::String& name, const juce::String& color, const juce::String& actionName)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    auto markers = state.getChildWithName(ID_MARKERS);
    if (!markers.isValid())
    {
        markers = juce::ValueTree(ID_MARKERS);
        state.addChild(markers, -1, &undoManager);
    }

    // Create marker
    juce::ValueTree marker(ID_MARKER);
    juce::String markerId = generateUniqueId("marker");
    
    marker.setProperty(PROP_ID, markerId, nullptr);
    marker.setProperty(PROP_TIME_BEATS, beatPosition, nullptr);
    marker.setProperty(PROP_NAME, name, nullptr);
    marker.setProperty(PROP_COLOR, color, nullptr);

    // Insert in sorted order
    int insertIndex = 0;
    for (int i = 0; i < markers.getNumChildren(); ++i)
    {
        if ((double)markers.getChild(i)[PROP_TIME_BEATS] > beatPosition)
        {
            insertIndex = i;
            break;
        }
        insertIndex = i + 1;
    }

    undoManager.beginNewTransaction(actionName);
    markers.addChild(marker, insertIndex, &undoManager);

    DBG("ProjectState: Added marker '" + name + "' at " + juce::String(beatPosition) + " beats");
    return markerId;
}

bool ProjectState::deleteMarker(const juce::String& markerId, const juce::String& actionName)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    auto markers = state.getChildWithName(ID_MARKERS);
    if (!markers.isValid())
        return false;

    for (int i = 0; i < markers.getNumChildren(); ++i)
    {
        if (markers.getChild(i)[PROP_ID].toString() == markerId)
        {
            undoManager.beginNewTransaction(actionName);
            markers.removeChild(i, &undoManager);
            DBG("ProjectState: Deleted marker " + markerId);
            return true;
        }
    }

    return false;
}

juce::ValueTree ProjectState::getMarkers() const
{
    return state.getChildWithName(ID_MARKERS);
}

