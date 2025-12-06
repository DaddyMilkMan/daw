/*
  ==============================================================================

    BrowserPreviewEngine.h
    Created: 2025-12-05
    Author:  Zenith DAW

    Audio preview engine for the browser.
    Allows instant playback of audio samples when selected.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include "BrowserData.h"
#include <atomic>
#include <memory>

namespace zenith {

/**
 * @brief Audio preview engine for instant sample playback
 * 
 * Features:
 * - Auto-play on selection (toggleable)
 * - Volume control
 * - Stop on deselection
 * - Looping toggle
 */
class BrowserPreviewEngine : public juce::AudioSource
{
public:
    BrowserPreviewEngine();
    ~BrowserPreviewEngine() override;

    //==============================================================================
    // AudioSource Implementation
    //==============================================================================
    
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;

    //==============================================================================
    // Preview Control
    //==============================================================================
    
    /**
     * @brief Load and optionally play an audio file
     * @param file Audio file to preview
     * @param autoPlay If true, start playing immediately
     */
    void loadFile(const juce::File& file, bool autoPlay = true);
    
    /**
     * @brief Load from a BrowserItem
     */
    void loadItem(std::shared_ptr<BrowserItem> item, bool autoPlay = true);
    
    /**
     * @brief Start/resume playback
     */
    void play();
    
    /**
     * @brief Pause playback
     */
    void pause();
    
    /**
     * @brief Stop and reset position
     */
    void stop();
    
    /**
     * @brief Toggle play/pause
     */
    void togglePlayback();

    //==============================================================================
    // State
    //==============================================================================
    
    bool isPlaying() const { return isPlaying_.load(); }
    bool isLoaded() const { return currentFile_.existsAsFile(); }
    
    juce::File getCurrentFile() const { return currentFile_; }
    
    /**
     * @brief Get playback position (0.0 - 1.0)
     */
    float getPlaybackPosition() const;
    
    /**
     * @brief Set playback position (0.0 - 1.0)
     */
    void setPlaybackPosition(float position);
    
    /**
     * @brief Get total duration in seconds
     */
    double getDuration() const;

    //==============================================================================
    // Settings
    //==============================================================================
    
    void setVolume(float volume) { volume_.store(juce::jlimit(0.0f, 1.0f, volume)); }
    float getVolume() const { return volume_.load(); }
    
    void setLooping(bool shouldLoop) { looping_.store(shouldLoop); }
    bool isLooping() const { return looping_.load(); }
    
    void setAutoPlayEnabled(bool enabled) { autoPlayEnabled_.store(enabled); }
    bool isAutoPlayEnabled() const { return autoPlayEnabled_.load(); }

private:
    juce::AudioFormatManager formatManager_;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource_;
    juce::AudioTransportSource transportSource_;
    
    juce::File currentFile_;
    
    std::atomic<bool> isPlaying_{false};
    std::atomic<float> volume_{0.7f};
    std::atomic<bool> looping_{false};
    std::atomic<bool> autoPlayEnabled_{true};
    
    double sampleRate_ = 44100.0;
    int blockSize_ = 512;
    
    juce::CriticalSection lock_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BrowserPreviewEngine)
};

} // namespace zenith
