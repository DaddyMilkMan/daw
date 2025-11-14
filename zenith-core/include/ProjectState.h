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
 *
 * Benefits of ValueTree:
 * - Built-in undo/redo support
 * - Serialization to XML/JSON
 * - Efficient change notifications
 * - Thread-safe with proper listeners
 *
 * Phase 0: Foundation
 * - Basic project structure
 * - Tempo and time signature
 * - Save/load to XML
 */

#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>

//==============================================================================
/**
 * @class ProjectState
 * @brief Manages all project state using ValueTree
 *
 * The state tree structure:
 *
 * PROJECT
 * ├── name: "Untitled"
 * ├── tempo: 120.0
 * ├── timeSignatureNumerator: 4
 * ├── timeSignatureDenominator: 4
 * ├── sampleRate: 44100.0
 * ├── TRACKS
 * │   ├── TRACK
 * │   │   ├── id: "track_1"
 * │   │   ├── name: "Audio 1"
 * │   │   ├── type: "audio" or "midi"
 * │   │   ├── volume: 0.8
 * │   │   ├── pan: 0.0
 * │   │   ├── mute: false
 * │   │   ├── solo: false
 * │   │   └── CLIPS
 * │   │       └── CLIP
 * │   │           ├── id: "clip_1"
 * │   │           ├── start: 0.0
 * │   │           ├── length: 4.0
 * │   │           └── ...
 * │   └── ...
 * └── MIXER
 *     ├── masterVolume: 0.8
 *     └── ...
 */
class ProjectState
{
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
    static const juce::Identifier ID_MIDI_NOTES;  // Phase 8: MIDI note container
    static const juce::Identifier ID_MIDI_NOTE;   // Phase 8: Individual MIDI note

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

    static const juce::Identifier PROP_START;
    static const juce::Identifier PROP_LENGTH;

    // Phase 8: MIDI note properties
    static const juce::Identifier PROP_PITCH;         // 0-127
    static const juce::Identifier PROP_START_BEATS;   // Note start in beats (relative to clip)
    static const juce::Identifier PROP_LENGTH_BEATS;  // Note length in beats
    static const juce::Identifier PROP_VELOCITY;      // 0-127

    //==========================================================================
    ProjectState();
    ~ProjectState();

    //==========================================================================
    // Project Management
    //==========================================================================

    /**
     * @brief Create a new empty project
     */
    void newProject();

    /**
     * @brief Load project from file
     * @param file Project file (.zth)
     * @return true if loaded successfully
     */
    bool loadFromFile(const juce::File& file);

    /**
     * @brief Save project to file
     * @param file Project file (.zth)
     * @return true if saved successfully
     */
    bool saveToFile(const juce::File& file);

    //==========================================================================
    // Project Properties
    //==========================================================================

    juce::String getProjectName() const;
    void setProjectName(const juce::String& name);

    double getTempo() const;
    void setTempo(double tempo);

    int getTimeSignatureNumerator() const;
    int getTimeSignatureDenominator() const;
    void setTimeSignature(int numerator, int denominator);

    //==========================================================================
    // Track Management
    //==========================================================================

    /**
     * @brief Add a new track
     * @param name Track name
     * @param type "audio" or "midi"
     * @return Track ID
     */
    juce::String addTrack(const juce::String& name, const juce::String& type);

    /**
     * @brief Remove a track
     * @param trackId Track ID
     */
    void removeTrack(const juce::String& trackId);

    /**
     * @brief Get number of tracks
     */
    int getNumTracks() const;

    //==========================================================================
    // MIDI Note Management (Phase 8)
    //==========================================================================

    /**
     * @brief Specification for a MIDI note
     */
    struct MidiNoteSpec
    {
        juce::String id;         // Unique note ID (e.g., "note_42")
        int pitch;               // MIDI note number (0-127)
        double startBeats;       // Start time in beats (relative to clip start)
        double lengthBeats;      // Duration in beats
        int velocity;            // Note velocity (0-127)
        bool muted;              // Muted flag (default false)

        MidiNoteSpec() : pitch(60), startBeats(0.0), lengthBeats(1.0), velocity(100), muted(false) {}
    };

    /**
     * @brief Get all MIDI notes for a clip
     * @param clipId Clip ID
     * @return Array of note specifications
     */
    juce::Array<MidiNoteSpec> getMidiNotesForClip(const juce::String& clipId) const;

    /**
     * @brief Add a MIDI note to a clip
     * @param clipId Clip ID
     * @param note Note specification (id will be auto-generated if empty)
     * @param actionName Undo action name
     * @return The note ID (auto-generated or provided)
     */
    juce::String addMidiNote(const juce::String& clipId, const MidiNoteSpec& note, const juce::String& actionName);

    /**
     * @brief Remove a MIDI note from a clip
     * @param clipId Clip ID
     * @param noteId Note ID to remove
     * @param actionName Undo action name
     */
    void removeMidiNote(const juce::String& clipId, const juce::String& noteId, const juce::String& actionName);

    /**
     * @brief Move/modify a MIDI note
     * @param clipId Clip ID
     * @param noteId Note ID to modify
     * @param newStartBeats New start time in beats
     * @param newPitch New pitch
     * @param actionName Undo action name
     */
    void moveMidiNote(const juce::String& clipId, const juce::String& noteId,
                      double newStartBeats, int newPitch, const juce::String& actionName);

    /**
     * @brief Quantize all notes in a clip to a grid
     * @param clipId Clip ID
     * @param gridBeats Grid size in beats (e.g., 0.25 for 1/16 at 4/4)
     * @param actionName Undo action name
     */
    void quantizeClip(const juce::String& clipId, double gridBeats, const juce::String& actionName);

    //==========================================================================
    // Undo/Redo
    //==========================================================================

    /**
     * @brief Get undo manager
     */
    juce::UndoManager& getUndoManager() { return undoManager; }

    /**
     * @brief Undo last action
     */
    void undo();

    /**
     * @brief Redo last undone action
     */
    void redo();

    /**
     * @brief Check if can undo
     */
    bool canUndo() const { return undoManager.canUndo(); }

    /**
     * @brief Check if can redo
     */
    bool canRedo() const { return undoManager.canRedo(); }

    //==========================================================================
    // State Access
    //==========================================================================

    /**
     * @brief Get the root ValueTree
     */
    juce::ValueTree& getState() { return state; }

    /**
     * @brief Get the root ValueTree (const)
     */
    const juce::ValueTree& getState() const { return state; }

private:
    //==========================================================================
    // Helper Methods
    //==========================================================================

    /**
     * @brief Create default project structure
     */
    void createDefaultState();

    /**
     * @brief Generate unique ID
     */
    juce::String generateUniqueId(const juce::String& prefix);

    /**
     * @brief Find track by ID
     */
    juce::ValueTree findTrack(const juce::String& trackId);

    /**
     * @brief Find clip by ID (Phase 8)
     * @param clipId Clip ID to search for
     * @return ValueTree for the clip, or invalid tree if not found
     */
    juce::ValueTree findClip(const juce::String& clipId);

    /**
     * @brief Find clip by ID (const version)
     */
    juce::ValueTree findClip(const juce::String& clipId) const;

    /**
     * @brief Find MIDI note by ID within a clip (Phase 8)
     * @param clipId Clip ID
     * @param noteId Note ID
     * @return ValueTree for the note, or invalid tree if not found
     */
    juce::ValueTree findMidiNote(const juce::String& clipId, const juce::String& noteId);

    /**
     * @brief Rebuilds the ID counter based on the current state tree
     */
    void rebuildIdCounter();

    //==========================================================================
    // Member Variables
    //==========================================================================

    juce::ValueTree state;
    juce::UndoManager undoManager;

    // ID counter for generating unique IDs
    std::atomic<int> idCounter{0};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProjectState)
};
