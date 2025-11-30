/**
 * @file PianoRollEditor.cpp
 * @brief Piano roll editor implementation
 */

// POLISH: spacing normalized to 8px grid (note labels at Typography.tiny)

#include "../../include/ui/PianoRollEditor.h"

#ifdef ZENITH_USE_SKIA
#include "../ui/skia/SkiaTheme.h"
#include <include/core/SkCanvas.h>
#include <include/core/SkFont.h>
#include <include/core/SkPaint.h>
#include <include/core/SkRRect.h>
#endif

PianoRollEditor::PianoRollEditor(ProjectState &ps, const juce::String &tid,
                                 const juce::String &cid)
    : projectState(ps), trackId(tid), clipId(cid) {
  // Find clip node
  auto track = projectState.getTrack(trackId);
  if (track.isValid()) {
    auto clipsNode = track.getChildWithName(ProjectState::ID_CLIPS);
    if (clipsNode.isValid()) {
      for (const auto &clip : clipsNode) {
        if (clip[ProjectState::PROP_ID].toString() == clipId) {
          clipNode = clip;
          break;
        }
      }
    }
  }

  if (clipNode.isValid()) {
    clipNode.addListener(this);
  }

  // Add ruler
  addAndMakeVisible(ruler);
  ruler.setVisibleRange(viewStartBeat, viewLengthBeats);

  setSize(800, 600);
}

PianoRollEditor::~PianoRollEditor() {
  if (clipNode.isValid())
    clipNode.removeListener(this);
}

#ifdef ZENITH_USE_SKIA
void PianoRollEditor::drawSkia(SkCanvas* canvas) {
  if (!canvas) return;

  auto bounds = getLocalBounds();
  auto &theme = ::zenith::SkiaTheme::getInstance();
  auto &colors = theme.getColors();
  auto &typo = theme.getTypography();

  // POLISH: Flat background using bg1
  SkPaint bgPaint;
  bgPaint.setAntiAlias(true);
  bgPaint.setColor(colors.bg1);
  canvas->drawRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()),
                  bgPaint);

  int numNotes = highestNote - lowestNote + 1;

  // POLISH: Piano keys on left using bg2/bg3
  SkFont labelFont;
  labelFont.setSize(typo.tiny.size);
  labelFont.setEdging(SkFont::Edging::kAntiAlias);

  for (int i = 0; i < numNotes; ++i) {
    int noteNumber = highestNote - i;
    float y = RULER_HEIGHT + i * noteHeight;

    int noteInOctave = noteNumber % 12;
    bool isBlackKey =
        (noteInOctave == 1 || noteInOctave == 3 || noteInOctave == 6 ||
         noteInOctave == 8 || noteInOctave == 10);

    // Piano key background
    SkPaint keyPaint;
    keyPaint.setAntiAlias(true);
    keyPaint.setColor(isBlackKey ? colors.bg3 : colors.bg2);
    canvas->drawRect(SkRect::MakeXYWH(0, y, PIANO_WIDTH, noteHeight), keyPaint);

    // Key border
    SkPaint keyBorderPaint;
    keyBorderPaint.setAntiAlias(true);
    keyBorderPaint.setColor(colors.borderSubtle);
    keyBorderPaint.setStrokeWidth(1.0f);
    canvas->drawLine(0, y + noteHeight, PIANO_WIDTH, y + noteHeight,
                    keyBorderPaint);

    // POLISH: C note labels using Typography.tiny
    if (noteInOctave == 0) {
      SkPaint textPaint;
      textPaint.setAntiAlias(true);
      textPaint.setColor(colors.textMuted);

      juce::String label = "C" + juce::String(noteNumber / 12 - 2);
      canvas->drawString(label.toRawUTF8(), 4,
                        y + noteHeight * 0.5f + typo.tiny.size * 0.5f,
                        labelFont, textPaint);
    }
  }

  // POLISH: Grid background using bg1/bg2 alternating
  for (int i = 0; i < numNotes; ++i) {
    int noteNumber = highestNote - i;
    float y = RULER_HEIGHT + i * noteHeight;
    int noteInOctave = noteNumber % 12;

    // Highlight C rows slightly
    SkPaint rowPaint;
    rowPaint.setAntiAlias(true);
    rowPaint.setColor(noteInOctave == 0 ? colors.bg2 : colors.bg1);
    canvas->drawRect(SkRect::MakeXYWH(PIANO_WIDTH, y,
                                     bounds.getWidth() - PIANO_WIDTH,
                                     noteHeight),
                    rowPaint);

    // Row separator using borderSubtle
    SkPaint separatorPaint;
    separatorPaint.setAntiAlias(true);
    separatorPaint.setColor(colors.borderSubtle);
    separatorPaint.setStrokeWidth(1.0f);
    canvas->drawLine(PIANO_WIDTH, y + noteHeight, bounds.getWidth(),
                    y + noteHeight, separatorPaint);
  }

  // Beat grid using borderSubtle/borderStrong
  int startBeat = static_cast<int>(std::floor(viewStartBeat));
  int endBeat = static_cast<int>(std::ceil(viewStartBeat + viewLengthBeats));

  for (int beat = startBeat; beat <= endBeat; ++beat) {
    int x = getXForBeat(beat);

    if (x < PIANO_WIDTH || x > bounds.getWidth())
      continue;

    SkPaint gridPaint;
    gridPaint.setAntiAlias(true);
    gridPaint.setStrokeWidth(1.0f);

    // Measure lines (every 4 beats) using borderStrong
    if (beat % 4 == 0) {
      gridPaint.setColor(colors.borderStrong);
    } else {
      gridPaint.setColor(colors.borderSubtle);
    }

    canvas->drawLine(x, RULER_HEIGHT, x, bounds.getHeight(), gridPaint);
  }

  // POLISH: Draw MIDI notes as rounded rects using theme waveformMidi color
  if (!clipNode.isValid())
    return;

  auto notesNode = clipNode.getChildWithName(ProjectState::ID_NOTES);
  if (!notesNode.isValid())
    return;

  for (const auto &note : notesNode) {
    double startBeats = note[ProjectState::PROP_START_BEATS];
    double lengthBeats = note[ProjectState::PROP_LENGTH_BEATS];
    int noteNumber = note[ProjectState::PROP_PITCH];
    int x = getXForBeat(startBeats);
    int y = getYForNote(noteNumber);
    int width = static_cast<int>(lengthBeats * pixelsPerBeat);

    if (x + width < PIANO_WIDTH || x > bounds.getWidth())
      continue;

    SkRect noteRect = SkRect::MakeXYWH(x, y, width, noteHeight - 2);
    SkRRect noteRRect = SkRRect::MakeRectXY(noteRect, 4.0f, 4.0f);

    // Note fill using waveformMidi color
    SkPaint noteFillPaint;
    noteFillPaint.setAntiAlias(true);
    noteFillPaint.setColor(SkColorSetARGB(180, SkColorGetR(colors.waveformMidi),
                                          SkColorGetG(colors.waveformMidi),
                                          SkColorGetB(colors.waveformMidi)));
    canvas->drawRRect(noteRRect, noteFillPaint);

    // Note border
    SkPaint noteBorderPaint;
    noteBorderPaint.setAntiAlias(true);
    noteBorderPaint.setColor(colors.waveformMidi);
    noteBorderPaint.setStyle(SkPaint::kStroke_Style);
    noteBorderPaint.setStrokeWidth(1.0f);
    canvas->drawRRect(noteRRect, noteBorderPaint);
  }
}
#else
void PianoRollEditor::paint(juce::Graphics &g) {
  auto bounds = getLocalBounds();
  bounds.removeFromTop(RULER_HEIGHT);
  bounds.removeFromLeft(PIANO_WIDTH);

  // Background
  g.fillAll(juce::Colour(0xff1e1e1e));

  // Piano keys (left side)
  int numNotes = highestNote - lowestNote + 1;
  for (int i = 0; i < numNotes; ++i) {
    int noteNumber = highestNote - i;
    int y = RULER_HEIGHT + i * noteHeight;

    // Determine if black or white key
    int noteInOctave = noteNumber % 12;
    bool isBlackKey =
        (noteInOctave == 1 || noteInOctave == 3 || noteInOctave == 6 ||
         noteInOctave == 8 || noteInOctave == 10);

    // Piano key color
    if (isBlackKey)
      g.setColour(juce::Colour(0xff3a3a3a));
    else
      g.setColour(juce::Colour(0xff2a2a2a));

    g.fillRect(0, y, PIANO_WIDTH, noteHeight);

    // Key border
    g.setColour(juce::Colour(0xff404040));
    g.drawLine(0, y + noteHeight - 1.0f, static_cast<float>(PIANO_WIDTH),
               y + noteHeight - 1.0f, 1.0f);

    // Note name for C notes
    if (noteInOctave == 0) {
      g.setColour(juce::Colours::white);
      g.setFont(juce::FontOptions(10.0f));
      g.drawText("C" + juce::String(noteNumber / 12 - 2), 2, y, PIANO_WIDTH - 4,
                 noteHeight, juce::Justification::centredLeft, false);
    }
  }

  // Grid background
  for (int i = 0; i < numNotes; ++i) {
    int noteNumber = highestNote - i;
    int y = RULER_HEIGHT + i * noteHeight;
    int noteInOctave = noteNumber % 12;

    // Alternating row colors (highlight C rows)
    if (noteInOctave == 0)
      g.setColour(juce::Colour(0xff252525));
    else
      g.setColour(juce::Colour(0xff1e1e1e));

    g.fillRect(PIANO_WIDTH, y, getWidth() - PIANO_WIDTH, noteHeight);

    // Row separator
    g.setColour(juce::Colour(0xff303030));
    g.drawLine(PIANO_WIDTH, y + noteHeight - 1.0f,
               static_cast<float>(getWidth()), y + noteHeight - 1.0f, 1.0f);
  }

  // Beat grid
  g.setColour(juce::Colour(0xff303030));
  int startBeat = static_cast<int>(std::floor(viewStartBeat));
  int endBeat = static_cast<int>(std::ceil(viewStartBeat + viewLengthBeats));

  for (int beat = startBeat; beat <= endBeat; ++beat) {
    int x = getXForBeat(beat);

    if (x < PIANO_WIDTH || x > getWidth())
      continue;

    // Measure lines (every 4 beats)
    if (beat % 4 == 0) {
      g.setColour(juce::Colour(0xff404040));
      g.drawLine(static_cast<float>(x), RULER_HEIGHT, static_cast<float>(x),
                 static_cast<float>(getHeight()), 1.0f);
    } else {
      g.setColour(juce::Colour(0xff303030));
      g.drawLine(static_cast<float>(x), RULER_HEIGHT, static_cast<float>(x),
                 static_cast<float>(getHeight()), 1.0f);
    }
  }

  // Draw MIDI notes
  if (!clipNode.isValid())
    return;

  auto notesNode = clipNode.getChildWithName(ProjectState::ID_NOTES);
  if (!notesNode.isValid())
    return;

  g.setColour(juce::Colours::green.withAlpha(0.7f));

  for (const auto &note : notesNode) {
    double startBeats = note[ProjectState::PROP_START_BEATS];
    double lengthBeats = note[ProjectState::PROP_LENGTH_BEATS];
    int noteNumber = note[ProjectState::PROP_PITCH];

    int x = getXForBeat(startBeats);
    int y = getYForNote(noteNumber);
    int width = static_cast<int>(lengthBeats * pixelsPerBeat);

    if (x + width < PIANO_WIDTH || x > getWidth())
      continue;

    // Note rectangle
    g.fillRoundedRectangle(static_cast<float>(x), static_cast<float>(y),
                           static_cast<float>(width),
                           static_cast<float>(noteHeight - 2), 2.0f);

    // Note border
    g.setColour(juce::Colours::green);
    g.drawRoundedRectangle(static_cast<float>(x), static_cast<float>(y),
                           static_cast<float>(width),
                           static_cast<float>(noteHeight - 2), 2.0f, 1.0f);

    g.setColour(juce::Colours::green.withAlpha(0.7f));
  }
}
#endif
void PianoRollEditor::mouseDown(const juce::MouseEvent &event) {
  if (!event.mods.isLeftButtonDown())
    return;

  if (event.x < PIANO_WIDTH || event.y < RULER_HEIGHT)
    return;

  int noteNumber = getNoteAtY(event.y);
  double beat = getBeatAtX(event.x);

  // Check if clicking on existing note
  draggedNote = findNoteAtPosition(event.x, event.y);

  if (draggedNote.isValid()) {
    // Start dragging existing note
    dragStartPos = event.getPosition();
    dragStartBeats = draggedNote[ProjectState::PROP_START_BEATS];
    dragStartNoteNumber = draggedNote[ProjectState::PROP_PITCH];
  } else {
    // Add std::make_unique<note>(snap to grid)
    double snappedBeat = std::round(beat * 4.0) / 4.0; // Snap to 1/4 beat
    double noteLength = 1.0;                           // Default 1 beat

    projectState.addNote(clipId, snappedBeat, noteLength, noteNumber, 100,
                         "Add Note");
    DBG("Added note " + juce::String(noteNumber) + " at beat " +
        juce::String(snappedBeat));
  }
}

void PianoRollEditor::mouseDrag(const juce::MouseEvent &event) {
  if (!draggedNote.isValid())
    return;

  // Calculate delta in beats and semitones
  int deltaX = event.x - dragStartPos.x;
  int deltaY = event.y - dragStartPos.y;

  double deltaBeat = deltaX / pixelsPerBeat;
  int deltaSemitones = -deltaY / noteHeight;

  double newStartBeats = juce::jmax(0.0, dragStartBeats + deltaBeat);
  int newNoteNumber =
      juce::jlimit(0, 127, dragStartNoteNumber + deltaSemitones);

  // Snap to grid
  newStartBeats = std::round(newStartBeats * 4.0) / 4.0;

  // Update note in state
  double lengthBeats = draggedNote[ProjectState::PROP_LENGTH_BEATS];
  int velocity = draggedNote[ProjectState::PROP_VELOCITY];
  juce::String noteId = draggedNote[ProjectState::PROP_ID].toString();

  // Set note number directly (moveNote doesn't change pitch)
  draggedNote.setProperty(ProjectState::PROP_PITCH, newNoteNumber,
                          &projectState.getUndoManager());

  projectState.moveNote(clipId, noteId, newStartBeats, lengthBeats,
                        newNoteNumber, velocity, "Move Note");
}

void PianoRollEditor::mouseDoubleClick(const juce::MouseEvent &event) {
  if (event.x < PIANO_WIDTH || event.y < RULER_HEIGHT)
    return;

  // Find and delete note
  auto note = findNoteAtPosition(event.x, event.y);
  if (note.isValid()) {
    juce::String noteId = note[ProjectState::PROP_ID].toString();
    projectState.deleteNote(clipId, noteId, "Delete Note");
    DBG("Deleted note " + noteId);
  }
}

juce::String PianoRollEditor::getTooltip() { return "Piano Roll Editor"; }

//==========================================================================
// ValueTree::Listener implementation
//==========================================================================

void PianoRollEditor::valueTreePropertyChanged(
    juce::ValueTree &tree, const juce::Identifier &property) {
  repaint();
}

void PianoRollEditor::valueTreeChildAdded(juce::ValueTree &parent,
                                          juce::ValueTree &child) {
  repaint();
}

void PianoRollEditor::valueTreeChildRemoved(juce::ValueTree &parent,
                                            juce::ValueTree &child, int index) {
  repaint();
}

//==========================================================================
// Helper methods
//==========================================================================

int PianoRollEditor::getNoteAtY(int y) const {
  if (y < RULER_HEIGHT)
    return -1;

  int noteY = y - RULER_HEIGHT;
  int noteIndex = noteY / noteHeight;

  return highestNote - noteIndex;
}

double PianoRollEditor::getBeatAtX(int x) const {
  if (x < PIANO_WIDTH)
    return 0.0;

  int gridX = x - PIANO_WIDTH;
  return viewStartBeat + (gridX / pixelsPerBeat);
}

int PianoRollEditor::getYForNote(int noteNumber) const {
  int noteIndex = highestNote - noteNumber;
  return RULER_HEIGHT + noteIndex * noteHeight + 1;
}

int PianoRollEditor::getXForBeat(double beat) const {
  return PIANO_WIDTH + static_cast<int>((beat - viewStartBeat) * pixelsPerBeat);
}

juce::ValueTree PianoRollEditor::findNoteAtPosition(int x, int y) {
  if (!clipNode.isValid())
    return {};

  auto notesNode = clipNode.getChildWithName(ProjectState::ID_NOTES);
  if (!notesNode.isValid())
    return {};

  int noteNumber = getNoteAtY(y);
  double beat = getBeatAtX(x);

  for (const auto &note : notesNode) {
    int nn = note[ProjectState::PROP_PITCH];
    double startBeats = note[ProjectState::PROP_START_BEATS];
    double lengthBeats = note[ProjectState::PROP_LENGTH_BEATS];

    if (nn == noteNumber && beat >= startBeats &&
        beat < startBeats + lengthBeats) {
      return note;
    }
  }

  return {};
}

void PianoRollEditor::resized() {
  auto bounds = getLocalBounds();

  // Position ruler at top
  ruler.setBounds(bounds.removeFromTop(32));

  // Remaining space for piano roll content
  // (Override in subclass if needed for additional layout)
}
