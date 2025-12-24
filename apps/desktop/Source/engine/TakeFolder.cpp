/*
  ==============================================================================

    TakeFolder.cpp
    Created: 2025-12-24
    Author:  Zenith DAW

    Implementation of TakeFolder for multi-take recording and comping.

  ==============================================================================
*/

#include "TakeFolder.h"
#include <algorithm>

namespace zenith {

//==============================================================================
TakeFolder::TakeFolder() { id_ = juce::Uuid().toString(); }

TakeFolder::~TakeFolder() = default;

//==============================================================================
// Take Management
//==============================================================================

int TakeFolder::addTake(std::shared_ptr<Clip> clip) {
  if (!clip)
    return -1;

  takes_.push_back(clip);
  int index = static_cast<int>(takes_.size()) - 1;

  // If this is the first take, set position and length from it
  if (index == 0) {
    startPosition_.store(clip->getStartPosition());
    length_.store(clip->getLength());

    // Initialize with single comp region covering entire range
    clearCompRegions();
  } else {
    // Extend folder length if new take is longer
    recalculateLength();
  }

  return index;
}

void TakeFolder::removeTake(int index) {
  if (index < 0 || index >= static_cast<int>(takes_.size()))
    return;

  takes_.erase(takes_.begin() + index);

  // Adjust comp regions
  juce::SpinLock::ScopedLockType lock(compRegionLock_);
  for (auto &region : compRegions_) {
    if (region.takeIndex == index) {
      // Point to take 0 if the referenced take was removed
      region.takeIndex = 0;
    } else if (region.takeIndex > index) {
      // Shift down indices above the removed one
      region.takeIndex--;
    }
  }

  // If active take was removed, switch to comp mode
  int active = activeTakeIndex_.load();
  if (active == index) {
    activeTakeIndex_.store(-1);
  } else if (active > index) {
    activeTakeIndex_.store(active - 1);
  }

  recalculateLength();
}

Clip *TakeFolder::getTake(int index) const {
  if (index < 0 || index >= static_cast<int>(takes_.size()))
    return nullptr;
  return takes_[index].get();
}

std::shared_ptr<Clip> TakeFolder::getTakeShared(int index) const {
  if (index < 0 || index >= static_cast<int>(takes_.size()))
    return nullptr;
  return takes_[index];
}

//==============================================================================
// Comp Region Management
//==============================================================================

void TakeFolder::setCompRegion(int64_t startSamples, int64_t lengthSamples,
                               int takeIndex) {
  if (takeIndex < 0 || takeIndex >= static_cast<int>(takes_.size()))
    return;
  if (lengthSamples <= 0)
    return;

  juce::SpinLock::ScopedLockType lock(compRegionLock_);

  // Find and remove/split any overlapping regions
  std::vector<CompRegion> newRegions;
  int64_t newEnd = startSamples + lengthSamples;

  for (const auto &existing : compRegions_) {
    int64_t existEnd = existing.getEndSamples();

    // No overlap - keep as is
    if (existEnd <= startSamples || existing.startSamples >= newEnd) {
      newRegions.push_back(existing);
      continue;
    }

    // Partial overlap - split/trim
    // Left portion (before new region)
    if (existing.startSamples < startSamples) {
      CompRegion left;
      left.id = juce::Uuid().toString();
      left.startSamples = existing.startSamples;
      left.lengthSamples = startSamples - existing.startSamples;
      left.takeIndex = existing.takeIndex;
      newRegions.push_back(left);
    }

    // Right portion (after new region)
    if (existEnd > newEnd) {
      CompRegion right;
      right.id = juce::Uuid().toString();
      right.startSamples = newEnd;
      right.lengthSamples = existEnd - newEnd;
      right.takeIndex = existing.takeIndex;
      newRegions.push_back(right);
    }
  }

  // Add the new region
  CompRegion newRegion(startSamples, lengthSamples, takeIndex);
  newRegions.push_back(newRegion);

  // Sort by start position
  std::sort(newRegions.begin(), newRegions.end(),
            [](const CompRegion &a, const CompRegion &b) {
              return a.startSamples < b.startSamples;
            });

  compRegions_ = std::move(newRegions);
  normalizeCompRegions();
}

int TakeFolder::getTakeIndexAt(int64_t positionInFolder) const {
  juce::SpinLock::ScopedLockType lock(compRegionLock_);

  for (const auto &region : compRegions_) {
    if (positionInFolder >= region.startSamples &&
        positionInFolder < region.getEndSamples()) {
      return region.takeIndex;
    }
  }

  // Default to first take if no region covers this position
  return 0;
}

void TakeFolder::clearCompRegions() {
  juce::SpinLock::ScopedLockType lock(compRegionLock_);
  compRegions_.clear();

  // Create single region covering entire folder with take 0
  if (length_.load() > 0) {
    compRegions_.emplace_back(0, length_.load(), 0);
  }
}

const std::vector<CompRegion> &TakeFolder::getCompRegions() const {
  // Caller should hold lock or be on message thread
  return compRegions_;
}

//==============================================================================
// Audio Rendering
//==============================================================================

void TakeFolder::getNextAudioBlock(juce::AudioBuffer<float> &buffer,
                                   int64_t startSample, int numSamples,
                                   double sampleRate) {
  juce::ignoreUnused(sampleRate);

  if (takes_.empty()) {
    buffer.clear();
    return;
  }

  int64_t folderStart = startPosition_.load();
  int64_t folderEnd = folderStart + length_.load();

  // Check if we're outside the folder range
  if (startSample >= folderEnd || startSample + numSamples <= folderStart) {
    buffer.clear();
    return;
  }

  // Are we auditioning a single take?
  int activeIdx = activeTakeIndex_.load();
  if (activeIdx >= 0 && activeIdx < static_cast<int>(takes_.size())) {
    // Audition mode: play single take
    Clip *take = takes_[activeIdx].get();
    if (take) {
      take->getAudioSamples(buffer, startSample, numSamples);
    }
    return;
  }

  // Comp mode: render from different takes based on regions
  buffer.clear();

  // Process each region that overlaps with the requested range
  juce::SpinLock::ScopedLockType lock(compRegionLock_);

  for (const auto &region : compRegions_) {
    int64_t regionStart = folderStart + region.startSamples;
    int64_t regionEnd = regionStart + region.lengthSamples;

    // Check overlap with requested range
    int64_t overlapStart = juce::jmax(startSample, regionStart);
    int64_t overlapEnd = juce::jmin(startSample + numSamples, regionEnd);

    if (overlapStart >= overlapEnd)
      continue; // No overlap

    int bufferOffset = static_cast<int>(overlapStart - startSample);
    int samplesToRender = static_cast<int>(overlapEnd - overlapStart);

    // Get audio from the appropriate take
    if (region.takeIndex >= 0 &&
        region.takeIndex < static_cast<int>(takes_.size())) {
      Clip *take = takes_[region.takeIndex].get();
      if (take) {
        // Create sub-buffer for this region
        juce::AudioBuffer<float> regionBuffer(buffer.getNumChannels(),
                                              samplesToRender);
        take->getAudioSamples(regionBuffer, overlapStart, samplesToRender);

        // Copy to output buffer at correct offset
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
          buffer.copyFrom(ch, bufferOffset, regionBuffer, ch, 0,
                          samplesToRender);
        }
      }
    }
  }
}

//==============================================================================
// Flatten
//==============================================================================

std::unique_ptr<Clip> TakeFolder::flatten(double sampleRate) {

  int64_t totalLength = length_.load();
  if (totalLength <= 0 || takes_.empty())
    return nullptr;

  // Create buffer for entire folder length
  juce::AudioBuffer<float> flattenedBuffer(2, static_cast<int>(totalLength));
  flattenedBuffer.clear();

  // Render the entire comp
  int64_t folderStart = startPosition_.load();

  // Process in chunks to avoid huge single allocations
  constexpr int chunkSize = 65536;
  int64_t position = folderStart;
  int remaining = static_cast<int>(totalLength);

  while (remaining > 0) {
    int samplesToProcess = juce::jmin(remaining, chunkSize);
    int bufferOffset = static_cast<int>(position - folderStart);

    juce::AudioBuffer<float> chunkBuffer(2, samplesToProcess);
    getNextAudioBlock(chunkBuffer, position, samplesToProcess, sampleRate);

    // Copy chunk to flattened buffer
    for (int ch = 0; ch < 2; ++ch) {
      flattenedBuffer.copyFrom(ch, bufferOffset, chunkBuffer, ch, 0,
                               samplesToProcess);
    }

    position += samplesToProcess;
    remaining -= samplesToProcess;
  }

  // Create new clip with flattened audio
  auto flatClip = std::make_unique<Clip>();
  flatClip->setType(Clip::Type::Audio);
  flatClip->setName(name_ + " (Flattened)");
  flatClip->setStartPosition(folderStart);
  flatClip->setLength(totalLength);

  // Set the audio buffer
  flatClip->setAudioBuffer(flattenedBuffer);

  return flatClip;
}

//==============================================================================
// Helpers
//==============================================================================

void TakeFolder::recalculateLength() {
  int64_t maxLength = 0;
  for (const auto &take : takes_) {
    if (take) {
      maxLength = juce::jmax(maxLength, take->getLength());
    }
  }
  length_.store(maxLength);
}

void TakeFolder::normalizeCompRegions() {
  // This is called with lock held
  if (compRegions_.empty())
    return;

  // Merge adjacent regions with same take index
  std::vector<CompRegion> merged;
  merged.reserve(compRegions_.size());

  for (const auto &region : compRegions_) {
    if (merged.empty()) {
      merged.push_back(region);
      continue;
    }

    CompRegion &last = merged.back();
    // Can we merge?
    if (last.getEndSamples() == region.startSamples &&
        last.takeIndex == region.takeIndex) {
      last.lengthSamples += region.lengthSamples;
    } else {
      merged.push_back(region);
    }
  }

  compRegions_ = std::move(merged);
}

} // namespace zenith
