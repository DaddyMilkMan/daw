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
 * - Automation (Phase 13)
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
 * │   │   ├── CLIPS
 * │   │   │   └── CLIP
 * │   │   │       ├── id: "clip_1"
 * │   │   │       ├── start: 0.0
 * │   │   │       ├── length: 4.0
 * │   │   │       └── ...
 * │   │   └── AUTOMATION (Phase 13)
 * │   │       ├── ENVELOPE
 * │   │       │   ├── param: "volume"
 * │   │       │   └── POINT
 * │   │       │       ├── id: "point_0"
 * │   │       │       ├── timeBeats: 0.0
 * │   │       │       └── value: 0.8
 * │   │       ├── ENVELOPE
 * │   │       │   ├── param: "pan"
 * │   │       │   └── ...
 * │   │       └── ENVELOPE
 * │   │           ├── param: "mute"
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
    static const juce::Identifier ID_AUTOMATION;
    static const juce::Identifier ID_ENVELOPE;
    static const juce::Identifier ID_POINT;

    // Phase 15: Tempo map and markers
    static const juce::Identifier ID_TEMPO_MAP;
    static const juce::Identifier ID_TEMPO_CHANGE;
    static const juce::Identifier ID_MARKERS;
    static const juce::Identifier ID_MARKER;

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

    // Phase 13: Automation properties
    static const juce::Identifier PROP_PARAM;
    static const juce::Identifier PROP_TIME_BEATS;
    static const juce::Identifier PROP_VALUE;

    // Phase 15: Tempo map and marker properties
    static const juce::Identifier PROP_BEAT_POSITION;
    static const juce::Identifier PROP_BPM;
    static const juce::Identifier PROP_TIME_SIG_NUM_CHANGE;  // For tempo changes
    static const juce::Identifier PROP_TIME_SIG_DEN_CHANGE;  // For tempo changes
    static const juce::Identifier PROP_COLOR;

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
    // Phase 15: Tempo Map Management
    //==========================================================================

    /**
     * @brief Tempo change specification (for returning tempo map data)
     */
    struct TempoChangeSpec
    {
        juce::String id;
        double beatPosition{0.0};
        double bpm{120.0};
        int timeSigNumerator{4};
        int timeSigDenominator{4};
    };

    /**
     * @brief Get the tempo map ValueTree (creates if missing)
     * @return TEMPO_MAP ValueTree
     * @note Message thread only
     */
    juce::ValueTree getTempoMapNode();

    /**
     * @brief Get all tempo changes (sorted by beat position)
     * @return Array of tempo change specifications
     * @note Message thread only
     */
    juce::Array<TempoChangeSpec> getTempoChanges() const;

    /**
     * @brief Add a tempo change
     * @param beatPosition Beat position (>= 0.0)
     * @param bpm Tempo in BPM (> 0.0)
     * @param numerator Time signature numerator
     * @param denominator Time signature denominator
     * @param actionName Undo action name
     * @return Generated tempo change ID
     * @note Message thread only, undoable
     */
    juce::String addTempoChange(double beatPosition, double bpm,
                                 int numerator, int denominator,
                                 const juce::String& actionName);

    /**
     * @brief Move a tempo change to a new beat position
     * @param tempoId Tempo change ID
     * @param newBeatPosition New beat position
     * @param actionName Undo action name
     * @return true if found and moved
     * @note Message thread only, undoable
     */
    bool moveTempoChange(const juce::String& tempoId, double newBeatPosition,
                         const juce::String& actionName);

    /**
     * @brief Set tempo change BPM
     * @param tempoId Tempo change ID
     * @param newBpm New BPM value
     * @param actionName Undo action name
     * @return true if found and updated
     * @note Message thread only, undoable
     */
    bool setTempoChangeBpm(const juce::String& tempoId, double newBpm,
                           const juce::String& actionName);

    /**
     * @brief Set tempo change time signature
     * @param tempoId Tempo change ID
     * @param numerator Time signature numerator
     * @param denominator Time signature denominator
     * @param actionName Undo action name
     * @return true if found and updated
     * @note Message thread only, undoable
     */
    bool setTempoChangeTimeSig(const juce::String& tempoId, int numerator, int denominator,
                                const juce::String& actionName);

    /**
     * @brief Delete a tempo change
     * @param tempoId Tempo change ID
     * @param actionName Undo action name
     * @return true if found and deleted
     * @note Message thread only, undoable
     * @note Ensures there is always a tempo at beat 0.0
     */
    bool deleteTempoChange(const juce::String& tempoId, const juce::String& actionName);

    /**
     * @brief Get tempo at a specific beat position
     * @param beat Beat position
     * @return BPM value at that position
     * @note Message thread only
     */
    double getTempoAtBeat(double beat) const;

    /**
     * @brief Convert beats to seconds
     * @param beat Beat position
     * @return Time in seconds
     * @note Message thread only
     */
    double beatToSeconds(double beat) const;

    /**
     * @brief Convert seconds to beats
     * @param seconds Time in seconds
     * @return Beat position
     * @note Message thread only
     */
    double secondsToBeat(double seconds) const;

    //==========================================================================
    // Phase 15: Markers Management
    //==========================================================================

    /**
     * @brief Marker specification (for returning marker data)
     */
    struct MarkerSpec
    {
        juce::String id;
        juce::String name;
        double beatPosition{0.0};
        juce::String color{"#FFCC00"};
    };

    /**
     * @brief Get all markers (sorted by beat position)
     * @return Array of marker specifications
     * @note Message thread only
     */
    juce::Array<MarkerSpec> getMarkers() const;

    /**
     * @brief Add a marker
     * @param beatPosition Beat position (>= 0.0)
     * @param name Marker name
     * @param colorHex Color in hex format (e.g., "#FFCC00")
     * @param actionName Undo action name
     * @return Generated marker ID
     * @note Message thread only, undoable
     */
    juce::String addMarker(double beatPosition, const juce::String& name,
                           const juce::String& colorHex, const juce::String& actionName);

    /**
     * @brief Move a marker to a new beat position
     * @param markerId Marker ID
     * @param newBeatPosition New beat position
     * @param actionName Undo action name
     * @return true if found and moved
     * @note Message thread only, undoable
     */
    bool moveMarker(const juce::String& markerId, double newBeatPosition,
                    const juce::String& actionName);

    /**
     * @brief Rename a marker
     * @param markerId Marker ID
     * @param newName New name
     * @param actionName Undo action name
     * @return true if found and renamed
     * @note Message thread only, undoable
     */
    bool renameMarker(const juce::String& markerId, const juce::String& newName,
                      const juce::String& actionName);

    /**
     * @brief Recolor a marker
     * @param markerId Marker ID
     * @param newColorHex New color in hex format
     * @param actionName Undo action name
     * @return true if found and recolored
     * @note Message thread only, undoable
     */
    bool recolorMarker(const juce::String& markerId, const juce::String& newColorHex,
                       const juce::String& actionName);

    /**
     * @brief Delete a marker
     * @param markerId Marker ID
     * @param actionName Undo action name
     * @return true if found and deleted
     * @note Message thread only, undoable
     */
    bool deleteMarker(const juce::String& markerId, const juce::String& actionName);

    //==========================================================================
    // Phase 13: Automation Management
    //==========================================================================

    /**
     * @brief Get or create automation envelope for a track parameter
     * @param trackId Track ID
     * @param paramId Parameter ID ("volume", "pan", or "mute")
     * @return Envelope ValueTree (creates if doesn't exist)
     * @note Message thread only
     */
    juce::ValueTree getOrCreateAutomationEnvelope(const juce::String& trackId, const juce::String& paramId);

    /**
     * @brief Get automation envelope for a track parameter
     * @param trackId Track ID
     * @param paramId Parameter ID ("volume", "pan", or "mute")
     * @return Envelope ValueTree (invalid if doesn't exist)
     * @note Message thread only
     */
    juce::ValueTree getAutomationEnvelope(const juce::String& trackId, const juce::String& paramId) const;

    /**
     * @brief Check if track has automation for a parameter
     * @param trackId Track ID
     * @param paramId Parameter ID ("volume", "pan", or "mute")
     * @return true if automation exists
     * @note Message thread only
     */
    bool hasAutomation(const juce::String& trackId, const juce::String& paramId) const;

    /**
     * @brief Add automation point
     * @param trackId Track ID
     * @param paramId Parameter ID ("volume", "pan", or "mute")
     * @param timeBeats Time in beats
     * @param value Normalized value (0-1 for volume, -1 to 1 for pan, 0/1 for mute)
     * @param actionName Undo action name
     * @return Generated point ID
     * @note Message thread only, undoable
     */
    juce::String addAutomationPoint(const juce::String& trackId, const juce::String& paramId,
                                     double timeBeats, double value, const juce::String& actionName);

    /**
     * @brief Move automation point
     * @param trackId Track ID
     * @param paramId Parameter ID ("volume", "pan", or "mute")
     * @param pointId Point ID
     * @param newTimeBeats New time in beats
     * @param newValue New value
     * @param actionName Undo action name
     * @return true if point was found and moved
     * @note Message thread only, undoable
     */
    bool moveAutomationPoint(const juce::String& trackId, const juce::String& paramId,
                             const juce::String& pointId, double newTimeBeats, double newValue,
                             const juce::String& actionName);

    /**
     * @brief Delete automation point
     * @param trackId Track ID
     * @param paramId Parameter ID ("volume", "pan", or "mute")
     * @param pointId Point ID
     * @param actionName Undo action name
     * @return true if point was found and deleted
     * @note Message thread only, undoable
     */
    bool deleteAutomationPoint(const juce::String& trackId, const juce::String& paramId,
                                const juce::String& pointId, const juce::String& actionName);

    /**
     * @brief Clear all automation for a parameter
     * @param trackId Track ID
     * @param paramId Parameter ID ("volume", "pan", or "mute")
     * @param actionName Undo action name
     * @return true if automation was found and cleared
     * @note Message thread only, undoable
     */
    bool clearAutomation(const juce::String& trackId, const juce::String& paramId,
                         const juce::String& actionName);

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
