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

/*
    ==============================================================================
    Original file header:
*/

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
