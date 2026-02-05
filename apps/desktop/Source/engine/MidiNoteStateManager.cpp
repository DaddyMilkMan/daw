#include "MidiNoteStateManager.h"
#include "EngineConstants.h"
#include "ProjectState.h"
#include "ZenithLogger.h"
#include <algorithm>
#include <random>

namespace zenith {

using namespace zenith::constants;

namespace {
juce::ValueTree getOrCreateExpressionsNode(juce::ValueTree noteTree,
                                           juce::UndoManager& undoManager) {
    auto expressions = noteTree.getChildWithName(ProjectState::ID_EXPRESSIONS);
    if (!expressions.isValid()) {
        expressions = juce::ValueTree(ProjectState::ID_EXPRESSIONS);
        noteTree.addChild(expressions, -1, &undoManager);
    }
    return expressions;
}

juce::ValueTree findExpressionNode(const juce::ValueTree& expressionsNode,
                                   NoteExpressionType type) {
    const int typeValue = static_cast<int>(type);
    for (auto child : expressionsNode) {
        if (child.hasType(ProjectState::ID_EXPRESSION) &&
            static_cast<int>(child.getProperty(ProjectState::PROP_EXPRESSION_TYPE, -1)) == typeValue) {
            return child;
        }
    }
    return {};
}

void clearExpressionPoints(juce::ValueTree expressionNode,
                           juce::UndoManager& undoManager) {
    auto pointsNode = expressionNode.getChildWithName(ProjectState::ID_POINTS);
    if (!pointsNode.isValid())
        return;
    pointsNode.removeAllChildren(&undoManager);
}

void writeExpressionPoints(juce::ValueTree expressionNode,
                           const std::vector<NoteExpressionPoint>& points,
                           juce::UndoManager& undoManager) {
    auto pointsNode = expressionNode.getChildWithName(ProjectState::ID_POINTS);
    if (!pointsNode.isValid()) {
        pointsNode = juce::ValueTree(ProjectState::ID_POINTS);
        expressionNode.addChild(pointsNode, -1, &undoManager);
    }

    std::vector<NoteExpressionPoint> sorted = points;
    std::sort(sorted.begin(), sorted.end(),
              [](const NoteExpressionPoint& a, const NoteExpressionPoint& b) {
                  return a.timeOffset < b.timeOffset;
              });

    for (const auto& pt : sorted) {
        juce::ValueTree pointNode(ProjectState::ID_POINT);
        pointNode.setProperty(ProjectState::PROP_ID,
                              "exprpt_" + juce::Uuid().toString().substring(0, 8),
                              nullptr);
        pointNode.setProperty(ProjectState::PROP_TIME_OFFSET,
                              juce::jmax(0.0, pt.timeOffset), nullptr);
        pointNode.setProperty(ProjectState::PROP_VALUE,
                              juce::jlimit(0.0f, 1.0f, pt.value), nullptr);
        pointNode.setProperty(ProjectState::PROP_TENSION,
                              juce::jlimit(-1.0f, 1.0f, pt.tension), nullptr);
        pointsNode.addChild(pointNode, -1, &undoManager);
    }
}

NoteExpressionMap readExpressionMap(const juce::ValueTree& noteNode) {
    NoteExpressionMap map;
    auto expressionsNode = noteNode.getChildWithName(ProjectState::ID_EXPRESSIONS);
    if (!expressionsNode.isValid())
        return map;

    for (auto exprNode : expressionsNode) {
        if (!exprNode.hasType(ProjectState::ID_EXPRESSION))
            continue;
        int typeValue = static_cast<int>(exprNode.getProperty(ProjectState::PROP_EXPRESSION_TYPE, -1));
        if (typeValue < 0 || typeValue >= static_cast<int>(NoteExpressionType::Count))
            continue;

        auto pointsNode = exprNode.getChildWithName(ProjectState::ID_POINTS);
        if (!pointsNode.isValid())
            continue;

        auto& series = map[static_cast<size_t>(typeValue)];
        for (auto pointNode : pointsNode) {
            if (!pointNode.hasType(ProjectState::ID_POINT))
                continue;
            NoteExpressionPoint pt;
            pt.timeOffset = pointNode.getProperty(ProjectState::PROP_TIME_OFFSET,
                                                  pointNode.getProperty(ProjectState::PROP_TIME_BEATS, 0.0));
            pt.value = pointNode.getProperty(ProjectState::PROP_VALUE, 0.0f);
            pt.tension = pointNode.getProperty(ProjectState::PROP_TENSION, 0.0f);
            series.push_back(pt);
        }

        if (!series.empty()) {
            std::sort(series.begin(), series.end(),
                      [](const NoteExpressionPoint& a, const NoteExpressionPoint& b) {
                          return a.timeOffset < b.timeOffset;
                      });
        }
    }
    return map;
}
} // namespace

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

    for (size_t typeIndex = 0; typeIndex < note.expressions.size(); ++typeIndex) {
        const auto& series = note.expressions[typeIndex];
        if (series.empty())
            continue;
        auto expressionsNode = getOrCreateExpressionsNode(noteTree, projectState_.undoManager);
        juce::ValueTree expressionNode(ProjectState::ID_EXPRESSION);
        expressionNode.setProperty(ProjectState::PROP_EXPRESSION_TYPE,
                                   static_cast<int>(typeIndex), nullptr);
        expressionsNode.addChild(expressionNode, -1, &projectState_.undoManager);
        writeExpressionPoints(expressionNode, series, projectState_.undoManager);
    }

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

void MidiNoteStateManager::setNoteExpression(
    const juce::String& clipId, const juce::String& noteId,
    NoteExpressionType type,
    const std::vector<NoteExpressionPoint>& points,
    const juce::String& actionName) {
    auto note = findNote(clipId, noteId);
    if (!note.isValid())
        return;

    projectState_.undoManager.beginNewTransaction(actionName);

    auto expressionsNode = getOrCreateExpressionsNode(note, projectState_.undoManager);
    auto existing = findExpressionNode(expressionsNode, type);
    if (points.empty()) {
        if (existing.isValid())
            expressionsNode.removeChild(existing, &projectState_.undoManager);
        return;
    }

    if (!existing.isValid()) {
        existing = juce::ValueTree(ProjectState::ID_EXPRESSION);
        existing.setProperty(ProjectState::PROP_EXPRESSION_TYPE,
                             static_cast<int>(type), nullptr);
        expressionsNode.addChild(existing, -1, &projectState_.undoManager);
    } else {
        clearExpressionPoints(existing, projectState_.undoManager);
    }

    writeExpressionPoints(existing, points, projectState_.undoManager);
}

std::vector<NoteExpressionPoint> MidiNoteStateManager::getNoteExpression(
    const juce::String& clipId, const juce::String& noteId,
    NoteExpressionType type) const {
    auto note = findNote(clipId, noteId);
    if (!note.isValid())
        return {};

    auto expressionsNode = note.getChildWithName(ProjectState::ID_EXPRESSIONS);
    if (!expressionsNode.isValid())
        return {};

    auto expressionNode = findExpressionNode(expressionsNode, type);
    if (!expressionNode.isValid())
        return {};

    auto pointsNode = expressionNode.getChildWithName(ProjectState::ID_POINTS);
    if (!pointsNode.isValid())
        return {};

    std::vector<NoteExpressionPoint> points;
    for (auto pointNode : pointsNode) {
        if (!pointNode.hasType(ProjectState::ID_POINT))
            continue;
        NoteExpressionPoint pt;
        pt.timeOffset = pointNode.getProperty(ProjectState::PROP_TIME_OFFSET,
                                              pointNode.getProperty(ProjectState::PROP_TIME_BEATS, 0.0));
        pt.value = pointNode.getProperty(ProjectState::PROP_VALUE, 0.0f);
        pt.tension = pointNode.getProperty(ProjectState::PROP_TENSION, 0.0f);
        points.push_back(pt);
    }

    if (!points.empty()) {
        std::sort(points.begin(), points.end(),
                  [](const NoteExpressionPoint& a, const NoteExpressionPoint& b) {
                      return a.timeOffset < b.timeOffset;
                  });
    }
    return points;
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
        note.expressions = readExpressionMap(noteNode);

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
