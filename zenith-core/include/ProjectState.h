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
    static const juce::Identifier PROP_ARMED;
    static const juce::Identifier PROP_COLOUR;

    static const juce::Identifier PROP_START;
    static const juce::Identifier PROP_LENGTH;

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
     * @brief Get track node by ID
     * @param trackId Track ID
     * @return Track ValueTree (invalid if not found)
     */
    juce::ValueTree getTrack(const juce::String& trackId);

    //==========================================================================
    // Track Properties (Header Controls)
    //==========================================================================

    /**
     * @brief Set track name
     * @param trackId Track ID
     * @param name New track name
     * @param actionName Undo action name
     * @note Message thread only, undoable
     */
    void setTrackName(const juce::String& trackId, const juce::String& name, const juce::String& actionName);

    /**
     * @brief Set track colour
     * @param trackId Track ID
     * @param colour Track colour
     * @param actionName Undo action name
     * @note Message thread only, undoable
     */
    void setTrackColour(const juce::String& trackId, juce::Colour colour, const juce::String& actionName);

    /**
     * @brief Set track mute state
     * @param trackId Track ID
     * @param muted Mute state
     * @param actionName Undo action name
     * @note Message thread only, undoable
     */
    void setTrackMute(const juce::String& trackId, bool muted, const juce::String& actionName);

    /**
     * @brief Set track solo state
     * @param trackId Track ID
     * @param soloed Solo state
     * @param actionName Undo action name
     * @note Message thread only, undoable
     */
    void setTrackSolo(const juce::String& trackId, bool soloed, const juce::String& actionName);

    /**
     * @brief Set track armed state
     * @param trackId Track ID
     * @param armed Armed state
     * @param actionName Undo action name
     * @note Message thread only, undoable
     */
    void setTrackArmed(const juce::String& trackId, bool armed, const juce::String& actionName);

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
