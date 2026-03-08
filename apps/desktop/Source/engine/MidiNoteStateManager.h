#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include "MidiNote.h"

namespace zenith {

class ProjectState;

/**
 * @brief Manages MIDI note operations for ProjectState.
 * 
 * Separates note-level CRUD and batch operations (quantize, humanize, etc.)
 * from the main ProjectState class.
 */
class MidiNoteStateManager {
public:
    explicit MidiNoteStateManager(ProjectState& projectState);
    ~MidiNoteStateManager() = default;

    // CRUD Operations
    juce::String addNote(const juce::String& clipId, const zenith::MidiNote& note, 
                         const juce::String& actionName = "Add Note");
    
    // Legacy compatible version
    juce::String addNote(const juce::String& clipId, double startBeats, double lengthBeats,
                         int pitch, int velocity, const juce::String& actionName = "Add Note");

    void addNotes(const juce::String& clipId, const std::vector<zenith::MidiNote>& notes,
                  const juce::String& actionName = "Add Notes");

    bool moveNote(const juce::String& clipId, const juce::String& noteId,
                  double newStartBeats, double newLengthBeats, int newPitch,
                  float newVelocity, const juce::String& actionName = "Move Note");

    bool deleteNote(const juce::String& clipId, const juce::String& noteId,
                    const juce::String& actionName = "Delete Note");

    // Batch Operations
    void quantizeNotes(const juce::String& clipId, double gridBeats, float strength,
                       const juce::String& actionName = "Quantize Notes");
    
    void humanizeNotes(const juce::String& clipId, double timeRangeBeats, int velRange,
                       const juce::String& actionName = "Humanize Notes");
    
    void legatoNotes(const juce::String& clipId, bool adjustOverlap,
                     const juce::String& actionName = "Legato Notes");

    // Queries
    juce::Array<zenith::MidiNote> getNotesForClip(const juce::String& clipId) const;
    juce::ValueTree getNotesContainer(const juce::String& clipId) const;
    juce::ValueTree getOrCreateNotesContainer(const juce::String& clipId);
    juce::ValueTree findNote(const juce::String& clipId, const juce::String& noteId) const;

private:
    ProjectState& projectState_;

    juce::ValueTree findClipNode(const juce::String& clipId) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiNoteStateManager)
};

} // namespace zenith
