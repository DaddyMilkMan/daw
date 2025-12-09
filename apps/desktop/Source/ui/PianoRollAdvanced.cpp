/**
 * @file PianoRollAdvanced.cpp
 * @brief Advanced features and algorithmic logic for PianoRollComponent
 */

#include "../../include/ui/PianoRollComponent.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <set>

using namespace zenith;

//==============================================================================
// Advanced Features
//==============================================================================

void PianoRollComponent::quantizeSelected(double gridSize, float strength,
                                          float swing) {
  if (getSelectedNoteCount() == 0)
    return;

  projectState.getUndoManager().beginNewTransaction("Quantize MIDI notes");

  for (auto &note : noteRects) {
    if (note.selected) {
      double originalStart = note.startBeats;
      double quantizedStart = std::round(originalStart / gridSize) * gridSize;

      // Apply swing (offset every other grid position)
      int gridIndex = static_cast<int>(std::round(quantizedStart / gridSize));
      if (gridIndex % 2 == 1 && swing != 0.0f) {
        quantizedStart += gridSize * swing * 0.5f;
      }

      // Blend between original and quantized based on strength
      double newStart =
          originalStart + (quantizedStart - originalStart) * strength;
      newStart = juce::jmax(0.0, newStart);

      if (std::abs(newStart - originalStart) > 0.001) {
        projectState.moveMidiNote(currentClip.clipId, note.id, newStart,
                                  note.pitch, "");
      }
    }
  }
}

void PianoRollComponent::humanizeVelocity(float amount) {
  if (getSelectedNoteCount() == 0)
    return;

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<float> dis(-1.0f, 1.0f);

  projectState.getUndoManager().beginNewTransaction("Humanize MIDI velocity");

  for (auto &note : noteRects) {
    if (note.selected) {
      int originalVelocity = note.velocity;

      // Random variation proportional to current velocity
      float variation = dis(gen) * amount * 30.0f; // Up to Â±30 at full amount
      int newVelocity = originalVelocity + static_cast<int>(variation);
      newVelocity = juce::jlimit(1, 127, newVelocity);

      if (newVelocity != originalVelocity) {
        projectState.setMidiNoteVelocity(currentClip.clipId, note.id,
                                         newVelocity, "");
      }
    }
  }
}

void PianoRollComponent::smartDuplicate() {
  if (getSelectedNoteCount() < 2)
    return;

  // Find time range of selection
  double minTime = std::numeric_limits<double>::max();
  double maxTime = 0.0;

  for (const auto &note : noteRects) {
    if (note.selected) {
      minTime = std::min(minTime, note.startBeats);
      maxTime = std::max(maxTime, note.startBeats + note.lengthBeats);
    }
  }

  double patternLength = maxTime - minTime;

  projectState.getUndoManager().beginNewTransaction(
      "Smart duplicate MIDI notes");

  clearSelection();

  for (const auto &note : noteRects) {
    if (note.selected) {
      zenith::ProjectState::MidiNoteSpec newNote;
      newNote.pitch = note.pitch;
      newNote.startBeats = note.startBeats + patternLength;
      newNote.lengthBeats = note.lengthBeats;
      newNote.velocity = note.velocity;
      newNote.muted = note.muted;

      projectState.addMidiNote(currentClip.clipId, newNote, "");
    }
  }
}

void PianoRollComponent::createRoll(float x, float y, double rollSpeed) {
  int pitch = pixelsToPitch(y);
  double startBeats = pixelsToBeats(x - PIANO_WIDTH);

  if (snapEnabled)
    startBeats = snapToGrid(startBeats);

  // Create 16 notes for the roll
  int numNotes = 16;
  double totalLength = rollSpeed * numNotes;

  projectState.getUndoManager().beginNewTransaction("Create MIDI roll");

  for (int i = 0; i < numNotes; ++i) {
    zenith::ProjectState::MidiNoteSpec note;
    note.pitch = pitch;
    note.startBeats = startBeats + (i * rollSpeed);
    note.lengthBeats = rollSpeed * 0.9; // Slight gap
    note.velocity = 100 - (i * 3);      // Decreasing velocity
    note.muted = false;

    projectState.addMidiNote(currentClip.clipId, note, "");
  }
}

void PianoRollComponent::toggleMuteSelected() {
  if (getSelectedNoteCount() == 0)
    return;

  projectState.getUndoManager().beginNewTransaction("Toggle MIDI note mute");

  for (auto &note : noteRects) {
    if (note.selected) {
      // Toggle mute state
      bool newMuteState = !note.muted;

      // Persist to ProjectState with undo support
      projectState.setMidiNoteMuted(currentClip.clipId, note.id, newMuteState,
                                    "");

      note.muted = newMuteState;
    }
  }

  repaint();
}

//==============================================================================
// Scale & Chord
//==============================================================================

static std::vector<int> getScaleIntervals(PianoRollComponent::ScaleType type) {
  using T = PianoRollComponent::ScaleType;
  switch (type) {
  case T::Chromatic:
    return {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
  case T::Major:
    return {0, 2, 4, 5, 7, 9, 11};
  case T::Minor:
    return {0, 2, 3, 5, 7, 8, 10};
  default:
    return {0, 2, 4, 5, 7, 9, 11};
  }
}

void PianoRollComponent::setScaleHighlight(int rootNote, ScaleType scale) {
  scaleHighlight.rootNote = rootNote;
  scaleHighlight.scale = scale;
  scaleHighlight.enabled = true;
  updateScaleHighlight();
  repaint();
}

void PianoRollComponent::clearScaleHighlight() {
  scaleHighlight.enabled = false;
  repaint();
}

void PianoRollComponent::updateScaleHighlight() {
  auto intervals = getScaleIntervals(scaleHighlight.scale);
  scaleHighlight.highlightedPitches.assign(128, false);

  if (!scaleHighlight.enabled)
    return;

  for (int i = 0; i < 128; ++i) {
    int pitchClass = (i - scaleHighlight.rootNote) % 12;
    if (pitchClass < 0)
      pitchClass += 12;

    for (int interval : intervals) {
      if (interval == pitchClass) {
        scaleHighlight.highlightedPitches[i] = true;
        break;
      }
    }
  }
}

void PianoRollComponent::setScaleLock(bool enabled) {
  scaleLockEnabled = enabled;
  if (enabled && !scaleHighlight.enabled) {
    setScaleHighlight(0, ScaleType::Major);
  }
}

int PianoRollComponent::snapPitchToScale(int pitch) const {
  if (!scaleLockEnabled || !scaleHighlight.enabled)
    return pitch;

  if (scaleHighlight.highlightedPitches[pitch])
    return pitch;

  // Search up and down
  for (int i = 1; i < 12; ++i) {
    int up = pitch + i;
    int down = pitch - i;

    if (up <= 127 && scaleHighlight.highlightedPitches[up]) {
      return up;
    }
    if (down >= 0 && scaleHighlight.highlightedPitches[down]) {
      return down;
    }
  }
  return pitch;
}

bool PianoRollComponent::isNoteInScale(int pitch) const {
  if (!scaleLockEnabled && !scaleHighlight.enabled)
    return true;
  if (scaleHighlight.highlightedPitches.empty())
    return true;
  if (pitch < 0 || pitch > 127)
    return false;
  return scaleHighlight.highlightedPitches[pitch];
}

void PianoRollComponent::updateVisiblePitches() {
  visiblePitches.clear();

  if (!foldMode) {
    return;
  }

  std::set<int> pitchSet;

  // 1. Add notes currently in clips
  for (const auto &note : noteRects) {
    pitchSet.insert(note.pitch);
  }

  // 2. Add Scale notes if Scale Lock/Highlighter is active
  if (scaleLockEnabled || scaleHighlight.enabled) {
    updateScaleHighlight(); // Ensure pitches are current
    for (int i = 0; i < 128; ++i) {
      if (scaleHighlight.highlightedPitches[i]) {
        pitchSet.insert(i);
      }
    }
  }

  // Convert to vector
  visiblePitches.assign(pitchSet.begin(), pitchSet.end());
  std::sort(visiblePitches.begin(), visiblePitches.end());
}

int PianoRollComponent::mapPitchToRow(int pitch) const {
  if (!foldMode)
    return pitch;
  auto it = std::find(visiblePitches.begin(), visiblePitches.end(), pitch);
  if (it != visiblePitches.end()) {
    return (int)std::distance(visiblePitches.begin(), it);
  }
  // If pitch not visible, map to nearest
  auto lower =
      std::lower_bound(visiblePitches.begin(), visiblePitches.end(), pitch);
  if (lower == visiblePitches.end())
    return (int)visiblePitches.size() - 1;
  return (int)std::distance(visiblePitches.begin(), lower);
}

int PianoRollComponent::mapRowToPitch(int row) const {
  if (!foldMode)
    return row;
  if (visiblePitches.empty())
    return 60;
  row = juce::jlimit(0, (int)visiblePitches.size() - 1, row);
  return visiblePitches[row];
}

//==============================================================================
// Chord Detection
//==============================================================================

juce::String
PianoRollComponent::detectChord(const std::vector<int> &pitches) const {
  if (pitches.size() < 3)
    return "";

  std::set<int> pitchClasses;
  for (int pitch : pitches)
    pitchClasses.insert(pitch % 12);

  std::vector<int> sortedClasses(pitchClasses.begin(), pitchClasses.end());
  std::sort(sortedClasses.begin(), sortedClasses.end());

  static const char *noteNames[] = {"C",  "C#", "D",  "D#", "E",  "F",
                                    "F#", "G",  "G#", "A",  "A#", "B"};

  for (int root : sortedClasses) {
    std::vector<int> intervals;
    for (int pc : sortedClasses) {
      int interval = (pc - root + 12) % 12;
      intervals.push_back(interval);
    }
    std::sort(intervals.begin(), intervals.end());

    if (intervals == std::vector<int>{0, 4, 7})
      return juce::String(noteNames[root]) + " Major";
    if (intervals == std::vector<int>{0, 3, 7})
      return juce::String(noteNames[root]) + " Minor";
  }

  return "Unknown Chord";
}

juce::String PianoRollComponent::getCurrentChordName() const {
  std::vector<int> selectedPitches;
  for (const auto &note : noteRects) {
    if (note.selected)
      selectedPitches.push_back(note.pitch);
  }

  return detectChord(selectedPitches);
}

std::vector<int> PianoRollComponent::getChordIntervals(ChordType type) {
  switch (type) {
  case ChordType::Major:
    return {0, 4, 7};
  case ChordType::Minor:
    return {0, 3, 7};
  default:
    return {0, 4, 7};
  }
}

void PianoRollComponent::insertChord(int rootPitch, ChordType type,
                                     double startBeat, double lengthBeats,
                                     int velocity) {
  if (!currentClip.isValid())
    return;

  auto intervals = getChordIntervals(type);

  projectState.getUndoManager().beginNewTransaction("Insert chord");

  for (int interval : intervals) {
    zenith::ProjectState::MidiNoteSpec note;
    note.pitch = juce::jlimit(0, 127, rootPitch + interval);
    note.startBeats = startBeat;
    note.lengthBeats = lengthBeats;
    note.velocity = velocity;
    note.muted = false;

    if (note.startBeats < currentClip.clipLengthBeats) {
      projectState.addMidiNote(currentClip.clipId, note, "");
    }
  }

  refreshNotesFromProjectState();
}

} // namespace zenith
