/*
  ==============================================================================

    AudioFilePool.cpp
    Created: 2025-11-14
    Author:  Zenith DAW

    Audio file pool implementation

  ==============================================================================
*/

#include "AudioFilePool.h"

namespace zenith {

//==============================================================================
AudioFilePool::AudioFilePool()
{
    // Register basic audio formats (WAV, AIFF, FLAC, OGG, etc.)
    formatManager.registerBasicFormats();
}

AudioFilePool::~AudioFilePool()
{
    clear();
}

//==============================================================================
std::shared_ptr<AudioFilePool::AudioFileHandle> AudioFilePool::loadFile(const juce::File& file)
{
    if (!file.existsAsFile())
    {
        DBG("AudioFilePool: File does not exist: " + file.getFullPathName());
        return nullptr;
    }

    const juce::String filePath = file.getFullPathName();

    // Check if already loaded
    {
        const juce::ScopedLock sl(poolLock);

        auto it = filePool.find(filePath);
        if (it != filePool.end())
        {
            DBG("AudioFilePool: File already loaded: " + filePath);
            return it->second;
        }
    }

    // Load the file
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));

    if (reader == nullptr)
    {
        DBG("AudioFilePool: Failed to create reader for: " + filePath);
        return nullptr;
    }

    // Create handle and load audio data
    auto handle = std::make_shared<AudioFileHandle>();
    handle->filePath = filePath;
    handle->sampleRate = reader->sampleRate;
    handle->numChannels = static_cast<int>(reader->numChannels);
    handle->lengthInSamples = reader->lengthInSamples;

    // Allocate buffer and read entire file
    handle->buffer.setSize(handle->numChannels, static_cast<int>(handle->lengthInSamples));

    if (!reader->read(&handle->buffer,
                     0,
                     static_cast<int>(handle->lengthInSamples),
                     0,
                     true,
                     true))
    {
        DBG("AudioFilePool: Failed to read audio data from: " + filePath);
        return nullptr;
    }

    // Add to pool
    {
        const juce::ScopedLock sl(poolLock);
        filePool[filePath] = handle;
    }

    DBG("AudioFilePool: Loaded file: " + filePath +
        " (" + juce::String(handle->numChannels) + " channels, " +
        juce::String(handle->lengthInSamples) + " samples @ " +
        juce::String(handle->sampleRate, 0) + " Hz)");

    return handle;
}

std::shared_ptr<AudioFilePool::AudioFileHandle> AudioFilePool::getFile(const juce::File& file) const
{
    const juce::String filePath = file.getFullPathName();

    const juce::ScopedLock sl(poolLock);

    auto it = filePool.find(filePath);
    if (it != filePool.end())
        return it->second;

    return nullptr;
}

void AudioFilePool::removeFile(const juce::File& file)
{
    const juce::String filePath = file.getFullPathName();

    const juce::ScopedLock sl(poolLock);

    auto it = filePool.find(filePath);
    if (it != filePool.end())
    {
        filePool.erase(it);
        DBG("AudioFilePool: Removed file: " + filePath);
    }
}

void AudioFilePool::clear()
{
    const juce::ScopedLock sl(poolLock);

    filePool.clear();
    DBG("AudioFilePool: Cleared all files");
}

int AudioFilePool::getNumFiles() const
{
    const juce::ScopedLock sl(poolLock);

    return static_cast<int>(filePool.size());
}

} // namespace zenith
