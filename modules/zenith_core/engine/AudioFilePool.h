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

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_events/juce_events.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_data_structures/juce_data_structures.h>
#include <memory>
#include <unordered_map>

namespace zenith {

//==============================================================================
/**
    Manages a pool of pre-loaded audio files for RT-safe playback.

    The pool:
    - Loads audio files on the message thread
    - Stores entire files in memory (for Phase 1; streaming in Phase 2+)
    - Provides shared_ptr access (atomic ref counting, RT-safe)
    - Tracks usage and can unload unused files
*/
class AudioFilePool
{
public:
    //==============================================================================
    struct AudioFileHandle
    {
        juce::AudioBuffer<float> buffer;
        double sampleRate = 44100.0;
        juce::File sourceFile;
        juce::int64 lengthInSamples = 0;
        int numChannels = 0;

        AudioFileHandle() = default;
        AudioFileHandle(const AudioFileHandle&) = delete;
        AudioFileHandle& operator=(const AudioFileHandle&) = delete;

        bool isValid() const { return numChannels > 0 && lengthInSamples > 0; }
    };

    using HandlePtr = std::shared_ptr<const AudioFileHandle>;


    //==============================================================================
    AudioFilePool();
    ~AudioFilePool();

    //==============================================================================
    /**
     * Load an audio file into the pool (MESSAGE THREAD ONLY)
     *
     * @param file The audio file to load
     * @param errorMessage Output parameter for error messages
     * @return Shared pointer to the loaded audio, or nullptr on failure
     *
     * Thread Safety: MUST be called from MESSAGE THREAD (does file I/O)
     */
    HandlePtr loadFile(const juce::File& file, juce::String& errorMessage);

    HandlePtr loadFile(const juce::File& file);

    /**
     * Load an audio file into the pool asynchronously.
     *
     * @param file The audio file to load
     * @param callback Function to call when loading is complete
     */
    void loadFileAsync(const juce::File& file,
                       std::function<void(HandlePtr loadedHandle, juce::String error)> callback);

    /**
     * Get a previously loaded file (thread-safe)
     *
     * @param file The file to retrieve
     * @return Shared pointer to the audio, or nullptr if not loaded
     *
     * Thread Safety: Safe to call from any thread (atomic operations only)
     */
    HandlePtr getFile(const juce::File& file) const;

    /**
     * Check if a file is loaded (thread-safe)
     */
    bool isLoaded(const juce::File& file) const;

    /**
     * Unload a file from the pool (MESSAGE THREAD ONLY)
     *
     * Note: File will only be truly unloaded when all shared_ptr references are released
     */
    void unloadFile(const juce::File& file);

    /**
     * Unload all files (MESSAGE THREAD ONLY)
     */
    void clear();

    /**
     * Get number of loaded files
     */
    int getNumLoadedFiles() const;

    /**
     * Get total memory used by pool (in bytes)
     */
    juce::int64 getMemoryUsage() const;

    /**
     * Get the format manager
     */
    juce::AudioFormatManager& getFormatManager() { return formatManager_; }

private:
    //==============================================================================
    // File cache: path -> audio handle
    std::unordered_map<juce::String, HandlePtr> fileCache_;
    mutable juce::CriticalSection cacheLock_;

    // Audio format manager for loading files
    juce::AudioFormatManager formatManager_;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioFilePool)
};

} // namespace zenith

