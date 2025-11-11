/**
 * @file AutoSaveManager.h
 * @brief Background auto-save manager for Zenith DAW
 *
 * Features:
 * - Non-blocking background saves using TimeSliceThread
 * - Throttled saves (prevents excessive I/O)
 * - Dirty state tracking (only saves when changes occur)
 * - Atomic file operations (temp file + rename)
 * - Thread-safe ValueTree listener
 * - Configurable auto-save interval
 * - Does NOT block audio thread
 *
 * Implementation based on professional DAW best practices:
 * - Uses throttle (not debounce) to prevent data loss
 * - Saves to temporary file first, then atomic rename
 * - Runs on background thread (TimeSliceThread)
 * - Listeners run on message thread (thread-safe)
 * - 3-5 second intervals optimal for DAW workflow
 */

#pragma once

#include <JuceHeader.h>
#include "ProjectState.h"

//==============================================================================
/**
 * @class AutoSaveManager
 * @brief Manages automatic background saving of project state
 *
 * Usage:
 * @code
 * AutoSaveManager autoSave;
 * autoSave.setProjectState(&projectState);
 * autoSave.setAutoSaveInterval(5000); // 5 seconds
 * autoSave.enable(true);
 * @endcode
 *
 * Thread Safety:
 * - ValueTree::Listener callbacks run on MESSAGE THREAD
 * - TimeSliceThread runs on BACKGROUND THREAD
 * - Uses std::atomic for thread-safe flags
 * - Never blocks AUDIO THREAD
 */
class AutoSaveManager : public juce::ValueTree::Listener,
                         public juce::TimeSliceClient
{
public:
    //==========================================================================
    AutoSaveManager();
    ~AutoSaveManager() override;

    //==========================================================================
    // Configuration
    //==========================================================================

    /**
     * @brief Set the project state to auto-save
     * @param state Pointer to ProjectState (must remain valid)
     */
    void setProjectState(ProjectState* state);

    /**
     * @brief Set the auto-save directory
     * @param directory Directory for auto-save files
     */
    void setAutoSaveDirectory(const juce::File& directory);

    /**
     * @brief Set the auto-save interval
     * @param intervalMs Interval in milliseconds (default: 5000ms)
     *
     * Recommended intervals:
     * - 3000ms (3s): Aggressive auto-save, minimal data loss
     * - 5000ms (5s): Balanced (recommended)
     * - 10000ms (10s): Less frequent, more user control
     */
    void setAutoSaveInterval(int intervalMs);

    /**
     * @brief Enable or disable auto-save
     * @param shouldBeEnabled true to enable, false to disable
     */
    void enable(bool shouldBeEnabled);

    /**
     * @brief Check if auto-save is enabled
     */
    bool isEnabled() const { return enabled.load(); }

    //==========================================================================
    // Manual Operations
    //==========================================================================

    /**
     * @brief Force an immediate save (bypasses throttle)
     * @return true if save succeeded
     */
    bool forceSave();

    /**
     * @brief Check if there are unsaved changes
     */
    bool isDirty() const { return dirty.load(); }

    /**
     * @brief Get time since last auto-save (ms)
     */
    juce::int64 getTimeSinceLastSave() const;

    /**
     * @brief Get the most recent auto-save file
     */
    juce::File getLastAutoSaveFile() const { return lastAutoSaveFile; }

    //==========================================================================
    // Listeners (for UI feedback)
    //==========================================================================

    /**
     * @class Listener
     * @brief Interface for auto-save status callbacks
     */
    class Listener
    {
    public:
        virtual ~Listener() = default;

        /** Called when auto-save starts */
        virtual void autoSaveStarted() {}

        /** Called when auto-save completes successfully */
        virtual void autoSaveCompleted(const juce::File& file) {}

        /** Called when auto-save fails */
        virtual void autoSaveFailed(const juce::String& errorMessage) {}

        /** Called when dirty state changes */
        virtual void dirtyStateChanged(bool isDirty) {}
    };

    /**
     * @brief Add a listener
     */
    void addListener(Listener* listener);

    /**
     * @brief Remove a listener
     */
    void removeListener(Listener* listener);

    //==========================================================================
    // ValueTree::Listener interface (MESSAGE THREAD)
    //==========================================================================

    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override;
    void valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index) override;
    void valueTreeChildOrderChanged(juce::ValueTree& parent, int oldIndex, int newIndex) override;
    void valueTreeParentChanged(juce::ValueTree& tree) override;

    //==========================================================================
    // TimeSliceClient interface (BACKGROUND THREAD)
    //==========================================================================

    /**
     * @brief Called periodically on background thread
     * @return Milliseconds until next call
     *
     * This is where we perform the actual save operation.
     * Runs on TimeSliceThread, NOT on audio thread.
     */
    int useTimeSlice() override;

private:
    //==========================================================================
    // Internal Methods
    //==========================================================================

    /**
     * @brief Mark project as dirty (has unsaved changes)
     */
    void markDirty();

    /**
     * @brief Perform the actual save operation
     * @return true if save succeeded
     *
     * This runs on the BACKGROUND THREAD (TimeSliceThread)
     * Safe to do I/O operations here.
     */
    bool performSave();

    /**
     * @brief Notify all listeners of an event
     */
    void notifyListeners(std::function<void(Listener*)> callback);

    //==========================================================================
    // Member Variables
    //==========================================================================

    // Project state to auto-save
    ProjectState* projectState{nullptr};

    // Auto-save directory
    juce::File autoSaveDirectory;
    juce::File lastAutoSaveFile;

    // Background thread for saving
    juce::TimeSliceThread saveThread{"AutoSave Thread"};

    // Listeners
    juce::ListenerList<Listener> listeners;

    // State flags (std::atomic for thread safety)
    std::atomic<bool> enabled{false};
    std::atomic<bool> dirty{false};
    std::atomic<bool> currentlySaving{false};

    // Timing
    std::atomic<int> autoSaveIntervalMs{5000}; // 5 seconds default
    juce::int64 lastSaveTime{0};
    std::atomic<juce::int64> lastModifiedTime{0};

    // Throttle control
    juce::int64 lastThrottleTime{0};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutoSaveManager)
};
