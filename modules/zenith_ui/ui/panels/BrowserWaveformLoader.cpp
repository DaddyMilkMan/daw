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

#include "BrowserWaveformLoader.h"
#include <algorithm>

namespace zenith {

BrowserWaveformLoader::BrowserWaveformLoader() : Thread("BrowserWaveformLoader") {
  formatManager_.registerBasicFormats();
  startThread();
}

BrowserWaveformLoader::~BrowserWaveformLoader() {
  stopThread(2000);
}

void BrowserWaveformLoader::request(const juce::String &path,
                                    std::function<void(const juce::String&, const std::vector<float>&)> callback) {
  {
    juce::ScopedLock sl(lock_);
    
    // Check cache first
    if (cache_.count(path)) {
      if (callback) {
        juce::MessageManager::callAsync([callback, path, peaks = cache_[path]]() {
          callback(path, peaks);
        });
      }
      return;
    }

    // Avoid duplicate requests
    for (const auto &req : queue_) {
      if (req.path == path) return;
    }

    queue_.push_back({path, std::move(callback)});
  }
  notify();
}

bool BrowserWaveformLoader::isCached(const juce::String &path) const {
  juce::ScopedLock sl(const_cast<juce::CriticalSection&>(lock_));
  return cache_.count(path) > 0;
}

std::vector<float> BrowserWaveformLoader::getCached(const juce::String &path) const {
  juce::ScopedLock sl(const_cast<juce::CriticalSection&>(lock_));
  auto it = cache_.find(path);
  if (it != cache_.end()) return it->second;
  return {};
}

void BrowserWaveformLoader::run() {
  // Pre-allocate buffer outside loop to prevent allocation churn
  juce::AudioBuffer<float> buffer(1, 2048);

  while (!threadShouldExit()) {
    Request req;
    {
      juce::ScopedLock sl(lock_);
      if (queue_.empty()) {
        wait(500);
        continue;
      }
      req = std::move(queue_.front());
      queue_.erase(queue_.begin());
    }

    if (threadShouldExit()) break;

    juce::File file(req.path);
    if (!file.existsAsFile()) continue;

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager_.createReaderFor(file));

    if (reader) {
      std::vector<float> peaks;
      int numPoints = 64; // Small cache for list items
      peaks.reserve(numPoints);

      juce::int64 length = reader->lengthInSamples;
      juce::int64 step = std::max(juce::int64(1), length / numPoints);

      for (int i = 0; i < numPoints; ++i) {
        if (threadShouldExit()) break;

        juce::int64 start = i * step;
        int numToRead = (int)std::min((juce::int64)buffer.getNumSamples(), length - start);

        if (numToRead <= 0) break;

        buffer.clear();
        reader->read(&buffer, 0, numToRead, start, true, false);

        float maxVal = 0.0f;
        if (auto *data = buffer.getReadPointer(0)) {
          for (int s = 0; s < numToRead; ++s) {
            float v = std::abs(data[s]);
            if (v > maxVal) maxVal = v;
          }
        }
        peaks.push_back(maxVal);
      }

      if (!threadShouldExit() && !peaks.empty()) {
        {
          juce::ScopedLock sl(lock_);
          cache_[req.path] = peaks;
          
          // Truncate cache if it gets too large
          if (cache_.size() > 1000) {
            cache_.erase(cache_.begin());
          }
        }

        if (req.callback) {
          juce::MessageManager::callAsync([cb = req.callback, path = req.path, peaks]() {
            cb(path, peaks);
          });
        }
      }
    }
  }
}

} // namespace zenith
