/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_data_structures/juce_data_structures.h>
#include <atomic>
#include <functional>
#include <memory>
#include <vector>

#include "../engine/EngineConstants.h"

namespace zenith {

// Forward declarations
class Track;
class Engine;

//==============================================================================
/**
    Per-track freeze state and buffer ownership.
*/
class TrackFreezeState {
public:
    TrackFreezeState() = default;
    ~TrackFreezeState() = default;

    void setFrozen(bool shouldBeFrozen) { frozen_.store(shouldBeFrozen); }
    bool isFrozen() const { return frozen_.load(); }

    void setBeingFrozen(bool shouldBeFrozen) { isBeingFrozen_.store(shouldBeFrozen); }
    bool isBeingFrozen() const { return isBeingFrozen_.load(); }

    void setFreezeFile(const juce::File& file);
    const juce::File& getFreezeFile() const { return freezeFile_; }

    juce::AudioBuffer<float>* getFreezeBuffer() const {
        return activeFreezeBuffer_.load(std::memory_order_acquire);
    }

private:
    std::atomic<bool> frozen_{false};
    std::atomic<bool> isBeingFrozen_{false};
    juce::File freezeFile_;

    std::shared_ptr<juce::AudioBuffer<float>> freezeBufferOwner_;
    std::atomic<juce::AudioBuffer<float>*> activeFreezeBuffer_{nullptr};
    juce::AudioFormatManager freezeFormatManager_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackFreezeState)
};

//==============================================================================
/**
    Manages track freeze/unfreeze operations for CPU optimization.
    
    Freezing renders the track with all its processing to an audio file,
    then disables the plugins to save CPU while maintaining the sound.
*/
class TrackFreezeManager {
public:
    //==========================================================================
    // Progress callback type: (progress 0.0-1.0, status message)
    using ProgressCallback = std::function<void(float, const juce::String&)>;

    //==========================================================================
    TrackFreezeManager() = default;
    ~TrackFreezeManager();

    //==========================================================================
    // Freeze Operations
    //==========================================================================

    /**
     * @brief Freeze a track, rendering its output to audio
     * @param track Track to freeze
     * @param engine Reference to engine for rendering
     * @param outputDir Directory for freeze files
     * @param progress Optional progress callback
     * @return true if freeze started successfully
     * @note MESSAGE THREAD ONLY - rendering happens asynchronously
     */
    bool freezeTrack(Track& track, 
                     Engine& engine,
                     const juce::File& outputDir,
                     ProgressCallback progress = nullptr);

    /**
     * @brief Unfreeze a track, restoring original plugins
     * @param track Track to unfreeze
     * @return true if unfreeze succeeded
     * @note MESSAGE THREAD ONLY
     */
    bool unfreezeTrack(Track& track);

    /**
     * @brief Check if a track is frozen
     * @param track Track to check
     * @return true if track is frozen
     * @note Thread-safe
     */
    bool isFrozen(const Track& track) const;

    /**
     * @brief Cancel an in-progress freeze operation
     */
    void cancelFreeze();

    /**
     * @brief Check if a freeze operation is in progress
     */
    bool isFreezing() const { return isFreezing_.load(); }

    //==========================================================================
    // Freeze State Structure
    //==========================================================================
    
    struct FreezeState {
        juce::String trackId;
        juce::File freezeFile;
        juce::ValueTree pluginStates;  // Saved states of all disabled plugins
        juce::MemoryBlock instrumentState;  // Saved instrument state
        bool wasArmed = false;
        bool hadInstrument = false;
        
        FreezeState() = default;
    };

private:
    //==========================================================================
    // Internal Methods
    //==========================================================================

    /**
     * @brief Render track to freeze file
     * @note Runs on background thread
     */
    void renderFreezeFile(Track& track, 
                          Engine& engine,
                          const juce::File& outputFile,
                          ProgressCallback progress);

    /**
     * @brief Save plugin states for a track
     */
    juce::ValueTree savePluginStates(Track& track);

    /**
     * @brief Restore plugin states for a track
     */
    void restorePluginStates(Track& track, const juce::ValueTree& states);

    /**
     * @brief Get or create freeze state for a track
     */
    FreezeState* getFreezeState(const juce::String& trackId);

    //==========================================================================
    // Member Variables
    //==========================================================================

    // Map of track ID to freeze state
    std::map<juce::String, FreezeState> freezeStates_;
    
    // Freeze operation state
    std::atomic<bool> isFreezing_{false};
    std::atomic<bool> shouldCancel_{false};
    
    // Background thread for rendering
    std::unique_ptr<juce::Thread> freezeThread_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackFreezeManager)
};

//==============================================================================
/**
    Background thread for freeze rendering.
*/
class FreezeRenderThread : public juce::Thread {
public:
    FreezeRenderThread(Track& track,
                       Engine& engine,
                       const juce::File& outputFile,
                       TrackFreezeManager::ProgressCallback progress,
                       std::atomic<bool>& shouldCancel)
        : juce::Thread("Freeze Render"),
          track_(track),
          engine_(engine),
          outputFile_(outputFile),
          progress_(progress),
          shouldCancel_(shouldCancel) {}

    void run() override;

private:
    Track& track_;
    Engine& engine_;
    juce::File outputFile_;
    TrackFreezeManager::ProgressCallback progress_;
    std::atomic<bool>& shouldCancel_;
};

} // namespace zenith
