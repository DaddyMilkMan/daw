/**
 * @file PianoRollComponent.h
 * @brief Professional-grade MIDI Piano Roll Editor
 *
 * FEATURES:
 * ✅ Core Editing: Create, move, delete, resize notes
 * ✅ Multi-Selection: Ctrl-click, marquee, batch operations
 * ✅ Velocity Editing: Lane + gradient visualization
 * ✅ Copy/Paste: Full clipboard support
 * ✅ Quantize: With strength & swing
 * ✅ Smart Duplicate: Pattern-aware duplication
 * ✅ Velocity Curves: Ramp, compress, humanize
 * ✅ Chord Detection: Real-time chord naming
 * ✅ Scale Highlighting: Visual scale guide
 * ✅ Note Muting: Per-note mute toggle
 * ✅ Batched Undo: Proper multi-operation undo/redo
 * ✅ Cursor Feedback: Context-aware cursors
 * ✅ Note Color by Velocity: Visual dynamics
 *
 * Architecture:
 * - Single unified implementation
 * - All edits through ProjectState with batched undo support
 * - ValueTree reactive (auto-refresh on changes)
 */

#pragma once

#include "../../Source/ui/skia/SkiaComponent.h"
#include "DrumPadComponent.h"
#include "ProjectState.h"
#include <functional>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <map>
#include <memory>
#include <set>
#include <vector>

#include <core/SkCanvas.h>
#include <core/SkColor.h>
#include <core/SkPaint.h>
#include <core/SkRect.h>

//==============================================================================
/**
 * @struct MidiClipContext
 * @brief Identifies which MIDI clip is being edited
 */
struct MidiClipContext {
  juce::String clipId;
  juce::String trackId;
  double clipStartBeats = 0.0;
  double clipLengthBeats = 4.0;
  juce::String clipName = "Untitled Clip";

  bool isValid() const { return clipId.isNotEmpty(); }
};

//==============================================================================
/**
 * @class PianoRollComponent
 * @brief Professional-grade MIDI piano roll editor component
 */
class PianoRollComponent : public zenith::SkiaComponent,
                           private juce::ValueTree::Listener {
public:
  //==========================================================================
  // Internal Note Representation (Public for API)
  //==========================================================================

  struct NoteRect {
    juce::String id;
    juce::String ownerClipId; // For Multi-Clip Editing
    int pitch;
    double startBeats;
    double lengthBeats;
    int velocity;
    bool muted;
    bool selected;
    float probability = 1.0f; // New Probability Field
    juce::String condition;   // Logic Operator Text
    juce::String recurrence;  // Recurrence Text
    int articulationId = 0;   // Articulation icon index

    juce::Rectangle<float> bounds;
    juce::Rectangle<float> velocityBounds;

    // Visual state
    bool isHovered = false;
    bool hasCollision =
        false; // True if this note overlaps with another note on same pitch

    NoteRect()
        : pitch(60), startBeats(0.0), lengthBeats(1.0), velocity(100),
          muted(false), selected(false), probability(1.0f) {}
  };

  //==========================================================================
  explicit PianoRollComponent(zenith::ProjectState &state);
  ~PianoRollComponent() override;

  //==========================================================================
  // Clip Management
  //==========================================================================

  void setClipContext(const MidiClipContext &context);
  const MidiClipContext &getClipContext() const { return currentClip; }

  //==========================================================================
  // Component Interface
  //==========================================================================

  void resized() override;

  // Skia Rendering
  void drawSkia(SkCanvas *canvas) override;
  void drawModernToolbar(SkCanvas *canvas);

  void mouseDown(const juce::MouseEvent &e) override;
  void mouseDrag(const juce::MouseEvent &e) override;
  void mouseUp(const juce::MouseEvent &e) override;
  void mouseMove(const juce::MouseEvent &e) override;
  void mouseWheelMove(const juce::MouseEvent &e,
                      const juce::MouseWheelDetails &wheel) override;
  void mouseDoubleClick(const juce::MouseEvent &e) override;

  bool keyPressed(
      const juce::KeyPress &key) override; // from SkiaComponent/Component

  juce::MouseCursor getMouseCursor() override; // from SkiaComponent/Component

  //==========================================================================
  // Public API - Advanced Features
  //==========================================================================

  /** Quantize selected notes with strength and swing */
  void quantizeSelected(double gridSize, float strength = 1.0f,
                        float swing = 0.0f);

  /** Humanize velocities of selected notes */
  void humanizeVelocity(float amount = 0.3f);

  /** Apply velocity curve to selected notes */
  enum class VelocityCurve { RampUp, RampDown, Compress, Expand, Invert };
  void applyVelocityCurve(VelocityCurve curve, float amount = 1.0f);

  //==========================================================================
  // Tool System
  //==========================================================================

  /** Available editing tools */
  enum class Tool {
    Select, // Selection and manipulation of existing notes
    Draw,   // Create notes on click
    Erase,  // Delete notes on click
    Slice   // Split notes at cursor position
  };

  /** Set current editing tool */
  void setCurrentTool(Tool tool);
  Tool getCurrentTool() const { return currentTool; }

  /** Piano key interaction */
  void playPianoKey(int pitch, int velocity = 100);
  void stopPianoKey(int pitch);
  int getHoveredPianoKey() const { return hoveredPianoKey; }

  /** Duplicate selected notes with smart offset */
  void smartDuplicate();

  /** Create note roll/trill */
  void createRoll(float x, float y, double rollSpeed);

  /** Set scale highlighting */
  /** Set scale highlighting */
  enum class ScaleType {
    Chromatic,
    Major,
    Minor,
    HarmonicMinor,
    MelodicMinor,
    // Modes
    Dorian,
    Phrygian,
    Lydian,
    Mixolydian,
    Aeolian,
    Locrian,
    // Pentatonic
    MajorPentatonic,
    MinorPentatonic,
    Egyptian,
    ManGong,
    Ritusen,
    Hirajoshi,
    InSen,
    Iwato,
    YoScale,
    // Blues
    MajorBlues,
    MinorBlues,
    // Symmetrical
    WholeTone,
    DiminishedHalfWhole,
    DiminishedWholeHalf,
    Augmented,
    Prometheus,
    // Bebop
    BebopMajor,
    BebopMinor,
    BebopDominant,
    BebopDorian,
    // Exotic / World
    HungarianMinor,
    HungarianMajor,
    Bhairav,
    Byzantine,
    Persian,
    Arabian,
    Japanese,
    Chinese,
    Balinese,
    NeapolitanMajor,
    NeapolitanMinor,
    Enigmatic,
    DoubleHarmonic,
    SpanishGypsy,
    Algierian
  };
  void setScaleHighlight(int rootNote, ScaleType scale);
  void clearScaleHighlight();

  /** Toggle note mute */
  void toggleMuteSelected();

  //==========================================================================
  // Groove Templates (Preset Quantization Patterns)
  //==========================================================================

  enum class GrooveTemplate {
    Straight,  // No swing
    Swing8th,  // 8th note swing (57%)
    Swing16th, // 16th note swing (57%)
    Shuffle,   // Triplet feel (66%)
    MPC,       // MPC-style swing
    JDilla,    // J Dilla feel (slight late)
    HipHop,    // Hip hop groove
    Funk       // Funk groove
  };

  /** Apply a groove template to selected notes */
  void applyGrooveTemplate(GrooveTemplate groove, float strength = 1.0f);

  /** Get groove template swing amount */
  static float getGrooveSwingAmount(GrooveTemplate groove);

  /** Get/set current groove for display */
  GrooveTemplate getCurrentGroove() const { return currentGroove; }
  void setCurrentGroove(GrooveTemplate groove) { currentGroove = groove; }

  //==========================================================================
  // Multi-Clip Context (Logic/Ableton style)
  //==========================================================================

  std::vector<MidiClipContext> multiClipContexts; // Additional clips
  void addMultiClipContext(const MidiClipContext &clip);
  void clearMultiClipContexts();

  //==========================================================================
  // Riff Machine (Generative)
  //==========================================================================

  struct RiffSettings {
    ScaleType scale = ScaleType::Minor;
    int rootNote = 0;
    float density = 0.5f;   // Note density
    float variation = 0.3f; // Randomness
    bool useChords = false;
    bool useArps = true;
  };

  void generateRiff(RiffSettings settings);

  //==========================================================================
  // Ghost Notes (Legacy - replaced by Multi-Clip Editing)
  //==========================================================================

  /** Enable ghost notes from other clips */
  void setGhostNotesEnabled(bool enabled);
  bool getGhostNotesEnabled() const { return ghostNotesEnabled; }

  /** Set ghost note opacity (0.0 - 1.0) */
  void setGhostNoteOpacity(float opacity);
  float getGhostNoteOpacity() const { return ghostNoteOpacity; }

  /** Add a clip to show as ghost notes */
  void addGhostClip(const juce::String &clipId);
  void removeGhostClip(const juce::String &clipId);
  void clearGhostClips();

  //==========================================================================
  // Note Collision Detection
  //==========================================================================

  /** Check for overlapping notes and update visual state */
  void detectNoteCollisions();

  /** Get count of notes with collisions */
  int getCollisionCount() const;

  //==========================================================================
  // Note Preview Audio
  //==========================================================================

  /** Enable/disable note preview on create/hover */
  void setNotePreviewEnabled(bool enabled) { notePreviewEnabled = enabled; }
  bool getNotePreviewEnabled() const { return notePreviewEnabled; }

  /** Set preview velocity (0-127) */
  void setPreviewVelocity(int velocity) {
    previewVelocity = juce::jlimit(1, 127, velocity);
  }

  /** Trigger a note preview */
  void previewNote(int pitch, int velocity = 100);
  void stopNotePreview();

  /** Set callback for note preview audio (pitch, velocity, noteOn) */
  void setNotePreviewCallback(std::function<void(int, int, bool)> callback) {
    notePreviewCallback = callback;
  }

  //==========================================================================
  // MIDI Input Recording
  //==========================================================================

  /** Enable/disable MIDI input recording */
  void setMidiInputEnabled(bool enabled);
  bool getMidiInputEnabled() const { return midiInputEnabled; }

  /** Handle incoming MIDI note (from external MIDI device) */
  void handleMidiNoteOn(int pitch, int velocity);
  void handleMidiNoteOff(int pitch);

  /** Update playhead position from engine (call periodically) */
  void updatePlayheadPosition(double beats);

  //==========================================================================
  // Note Probability
  //==========================================================================

  /** Set probability (0.0-1.0) for selected notes to play */
  void setNoteProbability(float probability);

  /** Get probability for a specific note */
  float getNoteProbability(const juce::String &noteId) const;

  /** Randomize probabilities for selected notes within a range */
  void randomizeProbabilities(float minProb, float maxProb);

  //==========================================================================
  // MPE / Expression Lanes
  //==========================================================================

  enum class ExpressionType {
    PitchBend, // Per-note pitch bend
    Pressure,  // Aftertouch/Pressure
    Slide,     // Slide/Timbre (CC74)
    Expression // Expression pedal (CC11)
  };

  /** Enable/disable expression lane display */
  void setExpressionLaneVisible(ExpressionType type, bool visible);
  bool getExpressionLaneVisible(ExpressionType type) const;

  /** Expression automation point for a note */
  struct ExpressionPoint {
    double timeOffset;    // Offset from note start (0.0 = note start)
    float value;          // 0.0 to 1.0
    float tension = 0.0f; // Bezier curve tension (-1.0 to 1.0, 0 = linear)
  };

  /** Set expression automation points for a note */
  void setNoteExpression(const juce::String &noteId, ExpressionType type,
                         const std::vector<ExpressionPoint> &points);

  /** Get expression automation points for a note */
  std::vector<ExpressionPoint> getNoteExpression(const juce::String &noteId,
                                                 ExpressionType type) const;

  //==========================================================================
  // MIDI CC Lanes (Standard MIDI Control Change)
  //==========================================================================

  /** MIDI CC point (time, value) */
  struct CCPoint {
    double timeBeats;
    int value;       // 0-127
    juce::String id; // Unique ID for undo/redo
  };

  /** Enable/disable MIDI CC lane display */
  void setCCLaneVisible(int ccNumber, bool visible);
  bool getCCLaneVisible(int ccNumber) const;

  /** Add or update CC point */
  void setCCPoint(int ccNumber, double timeBeats, int value);

  /** Remove CC point */
  void removeCCPoint(int ccNumber, const juce::String &pointId);

  /** Get all CC points for a CC number in time range */
  std::vector<CCPoint> getCCPoints(int ccNumber, double startBeats,
                                   double endBeats) const;

  /** Get all CC points for a CC number */
  std::vector<CCPoint> getAllCCPoints(int ccNumber) const;

  /** Common CC numbers as constants */
  static constexpr int CC_MODULATION = 1;
  static constexpr int CC_VOLUME = 7;
  static constexpr int CC_PAN = 10;
  static constexpr int CC_EXPRESSION = 11;
  static constexpr int CC_SUSTAIN = 64;
  static constexpr int CC_PORTAMENTO = 65;
  static constexpr int CC_SOSTENUTO = 66;
  static constexpr int CC_SOFT_PEDAL = 67;
  static constexpr int CC_FILTER_RESONANCE = 71;
  static constexpr int CC_RELEASE = 72;
  static constexpr int CC_ATTACK = 73;
  static constexpr int CC_CUTOFF = 74;
  static constexpr int CC_REVERB = 91;
  static constexpr int CC_CHORUS = 93;

  //==========================================================================
  // Step Sequencer Mode
  //==========================================================================

  /** Toggle between Piano Roll and Step Sequencer view */
  void setStepSequencerMode(bool enabled);
  bool getStepSequencerMode() const { return stepSequencerMode; }

  /** Set step sequencer parameters */
  void setStepSequencerSteps(int steps) {
    stepSequencerSteps = juce::jlimit(4, 64, steps);
  }
  int getStepSequencerSteps() const { return stepSequencerSteps; }

  /** Set which rows (pitches) to show in step sequencer */
  void setStepSequencerRows(const std::vector<int> &pitches);
  const std::vector<int> &getStepSequencerRows() const {
    return stepSequencerRows;
  }

  /** Toggle step at position */
  void toggleStep(int row, int step);
  bool getStep(int row, int step) const;

  //==========================================================================
  // Strumming Simulation
  //==========================================================================

  enum class StrumDirection {
    Down,     // High pitch to low
    Up,       // Low pitch to high
    Alternate // Alternates based on beat
  };

  /** Apply strumming effect to selected chord notes */
  void applyStrumming(StrumDirection direction, double strumTime = 0.03);

  /** Remove strumming (re-quantize to exact positions) */
  void removeStrumming();

  //==========================================================================
  // AI Melody Extension (Pattern-Based)
  //==========================================================================

  /** Extend melody pattern from selected notes */
  void extendMelody(int numBars = 4);

  /** Generate variation of selected notes */
  void generateVariation(float variationAmount = 0.3f);

  /** Auto-harmonize selected notes */
  enum class HarmonyType {
    Thirds,  // Add 3rds
    Fifths,  // Add 5ths
    Octaves, // Add octaves
    Power,   // Root + 5th
    Triad,   // Full triad
    Drop2    // Jazz voicing (Open Tetrad)
  };
  void autoHarmonize(HarmonyType harmony);
  void fillEuclideanRow(int row, int pulses, int offset = 0);

  //==========================================================================
  // Scale Lock / Snap to Scale (Competition Feature)
  //==========================================================================

  /** Lock notes to a specific scale when moving */
  void setScaleLock(bool enabled);
  bool getScaleLock() const { return scaleLockEnabled; }

  /** Set the scale for scale lock (root 0-11, scale type) */
  void setScaleLockKey(int rootNote, ScaleType scale);

  /** Snap a pitch to the nearest note in the current scale */
  int snapPitchToScale(int pitch) const;

  /** Quantize selected notes to scale (force into scale) */
  void quantizeToScale();

  //==========================================================================
  //==========================================================================
  // MIDI Transformations (Competition Feature)
  //==========================================================================

  /** Retrograde - Reverse the time order of selected notes */
  void transformRetrograde();

  /** Inversion - Flip notes around a pivot pitch */
  void transformInversion(int pivotPitch = -1);

  /** Augmentation - Scale note durations */
  void transformAugmentation(double factor);

  /** Time stretch - Scale entire pattern duration */
  void transformTimeStretch(double factor);

  /** Transpose with scale awareness */
  void transformTransposeInScale(int steps); // +1 = next scale degree

  //==========================================================================
  // Note Split / Join (Competition Feature)
  //==========================================================================

  /** Split selected notes at a beat position */
  void splitNotesAtBeat(double beatPosition);

  /** Split selected notes into equal divisions */
  void splitNotesEqual(int divisions);

  /** Join consecutive notes on same pitch */
  void joinConsecutiveNotes();

  /** Legato - Extend notes to meet the next note */
  void applyLegato();

  //==========================================================================
  // Arpeggiator Preview (Competition Feature)
  //==========================================================================

  enum class ArpPattern {
    Up,
    Down,
    UpDown,
    DownUp,
    Random,
    Order // As played
  };

  /** Enable arpeggiator preview overlay */
  void setArpeggiatorPreview(bool enabled, ArpPattern pattern = ArpPattern::Up,
                             double rate = 0.25, int octaves = 1);
  bool getArpeggiatorPreviewEnabled() const { return arpPreviewEnabled; }

  /** Convert arpeggiator preview to actual notes */
  void commitArpeggiator();

  //==========================================================================
  // Pattern Library (Competition Feature)
  //==========================================================================

  struct MidiPattern {
    juce::String name;
    juce::String category;
    std::vector<zenith::ProjectState::MidiNoteSpec> notes;
    double lengthBeats;
  };

  /** Save selected notes as a pattern */
  MidiPattern saveAsPattern(const juce::String &name,
                            const juce::String &category);

  /** Load a pattern at specified position */
  void loadPattern(const MidiPattern &pattern, double startBeat,
                   int transposition = 0);

  /** Get built-in patterns */
  static std::vector<MidiPattern> getBuiltInPatterns();

  /** Save pattern to JSON file */
  bool savePatternToFile(const MidiPattern &pattern, const juce::File &file);

  /** Load pattern from JSON file */
  MidiPattern loadPatternFromFile(const juce::File &file);

  /** Get user patterns directory */
  static juce::File getUserPatternsDirectory();

  //==========================================================================
  // MIDI Echo / Delay (Competition Feature)
  //==========================================================================

  /** Apply echo effect to selected notes */
  void applyMidiEcho(int repeats, double delayBeats, float velocityDecay = 0.7f,
                     int pitchShiftPerRepeat = 0);

  //==========================================================================
  // Chord Presets (Competition Feature)
  //==========================================================================

  enum class ChordType {
    Major,
    Minor,
    Diminished,
    Augmented,
    Major7,
    Minor7,
    Dominant7,
    Diminished7,
    Sus2,
    Sus4,
    Add9,
    Minor9,
    Power,
    Sixth,
    Minor6
  };

  /** Insert a chord at position */
  void insertChord(int rootPitch, ChordType type, double startBeat,
                   double lengthBeats, int velocity = 100);

  /** Get chord intervals for a type */
  static std::vector<int> getChordIntervals(ChordType type);

  //==========================================================================
  // Spray Can Tool (Bitwig-style) (Competition Feature)
  //==========================================================================

  /** Enable spray can mode for rapid note drawing */
  void setSprayCanMode(bool enabled);
  bool getSprayCanMode() const { return sprayCanMode; }

  /** Set spray parameters */
  void setSprayDensity(float notesPerBeat) { sprayDensity = notesPerBeat; }
  void setSprayVelocityRange(int min, int max);
  void setSprayPitchRange(int semitones) { sprayPitchRange = semitones; }

  //==========================================================================
  // Scripting API Hooks (FL Studio-style) (Competition Feature)
  //==========================================================================

  /** Set a callback for custom note processing */
  using ScriptCallback = std::function<void(std::vector<NoteRect> &notes)>;
  void setScriptCallback(const juce::String &name, ScriptCallback callback);
  void executeScript(const juce::String &name);

  /** Run a script from a file (Simple Command Language) */
  void runScriptFromFile(const juce::File &file);

  /** Get all note data for scripting */
  std::vector<NoteRect> &getNotesForScripting() { return noteRects; }

private:


  //==========================================================================
  // Internal Note Representation
  //==========================================================================

  // Moved to public section

  //==========================================================================
  // Drag Modes (with cursor support)
  //==========================================================================

  enum class DragMode {
    None,
    MoveNote,
    ResizeLeft,
    ResizeRight,
    VelocityEdit,
    MarqueeSelect,
    CCEditPoint,      // Editing an existing CC point
    CCNewPoint,       // Creating and dragging a new CC point
    ExpressionTension // Editing expression curve tension
  };

  enum class CursorType {
    Normal,
    Hand,
    ResizeHorizontal,
    ResizeLeft,
    ResizeRight,
    Crosshair
  };

  //==========================================================================
  // Clipboard Support
  //==========================================================================

  struct ClipboardNote {
    int pitch;
    double startBeats;
    double lengthBeats;
    int velocity;
    bool muted;
    // New properties for advanced MIDI features
    float probability = 1.0f;
    juce::String condition;
    juce::String recurrence;
    int articulationId = 0;
  };

  std::vector<ClipboardNote> clipboard;
  double clipboardReferenceTime = 0.0; // For relative paste

  //==========================================================================
  // Scale Highlighting
  //==========================================================================

  struct ScaleHighlight {
    bool enabled = false;
    int rootNote = 0; // 0-11 (C-B)
    ScaleType scale = ScaleType::Major;
    std::vector<bool> highlightedPitches; // 128 bools for each MIDI note
  };

  ScaleHighlight scaleHighlight;
  void updateScaleHighlight();
  bool isNoteInScale(int pitch) const;

  //==========================================================================
  // Chord Detection
  //==========================================================================

  juce::String detectChord(const std::vector<int> &pitches) const;
  juce::String getCurrentChordName() const;

  //==========================================================================
  // Data Management
  //==========================================================================

  void refreshNotesFromProjectState();
  void updateNoteRectangles();

  //==========================================================================
  // Spatial Indexing (Performance Optimization)
  //==========================================================================

  /** Spatial hash grid for fast note lookups */
  struct SpatialGrid {
    static constexpr int GRID_CELLS_X = 64; // Time divisions
    static constexpr int GRID_CELLS_Y = 16; // Pitch divisions (128 pitches / 8)

    std::vector<NoteRect *> cells[GRID_CELLS_Y][GRID_CELLS_X];

    // Helper to avoid duplicates in queries
    mutable std::set<NoteRect *> queryCache;

    void clear() {
      for (int y = 0; y < GRID_CELLS_Y; ++y) {
        for (int x = 0; x < GRID_CELLS_X; ++x) {
          cells[y][x].clear();
        }
      }
    }

    void addNote(NoteRect *note, double clipLengthBeats) {
      if (!note)
        return;

      // Calculate grid cell bounds
      int minX =
          static_cast<int>((note->startBeats / clipLengthBeats) * GRID_CELLS_X);
      int maxX = static_cast<int>(
          ((note->startBeats + note->lengthBeats) / clipLengthBeats) *
          GRID_CELLS_X);
      int y = (note->pitch / 8); // 128 pitches / 8 = 16 cells

      minX = juce::jlimit(0, GRID_CELLS_X - 1, minX);
      maxX = juce::jlimit(0, GRID_CELLS_X - 1, maxX);
      y = juce::jlimit(0, GRID_CELLS_Y - 1, y);

      // Add note to all cells it overlaps
      for (int x = minX; x <= maxX; ++x) {
        cells[y][x].push_back(note);
      }
    }

    std::vector<NoteRect *> query(double startBeats, double endBeats,
                                  int minPitch, int maxPitch,
                                  double clipLengthBeats) const {
      std::vector<NoteRect *> results;
      queryCache.clear(); // Clear cache for this query

      int minX =
          static_cast<int>((startBeats / clipLengthBeats) * GRID_CELLS_X);
      int maxX = static_cast<int>((endBeats / clipLengthBeats) * GRID_CELLS_X);
      int minY = minPitch / 8;
      int maxY = maxPitch / 8;

      minX = juce::jlimit(0, GRID_CELLS_X - 1, minX);
      maxX = juce::jlimit(0, GRID_CELLS_X - 1, maxX);
      minY = juce::jlimit(0, GRID_CELLS_Y - 1, minY);
      maxY = juce::jlimit(0, GRID_CELLS_Y - 1, maxY);

      for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
          for (NoteRect *note : cells[y][x]) {
            if (queryCache.find(note) == queryCache.end()) {
              // Check if note actually overlaps query region
              if (note && note->startBeats < endBeats &&
                  (note->startBeats + note->lengthBeats) > startBeats &&
                  note->pitch >= minPitch && note->pitch <= maxPitch) {
                results.push_back(note);
                queryCache.insert(note);
              }
            }
          }
        }
      }

      return results;
    }
  };

  SpatialGrid spatialGrid;
  bool spatialGridDirty = true;
  void rebuildSpatialGrid();

  //==========================================================================
  // Coordinate Conversion
  //==========================================================================

  int pixelsToPitch(float y) const;
  float pitchToPixels(int pitch) const;
  double pixelsToBeats(float x) const;
  float beatsToPixels(double beats) const;
  double snapToGrid(double beats) const;

  int pixelsToVelocity(float y) const;
  float velocityToPixels(int velocity) const;

  //==========================================================================
  // Mouse Interaction Helpers
  //==========================================================================

  NoteRect *findNoteAtPosition(float x, float y);
  NoteRect *findNoteInVelocityLane(float x, float y);
  const NoteRect *findNoteInVelocityLane(float x, float y) const;
  DragMode detectNoteHitRegion(const NoteRect &note, float x, float y) const;
  CursorType getCursorForPosition(float x, float y) const;

  // New CC Lane Interaction Helpers
  bool findCCLaneAtPosition(float x, float y, int &ccNumber,
                            juce::Rectangle<float> &laneRect) const;
  // Const version for querying (cursor detection)
  const CCPoint *findCCPointAtPosition(int ccNumber, float x, float y,
                                       juce::Rectangle<float> &laneRect) const;
  // Non-const version for modification (mouse down)
  CCPoint *findCCPointAtPosition(int ccNumber, float x, float y,
                                 juce::Rectangle<float> &laneRect);

  //==========================================================================
  // Editing Operations (with batched undo)
  //==========================================================================

  void createNoteAtPosition(float x, float y);
  void deleteSelectedNotes();
  void copySelectedNotes();
  void pasteNotes();
  void cutSelectedNotes();

  //==========================================================================
  // Selection Management
  //==========================================================================

  void clearSelection();
  void selectNote(NoteRect *note, bool addToSelection);
  void selectNotesInRectangle(const juce::Rectangle<float> &rect);
  void selectAll();
  void invertSelection();
  int getSelectedNoteCount() const;

  //==========================================================================
  // Drag Operations
  //==========================================================================

  void startMovingSelection(const juce::MouseEvent &e);
  void updateSelectionMove(const juce::MouseEvent &e);
  void finishSelectionMove();

  void startResizingNote(NoteRect *note, DragMode mode,
                         const juce::MouseEvent &e);
  void updateNoteResize(const juce::MouseEvent &e);
  void finishNoteResize();

  void startEditingVelocity(NoteRect *note, const juce::MouseEvent &e);
  void updateVelocityEdit(const juce::MouseEvent &e);
  void finishVelocityEdit();

  void startMarqueeSelect(const juce::MouseEvent &e);
  void updateMarqueeSelect(const juce::MouseEvent &e);
  void finishMarqueeSelect();

  void startEditingExpressionTension(const juce::MouseEvent &e);
  void updateExpressionTension(const juce::MouseEvent &e);
  void finishExpressionTension();

  //==========================================================================
  // Zoom & Scroll
  //==========================================================================

  void zoomHorizontal(float factor, float centerX);
  void zoomVertical(float factor, float centerY);
  void scrollHorizontal(float delta);
  void scrollVertical(float delta);

  //==========================================================================
  // Rendering Helpers
  //==========================================================================

  juce::Colour getColorForVelocity(int velocity) const;
  SkColor getSkiaColorForVelocity(int velocity) const;

  // Skia Drawing Helpers
  void drawPianoKeys(SkCanvas *canvas, const SkRect &area);
  void drawGrid(SkCanvas *canvas, const SkRect &area);
  void drawPlayhead(SkCanvas *canvas, const SkRect &area);
  void drawNotes(SkCanvas *canvas, const SkRect &area);
  void drawVelocityLane(SkCanvas *canvas, const SkRect &area);
  void drawChordName(SkCanvas *canvas);

  // Modern UI Drawing
  void drawModernToolbar(SkCanvas *canvas, const SkRect &fullRect);
  void drawExpressionLanes(SkCanvas *canvas, const SkRect &area);

  //==========================================================================
  // ValueTree::Listener
  //==========================================================================

  void valueTreeChildAdded(juce::ValueTree &parent,
                           juce::ValueTree &child) override;
  void valueTreeChildRemoved(juce::ValueTree &parent, juce::ValueTree &child,
                             int index) override;
  void valueTreePropertyChanged(juce::ValueTree &tree,
                                const juce::Identifier &property) override;

  //==========================================================================
  // Member Variables
  //==========================================================================

  zenith::ProjectState &projectState;
  MidiClipContext currentClip;

  std::vector<NoteRect> noteRects;

  // Grid & Snap
  double gridBeats = 0.25; // 1/16 note
  bool snapEnabled = true;

  // View & Zoom
  double pixelsPerBeat = 80.0;
  double pixelsPerPitch = 16.0;
  double viewStartBeats = 0.0;
  int viewLowestPitch = 0;
  int scrollOffsetX = 0;
  int scrollOffsetY = 0;

  // Layout
  static constexpr int PIANO_WIDTH = 60;
  static constexpr int RULER_HEIGHT = 30;
  int velocityLaneHeight = 160; // Increased from 120 for better precision
                                // (~1.26px per velocity value)
  float noteGridHeight = 0.0f;  // Cached note grid height
  float resizeHandleWidth = 8.0f;

  // Interaction State

  DragMode currentDragMode = DragMode::None;
  NoteRect *activeNote = nullptr;
  NoteRect *hoveredNote = nullptr;
  CCPoint *activeCCPoint = nullptr; // New: Currently dragged CC point
  int activeCCNumber = -1;          // New: CC number of the activeCCPoint
  juce::Rectangle<float>
      currentCCLaneBounds; // New: Bounds of the active CC lane during drag

  juce::Point<float> dragStartPos;
  juce::Rectangle<float> marqueeRect;

  // Multi-drag state cache
  struct NoteDragState {
    juce::String id;
    int originalPitch;
    double originalStartBeats;
    double originalLengthBeats;
    int originalVelocity;
  };
  std::vector<NoteDragState> dragStates;

  // Cursor state
  CursorType currentCursorType = CursorType::Normal;

  // Tool state
  Tool currentTool = Tool::Select;
  int hoveredPianoKey = -1; // -1 = no key hovered
  int playingPianoKey = -1; // -1 = no key being played

  //==========================================================================
  // Ghost Notes State
  //==========================================================================

  struct GhostNote {
    int pitch;
    double startBeats;
    double lengthBeats;
    juce::Rectangle<float> bounds;
  };

  bool ghostNotesEnabled = false;
  float ghostNoteOpacity = 0.3f;
  std::vector<juce::String> ghostClipIds;
  std::vector<GhostNote> ghostNotes;

  // Groove state
  GrooveTemplate currentGroove = GrooveTemplate::Straight;

  void refreshGhostNotes();
  void drawGhostNotes(SkCanvas *canvas, const SkRect &area);

  //==========================================================================
  // Note Preview State
  //==========================================================================

  bool notePreviewEnabled = true;
  int previewVelocity = 100;
  int currentPreviewPitch = -1; // -1 = no preview active

  // Callback for audio engine to trigger note preview
  std::function<void(int pitch, int velocity, bool noteOn)> notePreviewCallback;

  //==========================================================================
  // MIDI Input Recording State
  //==========================================================================

  bool midiInputEnabled = false;
  std::map<int, double>
      activeInputNotes; // pitch -> start time (for recording durations)
  double currentPlayheadBeats = 0.0; // For recording, synced with transport

  //==========================================================================
  // Note Probability State
  //==========================================================================

  std::map<juce::String, float>
      noteProbabilities; // noteId -> probability (0.0-1.0)

  //==========================================================================
  // MPE Expression State
  //==========================================================================

  // Map-based NoteExpressionData for dynamic ExpressionType lookup
  using NoteExpressionData =
      std::map<ExpressionType, std::vector<ExpressionPoint>>;

  std::map<juce::String, NoteExpressionData>
      noteExpressions; // noteId -> expression data
  bool expressionLaneVisible[4] = {false, false, false,
                                   false}; // One per ExpressionType
  int expressionLaneHeight = 60;           // Height of each expression lane

  //==========================================================================
  // MIDI CC State
  //==========================================================================

  struct CCLane {
    int ccNumber;
    bool visible = false;
    std::vector<CCPoint> points;
    juce::String name; // Display name (e.g., "Modulation", "Volume")
  };

  std::map<int, CCLane> ccLanes; // ccNumber -> lane data
  int ccLaneHeight = 50;         // Height of each CC lane
  std::set<int> visibleCCLanes;  // Quick lookup for visible CC lanes

  void updateCCLaneNames();
  juce::String getCCLaneName(int ccNumber) const;

  //==========================================================================
  // Step Sequencer State
  //==========================================================================

  bool stepSequencerMode = false;
  int stepSequencerSteps = 16;
  std::vector<int> stepSequencerRows; // Pitches to show (e.g., drum kit)
  std::vector<std::vector<bool>> stepSequencerGrid; // [row][step]

  void drawStepSequencer(SkCanvas *canvas, const SkRect &area);
  void syncStepSequencerToNotes();
  void syncNotesToStepSequencer();

  //==========================================================================
  // Strumming State
  //==========================================================================

  double lastStrumOffset = 0.0; // For alternate strumming
  int strumBeatCounter = 0;

  //==========================================================================
  // AI Melody State
  //==========================================================================

  struct MelodyPattern {
    std::vector<int> intervals;    // Pitch intervals from first note
    std::vector<double> durations; // Note durations
    std::vector<double> timings;   // Start times relative to pattern start
    std::vector<int> velocities;   // Velocity values
  };

  MelodyPattern analyzePattern(const std::vector<NoteRect *> &notes);
  void applyPattern(const MelodyPattern &pattern, double startBeat,
                    int transposition);

  //==========================================================================
  // Scale Lock State
  //==========================================================================

  bool scaleLockEnabled = false;
  int scaleLockRoot = 0; // 0 = C
  ScaleType scaleLockType = ScaleType::Major;
  std::vector<bool> scaleLockNotes; // 12 bools for which notes are in scale

  void updateScaleLockNotes();

  //==========================================================================
  // Fold Mode State (Ableton-style)
  //==========================================================================

  bool foldMode = false;
  std::vector<int> visiblePitches; // List of pitches 0-127 that are visible
  void updateVisiblePitches();
  int mapPitchToRow(int pitch) const; // Returns row index for pitch
  int mapRowToPitch(int row) const;   // Returns pitch for row index

  //==========================================================================
  // Arpeggiator State
  //==========================================================================

  bool arpPreviewEnabled = false;
  ArpPattern arpPattern = ArpPattern::Up;
  double arpRate = 0.25; // In beats
  int arpOctaves = 1;
  std::vector<NoteRect> arpPreviewNotes; // Generated preview notes

  void generateArpPreview();
  void drawArpPreview(SkCanvas *canvas, const SkRect &area);

  //==========================================================================
  // Spray Can State
  //==========================================================================

  bool sprayCanMode = false;
  float sprayDensity = 4.0f; // Notes per beat
  int sprayVelocityMin = 80;
  int sprayVelocityMax = 127;
  int sprayPitchRange = 0; // Semitones of random pitch variation
  juce::Point<float> lastSprayPosition;

  void handleSprayPaint(float x, float y);

  //==========================================================================
  // Scripting State
  //==========================================================================

  std::map<juce::String, ScriptCallback> scriptCallbacks;

  //==========================================================================
  // Timer Callback
  //==========================================================================

  void timerCallback() override;



  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoRollComponent)
};

//==============================================================================
/**
 * @class MidiEditorContainer
 * @brief Container that switches between Piano Roll and Drum Pad views
 */
class MidiEditorContainer : public juce::Component {
public:
  MidiEditorContainer(zenith::ProjectState &state, zenith::Engine &engine)
      : projectState(state), engine_(engine) {
    pianoRoll = std::make_unique<PianoRollComponent>(state);
    addAndMakeVisible(pianoRoll.get());

    drumPad = std::make_unique<DrumPadComponent>(engine, state);
    addChildComponent(drumPad.get()); // Hidden by default

    // Toggle Button
    toggleButton.setButtonText("Switch to Drum View");
    toggleButton.onClick = [this] { toggleView(); };
    addAndMakeVisible(toggleButton);
  }

  void setClipContext(const MidiClipContext &context) {
    pianoRoll->setClipContext(context);
    drumPad->setClipContext(context.clipId);

    // Auto-detect mode based on track name? For now manual.
    if (context.trackId.containsIgnoreCase("drum")) {
      if (activeView == View::PianoRoll)
        toggleView();
    }
  }

  void resized() override {
    auto area = getLocalBounds();
    auto topBar = area.removeFromTop(30);

    toggleButton.setBounds(topBar.removeFromRight(150).reduced(2));

    if (activeView == View::PianoRoll) {
      pianoRoll->setBounds(area);
    } else {
      drumPad->setBounds(area);
    }
  }

  void toggleView() {
    if (activeView == View::PianoRoll) {
      activeView = View::DrumPad;
      pianoRoll->setVisible(false);
      drumPad->setVisible(true);
      toggleButton.setButtonText("Switch to Piano Roll");
    } else {
      activeView = View::PianoRoll;
      pianoRoll->setVisible(true);
      drumPad->setVisible(false);
      toggleButton.setButtonText("Switch to Drum View");
    }
    resized();
  }

private:
  zenith::ProjectState &projectState;
  zenith::Engine &engine_;
  std::unique_ptr<PianoRollComponent> pianoRoll;
  std::unique_ptr<DrumPadComponent> drumPad;
  juce::TextButton toggleButton;

  enum class View { PianoRoll, DrumPad };
  View activeView = View::PianoRoll;
};

//==============================================================================
/**
 * @class PianoRollWindow
 * @brief Standalone window wrapper for PianoRollComponent (and Drum Pad)
 */
class PianoRollWindow : public juce::DocumentWindow {
public:
  PianoRollWindow(zenith::ProjectState &state, zenith::Engine &engine,
                  const juce::String &trackId, const juce::String &clipId)
      : DocumentWindow(
            "MIDI Editor",
            juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(
                juce::ResizableWindow::backgroundColourId),
            DocumentWindow::allButtons) {
    setUsingNativeTitleBar(true);

    auto *content = new MidiEditorContainer(state, engine);
    setContentOwned(content, true);

    // Setup clip context
    MidiClipContext context;
    context.clipId = clipId;
    context.trackId = trackId;

    // Find clip info from state
    auto [track, clip] = state.findClip(clipId);
    if (clip.isValid()) {
      context.clipName =
          clip.getProperty(zenith::ProjectState::PROP_NAME).toString();
      context.clipStartBeats =
          clip.getProperty(zenith::ProjectState::PROP_START);
      context.clipLengthBeats =
          clip.getProperty(zenith::ProjectState::PROP_LENGTH);
    }

    content->setClipContext(context);

    setResizable(true, true);
    centreWithSize(1000, 600);
    setVisible(true);
  }

  ~PianoRollWindow() override = default;

  void closeButtonPressed() override { delete this; }

private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoRollWindow)
};
