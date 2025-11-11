/**
 * @file AutoSaveManager.cpp
 * @brief Auto-save manager implementation
 */

#include "../include/AutoSaveManager.h"

//==============================================================================
AutoSaveManager::AutoSaveManager()
{
    DBG("AutoSaveManager: Constructor");

    // Start background thread
    saveThread.startThread(juce::Thread::Priority::background);
}

AutoSaveManager::~AutoSaveManager()
{
    DBG("AutoSaveManager: Destructor");

    // Disable auto-save
    enable(false);

    // Stop background thread
    saveThread.stopThread(1000); // Wait up to 1 second

    // Remove listener if attached
    if (projectState != nullptr)
    {
        projectState->getState().removeListener(this);
    }
}

//==============================================================================
// Configuration
//==============================================================================

void AutoSaveManager::setProjectState(ProjectState* state)
{
    // Remove old listener
    if (projectState != nullptr)
    {
        projectState->getState().removeListener(this);
    }

    projectState = state;

    // Add new listener
    if (projectState != nullptr)
    {
        projectState->getState().addListener(this);
        DBG("AutoSaveManager: Attached to project state");
    }
}

void AutoSaveManager::setAutoSaveDirectory(const juce::File& directory)
{
    autoSaveDirectory = directory;

    // Create directory if it doesn't exist
    if (!autoSaveDirectory.exists())
    {
        autoSaveDirectory.createDirectory();
    }

    DBG("AutoSaveManager: Auto-save directory set to " + directory.getFullPathName());
}

void AutoSaveManager::setAutoSaveInterval(int intervalMs)
{
    // Clamp to reasonable range (1-60 seconds)
    intervalMs = juce::jlimit(1000, 60000, intervalMs);
    autoSaveIntervalMs.store(intervalMs);

    DBG("AutoSaveManager: Auto-save interval set to " + juce::String(intervalMs) + "ms");
}

void AutoSaveManager::enable(bool shouldBeEnabled)
{
    if (shouldBeEnabled == enabled.load())
        return;

    enabled.store(shouldBeEnabled);

    if (shouldBeEnabled)
    {
        if (projectState == nullptr)
        {
            DBG("AutoSaveManager: ERROR - Cannot enable without project state");
            enabled.store(false);
            return;
        }

        if (!autoSaveDirectory.isDirectory())
        {
            DBG("AutoSaveManager: ERROR - Invalid auto-save directory");
            enabled.store(false);
            return;
        }

        // Add to time slice thread
        saveThread.addTimeSliceClient(this);

        DBG("AutoSaveManager: Enabled");
    }
    else
    {
        // Remove from time slice thread
        saveThread.removeTimeSliceClient(this);

        DBG("AutoSaveManager: Disabled");
    }
}

//==============================================================================
// Manual Operations
//==============================================================================

bool AutoSaveManager::forceSave()
{
    if (projectState == nullptr)
        return false;

    DBG("AutoSaveManager: Force save requested");

    bool success = performSave();

    if (success)
    {
        dirty.store(false);
        notifyListeners([this](Listener* l) { l->dirtyStateChanged(false); });
    }

    return success;
}

juce::int64 AutoSaveManager::getTimeSinceLastSave() const
{
    if (lastSaveTime == 0)
        return -1;

    return juce::Time::getCurrentTime().toMilliseconds() - lastSaveTime;
}

//==============================================================================
// Listeners
//==============================================================================

void AutoSaveManager::addListener(Listener* listener)
{
    listeners.add(listener);
}

void AutoSaveManager::removeListener(Listener* listener)
{
    listeners.remove(listener);
}

//==============================================================================
// ValueTree::Listener interface (MESSAGE THREAD)
//==============================================================================

void AutoSaveManager::valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property)
{
    markDirty();
}

void AutoSaveManager::valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child)
{
    markDirty();
}

void AutoSaveManager::valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index)
{
    markDirty();
}

void AutoSaveManager::valueTreeChildOrderChanged(juce::ValueTree& parent, int oldIndex, int newIndex)
{
    markDirty();
}

void AutoSaveManager::valueTreeParentChanged(juce::ValueTree& tree)
{
    markDirty();
}

//==============================================================================
// TimeSliceClient interface (BACKGROUND THREAD)
//==============================================================================

int AutoSaveManager::useTimeSlice()
{
    if (!enabled.load() || !dirty.load() || currentlySaving.load())
    {
        // Nothing to do - check again in 1 second
        return 1000;
    }

    const auto now = juce::Time::getCurrentTime().toMilliseconds();
    const auto timeSinceLastSave = now - lastSaveTime;
    const auto interval = autoSaveIntervalMs.load();

    // Throttle: only save if enough time has passed
    if (timeSinceLastSave < interval)
    {
        // Not enough time has passed - check again when interval is up
        return static_cast<int>(interval - timeSinceLastSave);
    }

    // Perform the save
    performSave();

    // Check again after the interval
    return interval;
}

//==============================================================================
// Internal Methods
//==============================================================================

void AutoSaveManager::markDirty()
{
    if (!dirty.load())
    {
        dirty.store(true);
        lastModifiedTime.store(juce::Time::getCurrentTime().toMilliseconds());

        DBG("AutoSaveManager: Project marked as dirty");

        notifyListeners([](Listener* l) { l->dirtyStateChanged(true); });
    }
}

bool AutoSaveManager::performSave()
{
    if (projectState == nullptr || currentlySaving.load())
        return false;

    currentlySaving.store(true);

    DBG("AutoSaveManager: Starting auto-save...");

    notifyListeners([](Listener* l) { l->autoSaveStarted(); });

    const auto startTime = juce::Time::getMillisecondCounterHiRes();

    try
    {
        // Generate auto-save filename with timestamp
        const auto timestamp = juce::Time::getCurrentTime().formatted("%Y-%m-%d_%H-%M-%S");
        const auto projectName = projectState->getProjectName().replaceCharacters(" /\\", "___");
        const auto filename = "autosave_" + projectName + "_" + timestamp + ".zth";

        // Create temporary file (atomic save: write to temp, then rename)
        const auto tempFile = autoSaveDirectory.getChildFile(filename + ".tmp");
        const auto finalFile = autoSaveDirectory.getChildFile(filename);

        // Save to temporary file
        // NOTE: This is safe to do on background thread - doesn't block audio
        bool success = projectState->saveToFile(tempFile);

        if (!success)
        {
            DBG("AutoSaveManager: ERROR - Failed to save to temporary file");
            tempFile.deleteFile();

            notifyListeners([](Listener* l) {
                l->autoSaveFailed("Failed to save to temporary file");
            });

            currentlySaving.store(false);
            return false;
        }

        // Atomic rename (much faster than copy, appears instant to OS)
        success = tempFile.moveFileTo(finalFile);

        if (!success)
        {
            DBG("AutoSaveManager: ERROR - Failed to rename temporary file");
            tempFile.deleteFile();

            notifyListeners([](Listener* l) {
                l->autoSaveFailed("Failed to rename temporary file");
            });

            currentlySaving.store(false);
            return false;
        }

        // Success!
        lastAutoSaveFile = finalFile;
        lastSaveTime = juce::Time::getCurrentTime().toMilliseconds();
        dirty.store(false);

        const auto elapsed = juce::Time::getMillisecondCounterHiRes() - startTime;

        DBG("AutoSaveManager: Auto-save completed in " + juce::String(elapsed, 2) + "ms");
        DBG("AutoSaveManager: Saved to " + finalFile.getFullPathName());

        notifyListeners([finalFile](Listener* l) {
            l->autoSaveCompleted(finalFile);
            l->dirtyStateChanged(false);
        });

        // Clean up old auto-save files (keep only last 10)
        auto autoSaveFiles = autoSaveDirectory.findChildFiles(
            juce::File::findFiles,
            false,
            "autosave_*.zth"
        );

        // Sort by modification time (newest first)
        autoSaveFiles.sort();
        std::reverse(autoSaveFiles.begin(), autoSaveFiles.end());

        // Delete old files (keep first 10)
        for (int i = 10; i < autoSaveFiles.size(); ++i)
        {
            DBG("AutoSaveManager: Deleting old auto-save: " + autoSaveFiles[i].getFullPathName());
            autoSaveFiles[i].deleteFile();
        }

        currentlySaving.store(false);
        return true;
    }
    catch (const std::exception& e)
    {
        DBG("AutoSaveManager: EXCEPTION - " + juce::String(e.what()));

        notifyListeners([e](Listener* l) {
            l->autoSaveFailed(e.what());
        });

        currentlySaving.store(false);
        return false;
    }
}

void AutoSaveManager::notifyListeners(std::function<void(Listener*)> callback)
{
    // Call listeners on the message thread (thread-safe)
    juce::MessageManager::callAsync([this, callback]()
    {
        listeners.call(callback);
    });
}
