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

 * @file PianoRollTools.cpp
 * @brief Piano Roll Tools and Logic - Part of PianoRollComponent
 */


//==============================================================================
// Tool System Implementation
//==============================================================================

void PianoRollComponent::setCurrentTool(Tool tool) {
    if (currentTool != tool) {
        currentTool = tool;
        repaint();
    }
}

void PianoRollComponent::quantizeSelected(double gridSize, float strength, float swing) {
    QuantizeOptions options;
    options.gridSize = gridSize;
    options.strength = strength;
    options.swingAmount = swing;
    quantizeSelected(options);
}

void PianoRollComponent::quantizeSelected(const QuantizeOptions& options) {
    if (!currentClip.isValid()) return;
    
    projectState.getUndoManager().beginNewTransaction("Quantize Selected");
    
    // Robust grid resolution matching
    double effectiveGrid = options.gridSize > 0 ? options.gridSize : gridBeats;
    
    for (const auto& note : noteRects) {
        if (note.selected) {
            double start = note.startBeats;
            double quantized = std::round(start / effectiveGrid) * effectiveGrid;
            
            // Apply strength (interpolation)
            quantized = start + (quantized - start) * options.strength;
            
            // Simple swing implementation
            if (options.swingAmount > 0) {
                // If it's on an off-beat (approx), shift it
                double offbeatThreshold = effectiveGrid / 2.0;
                double positionInGrid = fmod(quantized, effectiveGrid * 2.0);
                if (positionInGrid > 0.001) {
                     quantized += (effectiveGrid * options.swingAmount * 0.5);
                }
            }
            
            quantized = std::max(0.0, quantized);
            if (std::abs(quantized - start) > 0.001) {
                projectState.moveMidiNote(currentClip.clipId, note.id, quantized, note.pitch, "");
            }
        }
    }
}

void PianoRollComponent::humanizeVelocity(float amount) {
    projectState.humanizeClip(currentClip.clipId, amount * 100.0f, 0.0, "Humanize Velocity");
}

void PianoRollComponent::humanizeTiming(float amount) {
    projectState.humanizeClip(currentClip.clipId, 0.0, (double)amount, "Humanize Timing");
}

void PianoRollComponent::applyVelocityCurve(VelocityCurve curve, float amount) {
    // Validate selected notes
    std::vector<NoteRect*> selectedNotes;
    for (auto& note : noteRects) {
        if (note.selected) {
            selectedNotes.push_back(&note);
        }
    }

    if (selectedNotes.empty()) {
        return;
    }

    projectState.getUndoManager().beginNewTransaction("Apply Velocity Curve");

    // Calculate range for ramp operations
    double minStart = 1e9;
    double maxStart = -1e9;
    for (const auto* note : selectedNotes) {
        minStart = std::min(minStart, note->startBeats);
        maxStart = std::max(maxStart, note->startBeats);
    }
    double range = maxStart - minStart;
    if (range < 0.001) range = 1.0;

    // Calculate average velocity for compress/expand
    int sumVel = 0;
    for (const auto* note : selectedNotes) {
        sumVel += note->velocity;
    }
    float avgVel = static_cast<float>(sumVel) / static_cast<float>(selectedNotes.size());

    // Apply curve to each selected note
    for (const auto* note : selectedNotes) {
        int newVel = note->velocity;

        switch (curve) {
            case VelocityCurve::RampUp: {
                // Velocity increases from left to right
                float t = static_cast<float>((note->startBeats - minStart) / range);
                float delta = (127.0f - static_cast<float>(note->velocity)) * amount * t;
                newVel = juce::jlimit(0, 127, static_cast<int>(note->velocity + delta));
                break;
            }
            case VelocityCurve::RampDown: {
                // Velocity decreases from left to right
                float t = static_cast<float>((note->startBeats - minStart) / range);
                float delta = static_cast<float>(note->velocity) * amount * t;
                newVel = juce::jlimit(0, 127, static_cast<int>(note->velocity - delta));
                break;
            }
            case VelocityCurve::Compress: {
                // Pull velocities toward average
                float delta = (avgVel - static_cast<float>(note->velocity)) * amount;
                newVel = juce::jlimit(0, 127, static_cast<int>(note->velocity + delta));
                break;
            }
            case VelocityCurve::Expand: {
                // Push velocities away from average
                float delta = (static_cast<float>(note->velocity) - avgVel) * amount;
                newVel = juce::jlimit(0, 127, static_cast<int>(note->velocity + delta));
                break;
            }
            case VelocityCurve::Invert: {
                // Invert velocity (high -> low, low -> high)
                newVel = juce::jlimit(0, 127, static_cast<int>(127 - note->velocity));
                break;
            }
        }

        projectState.setMidiNoteVelocity(currentClip.clipId, note->id, newVel, "");
    }
}

void PianoRollComponent::smartDuplicate() {
    if (getSelectedNoteCount() == 0) return;
    
    projectState.getUndoManager().beginNewTransaction("Smart Duplicate");
    
    // Calculate selection range
    double minStart = 1e9;
    double maxEnd = -1e9;
    for (const auto& n : noteRects) {
        if (n.selected) {
            minStart = std::min(minStart, n.startBeats);
            maxEnd = std::max(maxEnd, n.startBeats + n.lengthBeats);
        }
    }
    
    double offset = (maxEnd - minStart);
    // Align to grid if needed
    if (offset < gridBeats) offset = gridBeats;
    
    for (const auto& note : noteRects) {
        if (note.selected) {
             zenith::ProjectState::MidiNoteSpec newNote;
             newNote.id = juce::Uuid().toString();
             newNote.pitch = note.pitch;
             newNote.startBeats = note.startBeats + offset;
             newNote.lengthBeats = note.lengthBeats;
             newNote.velocity = note.velocity;
             newNote.muted = note.muted;
             projectState.addMidiNote(currentClip.clipId, newNote, "");
        }
    }
}

//==============================================================================
// Step Sequencer Logic
//==============================================================================

void PianoRollComponent::setStepSequencerMode(bool enabled) {
  stepSequencerMode = enabled;
  if (enabled) {
    currentTool = Tool::Select;
    syncStepSequencerToNotes();
  }
  repaint();
}

void PianoRollComponent::setStepSequencerRows(const std::vector<int> \u0026pitches) {
  stepSequencerRows = pitches;
  repaint();
}

void PianoRollComponent::toggleStep(int pitch, int step) {
  double stepSize = gridBeats;
  double startBeat = step * stepSize;

  bool exists = false;
  for (const auto \u0026note : noteRects) {
    if (note.pitch == pitch \u0026\u0026 std::abs(note.startBeats - startBeat) \u003c 0.01) {
      projectState.removeMidiNote(currentClip.clipId, note.id, "Toggle Step (Remove)");
      exists = true;
      break;
    }
  }

  if (!exists) {
    zenith::ProjectState::MidiNoteSpec note;
    note.id = juce::Uuid().toString();
    note.pitch = pitch;
    note.startBeats = startBeat;
    note.lengthBeats = stepSize;
    note.velocity = 100;
    note.muted = false;
    projectState.addMidiNote(currentClip.clipId, note, "Toggle Step (Add)");
  }
}

bool PianoRollComponent::getStep(int pitch, int step) const {
  double stepSize = gridBeats;
  double startBeat = step * stepSize;

  for (const auto \u0026note : noteRects) {
    if (note.pitch == pitch \u0026\u0026 std::abs(note.startBeats - startBeat) \u003c 0.01) {
      return true;
    }
  }
  return false;
}

void PianoRollComponent::syncStepSequencerToNotes() {
    // Optimization: Use a fixed-size boolean array for O(1) lookups and O(N) linear scan
    // This avoids std::set allocations and tree balancing overhead for 10k+ notes.
    std::array<bool, 128> pitchPresent = { false };
    bool anyNoteFound = false;

    for (const auto& note : noteRects) {
        if (note.pitch >= 0 && note.pitch < 128) {
            pitchPresent[static_cast<size_t>(note.pitch)] = true;
            anyNoteFound = true;
        }
    }
    
    // Always include a default range if empty (e.g. C3 octave)
    if (!anyNoteFound) {
        for (int i = 60; i <= 72; ++i) pitchPresent[i] = true;
    }

    // Reconstruct the vector in sorted order (descending for piano roll layout)
    stepSequencerRows.clear();
    stepSequencerRows.reserve(128);
    for (int i = 127; i >= 0; --i) {
        if (pitchPresent[i]) {
            stepSequencerRows.push_back(i);
        }
    }
}

void PianoRollComponent::detectNoteCollisions() {
  for (auto \u0026note : noteRects) note.hasCollision = false;
  for (size_t i = 0; i \u003c noteRects.size(); ++i) {
    for (size_t j = i + 1; j \u003c noteRects.size(); ++j) {
      auto \u0026a = noteRects[i]; auto \u0026b = noteRects[j];
      if (a.pitch == b.pitch) {
        double aEnd = a.startBeats + a.lengthBeats;
        double bEnd = b.startBeats + b.lengthBeats;
        if (a.startBeats \u003c bEnd \u0026\u0026 aEnd \u003e b.startBeats) {
          a.hasCollision = true; b.hasCollision = true;
        }
      }
    }
  }
}

void PianoRollComponent::updateVisiblePitches() {
  visiblePitches.clear();
  if (!foldMode) {
    for (int i = 0; i \u003c 128; ++i) visiblePitches.push_back(i);
  } else {
    std::set\u003cint\u003e usedPitches;
    for (const auto \u0026note : noteRects) usedPitches.insert(note.pitch);
    for (const auto \u0026ghost : ghostNotes) usedPitches.insert(ghost.pitch);
    for (int p : usedPitches) visiblePitches.push_back(p);
    std::sort(visiblePitches.begin(), visiblePitches.end(), std::greater\u003cint\u003e());
    if (visiblePitches.empty()) {
      for (int i = 72; i \u003e= 48; --i) visiblePitches.push_back(i);
    }
  }
}

int PianoRollComponent::mapPitchToRow(int pitch) const {
  if (!foldMode) return 127 - pitch;
  for (size_t i = 0; i \u003c visiblePitches.size(); ++i) {
    if (visiblePitches[i] == pitch) return static_cast\u003cint\u003e(i);
  }
  return -1;
}

int PianoRollComponent::mapRowToPitch(int row) const {
  if (!foldMode) return 127 - row;
  if (row \u003e= 0 \u0026\u0026 row \u003c static_cast\u003cint\u003e(visiblePitches.size()))
    return visiblePitches[row];
  return -1;
}

bool PianoRollComponent::isNoteInScale(int pitch) const {
  if (!scaleHighlight.enabled) return true;
  int note = pitch % 12;
  int root = scaleHighlight.rootNote % 12;
  int degree = (note - root + 12) % 12;

  static const std::map\u003cScaleType, std::vector\u003cint\u003e\u003e SCALE_INTERVALS = {
    {ScaleType::Chromatic, {0,1,2,3,4,5,6,7,8,9,10,11}},
    {ScaleType::Major, {0, 2, 4, 5, 7, 9, 11}},
    {ScaleType::Minor, {0, 2, 3, 5, 7, 8, 10}},
    {ScaleType::HarmonicMinor, {0, 2, 3, 5, 7, 8, 11}},
    {ScaleType::MelodicMinor, {0, 2, 3, 5, 7, 9, 11}},
    {ScaleType::Dorian, {0, 2, 3, 5, 7, 9, 10}},
    {ScaleType::Phrygian, {0, 1, 3, 5, 7, 8, 10}},
    {ScaleType::Lydian, {0, 2, 4, 6, 7, 9, 11}},
    {ScaleType::Mixolydian, {0, 2, 4, 5, 7, 9, 10}},
    {ScaleType::Aeolian, {0, 2, 3, 5, 7, 8, 10}},
    {ScaleType::Locrian, {0, 1, 3, 5, 6, 8, 10}},
    {ScaleType::MajorPentatonic,{0, 2, 4, 7, 9}},
    {ScaleType::MinorPentatonic,{0, 3, 5, 7, 10}},
    {ScaleType::MajorBlues, {0, 2, 3, 4, 7, 9}},
    {ScaleType::MinorBlues, {0, 3, 5, 6, 7, 10}},
    {ScaleType::WholeTone, {0, 2, 4, 6, 8, 10}},
    {ScaleType::Augmented, {0, 3, 4, 7, 8, 11}},
    {ScaleType::DoubleHarmonic, {0, 1, 4, 5, 7, 8, 11}},
    {ScaleType::SpanishGypsy, {0, 1, 4, 5, 7, 8, 10}},
    {ScaleType::Prometheus, {0, 2, 4, 6, 9, 10}}
  };

  auto it = SCALE_INTERVALS.find(scaleHighlight.scale);
  if (it == SCALE_INTERVALS.end()) return true;
  for (int interval : it-\u003esecond) if (degree == interval) return true;
  return false;
}

//==============================================================================
// Chord \u0026 Scale Analysis
//==============================================================================

std::vector\u003cint\u003e PianoRollComponent::getChordIntervals(ChordType type) {
  switch (type) {
  case ChordType::Major: return {0, 4, 7};
  case ChordType::Minor: return {0, 3, 7};
  case ChordType::Diminished: return {0, 3, 6};
  case ChordType::Augmented: return {0, 4, 8};
  case ChordType::Major7: return {0, 4, 7, 11};
  case ChordType::Minor7: return {0, 3, 7, 10};
  case ChordType::Dominant7: return {0, 4, 7, 10};
  case ChordType::Diminished7: return {0, 3, 6, 9};
  case ChordType::Sus2: return {0, 2, 7};
  case ChordType::Sus4: return {0, 5, 7};
  case ChordType::Add9: return {0, 4, 7, 14};
  case ChordType::Minor9: return {0, 3, 7, 10, 14};
  case ChordType::Power: return {0, 7};
  case ChordType::Sixth: return {0, 4, 7, 9};
  case ChordType::Minor6: return {0, 3, 7, 9};
  default: return {0, 4, 7};
  }
}

void PianoRollComponent::insertChord(int rootPitch, ChordType type,
                                     double startBeat, double lengthBeats,
                                     int velocity) {
  if (!currentClip.isValid()) return;
  projectState.getUndoManager().beginNewTransaction("Insert Chord");
  auto intervals = getChordIntervals(type);
  for (int interval : intervals) {
    int pitch = rootPitch + interval;
    if (pitch \u003e 127) continue;
    zenith::ProjectState::MidiNoteSpec note;
    note.id = juce::Uuid().toString();
    note.pitch = pitch;
    note.startBeats = startBeat;
    note.lengthBeats = lengthBeats;
    note.velocity = velocity;
    note.muted = false;
    projectState.addMidiNote(currentClip.clipId, note, "");
  }
  repaint();
}

juce::String PianoRollComponent::detectChord(const std::vector\u003cint\u003e \u0026pitches) const {
  if (pitches.empty()) return "";
  if (pitches.size() == 1) {
    static const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    return noteNames[pitches[0] % 12];
  }
  
  std::vector\u003cint\u003e sortedPitches = pitches;
  std::sort(sortedPitches.begin(), sortedPitches.end());
  
  struct ChordDef { juce::String name; std::vector\u003cint\u003e intervals; };
  static const std::vector\u003cChordDef\u003e CHORD_DEFS = {
    {"Major", {0, 4, 7}}, {"Minor", {0, 3, 7}}, {"Dim", {0, 3, 6}},
    {"Aug", {0, 4, 8}}, {"Sus2", {0, 2, 7}}, {"Sus4", {0, 5, 7}},
    {"Maj7", {0, 4, 7, 11}}, {"Min7", {0, 3, 7, 10}}, {"Dom7", {0, 4, 7, 10}}
  };
  
  static const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
  for (size_t i = 0; i \u003c sortedPitches.size(); ++i) {
    int root = sortedPitches[i];
    int rootClass = root % 12;
    std::set\u003cint\u003e presentIntervals;
    for (int p : sortedPitches) {
        int intervalClass = (p - root + 120) % 12; 
        presentIntervals.insert(intervalClass);
    }
    for (const auto\u0026 def : CHORD_DEFS) {
      bool match = true;
      for (int req : def.intervals) {
        if (presentIntervals.find(req % 12) == presentIntervals.end()) {
          match = false; break;
        }
      }
      if (match \u0026\u0026 presentIntervals.size() == def.intervals.size()) {
          juce::String chordName = noteNames[rootClass];
          chordName += " " + def.name;
          if (sortedPitches[0] != root) chordName += "/" + juce::String(noteNames[sortedPitches[0]%12]);
          return chordName;
      }
    }
  }
  return "Unknown";
}

juce::String PianoRollComponent::getCurrentChordName() const {
  std::vector\u003cint\u003e pitches;
  for (const auto \u0026note : noteRects) {
    if (note.selected) pitches.push_back(note.pitch);
  }
  return detectChord(pitches);
}

//==============================================================================
// Advanced Transformations
//==============================================================================

void PianoRollComponent::applyLegato() {
    if (!currentClip.isValid()) return;
    projectState.getUndoManager().beginNewTransaction("Apply Legato");
    
    // Sort selected notes by pitch and then start time
    std::vector\u003cNoteRect*\u003e selected;
    for (auto\u0026 n : noteRects) if (n.selected) selected.push_back(\u0026n);
    
    std::sort(selected.begin(), selected.end(), [](NoteRect* a, NoteRect* b) {
        if (a-\u003epitch != b-\u003epitch) return a-\u003epitch \u003c b-\u003epitch;
        return a-\u003estartBeats \u003c b-\u003estartBeats;
    });
    
    for (size_t i = 0; i \u003c selected.size(); ++i) {
        if (i \u003c selected.size() - 1 \u0026\u0026 selected[i]-\u003epitch == selected[i+1]-\u003epitch) {
            double nextStart = selected[i+1]-\u003estartBeats;
            double newLength = nextStart - selected[i]-\u003estartBeats;
            if (newLength \u003e 0.001)
                projectState.setMidiNoteLength(currentClip.clipId, selected[i]-\u003eid, newLength, "");
        }
    }
}

void PianoRollComponent::toggleMuteSelected() {
    if (!currentClip.isValid()) return;
    projectState.getUndoManager().beginNewTransaction("Toggle Mute");
    for (const auto\u0026 note : noteRects) {
        if (note.selected) {
            projectState.setMidiNoteMuted(currentClip.clipId, note.id, !note.muted, "");
        }
    }
}

//==============================================================================
// Note Editing Operations
//==============================================================================

void PianoRollComponent::createNoteAtPosition(float x, float y) {
  int pitch = pixelsToPitch(y);
  double startBeats = pixelsToBeats(x - PIANO_WIDTH);
  if (snapEnabled)
    startBeats = snapToGrid(startBeats);
  startBeats = juce::jmax(0.0, startBeats);
  pitch = juce::jlimit(0, 127, pitch);
  double lengthBeats = gridBeats;

  zenith::ProjectState::MidiNoteSpec note;
  note.id = juce::Uuid().toString();
  note.pitch = pitch;
  note.startBeats = startBeats;
  note.lengthBeats = lengthBeats;
  note.velocity = 100;
  note.muted = false;

  projectState.addMidiNote(currentClip.clipId, note, "Create MIDI note");

  for (auto &n : noteRects) {
    if (n.id == note.id) {
      selectNote(\u0026n, false);
      break;
    }
  }
}

void PianoRollComponent::deleteSelectedNotes() {
  if (!currentClip.isValid() || getSelectedNoteCount() == 0)
    return;
  std::vector\u003cjuce::String\u003e selectedIds;
  for (const auto \u0026note : noteRects) {
    if (note.selected)
      selectedIds.push_back(note.id);
  }
  projectState.getUndoManager().beginNewTransaction("Delete MIDI notes");
  for (const auto \u0026noteId : selectedIds) {
    projectState.removeMidiNote(currentClip.clipId, noteId, "");
  }
}

//==============================================================================
// Copy/Paste/Cut
//==============================================================================

void PianoRollComponent::copySelectedNotes() {
  clipboard.clear();
  if (getSelectedNoteCount() == 0)
    return;
  double earliestTime = std::numeric_limits\u003cdouble\u003e::max();
  for (const auto \u0026note : noteRects) {
    if (note.selected)
      earliestTime = std::min(earliestTime, note.startBeats);
  }
  clipboardReferenceTime = earliestTime;
  for (const auto \u0026note : noteRects) {
    if (note.selected) {
      ClipboardNote clipNote;
      clipNote.pitch = note.pitch;
      clipNote.startBeats = note.startBeats - earliestTime;
      clipNote.lengthBeats = note.lengthBeats;
      clipNote.velocity = note.velocity;
      clipNote.muted = note.muted;
      clipboard.push_back(clipNote);
    }
  }
}

void PianoRollComponent::pasteNotes() {
  if (!currentClip.isValid() || clipboard.empty())
    return;
  double pasteTime = viewStartBeats;
  projectState.getUndoManager().beginNewTransaction("Paste MIDI notes");
  clearSelection();
  for (const auto \u0026clipNote : clipboard) {
    zenith::ProjectState::MidiNoteSpec note;
    note.pitch = clipNote.pitch;
    note.startBeats = pasteTime + clipNote.startBeats;
    note.lengthBeats = clipNote.lengthBeats;
    note.velocity = clipNote.velocity;
    note.muted = clipNote.muted;
    projectState.addMidiNote(currentClip.clipId, note, "");
  }
}

void PianoRollComponent::cutSelectedNotes() {
  copySelectedNotes();
  deleteSelectedNotes();
}

void PianoRollComponent::commitArpeggiator() {
  if (!arpPreviewEnabled || arpPreviewNotes.empty() || !currentClip.isValid()) return;

  projectState.getUndoManager().beginNewTransaction("Apply Arpeggiator");
  std::vector\u003cjuce::String\u003e idsToRemove;
  for (const auto \u0026n : noteRects) if (n.selected) idsToRemove.push_back(n.id);
  for (const auto \u0026id : idsToRemove) projectState.removeMidiNote(currentClip.clipId, id, "");

  for (const auto \u0026p : arpPreviewNotes) {
    zenith::ProjectState::MidiNoteSpec note;
    note.id = juce::Uuid().toString();
    note.pitch = p.pitch;
    note.startBeats = p.startBeats;
    note.lengthBeats = p.lengthBeats;
    note.velocity = p.velocity;
    note.muted = false;
    projectState.addMidiNote(currentClip.clipId, note, "");
  }
  setArpeggiatorPreview(false);
}

//==============================================================================
// Probability Tool Implementation
//==============================================================================

void PianoRollComponent::setNoteProbability(float probability) {
  probability = juce::jlimit(0.0f, 1.0f, probability);
  projectState.getUndoManager().beginNewTransaction("Set Probability");
  for (auto\u0026 note : noteRects) {
    if (note.selected) {
      projectState.setMidiNoteProbability(currentClip.clipId, note.id, probability, "");
    }
  }
}

float PianoRollComponent::getNoteProbability(const juce::String\u0026 noteId) const {
  // This would ideally fetch from ProjectState or cached NoteRects
  for (const auto\u0026 n : noteRects) if (n.id == noteId) return n.probability;
  return 1.0f;
}

void PianoRollComponent::randomizeProbabilities(float minProb, float maxProb) {
  juce::Random random;
  projectState.getUndoManager().beginNewTransaction("Randomize Probabilities");
  for (auto\u0026 note : noteRects) {
    if (note.selected) {
      float p = minProb + random.nextFloat() * (maxProb - minProb);
      projectState.setMidiNoteProbability(currentClip.clipId, note.id, p, "");
    }
  }
}

} // namespace zenith
