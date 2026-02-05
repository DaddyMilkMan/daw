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

/*
    ==============================================================================
    Original file header:
*/

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

    void setNoteExpression(const juce::String& clipId, const juce::String& noteId,
                           NoteExpressionType type,
                           const std::vector<NoteExpressionPoint>& points,
                           const juce::String& actionName = "Set Note Expression");

    std::vector<NoteExpressionPoint> getNoteExpression(
        const juce::String& clipId, const juce::String& noteId,
        NoteExpressionType type) const;

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
