/**
 * @file ProjectState.h
 * @brief Project state management using ValueTree
 *
 * Manages all project state using JUCE's ValueTree:
 * - Project metadata (name, tempo, time signature)
 * - Tracks
 * - Clips
 * - Mixer state
 * - Plugin state
 * - Automation
 *
 * Benefits of ValueTree:
 * - Built-in undo/redo support
 * - Serialization to XML/JSON
 * - Efficient change notifications
 * - Thread-safe with proper listeners
 *
 * Foundation:
 * - Basic project structure
 * - Tempo and time signature
 * - Save/load to XML
 */

#pragma once

#include <atomic>
#include <unordered_map>
#include <vector>

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_graphics/juce_graphics.h>
#include "engine/RoutingGraph.h"

//==============================================================================
//==============================================================================
namespace zenith {

/**
 * @class ProjectState
 * @brief Manages all project state using ValueTree
 */

class ProjectState : public juce::ValueTree::Listener {
  friend class ArrangerComponent;

public:
  //==========================================================================
  // Identifiers for ValueTree types and properties
  //==========================================================================

  static const juce::Identifier ID_PROJECT;
  static const juce::Identifier ID_TRACKS;
  static const juce::Identifier ID_TRACK;
  static const juce::Identifier ID_CLIPS;
  static const juce::Identifier ID_CLIP;
  static const juce::Identifier ID_MIXER;
  static const juce::Identifier ID_AUTOMATION;
  static const juce::Identifier ID_ENVELOPE;
  static const juce::Identifier ID_POINT;
  static const juce::Identifier ID_NOTES;       // MIDI notes container
  static const juce::Identifier ID_NOTE;        // Individual MIDI note
  static const juce::Identifier ID_TEMPO_MAP;   // Container for tempo changes
  static const juce::Identifier ID_TEMPO_POINT; // Individual tempo change
  static const juce::Identifier ID_MARKERS;     // Container for markers
  static const juce::Identifier ID_MARKER;      // Individual marker

  static const juce::Identifier PROP_NAME;
  static const juce::Identifier PROP_TEMPO;
  static const juce::Identifier PROP_TIME_SIG_NUM;
  static const juce::Identifier PROP_TIME_SIG_DEN;
  static const juce::Identifier PROP_SAMPLE_RATE;

  static const juce::Identifier PROP_ID;
  static const juce::Identifier PROP_TYPE;
  static const juce::Identifier PROP_VOLUME;
  static const juce::Identifier PROP_PAN;
  static const juce::Identifier PROP_MUTE;
  static const juce::Identifier PROP_SOLO;
  static const juce::Identifier PROP_ARMED;

  static const juce::Identifier PROP_START;
  static const juce::Identifier PROP_LENGTH;
  static const juce::Identifier PROP_OFFSET;
  static const juce::Identifier PROP_AUDIO_FILE;
  static const juce::Identifier PROP_LANE_INDEX;

  // Automation properties
  static const juce::Identifier PROP_PARAM;
  static const juce::Identifier PROP_PARAM_ID;
  static const juce::Identifier PROP_TIME_BEATS;
  static const juce::Identifier PROP_VALUE;

  // MIDI Note properties
  static const juce::Identifier PROP_START_BEATS;  // Note start time in beats
  static const juce::Identifier PROP_LENGTH_BEATS; // Note length in beats
  static const juce::Identifier PROP_PITCH;        // MIDI pitch (0-127)
  static const juce::Identifier PROP_VELOCITY;     // MIDI velocity (0-127)

  // Tempo/Marker properties
  static const juce::Identifier PROP_BPM;   // Tempo in BPM
  static const juce::Identifier PROP_COLOR; // Marker color (hex string)
  static const juce::Identifier PROP_NEXT_ID; // Next available ID (for O(1) generation)
  static const juce::Identifier PROP_INPUT_CHANNEL; // Input channel index for recording

  //==========================================================================
  ProjectState();
  ~ProjectState();

  //==========================================================================
  // Project Management
  //==========================================================================

  void newProject();
  bool loadFromFile(const juce::File &file);
  bool saveToFile(const juce::File &file);
  juce::File saveCrashDump();
  juce::File getProjectFile() const { return projectFile; }
  void setProjectFile(const juce::File& file) { projectFile = file; }
  bool hasUnsavedChanges() const { return isDirty.load(); }

  //==========================================================================
  // ValueTree::Listener overrides
  void valueTreePropertyChanged(juce::ValueTree &, const juce::Identifier &) override { isDirty = true; }
  void valueTreeChildAdded(juce::ValueTree &parent, juce::ValueTree &child) override;
  void valueTreeChildRemoved(juce::ValueTree &parent, juce::ValueTree &child, int) override;
  void valueTreeChildOrderChanged(juce::ValueTree &, int, int) override { isDirty = true; }
  void valueTreeParentChanged(juce::ValueTree &) override { isDirty = true; }

  //==========================================================================
  // Project Properties
  //==========================================================================

  juce::String getProjectName() const;
  void setProjectName(const juce::String &name);

  double getTempo() const;
  void setTempo(double tempo);

  int getTimeSignatureNumerator() const;
  int getTimeSignatureDenominator() const;
  void setTimeSignature(int numerator, int denominator);

  //==========================================================================
  // Track Management
  //==========================================================================

  juce::String addTrack(const juce::String &name, const juce::String &type);
  void removeTrack(const juce::String &trackId);
  int getNumTracks() const;
  juce::ValueTree getTrack(const juce::String &trackId) const;
  juce::ValueTree getTrackByIndex(int trackIndex);

  //===================================================================
  // Track Mixer API
  //===================================================================

  void renameTrack(const juce::String &trackId, const juce::String &newName,
                   const juce::String &actionName = "Rename track");
  void setTrackVolume(const juce::String &trackId, float volumeLinear,
                      const juce::String &actionName = "Set track volume");
  float getTrackVolume(const juce::String &trackId) const;
  void setTrackPan(const juce::String &trackId, float pan,
                   const juce::String &actionName = "Set track pan");
  float getTrackPan(const juce::String &trackId) const;
  void setTrackMute(const juce::String &trackId, bool muted,
                    const juce::String &actionName = "Set track mute");
  bool isTrackMuted(const juce::String &trackId) const;
  void setTrackSolo(const juce::String &trackId, bool soloed,
                    const juce::String &actionName = "Set track solo");
  bool isTrackSolo(const juce::String &trackId) const;
  void setTrackArmed(const juce::String &trackId, bool armed,
                     const juce::String &actionName = "Set track armed");
  bool isTrackArmed(const juce::String &trackId) const;
  juce::String getTrackName(const juce::String &trackId) const;
  juce::String getTrackType(const juce::String &trackId) const;

  //==========================================================================
  // Clip Management
  //==========================================================================

  juce::String createClip(const juce::String &trackId,
                          const juce::String &clipType,
                          juce::int64 startSamples, juce::int64 lengthSamples,
                          const juce::String &name,
                          const juce::String &actionName = "Create clip");

  juce::String createEmptyClip(const juce::String &trackId, double startBeats,
                               double lengthBeats, bool isMidi,
                               const juce::String &name,
                               const juce::String &actionName);

  juce::String createTrack(const juce::String &type, const juce::String &name,
                           const juce::String &actionName);

  void deleteClip(const juce::String &trackId, const juce::String &clipId,
                  const juce::String &actionName = "Delete clip");
  void deleteClip(const juce::String &clipId, const juce::String &actionName);

  void moveClip(const juce::String &trackId, const juce::String &clipId,
                juce::int64 newStartSamples,
                const juce::String &actionName = "Move clip");
  void moveClip(const juce::String &clipId, const juce::String &newTrackId,
                double newStartBeats, const juce::String &actionName);

  void setClipRange(const juce::String &clipId, double newStartBeats,
                    double newLengthBeats, const juce::String &actionName);

  void resizeClip(const juce::String &trackId, const juce::String &clipId,
                  juce::int64 newLengthSamples,
                  const juce::String &actionName = "Resize clip");

  std::pair<juce::String, juce::String>
  splitClip(const juce::String &trackId, const juce::String &clipId,
            juce::int64 splitSamples,
            const juce::String &actionName = "Split clip");

  juce::String addClip(const juce::String &trackId, double startBeats,
                       double lengthBeats, const juce::String &actionName);
  bool removeClip(const juce::String &trackId, const juce::String &clipId,
                  const juce::String &actionName);

  bool setClipAudioFile(const juce::String &trackId, const juce::String &clipId,
                        const juce::File &audioFile,
                        const juce::String &actionName);
  juce::String getClipAudioFile(const juce::String &trackId,
                                const juce::String &clipId) const;

  juce::ValueTree getClip(const juce::String &trackId,
                          const juce::String &clipId) const;

  std::pair<juce::ValueTree, juce::ValueTree>
  findClip(const juce::String &clipId) const;

  juce::String addClip(const juce::String &trackId,
                       const juce::String &clipType, double startBeats,
                       double lengthBeats, int laneIndex);
  bool removeClip(const juce::String &trackId, const juce::String &clipId);
  bool moveClip(const juce::String &trackId, const juce::String &clipId,
                double newStartBeats);
  bool resizeClip(const juce::String &trackId, const juce::String &clipId,
                  double newLengthBeats);
  juce::ValueTree findClip(const juce::String &trackId,
                           const juce::String &clipId) const;

  //==========================================================================
  // Automation Management
  //==========================================================================

  juce::ValueTree getOrCreateAutomationEnvelope(const juce::String &trackId,
                                                const juce::String &paramId);
  juce::ValueTree getAutomationEnvelope(const juce::String &trackId,
                                        const juce::String &paramId) const;
  bool hasAutomation(const juce::String &trackId,
                     const juce::String &paramId) const;

  juce::String addAutomationPoint(const juce::String &trackId,
                                  const juce::String &paramId, double timeBeats,
                                  double value, const juce::String &actionName);
  bool moveAutomationPoint(const juce::String &trackId,
                           const juce::String &paramId,
                           const juce::String &pointId, double newTimeBeats,
                           double newValue, const juce::String &actionName);
  bool deleteAutomationPoint(const juce::String &trackId,
                             const juce::String &paramId,
                             const juce::String &pointId,
                             const juce::String &actionName);
  bool clearAutomation(const juce::String &trackId, const juce::String &paramId,
                       const juce::String &actionName);

  //==========================================================================
  // MIDI Note Management
  //==========================================================================

  juce::ValueTree getOrCreateNotesContainer(const juce::String &clipId);

  juce::String addNote(const juce::String &clipId, double startBeats,
                       double lengthBeats, int pitch, int velocity,
                       const juce::String &actionName);

  struct MidiNoteSpec {
    juce::String id;
    int pitch;
    double startBeats;
    double lengthBeats;
    int velocity;
    bool muted;

    MidiNoteSpec()
        : pitch(60), startBeats(0.0), lengthBeats(1.0), velocity(100),
          muted(false) {}
  };

  void addNotes(const juce::String &clipId, 
                const std::vector<MidiNoteSpec> &notes,
                const juce::String &actionName);

  bool moveNote(const juce::String &clipId, const juce::String &noteId,
                double newStartBeats, double newLengthBeats, int newPitch,
                int newVelocity, const juce::String &actionName);

  bool deleteNote(const juce::String &clipId, const juce::String &noteId,
                  const juce::String &actionName);

  juce::ValueTree getNotes(const juce::String &clipId) const;

  juce::Array<MidiNoteSpec>
  getMidiNotesForClip(const juce::String &clipId) const;

  juce::String addMidiNote(const juce::String &clipId, const MidiNoteSpec &note,
                           const juce::String &actionName);

  void removeMidiNote(const juce::String &clipId, const juce::String &noteId,
                      const juce::String &actionName);

  void moveMidiNote(const juce::String &clipId, const juce::String &noteId,
                    double newStartBeats, int newPitch,
                    const juce::String &actionName);

  void quantizeClip(const juce::String &clipId, double gridBeats,
                    const juce::String &actionName);

  void setMidiNoteVelocity(const juce::String &clipId,
                           const juce::String &noteId, int newVelocity,
                           const juce::String &actionName);

  void setMidiNoteLength(const juce::String &clipId, const juce::String &noteId,
                         double newLengthBeats, const juce::String &actionName);

  //==========================================================================
  // Tempo Map & Markers
  //==========================================================================

  juce::String addTempoChange(double beatPosition, double bpm,
                              const juce::String &actionName);
  juce::ValueTree getTempoMap() const;

  juce::String addMarker(double beatPosition, const juce::String &name,
                         const juce::String &color,
                         const juce::String &actionName);
  bool deleteMarker(const juce::String &markerId,
                    const juce::String &actionName);
  void moveMarker(const juce::String &markerId, double newBeats,
                  const juce::String &actionName);
  void renameMarker(const juce::String &markerId, const juce::String &newName,
                    const juce::String &actionName);
  juce::ValueTree getMarkers() const;

  //==========================================================================
  // Undo/Redo
  //==========================================================================

  juce::UndoManager &getUndoManager() { return undoManager; }
  void undo();
  void redo();
  bool canUndo() const { return undoManager.canUndo(); }
  bool canRedo() const { return undoManager.canRedo(); }

  //==========================================================================
  // State Access
  //==========================================================================

  juce::ValueTree &getState() { return state; }
  const juce::ValueTree &getState() const { return state; }

  //==========================================================================
  // Routing Graph
  //==========================================================================
  zenith::RoutingGraph& getRoutingGraph() { return routingGraph; }
  const zenith::RoutingGraph& getRoutingGraph() const { return routingGraph; }

  //==========================================================================
  // Debug Helpers
  //==========================================================================

#if JUCE_DEBUG
  void dumpClipStructureToLog() const;
#endif

private:
  //==========================================================================
  // Helper Methods
  //==========================================================================

  void createDefaultState();
  juce::String generateUniqueId(const juce::String &prefix);
  juce::ValueTree findTrackInternal(const juce::String &trackId) const;
  juce::ValueTree findNote(const juce::String &trackId,
                           const juce::String &clipId,
                           const juce::String &noteId);
  juce::ValueTree findTrack(const juce::String &trackId) const;
  juce::ValueTree findAutomationPoint(const juce::ValueTree &envelope,
                                      const juce::String &pointId) const;
  void rebuildIdCounter();
  void rebuildTrackMap();
  juce::ValueTree findMidiNote(const juce::String &clipId, const juce::String &noteId) const;

  //==========================================================================
  // Member Variables
  //==========================================================================

  juce::ValueTree state;
  juce::UndoManager undoManager;
  std::atomic<int> idCounter{0};
  mutable std::unordered_map<juce::String, juce::ValueTree> trackIdMap_;
  std::atomic<bool> isDirty{false};
  juce::File projectFile;
  zenith::RoutingGraph routingGraph;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProjectState)
};
} // namespace zenith
