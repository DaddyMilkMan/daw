/*
  ==============================================================================

    ProjectState.h
    Created: 2025-11-10

    Project state management using JUCE ValueTree + UndoManager.

    Architecture:
    - Single ValueTree is the source of truth for all project data
    - UndoManager provides full undo/redo support
    - UI listens to ValueTree::Listener for reactive updates
    - Audio thread reads double-buffered snapshots (never directly)

    Schema:
      PROJECT
      ├── TRACKS
      │   ├── TRACK (id, name, numChannels, color, volume, pan, mute, solo)
      │   │   ├── CLIPS
      │   │   │   └── CLIP (id, type, startTime, length, offset, file)
      │   │   └── PLUGINS
      │   │       └── PLUGIN (id, path, state)
      │   └── ...
      ├── AUTOMATION
      │   └── CURVE (trackId, parameterId, points)
      └── SETTINGS (tempo, timeSignature, sampleRate, etc.)

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
    Manages the entire project state using a ValueTree.

    All state modifications are undoable via the built-in UndoManager.
    Components should listen to the ValueTree for reactive updates.
*/
class ProjectState
{
public:
    //==========================================================================
    ProjectState();
    ~ProjectState();

    //==========================================================================
    // ValueTree Access

    /** Returns the root ValueTree (read-only). */
    const juce::ValueTree& getState() const { return projectTree; }

    /** Returns the root ValueTree (for modification). */
    juce::ValueTree& getState() { return projectTree; }

    /** Returns the UndoManager for undo/redo operations. */
    juce::UndoManager& getUndoManager() { return undoManager; }

    //==========================================================================
    // Save/Load

    /** Saves the project to an XML file. */
    bool saveToFile(const juce::File& file);

    /** Loads the project from an XML file. */
    bool loadFromFile(const juce::File& file);

    /** Returns true if there are unsaved changes. */
    bool hasUnsavedChanges() const { return undoManager.getNumActionsInCurrentTransaction() > 0; }

    /** Marks the current state as saved (clears unsaved flag). */
    void markAsSaved();

    //==========================================================================
    // Project Settings

    void setTempo(double bpm);
    double getTempo() const;

    void setTimeSignature(int numerator, int denominator);
    void getTimeSignature(int& numerator, int& denominator) const;

    void setSampleRate(double rate);
    double getSampleRate() const;

    //==========================================================================
    // Track Management

    /** Adds a new track. Returns the track ValueTree. */
    juce::ValueTree addTrack(const juce::String& trackId,
                             const juce::String& trackName,
                             int numChannels);

    /** Removes a track by ID. */
    void removeTrack(const juce::String& trackId);

    /** Finds a track by ID. Returns invalid ValueTree if not found. */
    juce::ValueTree findTrack(const juce::String& trackId) const;

    /** Returns all tracks. */
    juce::ValueTree getTracksTree() const;

    //==========================================================================
    // Clip Management

    /** Adds a clip to a track. */
    juce::ValueTree addClip(const juce::String& trackId,
                            const juce::String& clipId,
                            const juce::String& clipType,
                            double startTime,
                            double length);

    /** Removes a clip by ID. */
    void removeClip(const juce::String& trackId, const juce::String& clipId);

    /** Finds a clip by ID within a track. */
    juce::ValueTree findClip(const juce::String& trackId, const juce::String& clipId) const;

    //==========================================================================
    // Plugin Management

    /** Adds a plugin to a track's insert chain. */
    juce::ValueTree addPlugin(const juce::String& trackId,
                              const juce::String& pluginId,
                              const juce::String& pluginPath);

    /** Removes a plugin from a track. */
    void removePlugin(const juce::String& trackId, const juce::String& pluginId);

    //==========================================================================
    // Identifiers (for ValueTree types and properties)

    static const juce::Identifier ID_PROJECT;
    static const juce::Identifier ID_TRACKS;
    static const juce::Identifier ID_TRACK;
    static const juce::Identifier ID_CLIPS;
    static const juce::Identifier ID_CLIP;
    static const juce::Identifier ID_PLUGINS;
    static const juce::Identifier ID_PLUGIN;
    static const juce::Identifier ID_AUTOMATION;
    static const juce::Identifier ID_SETTINGS;

    // Common properties
    static const juce::Identifier PROP_ID;
    static const juce::Identifier PROP_NAME;
    static const juce::Identifier PROP_TYPE;
    static const juce::Identifier PROP_NUM_CHANNELS;
    static const juce::Identifier PROP_COLOR;
    static const juce::Identifier PROP_VOLUME;
    static const juce::Identifier PROP_PAN;
    static const juce::Identifier PROP_MUTE;
    static const juce::Identifier PROP_SOLO;

    // Clip properties
    static const juce::Identifier PROP_START_TIME;
    static const juce::Identifier PROP_LENGTH;
    static const juce::Identifier PROP_OFFSET;
    static const juce::Identifier PROP_FILE;

    // Plugin properties
    static const juce::Identifier PROP_PATH;
    static const juce::Identifier PROP_STATE;

    // Settings properties
    static const juce::Identifier PROP_TEMPO;
    static const juce::Identifier PROP_TIME_SIG_NUM;
    static const juce::Identifier PROP_TIME_SIG_DEN;
    static const juce::Identifier PROP_SAMPLE_RATE;

private:
    //==========================================================================
    /** Initializes a new empty project tree. */
    void createDefaultProjectTree();

    /** Returns the settings subtree. */
    juce::ValueTree getSettingsTree() const;

    //==========================================================================
    juce::ValueTree projectTree;
    juce::UndoManager undoManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProjectState)
};
