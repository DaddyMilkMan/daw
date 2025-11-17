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

// U4.1: Beat-based clip properties
const juce::Identifier ProjectState::PROP_START_BEATS("startBeats");
const juce::Identifier ProjectState::PROP_LENGTH_BEATS("lengthBeats");
const juce::Identifier ProjectState::PROP_LANE_INDEX("laneIndex");

// U4.1: MIDI note properties
const juce::Identifier ProjectState::PROP_PITCH("pitch");
const juce::Identifier ProjectState::PROP_VELOCITY("velocity");

// Phase 13: Automation properties
const juce::Identifier ProjectState::PROP_PARAM("param");
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

    for (auto note : notesNode)
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

    auto tracksNode = state.getChildWithName(ID_TRACKS);
    if (!tracksNode.isValid())
    {
        DBG("No tracks found");
        return;
    }

    for (auto track : tracksNode)
    {
        if (!track.hasType(ID_TRACK))
            continue;

        juce::String trackId = track[PROP_ID].toString();
        juce::String trackName = track[PROP_NAME].toString();
        juce::String trackType = track[PROP_TYPE].toString();

        DBG("TRACK: " + trackId + " (" + trackName + ", type=" + trackType + ")");

        auto clipsNode = track.getChildWithName(ID_CLIPS);
        if (!clipsNode.isValid() || clipsNode.getNumChildren() == 0)
        {
            DBG("  (no clips)");
            continue;
        }

        for (auto clip : clipsNode)
        {
            if (!clip.hasType(ID_CLIP))
                continue;

            juce::String clipId = clip[PROP_ID].toString();
            juce::String clipType = clip[PROP_TYPE].toString();
            double startBeats = clip[PROP_START_BEATS];
            double lengthBeats = clip[PROP_LENGTH_BEATS];
            int laneIndex = clip[PROP_LANE_INDEX];

            DBG("  CLIP: " + clipId + " (type=" + clipType +
                ", start=" + juce::String(startBeats, 2) + " beats" +
                ", length=" + juce::String(lengthBeats, 2) + " beats" +
                ", lane=" + juce::String(laneIndex) + ")");

            // Show notes for MIDI clips
            if (clipType == "midi")
            {
                auto notesNode = clip.getChildWithName(ID_NOTES);
                if (!notesNode.isValid() || notesNode.getNumChildren() == 0)
                {
                    DBG("    (no notes)");
                    continue;
                }

                for (auto note : notesNode)
                {
                    if (!note.hasType(ID_NOTE))
                        continue;

                    juce::String noteId = note[PROP_ID].toString();
                    double noteStart = note[PROP_START_BEATS];
                    double noteLength = note[PROP_LENGTH_BEATS];
                    int pitch = note[PROP_PITCH];
                    int velocity = note[PROP_VELOCITY];

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
