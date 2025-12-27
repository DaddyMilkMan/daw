/*
  ==============================================================================

    BrowserPreviewEngine.cpp
    Created: 2025-12-05
    Author:  Zenith DAW

  ==============================================================================
*/

#include "BrowserPreviewEngine.h"

namespace zenith {

BrowserPreviewEngine::BrowserPreviewEngine()
{
    formatManager_.registerBasicFormats();
}

BrowserPreviewEngine::~BrowserPreviewEngine()
{
    stop();
    releaseResources();
}

//==============================================================================
// AudioSource Implementation
//==============================================================================

void BrowserPreviewEngine::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    sampleRate_ = sampleRate;
    blockSize_ = samplesPerBlockExpected;
    
    transportSource_.prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void BrowserPreviewEngine::releaseResources()
{
    transportSource_.releaseResources();
}

void BrowserPreviewEngine::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    // Use lock to prevent race with loadFile's swap
    const juce::ScopedLock sl(lock_);

    if (!isPlaying_.load())
    {
        bufferToFill.clearActiveBufferRegion();
        return;
    }
    
    // Get audio from transport source
    transportSource_.getNextAudioBlock(bufferToFill);
    
    // Apply volume
    float vol = volume_.load();
    if (vol < 1.0f)
    {
        bufferToFill.buffer->applyGain(bufferToFill.startSample, bufferToFill.numSamples, vol);
    }
    
    // Check if we've reached the end
    if (!transportSource_.isPlaying() && isPlaying_.load())
    {
        if (looping_.load())
        {
            // Loop back to start
            transportSource_.setPosition(0.0);
            transportSource_.start();
        }
        else
        {
            isPlaying_.store(false);
        }
    }
}

//==============================================================================
// Preview Control
//==============================================================================

void BrowserPreviewEngine::loadFile(const juce::File& file, bool autoPlay)
{
    // Async load to prevent UI blocking
    juce::Thread::launch([this, file, autoPlay]() {
        if (!file.existsAsFile()) return;
        
        // Blocking I/O on background thread
        auto* reader = formatManager_.createReaderFor(file);
        
        if (reader) {
            // Update state on message thread
            juce::MessageManager::callAsync([this, reader, file, autoPlay]() {
                const juce::ScopedLock sl(lock_);
                
                stop();
                
                // readerSource_ is unique_ptr, replacing it deletes the old one
                // AudioTransportSource must be updated first or safely
                // setSource(nullptr) stops it from using the old reader
                transportSource_.setSource(nullptr);
                
                readerSource_ = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
                
                transportSource_.setSource(readerSource_.get(), 0, nullptr, reader->sampleRate);
                currentFile_ = file;
                
                DBG("BrowserPreview: Loaded " + file.getFileName() + 
                    " (" + juce::String(getDuration(), 1) + "s)");
                
                if (autoPlay && autoPlayEnabled_.load())
                {
                    play();
                }
            });
        }
    });
}

void BrowserPreviewEngine::loadItem(std::shared_ptr<BrowserItem> item, bool autoPlay)
{
    if (!item) return;
    
    if (item->type == BrowserItemType::AudioFile)
    {
        juce::File file(item->id);
        loadFile(file, autoPlay);
    }
}

void BrowserPreviewEngine::play()
{
    const juce::ScopedLock sl(lock_);
    
    if (readerSource_ != nullptr)
    {
        transportSource_.start();
        isPlaying_.store(true);
        DBG("BrowserPreview: Playing");
    }
}

void BrowserPreviewEngine::pause()
{
    const juce::ScopedLock sl(lock_);
    
    transportSource_.stop();
    isPlaying_.store(false);
    DBG("BrowserPreview: Paused");
}

void BrowserPreviewEngine::stop()
{
    const juce::ScopedLock sl(lock_);
    
    transportSource_.stop();
    transportSource_.setPosition(0.0);
    isPlaying_.store(false);
    DBG("BrowserPreview: Stopped");
}

void BrowserPreviewEngine::togglePlayback()
{
    if (isPlaying_.load())
        pause();
    else
        play();
}

//==============================================================================
// State
//==============================================================================

float BrowserPreviewEngine::getPlaybackPosition() const
{
    double duration = getDuration();
    if (duration <= 0.0)
        return 0.0f;
    
    return static_cast<float>(transportSource_.getCurrentPosition() / duration);
}

void BrowserPreviewEngine::setPlaybackPosition(float position)
{
    double duration = getDuration();
    if (duration > 0.0)
    {
        double newPos = juce::jlimit(0.0, duration, static_cast<double>(position) * duration);
        transportSource_.setPosition(newPos);
    }
}

double BrowserPreviewEngine::getDuration() const
{
    if (readerSource_ != nullptr)
    {
        return transportSource_.getLengthInSeconds();
    }
    return 0.0;
}

} // namespace zenith
