/*
  ==============================================================================

    BrowserWaveformLoader.h
    Created: 2025-12-26
    Author:  Zenith DAW

    Standalone utility for background audio waveform loading and caching.

  ==============================================================================
*/

#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <map>
#include <vector>

namespace zenith {

/**
 * Handles background extraction of waveform peaks for various browser views.
 */
class BrowserWaveformLoader : public juce::Thread {
public:
  BrowserWaveformLoader();
  ~BrowserWaveformLoader() override;

  /**
   * Request a waveform for a file.
   * @param path Full path to the audio file.
   * @param callback Callback triggered on message thread when peaks are ready.
   */
  void request(const juce::String &path, 
               std::function<void(const juce::String&, const std::vector<float>&)> callback);

  /**
   * Check if we already have peaks for this path.
   */
  bool isCached(const juce::String &path) const;
  
  /**
   * Get cached peaks if available.
   */
  std::vector<float> getCached(const juce::String &path) const;

protected:
  void run() override;

private:
  struct Request {
    juce::String path;
    std::function<void(const juce::String&, const std::vector<float>&)> callback;
  };

  void processNextRequest();

  std::vector<Request> queue_;
  std::map<juce::String, std::vector<float>> cache_;
  juce::CriticalSection lock_;
  juce::AudioFormatManager formatManager_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BrowserWaveformLoader)
};

} // namespace zenith
