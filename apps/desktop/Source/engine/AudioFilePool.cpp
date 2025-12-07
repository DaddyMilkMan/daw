/*
  ==============================================================================

    AudioFilePool.cpp
    Created for Phase 1.2: Audio File Pool & Caching
    Author:  Zenith DAW

  ==============================================================================
*/

#include "AudioFilePool.h"

namespace zenith {

//==============================================================================
AudioFilePool::AudioFilePool()
{
    // Register basic audio formats (WAV, AIFF, OGG, FLAC, MP3)
    formatManager_.registerBasicFormats();
}

AudioFilePool::~AudioFilePool()
{
    clear();
}

//==============================================================================
AudioFilePool::HandlePtr AudioFilePool::loadFile(const juce::File& file, juce::String& errorMessage)
{
    // ⚠️ MESSAGE THREAD ONLY - Does file I/O!

    // Security: Validate and sanitize file path
    // Prevent path traversal attacks (e.g., ../../../etc/passwd)
    juce::String filePath = file.getFullPathName();
    
    // Check for path traversal attempts
    if (filePath.contains("..") || filePath.contains("~"))
    {
        errorMessage = "Invalid file path: path traversal detected";
        return nullptr;
    }
    
    // Check path length (prevent excessive paths)
    if (filePath.length() > 4096)  // MAX_PATH on Windows is 260, but allow longer for safety
    {
        errorMessage = "File path too long (max 4096 characters)";
        return nullptr;
    }
    
    // Validate file exists and is actually a file (not directory)
    if (!file.existsAsFile())
    {
        errorMessage = "File does not exist: " + file.getFullPathName();
        return nullptr;
    }
    
    // Additional security: Ensure file is readable
    if (!file.hasReadAccess())
    {
        errorMessage = "File is not readable: " + file.getFullPathName();
        return nullptr;
    }

    const auto filePath = file.getFullPathName();

    // Check if already loaded
    {
        const juce::ScopedLock sl(cacheLock_);
        auto it = fileCache_.find(filePath);
        if (it != fileCache_.end())
        {
            return it->second;  // Return existing handle
        }
    }

    // Load the file
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager_.createReaderFor(file));

    if (reader == nullptr)
    {
        errorMessage = "Failed to create reader for: " + file.getFullPathName();
        return nullptr;
    }

    // Create handle and load entire file into memory
    auto handle = std::make_shared<AudioFileHandle>();
    handle->sourceFile = file;
    handle->sampleRate = reader->sampleRate;
    handle->lengthInSamples = reader->lengthInSamples;
    handle->numChannels = static_cast<int>(reader->numChannels);

    // Allocate buffer and read entire file
    handle->buffer.setSize(static_cast<int>(reader->numChannels),
                          static_cast<int>(reader->lengthInSamples));

    if (!reader->read(&handle->buffer,
                      0,
                      static_cast<int>(reader->lengthInSamples),
                      0,
                      true,
                      true))
    {
        errorMessage = "Failed to read audio data from: " + file.getFullPathName();
        return nullptr;
    }

    // Store in cache
    {
        const juce::ScopedLock sl(cacheLock_);
        fileCache_[filePath] = handle;
    }

    DBG("AudioFilePool: Loaded " + file.getFileName() +
        " (" + juce::String(handle->lengthInSamples) + " samples, " +
        juce::String(handle->numChannels) + " channels)");

    return handle;
}

AudioFilePool::HandlePtr AudioFilePool::loadFile(const juce::File& file)
{
    juce::String errorMessage;
    return loadFile(file, errorMessage);
}

AudioFilePool::HandlePtr AudioFilePool::getFile(const juce::File& file) const
{
    const juce::ScopedLock sl(cacheLock_);
    auto it = fileCache_.find(file.getFullPathName());
    return (it != fileCache_.end()) ? it->second : nullptr;
}

bool AudioFilePool::isLoaded(const juce::File& file) const
{
    const juce::ScopedLock sl(cacheLock_);
    return fileCache_.find(file.getFullPathName()) != fileCache_.end();
}

void AudioFilePool::unloadFile(const juce::File& file)
{
    const juce::ScopedLock sl(cacheLock_);
    auto it = fileCache_.find(file.getFullPathName());
    if (it != fileCache_.end())
    {
        DBG("AudioFilePool: Unloading " + file.getFileName());
        fileCache_.erase(it);
        // Actual memory will be freed when last shared_ptr reference is released
    }
}

void AudioFilePool::clear()
{
    const juce::ScopedLock sl(cacheLock_);
    DBG("AudioFilePool: Clearing all files (" + juce::String(fileCache_.size()) + " loaded)");
    fileCache_.clear();
}

int AudioFilePool::getNumLoadedFiles() const
{
    const juce::ScopedLock sl(cacheLock_);
    return static_cast<int>(fileCache_.size());
}

juce::int64 AudioFilePool::getMemoryUsage() const
{
    const juce::ScopedLock sl(cacheLock_);
    juce::int64 totalBytes = 0;

    for (const auto& pair : fileCache_)
    {
        const auto& handle = pair.second;
        if (handle)
        {
            // Size = numChannels * numSamples * sizeof(float)
            totalBytes += handle->buffer.getNumChannels() *
                         handle->buffer.getNumSamples() *
                         static_cast<juce::int64>(sizeof(float));
        }
    }

    return totalBytes;
}

} // namespace zenith

