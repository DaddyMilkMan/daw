/*
  ==============================================================================

    AudioFilePool.h
    Created: 2025-11-14
    Author:  Zenith DAW

    Manages pooled audio file loading and playback for audio clips.

    Provides efficient shared access to audio files without redundant loading.
    Thread-safe for concurrent access from audio and message threads.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <unordered_map>
#include <memory>

namespace zenith {

//==============================================================================
/**
    Audio file pool for efficient loading and shared access to audio files.

    This class manages a pool of audio files, loading them into memory
    and providing shared access to avoid redundant disk reads.

    Thread-safe: Uses critical sections to protect the file map.
*/
class AudioFilePool
{
public:
    //==============================================================================
    /**
        Handle to an audio file in the pool.

        Contains the audio buffer and metadata for a loaded file.
    */
    struct AudioFileHandle
    {
        juce::AudioBuffer<float> buffer;
        double sampleRate = 44100.0;
        int numChannels = 0;
        juce::int64 lengthInSamples = 0;
        juce::String filePath;

        bool isValid() const { return numChannels > 0 && lengthInSamples > 0; }
    };

    //==============================================================================
    AudioFilePool();
    ~AudioFilePool();

    //==============================================================================
    /**
        Load an audio file into the pool.

        If the file is already loaded, returns the existing handle.

        @param file The audio file to load
        @return Shared pointer to the audio file handle, or nullptr on failure
    */
    std::shared_ptr<AudioFileHandle> loadFile(const juce::File& file);

    /**
        Get a handle to a previously loaded file.

        @param file The audio file to get
        @return Shared pointer to the audio file handle, or nullptr if not loaded
    */
    std::shared_ptr<AudioFileHandle> getFile(const juce::File& file) const;

    /**
        Remove a file from the pool.

        @param file The audio file to remove
    */
    void removeFile(const juce::File& file);

    /**
        Clear all files from the pool.
    */
    void clear();

    /**
        Get the number of files in the pool.
    */
    int getNumFiles() const;

private:
    //==============================================================================
    // File pool (path -> handle)
    std::unordered_map<juce::String, std::shared_ptr<AudioFileHandle>> filePool;

    // Thread safety
    mutable juce::CriticalSection poolLock;

    // Audio format manager for loading files
    juce::AudioFormatManager formatManager;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioFilePool)
};

} // namespace zenith
