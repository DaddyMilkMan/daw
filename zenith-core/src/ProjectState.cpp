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
const juce::Identifier ProjectState::ID_MIDI_NOTES("MIDI_NOTES");  // Phase 8
const juce::Identifier ProjectState::ID_MIDI_NOTE("MIDI_NOTE");    // Phase 8

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

// Phase 8: MIDI note properties
const juce::Identifier ProjectState::PROP_PITCH("pitch");
const juce::Identifier ProjectState::PROP_START_BEATS("startBeats");
const juce::Identifier ProjectState::PROP_LENGTH_BEATS("lengthBeats");
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
// MIDI Note Management (Phase 8)
//==============================================================================

juce::Array<ProjectState::MidiNoteSpec> ProjectState::getMidiNotesForClip(const juce::String& clipId) const
{
    juce::Array<MidiNoteSpec> notes;

    auto clip = findClip(clipId);
    if (!clip.isValid())
        return notes;

    auto midiNotesNode = clip.getChildWithName(ID_MIDI_NOTES);
    if (!midiNotesNode.isValid())
        return notes;

    for (auto noteTree : midiNotesNode)
    {
        if (!noteTree.hasType(ID_MIDI_NOTE))
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
    auto clip = findClip(clipId);
    if (!clip.isValid())
    {
        DBG("ProjectState: Cannot add MIDI note - clip not found: " + clipId);
        return {};
    }

    // Get or create MIDI_NOTES container
    auto midiNotesNode = clip.getChildWithName(ID_MIDI_NOTES);
    if (!midiNotesNode.isValid())
    {
        midiNotesNode = juce::ValueTree(ID_MIDI_NOTES);
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
    juce::ValueTree noteTree(ID_MIDI_NOTE);
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

juce::ValueTree ProjectState::findClip(const juce::String& clipId)
{
    auto tracksNode = state.getChildWithName(ID_TRACKS);
    if (!tracksNode.isValid())
        return {};

    // Search through all tracks
    for (auto track : tracksNode)
    {
        auto clipsNode = track.getChildWithName(ID_CLIPS);
        if (!clipsNode.isValid())
            continue;

        // Search through clips in this track
        for (auto clip : clipsNode)
        {
            if (clip[PROP_ID].toString() == clipId)
                return clip;
        }
    }

    return {};
}

juce::ValueTree ProjectState::findClip(const juce::String& clipId) const
{
    auto tracksNode = state.getChildWithName(ID_TRACKS);
    if (!tracksNode.isValid())
        return {};

    // Search through all tracks
    for (auto track : tracksNode)
    {
        auto clipsNode = track.getChildWithName(ID_CLIPS);
        if (!clipsNode.isValid())
            continue;

        // Search through clips in this track
        for (auto clip : clipsNode)
        {
            if (clip[PROP_ID].toString() == clipId)
                return clip;
        }
    }

    return {};
}

juce::ValueTree ProjectState::findMidiNote(const juce::String& clipId, const juce::String& noteId)
{
    auto clip = findClip(clipId);
    if (!clip.isValid())
        return {};

    auto midiNotesNode = clip.getChildWithName(ID_MIDI_NOTES);
    if (!midiNotesNode.isValid())
        return {};

    for (auto noteTree : midiNotesNode)
    {
        if (noteTree.hasType(ID_MIDI_NOTE) && noteTree[PROP_ID].toString() == noteId)
            return noteTree;
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
