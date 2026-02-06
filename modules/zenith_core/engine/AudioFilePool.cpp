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

#include "AudioFilePool.h"
#include "EngineConstants.h"
#include <filesystem>

namespace zenith {

using namespace zenith::constants;

namespace {
bool getCanonicalPath(const juce::File &file, juce::String &filePath,
                      juce::String &errorMessage) {
  std::error_code ec;
  auto canonicalPath = std::filesystem::weakly_canonical(
      file.getFullPathName().toStdString(), ec);
  if (ec) {
    errorMessage = "Invalid file path: cannot canonicalize path (" +
                   juce::String(ec.message()) + ")";
    return false;
  }

  filePath = juce::String(canonicalPath.string());
  return true;
}
} // namespace

//==============================================================================
AudioFilePool::AudioFilePool() {
  // Register basic audio formats (WAV, AIFF, OGG, FLAC, MP3)
  formatManager_.registerBasicFormats();
}

AudioFilePool::~AudioFilePool() { clear(); }

//==============================================================================
AudioFilePool::HandlePtr AudioFilePool::loadFile(const juce::File &file,
                                                 juce::String &errorMessage) {
  // ⚠️ MESSAGE THREAD ONLY - Does file I/O!

  // Security: Validate and sanitize file path using std::filesystem
  // This properly resolves symlinks and parent directory references
  juce::String filePath;
  if (!getCanonicalPath(file, filePath, errorMessage)) {
    return nullptr;
  }

  // Check path length (prevent excessive paths)
  if (filePath.length() > kMaxPathLength)
  {
    errorMessage = "File path too long (max " + juce::String(kMaxPathLength) + " characters)";
    return nullptr;
  }

  // Validate file exists and is actually a file (not directory)
  if (!file.existsAsFile()) {
    errorMessage = "File does not exist: " + file.getFullPathName();
    return nullptr;
  }

  // Additional security: Ensure file is readable
  if (!file.hasReadAccess()) {
    errorMessage = "File is not readable: " + file.getFullPathName();
    return nullptr;
  }

  // filePath already defined above at the security check

  // Check if already loaded
  {
    const juce::ScopedLock sl(cacheLock_);
    auto it = fileCache_.find(filePath);
    if (it != fileCache_.end()) {
      return it->second; // Return existing handle
    }
  }

  // Load the file
  std::unique_ptr<juce::AudioFormatReader> reader(
      formatManager_.createReaderFor(file));

  if (reader == nullptr) {
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

  if (!reader->read(&handle->buffer, 0,
                    static_cast<int>(reader->lengthInSamples), 0, true, true)) {
    errorMessage = "Failed to read audio data from: " + file.getFullPathName();
    return nullptr;
  }

  // Store in cache
  {
    const juce::ScopedLock sl(cacheLock_);
    fileCache_[filePath] = handle;
  }

  DBG("AudioFilePool: Loaded " + file.getFileName() + " (" +
      juce::String(handle->lengthInSamples) + " samples, " +
      juce::String(handle->numChannels) + " channels)");

  return handle;
}

AudioFilePool::HandlePtr AudioFilePool::loadFile(const juce::File &file) {
  juce::String errorMessage;
  return loadFile(file, errorMessage);
}

void AudioFilePool::loadFileAsync(const juce::File& file, std::function<void(HandlePtr loadedHandle, juce::String error)> callback) {
  // Capture basic info to avoid thread safety issues if possible
  // shared_ptr to this to ensure pool stays alive
  // Actually, pool is usually a singleton or long-lived in Zenith.
  
  juce::Thread::launch([this, file, callback]() {
    juce::String error;
    auto handle = loadFile(file, error);
    
    if (callback) {
      juce::MessageManager::callAsync([handle, error, callback]() {
        callback(handle, error);
      });
    }
  });
}

AudioFilePool::HandlePtr AudioFilePool::getFile(const juce::File &file) const {
  juce::String errorMessage;
  juce::String filePath;
  if (!getCanonicalPath(file, filePath, errorMessage)) {
    return nullptr;
  }
  const juce::ScopedLock sl(cacheLock_);
  auto it = fileCache_.find(filePath);
  return (it != fileCache_.end()) ? it->second : nullptr;
}

bool AudioFilePool::isLoaded(const juce::File &file) const {
  juce::String errorMessage;
  juce::String filePath;
  if (!getCanonicalPath(file, filePath, errorMessage)) {
    return false;
  }
  const juce::ScopedLock sl(cacheLock_);
  return fileCache_.find(filePath) != fileCache_.end();
}

void AudioFilePool::unloadFile(const juce::File &file) {
  juce::String errorMessage;
  juce::String filePath;
  if (!getCanonicalPath(file, filePath, errorMessage)) {
    return;
  }
  const juce::ScopedLock sl(cacheLock_);
  auto it = fileCache_.find(filePath);
  if (it != fileCache_.end()) {
    DBG("AudioFilePool: Unloading " + file.getFileName());
    fileCache_.erase(it);
    // Actual memory will be freed when last shared_ptr reference is released
  }
}

void AudioFilePool::clear() {
  const juce::ScopedLock sl(cacheLock_);
  DBG("AudioFilePool: Clearing all files (" + juce::String(fileCache_.size()) +
      " loaded)");
  fileCache_.clear();
}

int AudioFilePool::getNumLoadedFiles() const {
  const juce::ScopedLock sl(cacheLock_);
  return static_cast<int>(fileCache_.size());
}

juce::int64 AudioFilePool::getMemoryUsage() const {
  const juce::ScopedLock sl(cacheLock_);
  juce::int64 totalBytes = 0;

  for (const auto &pair : fileCache_) {
    const auto &handle = pair.second;
    if (handle) {
      // Size = numChannels * numSamples * sizeof(float)
      totalBytes += handle->buffer.getNumChannels() *
                    handle->buffer.getNumSamples() *
                    static_cast<juce::int64>(sizeof(float));
    }
  }

  return totalBytes;
}

} // namespace zenith
