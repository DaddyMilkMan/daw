/*
  ==============================================================================

    PianoRollComponent.cpp
    Created: 2025-11-14
    Author:  Zenith DAW - Phase 4: Piano Roll MIDI Editor

    Piano roll implementation

    DESIGN SYSTEM: Updated to use ZenithLookAndFeel design tokens

  ==============================================================================
*/

#include "PianoRollComponent.h"
#ifdef ZENITH_USE_SKIA
#include "../Source/ui/skia/SkiaTheme.h"
#include <include/core/SkCanvas.h>
#include <include/core/SkFont.h>
#include <include/core/SkPaint.h>
#include <include/core/SkPath.h>
#include <include/core/SkRRect.h>
#include <include/effects/SkGradientShader.h>
#endif
#include "../../include/Engine.h"
#include "../../include/ProjectState.h"
#include "../../include/TempoMap.h"
#include "../engine/Clip.h"
#include "../engine/Track.h"
#include "ZenithLookAndFeel.h" // DESIGN SYSTEM: Include for design tokens

#ifdef ZENITH_USE_SKIA
#include "skia/SkiaTheme.h"
#include <include/core/SkFont.h>
#include <include/core/SkPaint.h>
#include <include/core/SkRRect.h>
#include <include/core/SkRect.h>
#endif

//==============================================================================
// Piano RollComponent Implementation
//==============================================================================

PianoRollComponent::PianoRollComponent(zenith::Track::Clip *clipToEdit,
                                       Engine &engineRef)
    : clip(clipToEdit), engine(engineRef) {
  if (clip == nullptr) {
    DBG("PianoRollComponent: ERROR - null clip");
    return;
  }

  // Set up key listener for delete key
  addKeyListener(this);
  setWantsKeyboardFocus(true);

  // Get tempo and sample rate from Engine
  currentSampleRate = engine.getSampleRate();
  currentTempo = engine.getTempoMap().getTempoAt(0); // Start tempo for now

  // Build note cache from clip
  updateNoteCache();

  DBG("PianoRollComponent: Created for clip " + clip->getName());
}

PianoRollComponent::~PianoRollComponent() { removeKeyListener(this); }

//==============================================================================
// Component interface
//==============================================================================

#ifndef ZENITH_USE_SKIA
void PianoRollComponent::paint(juce::Graphics &g) {
  // DESIGN SYSTEM: Background using dp4 elevation
  g.fillAll(juce::Colour(zenith::ZenithLookAndFeel::Elevation::dp4));

  auto bounds = getLocalBounds();

  // Reserve left area for piano keys
  auto keysBounds = bounds.removeFromLeft(static_cast<int>(pianoKeysWidth));

  // Draw piano keys
  drawPianoKeys(g, keysBounds);

  // Draw grid in main area
  drawGrid(g, bounds);

  // Draw notes
  drawNotes(g, bounds);
}
#endif

#ifdef ZENITH_USE_SKIA
void PianoRollComponent::paintSkia(SkCanvas &canvas,
                                   const juce::Rectangle<int> &bounds) {
  auto &theme = zenith::SkiaTheme::getInstance();
  const auto &colors = theme.getColors();

  // Background
  SkPaint bgPaint;
  bgPaint.setColor(colors.bg1);
  bgPaint.setAntiAlias(true);
  canvas.drawRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()),
                  bgPaint);

  // Draw zebra striping first (subtle alternating background)
  drawZebraStripingSkia(canvas, bounds);

  // Draw grid lines
  drawGridSkia(canvas, bounds);

  // Draw piano keys on left side
  float pianoKeysX = bounds.getX();
  float pianoKeysY = bounds.getY();
  drawPianoKeysSkia(canvas, pianoKeysX, pianoKeysY, pianoKeysWidth,
                    bounds.getHeight());

  // Draw notes
  drawNotesSkia(canvas, bounds);
}
#endif

void PianoRollComponent::resized() {
  // Rebuild note cache when size changes
  updateNoteCache();
}

//==============================================================================
// Mouse interaction
//==============================================================================

void PianoRollComponent::mouseDown(const juce::MouseEvent &e) {
  // Hit-test for notes
  auto *hit = hitTestNote(e.position);

  if (hit != nullptr) {
    // Select this note
    selectedNote = hit;
    isDragging = false;
    dragStartPosition = e.position;
    noteDragStartNumber = hit->noteNumber;
    noteDragStartBeats =
        hit->startTime / (currentSampleRate * 60.0 / currentTempo);

    DBG("PianoRollComponent: Selected note " + juce::String(hit->noteNumber));

    repaint();
  } else {
    // Clicked empty space - create new note
    selectedNote = nullptr;

    // Calculate note number and start time from mouse position
    float relativeX = e.position.getX() - pianoKeysWidth;
    float relativeY = e.position.getY();

    int noteNumber = pixelsToNoteNumber(relativeY);
    double startBeats = snapToGrid(pixelsToBeats(relativeX));

    // Default note length (1/4 note)
    double lengthBeats = 0.25;

    // Create the note
    createNote(noteNumber, startBeats, lengthBeats, 100);

    DBG("PianoRollComponent: Created note " + juce::String(noteNumber) +
        " at " + juce::String(startBeats) + " beats");

    repaint();
  }
}

void PianoRollComponent::mouseDrag(const juce::MouseEvent &e) {
  if (selectedNote == nullptr)
    return;

  isDragging = true;

  // Calculate new position
  float relativeX = e.position.getX() - pianoKeysWidth;
  float relativeY = e.position.getY();

  int newNoteNumber = pixelsToNoteNumber(relativeY);
  double newStartBeats = snapToGrid(pixelsToBeats(relativeX));

  // Clamp to valid MIDI range
  newNoteNumber = juce::jlimit(0, 127, newNoteNumber);

  // Move the note
  moveNote(selectedNote, newNoteNumber, newStartBeats);

  repaint();
}

void PianoRollComponent::mouseUp(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  isDragging = false;
}

void PianoRollComponent::mouseMove(const juce::MouseEvent &e) {
  // Update hover state
  auto *hit = hitTestNote(e.position);

  if (hit != hoveredNote) {
    hoveredNote = hit;
    hoverAlpha = 0.0f;
    repaint();
  }
}

void PianoRollComponent::mouseExit(const juce::MouseEvent & /*e*/) {
  hoveredNote = nullptr;
  hoverAlpha = 0.0f;
  repaint();
}

//==============================================================================
// Timer callback
//==============================================================================

void PianoRollComponent::timerCallback() {
  bool needsRepaint = false;

  // Animate hover alpha
  if (hoveredNote != nullptr && hoverAlpha < 1.0f) {
    hoverAlpha = std::min(1.0f, hoverAlpha + 0.15f);
    needsRepaint = true;
  }

  if (needsRepaint) {
    repaint();
  }
}

//==============================================================================
// Key listener
//==============================================================================

bool PianoRollComponent::keyPressed(const juce::KeyPress &key,
                                    Component *originatingComponent) {
  juce::ignoreUnused(originatingComponent);

  if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey) {
    if (selectedNote != nullptr) {
      DBG("PianoRollComponent: Deleting note " +
          juce::String(selectedNote->noteNumber));

      deleteNote(selectedNote->noteNumber, selectedNote->startTime);
      selectedNote = nullptr;

      repaint();
      return true;
    }
  }

  return false;
}

//==============================================================================
// View control
//==============================================================================

void PianoRollComponent::setPixelsPerBeat(float ppb) {
  pixelsPerBeat = juce::jlimit(10.0f, 200.0f, ppb);
  updateNoteCache();
  repaint();
}

void PianoRollComponent::setNoteHeight(float height) {
  noteHeight = juce::jlimit(8.0f, 30.0f, height);
  updateNoteCache();
  repaint();
}

//==============================================================================
// Rendering helpers
//==============================================================================

void PianoRollComponent::drawPianoKeys(juce::Graphics &g,
                                       juce::Rectangle<int> bounds) {
  // DESIGN SYSTEM: Background using dp2 elevation
  g.setColour(juce::Colour(zenith::ZenithLookAndFeel::Elevation::dp2));
  g.fillRect(bounds);

  // Draw keys
  for (int noteNumber = highestNote; noteNumber >= lowestNote; --noteNumber) {
    float y = noteNumberToPixels(noteNumber);

    // Determine if black or white key
    int noteInOctave = noteNumber % 12;
    bool isBlackKey =
        (noteInOctave == 1 || noteInOctave == 3 || noteInOctave == 6 ||
         noteInOctave == 8 || noteInOctave == 10);

    // DESIGN SYSTEM: Draw key using elevation tokens
    if (isBlackKey)
      g.setColour(juce::Colour(
          zenith::ZenithLookAndFeel::Elevation::dp0)); // Darkest for black keys
    else
      g.setColour(juce::Colour(
          zenith::ZenithLookAndFeel::Elevation::dp8)); // Lighter for white keys

    g.fillRect(bounds.getX(), static_cast<int>(y), bounds.getWidth(),
               static_cast<int>(noteHeight));

    // DESIGN SYSTEM: Draw border using borderSubtle
    g.setColour(juce::Colour(zenith::ZenithLookAndFeel::Colors::borderSubtle));
    g.drawHorizontalLine(static_cast<int>(y), static_cast<float>(bounds.getX()),
                         static_cast<float>(bounds.getRight()));

    // Draw note name for C notes
    if (noteInOctave == 0) // C
    {
      int octave = noteNumber / 12 - 1;
      juce::String noteName = "C" + juce::String(octave);

      // DESIGN SYSTEM: Text using textSecondary
      g.setColour(
          juce::Colour(zenith::ZenithLookAndFeel::Colors::textSecondary));
      g.setFont(zenith::ZenithLookAndFeel::Typography::getTiny());
      g.drawText(noteName, bounds.getX() + 2, static_cast<int>(y),
                 bounds.getWidth() - 4, static_cast<int>(noteHeight),
                 juce::Justification::centredLeft, true);
    }
  }

  // DESIGN SYSTEM: Border using borderMedium
  g.setColour(juce::Colour(zenith::ZenithLookAndFeel::Colors::borderMedium));
  g.drawVerticalLine(bounds.getRight(), static_cast<float>(bounds.getY()),
                     static_cast<float>(bounds.getBottom()));
}

void PianoRollComponent::drawGrid(juce::Graphics &g,
                                  juce::Rectangle<int> bounds) {
  // DESIGN SYSTEM: Vertical grid lines (beats) using borderSubtle
  g.setColour(juce::Colour(zenith::ZenithLookAndFeel::Colors::borderSubtle)
                  .withAlpha(0.3f));

  int maxBeats = static_cast<int>(
      pixelsToBeats(static_cast<float>(getWidth() - pianoKeysWidth)) + 1);

  for (int beat = 0; beat <= maxBeats; ++beat) {
    float x = pianoKeysWidth + beatsToPixels(beat);

    if (x >= bounds.getX() && x <= bounds.getRight()) {
      // DESIGN SYSTEM: Thicker line every 4 beats using borderMedium
      if (beat % 4 == 0)
        g.setColour(
            juce::Colour(zenith::ZenithLookAndFeel::Colors::borderMedium)
                .withAlpha(0.5f));
      else
        g.setColour(
            juce::Colour(zenith::ZenithLookAndFeel::Colors::borderSubtle)
                .withAlpha(0.3f));

      g.drawVerticalLine(static_cast<int>(x), static_cast<float>(bounds.getY()),
                         static_cast<float>(bounds.getBottom()));
    }
  }

  // DESIGN SYSTEM: Horizontal grid lines using borderSubtle
  g.setColour(juce::Colour(zenith::ZenithLookAndFeel::Colors::borderSubtle)
                  .withAlpha(0.3f));

  for (int noteNumber = lowestNote; noteNumber <= highestNote; ++noteNumber) {
    float y = noteNumberToPixels(noteNumber);

    g.drawHorizontalLine(static_cast<int>(y), static_cast<float>(bounds.getX()),
                         static_cast<float>(bounds.getRight()));
  }
}

void PianoRollComponent::drawNotes(juce::Graphics &g,
                                   juce::Rectangle<int> bounds) {
  juce::ignoreUnused(bounds);

  for (const auto &note : noteCache) {
    // DESIGN SYSTEM: Note color using semantic colors
    if (&note == selectedNote)
      g.setColour(juce::Colour(
          zenith::ZenithLookAndFeel::Colors::warning)); // Yellow/amber for
                                                        // selected
    else
      g.setColour(juce::Colour(zenith::ZenithLookAndFeel::Colors::success)
                      .brighter(0.2f)); // Green for normal

    // Draw note rectangle
    g.fillRect(note.bounds.reduced(1.0f));

    // DESIGN SYSTEM: Border using dp0
    g.setColour(juce::Colour(zenith::ZenithLookAndFeel::Elevation::dp0));
    g.drawRect(note.bounds, 1.0f);
  }
}

//==============================================================================
// Time/pitch mapping
//==============================================================================

float PianoRollComponent::beatsToPixels(double beats) const {
  return static_cast<float>(beats * pixelsPerBeat);
}

double PianoRollComponent::pixelsToBeats(float pixels) const {
  return pixels / pixelsPerBeat;
}

int PianoRollComponent::pixelsToNoteNumber(float y) const {
  // Y=0 is highestNote, Y increases downward
  int noteNumber = highestNote - static_cast<int>(y / noteHeight);
  return juce::jlimit(lowestNote, highestNote, noteNumber);
}

float PianoRollComponent::noteNumberToPixels(int noteNumber) const {
  return (highestNote - noteNumber) * noteHeight;
}

//==============================================================================
// Note management
//==============================================================================

void PianoRollComponent::updateNoteCache() {
  noteCache.clear();

  if (clip == nullptr)
    return;

  const auto *midiSequence = clip->getMidiSequence();

  if (midiSequence == nullptr)
    return;

  // Iterate through MIDI events
  for (int i = 0; i < midiSequence->getNumEvents(); ++i) {
    const auto *event = midiSequence->getEventPointer(i);

    if (event == nullptr)
      continue;

    const auto &message = event->message;

    if (message.isNoteOn()) {
      int noteNumber = message.getNoteNumber();
      double startTime = event->message.getTimeStamp();
      int velocity = message.getVelocity();

      // Find corresponding note-off
      double endTime = startTime + 0.5; // Default duration

      for (int j = i + 1; j < midiSequence->getNumEvents(); ++j) {
        const auto *offEvent = midiSequence->getEventPointer(j);

        if (offEvent != nullptr && offEvent->message.isNoteOff() &&
            offEvent->message.getNoteNumber() == noteNumber) {
          endTime = offEvent->message.getTimeStamp();
          break;
        }
      }

      double duration = endTime - startTime;

      // Convert to screen coordinates
      double startBeats = startTime / (currentSampleRate * 60.0 / currentTempo);
      double durationBeats =
          duration / (currentSampleRate * 60.0 / currentTempo);

      float x = pianoKeysWidth + beatsToPixels(startBeats);
      float y = noteNumberToPixels(noteNumber);
      float width = beatsToPixels(durationBeats);
      float height = noteHeight;

      // Create NoteVisual
      NoteVisual visual;
      visual.noteNumber = noteNumber;
      visual.startTime = startTime;
      visual.duration = duration;
      visual.velocity = velocity;
      visual.bounds = juce::Rectangle<float>(x, y, width, height);

      noteCache.push_back(visual);
    }
  }

  DBG("PianoRollComponent: Note cache updated - " +
      juce::String(noteCache.size()) + " notes");
}

NoteVisual *PianoRollComponent::hitTestNote(juce::Point<float> position) {
  for (auto it = noteCache.rbegin(); it != noteCache.rend(); ++it) {
    if (it->bounds.contains(position)) {
      return std::to_address(it);
    }
  }

  return nullptr;
}

void PianoRollComponent::createNote(int noteNumber, double startBeats,
                                    double lengthBeats, int velocity) {
  if (clip == nullptr)
    return;

  // Convert beats to seconds
  double samplesPerBeat = currentSampleRate * 60.0 / currentTempo;
  double startTime = startBeats * samplesPerBeat / currentSampleRate;
  double endTime =
      (startBeats + lengthBeats) * samplesPerBeat / currentSampleRate;

  // Create note-on and note-off messages
  juce::MidiMessage noteOn = juce::MidiMessage::noteOn(
      1, noteNumber, static_cast<juce::uint8>(velocity));
  noteOn.setTimeStamp(startTime);

  juce::MidiMessage noteOff = juce::MidiMessage::noteOff(1, noteNumber);
  noteOff.setTimeStamp(endTime);

  // Add to clip's MIDI sequence (this modifies the clip)
  // For MVP, we directly modify the sequence
  // TODO(zenith-core#1): Use undo/redo system
  auto sequence = clip->getMidiSequence();
  if (sequence != nullptr) {
    auto newSequence = *sequence; // Copy
    newSequence.addEvent(noteOn);
    newSequence.addEvent(noteOff);
    newSequence.updateMatchedPairs();
    newSequence.sort();

    clip->setMidiSequence(newSequence);
  }

  // Rebuild cache
  updateNoteCache();
}

void PianoRollComponent::deleteNote(int noteNumber, double startTime) {
  if (clip == nullptr)
    return;

  auto sequence = clip->getMidiSequence();
  if (sequence == nullptr)
    return;

  auto newSequence = *sequence; // Copy

  // Find and remove the note-on/off pair
  const double timeEpsilon = 0.001; // Tolerance for time matching

  for (int i = newSequence.getNumEvents() - 1; i >= 0; --i) {
    const auto *event = newSequence.getEventPointer(i);

    if (event != nullptr) {
      const auto &msg = event->message;

      if ((msg.isNoteOn() || msg.isNoteOff()) &&
          msg.getNoteNumber() == noteNumber &&
          std::abs(msg.getTimeStamp() - startTime) < timeEpsilon) {
        newSequence.deleteEvent(i, false);
      }
    }
  }

  newSequence.updateMatchedPairs();
  clip->setMidiSequence(newSequence);

  // Rebuild cache
  updateNoteCache();
}

void PianoRollComponent::moveNote(NoteVisual *note, int newNoteNumber,
                                  double newStartBeats) {
  if (clip == nullptr || note == nullptr)
    return;

  // Delete old note
  deleteNote(note->noteNumber, note->startTime);

  // Create new note at new position (preserve duration and velocity)
  double durationBeats =
      note->duration / (currentSampleRate * 60.0 / currentTempo);
  createNote(newNoteNumber, newStartBeats, durationBeats, note->velocity);
}

//==============================================================================
// Grid snapping
//==============================================================================

double PianoRollComponent::snapToGrid(double beats) const {
  return std::round(beats / gridResolution) * gridResolution;
}

#ifdef ZENITH_USE_SKIA
//==============================================================================
// Skia Rendering Implementation
//==============================================================================

void PianoRollComponent::drawZebraStripingSkia(
    SkCanvas &canvas, const juce::Rectangle<int> &bounds) {
  // Zebra striping: alternating dark backgrounds for sharps/flats for visual
  // legibility This makes it easier to distinguish white keys from black keys
  // at a glance
  auto &theme = zenith::SkiaTheme::getInstance();
  const auto &colors = theme.getColors();

  SkPaint stripePaint;
  stripePaint.setAntiAlias(true);

  // Iterate through notes and draw alternating stripes
  for (int noteNumber = lowestNote; noteNumber <= highestNote; ++noteNumber) {
    int noteInOctave = noteNumber % 12;
    bool isBlackKey =
        (noteInOctave == 1 || noteInOctave == 3 || noteInOctave == 6 ||
         noteInOctave == 8 || noteInOctave == 10);

    // Only darken black key rows for zebra striping effect
    if (isBlackKey) {
      float y = noteNumberToPixels(noteNumber) +
                pianoKeysWidth; // Offset by piano width
      float height = noteHeight;

      // Subtle darkening for black key rows
      stripePaint.setColor(
          SkColorSetARGB(40, 0, 0, 0)); // Semi-transparent black

      canvas.drawRect(SkRect::MakeXYWH(pianoKeysWidth, y,
                                       bounds.getWidth() - pianoKeysWidth,
                                       height),
                      stripePaint);
    }
  }
}

void PianoRollComponent::drawPianoKeysSkia(SkCanvas &canvas, float x, float y,
                                           float width, float height) {
  auto &theme = zenith::SkiaTheme::getInstance();
  const auto &colors = theme.getColors();
  const auto &typo = theme.getTypography();

  SkPaint keyPaint;
  keyPaint.setAntiAlias(true);

  SkPaint borderPaint;
  borderPaint.setColor(colors.borderSubtle);
  borderPaint.setStrokeWidth(1.0f);
  borderPaint.setStyle(SkPaint::kStroke_Style);
  borderPaint.setAntiAlias(true);

  // Draw piano keys from highest to lowest note
  for (int noteNumber = highestNote; noteNumber >= lowestNote; --noteNumber) {
    int noteInOctave = noteNumber % 12;
    bool isBlackKey =
        (noteInOctave == 1 || noteInOctave == 3 || noteInOctave == 6 ||
         noteInOctave == 8 || noteInOctave == 10);

    float keyY = noteNumberToPixels(noteNumber);

    // Key background color
    if (isBlackKey)
      keyPaint.setColor(colors.bg0); // Dark for black keys
    else
      keyPaint.setColor(colors.bg2); // Lighter for white keys

    // Draw key rectangle
    SkRect keyRect = SkRect::MakeXYWH(x, keyY, width, noteHeight);
    canvas.drawRect(keyRect, keyPaint);

    // Draw border
    canvas.drawRect(keyRect, borderPaint);

    // Draw note name for C notes
    if (noteInOctave == 0) // C
    {
      int octave = noteNumber / 12 - 1;
      juce::String noteName = "C" + juce::String(octave);

      SkFont font;
      font.setSize(typo.tiny.size);

      SkPaint textPaint;
      textPaint.setColor(colors.textMuted);
      textPaint.setAntiAlias(true);

      auto nameStr = noteName.toStdString();
      canvas.drawString(nameStr.c_str(), x + 4, keyY + noteHeight * 0.7f, font,
                        textPaint);
    }
  }
}

void PianoRollComponent::drawGridSkia(SkCanvas &canvas,
                                      const juce::Rectangle<int> &bounds) {
  auto &theme = zenith::SkiaTheme::getInstance();
  const auto &colors = theme.getColors();

  // Vertical grid lines (beats)
  SkPaint beatLinePaint;
  beatLinePaint.setAntiAlias(true);

  int maxBeats = static_cast<int>(pixelsToBeats(bounds.getWidth()) + 1);

  for (int beat = 0; beat <= maxBeats; ++beat) {
    float x = pianoKeysWidth + beatsToPixels(beat);

    if (x >= bounds.getX() && x <= bounds.getRight()) {
      // Thicker line every 4 beats
      if (beat % 4 == 0) {
        beatLinePaint.setColor(
            SkColorSetARGB(128, SkColorGetR(colors.borderStrong),
                           SkColorGetG(colors.borderStrong),
                           SkColorGetB(colors.borderStrong)));
        beatLinePaint.setStrokeWidth(1.2f);
      } else {
        beatLinePaint.setColor(
            SkColorSetARGB(64, SkColorGetR(colors.borderSubtle),
                           SkColorGetG(colors.borderSubtle),
                           SkColorGetB(colors.borderSubtle)));
        beatLinePaint.setStrokeWidth(0.5f);
      }

      canvas.drawLine(x, bounds.getY(), x, bounds.getBottom(), beatLinePaint);
    }
  }

  // Horizontal grid lines (notes)
  SkPaint noteLinePaint;
  noteLinePaint.setColor(SkColorSetARGB(64, SkColorGetR(colors.borderSubtle),
                                        SkColorGetG(colors.borderSubtle),
                                        SkColorGetB(colors.borderSubtle)));
  noteLinePaint.setStrokeWidth(0.5f);
  noteLinePaint.setAntiAlias(true);

  for (int noteNumber = lowestNote; noteNumber <= highestNote; ++noteNumber) {
    float noteY = noteNumberToPixels(noteNumber);
    canvas.drawLine(pianoKeysWidth, noteY, bounds.getRight(), noteY,
                    noteLinePaint);
  }
}

void PianoRollComponent::drawNotesSkia(SkCanvas &canvas,
                                       const juce::Rectangle<int> &bounds) {
  auto &theme = zenith::SkiaTheme::getInstance();
  const auto &colors = theme.getColors();

  SkPaint notePaint;
  notePaint.setAntiAlias(true);

  SkPaint selectedPaint;
  selectedPaint.setColor(colors.accentAlt); // Selection color (blue)
  selectedPaint.setAntiAlias(true);

  SkPaint borderPaint;
  borderPaint.setStyle(SkPaint::kStroke_Style);
  borderPaint.setStrokeWidth(1.5f);
  borderPaint.setAntiAlias(true);

  // Draw each note
  for (const auto &note : noteCache) {
    // Determine color based on selection state
    bool isSelected = (&note == selectedNote);
    bool isHovered = (&note == hoveredNote);

    if (isSelected) {
      notePaint.setColor(colors.accentMain); // Teal for selected
      borderPaint.setColor(colors.accentAlt);
    } else if (isHovered) {
      // Lighter color with glow for hovered notes
      notePaint.setColor(SkColorSetARGB(220, SkColorGetR(colors.success),
                                        SkColorGetG(colors.success),
                                        SkColorGetB(colors.success)));
      borderPaint.setColor(colors.textStrong);
    } else {
      // Normal note color (green)
      notePaint.setColor(SkColorSetARGB(200, SkColorGetR(colors.success),
                                        SkColorGetG(colors.success),
                                        SkColorGetB(colors.success)));
      borderPaint.setColor(colors.success);
    }

    // Draw note rectangle with rounded corners
    SkRect noteRect =
        SkRect::MakeXYWH(note.bounds.getX(), note.bounds.getY(),
                         note.bounds.getWidth(), note.bounds.getHeight());

    // Rounded rectangle for softer appearance
    canvas.drawRRect(SkRRect::MakeRectXY(noteRect, 2.0f, 2.0f), notePaint);

    // Border
    canvas.drawRRect(SkRRect::MakeRectXY(noteRect, 2.0f, 2.0f), borderPaint);

    // Velocity indicator (brightness)
    if (note.velocity > 0) {
      float velocityBrightness = note.velocity / 127.0f;
      SkPaint velocityPaint;
      velocityPaint.setColor(SkColorSetARGB(
          static_cast<int>(100 * velocityBrightness), 255, 255, 255));
      velocityPaint.setAntiAlias(true);
      canvas.drawRRect(SkRRect::MakeRectXY(noteRect, 2.0f, 2.0f),
                       velocityPaint);
    }
  }
}

#endif // ZENITH_USE_SKIA

//==============================================================================
// PianoRollWindow Implementation
//==============================================================================

PianoRollWindow::PianoRollWindow(zenith::Track::Clip *clipToEdit,
                                 Engine &engineRef)
    : DocumentWindow(
          clipToEdit != nullptr ? "Piano Roll - " + clipToEdit->getName()
                                : "Piano Roll",
          juce::Colour(
              zenith::ZenithLookAndFeel::Elevation::dp2), // DESIGN SYSTEM: Use
                                                          // elevation
          DocumentWindow::allButtons),
      clip(clipToEdit), engine(engineRef) {
  if (clip == nullptr) {
    DBG("PianoRollWindow: ERROR - null clip");
    return;
  }

  // Create piano roll component
  pianoRoll = std::make_unique<PianoRollComponent>(clip, engine);

  // Set as content
  setContentNonOwned(pianoRoll.get(), true);

  // Set size
  setResizable(true, true);
  centreWithSize(1000, 600);

  // Show window
  setVisible(true);

  DBG("PianoRollWindow: Created for clip " + clip->getName());
}

PianoRollWindow::~PianoRollWindow() { clearContentComponent(); }

void PianoRollWindow::closeButtonPressed() {
  // Just delete this window
  delete this;
}
