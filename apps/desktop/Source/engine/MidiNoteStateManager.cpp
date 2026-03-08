#include "MidiNoteStateManager.h"
#include "EngineConstants.h"
#include "ProjectState.h"
#include "ZenithLogger.h"
#include <algorithm>
#include <random>

namespace zenith {

using namespace zenith::constants;

MidiNoteStateManager::MidiNoteStateManager(ProjectState& projectState)
    : projectState_(projectState)
{
}

juce::String MidiNoteStateManager::addNote(const juce::String& clipId, const zenith::MidiNote& note, 
                                          const juce::String& actionName)
{
    auto clip = findClipNode(clipId);
    if (!clip.isValid()) return {};

    auto notesNode = getOrCreateNotesContainer(clipId);
    if (!notesNode.isValid()) return {};

    juce::String noteId = note.id;
    if (noteId.isEmpty())
        noteId = "note_" + juce::Uuid().toString().substring(0, 8);

    juce::ValueTree noteTree(ProjectState::ID_NOTE);
    noteTree.setProperty(ProjectState::PROP_ID, noteId, nullptr);
    noteTree.setProperty(ProjectState::PROP_PITCH, juce::jlimit<int>(0, 127, note.pitch), nullptr);
    noteTree.setProperty(ProjectState::PROP_START_BEATS, std::max(0.0, note.startBeats), nullptr);
    noteTree.setProperty(ProjectState::PROP_LENGTH_BEATS, std::max(0.001, note.lengthBeats), nullptr);
    noteTree.setProperty(ProjectState::PROP_VELOCITY, zenith::MidiNote::toMidiVelocity(note.velocity), nullptr);
    
    if (note.muted) noteTree.setProperty(ProjectState::PROP_MUTE, true, nullptr);
    if (note.probability < 1.0f) noteTree.setProperty(ProjectState::PROP_PROBABILITY, note.probability, nullptr);
    if (note.condition.isNotEmpty()) noteTree.setProperty(ProjectState::PROP_CONDITION, note.condition, nullptr);
    if (note.recurrence.isNotEmpty()) noteTree.setProperty(ProjectState::PROP_RECURRENCE, note.recurrence, nullptr);
    if (note.articulationId != 0) noteTree.setProperty(ProjectState::PROP_ARTICULATION_ID, note.articulationId, nullptr);
    if (std::abs(note.tension) > 0.001f) noteTree.setProperty(ProjectState::PROP_NOTE_TENSION, note.tension, nullptr);

    projectState_.undoManager.beginNewTransaction(actionName);
    notesNode.appendChild(noteTree, &projectState_.undoManager);

    return noteId;
}

juce::String MidiNoteStateManager::addNote(const juce::String& clipId, double startBeats, double lengthBeats,
                                          int pitch, int velocity, const juce::String& actionName)
{
    zenith::MidiNote note;
    note.startBeats = startBeats;
    note.lengthBeats = lengthBeats;
    note.pitch = pitch;
    note.velocity = zenith::MidiNote::fromMidiVelocity(velocity);
    return addNote(clipId, note, actionName);
}

void MidiNoteStateManager::addNotes(const juce::String& clipId, const std::vector<zenith::MidiNote>& notes,
                                    const juce::String& actionName)
{
    projectState_.undoManager.beginNewTransaction(actionName);
    for (const auto& note : notes)
    {
        addNote(clipId, note, ""); // Empty action to group in transaction
    }
}

bool MidiNoteStateManager::moveNote(const juce::String& clipId, const juce::String& noteId,
                                    double newStartBeats, double newLengthBeats, int newPitch,
                                    float newVelocity, const juce::String& actionName)
{
    auto note = findNote(clipId, noteId);
    if (!note.isValid()) return false;

    projectState_.undoManager.beginNewTransaction(actionName);
    
    if (newStartBeats >= 0.0)
        note.setProperty(ProjectState::PROP_START_BEATS, newStartBeats, &projectState_.undoManager);
    
    if (newLengthBeats > 0.0)
        note.setProperty(ProjectState::PROP_LENGTH_BEATS, newLengthBeats, &projectState_.undoManager);
    
    if (newPitch >= 0 && newPitch <= 127)
        note.setProperty(ProjectState::PROP_PITCH, newPitch, &projectState_.undoManager);
    
    if (newVelocity >= 0.0f)
        note.setProperty(ProjectState::PROP_VELOCITY, zenith::MidiNote::toMidiVelocity(newVelocity), &projectState_.undoManager);

    return true;
}

bool MidiNoteStateManager::deleteNote(const juce::String& clipId, const juce::String& noteId,
                                      const juce::String& actionName)
{
    auto note = findNote(clipId, noteId);
    if (!note.isValid()) return false;

    auto parent = note.getParent();
    if (parent.isValid())
    {
        projectState_.undoManager.beginNewTransaction(actionName);
        parent.removeChild(note, &projectState_.undoManager);
        return true;
    }
    return false;
}

void MidiNoteStateManager::quantizeNotes(const juce::String& clipId, double gridBeats, float strength,
                                         const juce::String& actionName)
{
    auto container = getNotesContainer(clipId);
    if (!container.isValid() || gridBeats <= 0.0) return;

    projectState_.undoManager.beginNewTransaction(actionName);

    for (auto note : container)
    {
        if (!note.hasType(ProjectState::ID_NOTE)) continue;

        double current = note.getProperty(ProjectState::PROP_START_BEATS);
        double quantized = std::round(current / gridBeats) * gridBeats;
        double result = current + (quantized - current) * strength;
        
        note.setProperty(ProjectState::PROP_START_BEATS, result, &projectState_.undoManager);
    }
}

void MidiNoteStateManager::humanizeNotes(const juce::String& clipId, double timeRangeBeats, int velRange,
                                         const juce::String& actionName)
{
    auto container = getNotesContainer(clipId);
    if (!container.isValid()) return;

    projectState_.undoManager.beginNewTransaction(actionName);

    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<double> timeDist(-timeRangeBeats * 0.5, timeRangeBeats * 0.5);
    std::uniform_int_distribution<int> velDist(-velRange, velRange);

    for (auto note : container)
    {
        if (!note.hasType(ProjectState::ID_NOTE)) continue;

        if (timeRangeBeats > 0.0)
        {
            double current = note.getProperty(ProjectState::PROP_START_BEATS);
            note.setProperty(ProjectState::PROP_START_BEATS, std::max(0.0, current + timeDist(rng)), &projectState_.undoManager);
        }

        if (velRange > 0)
        {
            int currentVel = note.getProperty(ProjectState::PROP_VELOCITY);
            int newVel = juce::jlimit<int>(1, 127, currentVel + velDist(rng));
            note.setProperty(ProjectState::PROP_VELOCITY, newVel, &projectState_.undoManager);
        }
    }
}

void MidiNoteStateManager::legatoNotes(const juce::String& clipId, bool adjustOverlap,
                                       const juce::String& actionName)
{
    auto notes = getNotesForClip(clipId);
    if (notes.isEmpty()) return;

    std::sort(notes.begin(), notes.end(), [](const zenith::MidiNote& a, const zenith::MidiNote& b) {
        if (std::abs(a.startBeats - b.startBeats) < kEpsilonTime)
            return a.pitch < b.pitch;
        return a.startBeats < b.startBeats;
    });

    projectState_.undoManager.beginNewTransaction(actionName);

    for (int i = 0; i < notes.size() - 1; ++i)
    {
        const auto& current = notes.getReference(i);
        const auto& next = notes.getReference(i + 1);

        double distToNext = next.startBeats - current.startBeats;
        if (distToNext > 0.0)
        {
            auto noteTree = findNote(clipId, current.id);
            if (noteTree.isValid())
            {
                double currentLen = current.lengthBeats;
                if (adjustOverlap && currentLen > distToNext)
                    noteTree.setProperty(ProjectState::PROP_LENGTH_BEATS, distToNext, &projectState_.undoManager);
                else if (currentLen < distToNext)
                    noteTree.setProperty(ProjectState::PROP_LENGTH_BEATS, distToNext, &projectState_.undoManager);
            }
        }
    }
}

juce::Array<zenith::MidiNote> MidiNoteStateManager::getNotesForClip(const juce::String& clipId) const
{
    juce::Array<zenith::MidiNote> results;
    auto container = getNotesContainer(clipId);
    if (!container.isValid()) return results;

    for (auto noteNode : container)
    {
        if (!noteNode.hasType(ProjectState::ID_NOTE)) continue;

        zenith::MidiNote note;
        note.id = noteNode[ProjectState::PROP_ID].toString();
        note.pitch = noteNode[ProjectState::PROP_PITCH];
        note.startBeats = noteNode[ProjectState::PROP_START_BEATS];
        note.lengthBeats = noteNode[ProjectState::PROP_LENGTH_BEATS];
        note.velocity = zenith::MidiNote::fromMidiVelocity(static_cast<int>(noteNode[ProjectState::PROP_VELOCITY]));
        note.muted = noteNode.getProperty(ProjectState::PROP_MUTE, false);
        note.probability = noteNode.getProperty(ProjectState::PROP_PROBABILITY, 1.0f);
        note.condition = noteNode[ProjectState::PROP_CONDITION].toString();
        note.recurrence = noteNode[ProjectState::PROP_RECURRENCE].toString();
        note.articulationId = noteNode.getProperty(ProjectState::PROP_ARTICULATION_ID, 0);
        note.tension = noteNode.getProperty(ProjectState::PROP_NOTE_TENSION, 0.0f);

        results.add(note);
    }
    return results;
}

juce::ValueTree MidiNoteStateManager::getNotesContainer(const juce::String& clipId) const
{
    auto clip = findClipNode(clipId);
    if (clip.isValid())
        return clip.getChildWithName(ProjectState::ID_NOTES);
    return {};
}

juce::ValueTree MidiNoteStateManager::getOrCreateNotesContainer(const juce::String& clipId)
{
    auto clip = findClipNode(clipId);
    if (!clip.isValid()) return {};

    auto notesNode = clip.getChildWithName(ProjectState::ID_NOTES);
    if (!notesNode.isValid())
    {
        notesNode = juce::ValueTree(ProjectState::ID_NOTES);
        clip.appendChild(notesNode, &projectState_.undoManager);
    }
    return notesNode;
}

juce::ValueTree MidiNoteStateManager::findNote(const juce::String& clipId, const juce::String& noteId) const
{
    auto container = getNotesContainer(clipId);
    if (!container.isValid()) return {};

    for (auto note : container)
    {
        if (note[ProjectState::PROP_ID].toString() == noteId)
            return note;
    }
    return {};
}

juce::ValueTree MidiNoteStateManager::findClipNode(const juce::String& clipId) const
{
    auto [track, clip] = projectState_.findClip(clipId);
    return clip;
}

} // namespace zenith
