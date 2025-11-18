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
    static const juce::Identifier PROP_OFFSET;
    static const juce::Identifier PROP_AUDIO_FILE;
    static const juce::Identifier PROP_ARMED;

    // Phase 13: Automation properties
    static const juce::Identifier PROP_PARAM;
    static const juce::Identifier PROP_TIME_BEATS;
    static const juce::Identifier PROP_VALUE;

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

    /**
     * @brief Rename a track (undoable)
     * @param trackId Track ID
     * @param newName New track name
     * @param actionName Optional undo action name
     */
    void renameTrack(const juce::String& trackId, const juce::String& newName,
                     const juce::String& actionName = "Rename track");

    /**
     * @brief Set track volume (undoable)
     * @param trackId Track ID
     * @param volumeLinear Volume (0.0 to 2.0)
     * @param actionName Optional undo action name
     */
    void setTrackVolume(const juce::String& trackId, float volumeLinear,
                        const juce::String& actionName = "Set track volume");

    /**
     * @brief Set track pan (undoable)
     * @param trackId Track ID
     * @param pan Pan (-1.0 to 1.0)
     * @param actionName Optional undo action name
     */
    void setTrackPan(const juce::String& trackId, float pan,
                     const juce::String& actionName = "Set track pan");

    /**
     * @brief Set track mute (undoable)
     * @param trackId Track ID
     * @param muted Mute state
     * @param actionName Optional undo action name
     */
    void setTrackMute(const juce::String& trackId, bool muted,
                      const juce::String& actionName = "Set track mute");

    /**
     * @brief Set track solo (undoable)
     * @param trackId Track ID
     * @param soloed Solo state
     * @param actionName Optional undo action name
     */
    void setTrackSolo(const juce::String& trackId, bool soloed,
                      const juce::String& actionName = "Set track solo");

    /**
     * @brief Set track armed (undoable)
     * @param trackId Track ID
     * @param armed Armed state
     * @param actionName Optional undo action name
     */
    void setTrackArmed(const juce::String& trackId, bool armed,
                       const juce::String& actionName = "Set track armed");

    /**
     * @brief Get track ValueTree by ID
     * @param trackId Track ID
     * @return Track ValueTree (invalid if not found)
     */
    juce::ValueTree getTrack(const juce::String& trackId) const;

    //==========================================================================
    // Clip Management
    //==========================================================================

    /**
     * @brief Create a new clip (undoable)
     * @param trackId Track ID
     * @param clipType "audio" or "midi"
     * @param startSamples Start position in samples
     * @param lengthSamples Length in samples
     * @param name Clip name
     * @param actionName Optional undo action name
     * @return Clip ID
     */
    juce::String createClip(const juce::String& trackId, const juce::String& clipType,
                           juce::int64 startSamples, juce::int64 lengthSamples,
                           const juce::String& name,
                           const juce::String& actionName = "Create clip");

    /**
     * @brief Delete a clip (undoable)
     * @param trackId Track ID
     * @param clipId Clip ID
     * @param actionName Optional undo action name
     */
    void deleteClip(const juce::String& trackId, const juce::String& clipId,
                    const juce::String& actionName = "Delete clip");

    /**
     * @brief Move a clip (undoable)
     * @param trackId Track ID
     * @param clipId Clip ID
     * @param newStartSamples New start position in samples
     * @param actionName Optional undo action name
     */
    void moveClip(const juce::String& trackId, const juce::String& clipId,
                  juce::int64 newStartSamples,
                  const juce::String& actionName = "Move clip");

    /**
     * @brief Split a clip (undoable)
     * @param trackId Track ID
     * @param clipId Clip ID
     * @param splitSamples Split position in samples
     * @param actionName Optional undo action name
     * @return Pair of new clip IDs (left, right)
     */
    std::pair<juce::String, juce::String> splitClip(const juce::String& trackId,
                                                     const juce::String& clipId,
                                                     juce::int64 splitSamples,
                                                     const juce::String& actionName = "Split clip");

    /**
     * @brief Add a clip to a track
     * @param trackId Track ID
     * @param startBeats Start position in beats
     * @param lengthBeats Length in beats
     * @param actionName Undo action name
     * @return Clip ID
     */
    juce::String addClip(const juce::String& trackId, double startBeats, double lengthBeats, const juce::String& actionName);

    /**
     * @brief Remove a clip from a track
     * @param trackId Track ID
     * @param clipId Clip ID
     * @param actionName Undo action name
     * @return true if clip was found and removed
     */
    bool removeClip(const juce::String& trackId, const juce::String& clipId, const juce::String& actionName);

    /**
     * @brief Set audio file for a clip
     * @param trackId Track ID
     * @param clipId Clip ID
     * @param audioFile Audio file path (will be stored as relative to project file if possible)
     * @param actionName Undo action name
     * @return true if clip was found and updated
     */
    bool setClipAudioFile(const juce::String& trackId, const juce::String& clipId, const juce::File& audioFile, const juce::String& actionName);

    /**
     * @brief Get audio file for a clip
     * @param trackId Track ID
     * @param clipId Clip ID
     * @return Audio file path (empty if not set or clip not found)
     */
    juce::String getClipAudioFile(const juce::String& trackId, const juce::String& clipId) const;

    /**
     * @brief Get clip ValueTree
     * @param trackId Track ID
     * @param clipId Clip ID
     * @return Clip ValueTree (invalid if not found)
     */
    juce::ValueTree getClip(const juce::String& trackId, const juce::String& clipId) const;

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
