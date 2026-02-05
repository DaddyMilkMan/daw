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

    TakeFolder.cpp
    Created: 2025-12-24
    Author:  Zenith DAW

    Implementation of TakeFolder for multi-take recording and comping.

  ==============================================================================

*/

#include "TakeFolder.h"
#include <algorithm>
#include "RealTimeGarbageCollector.h"

namespace zenith {

//==============================================================================
TakeFolder::TakeFolder() {
  id_ = juce::Uuid().toString();
  // Pre-allocate buffer to max expected block size (8192 samples stereo)
  // This prevents RT-unsafe allocations in getNextAudioBlock
  regionBuffer_.setSize(2, 8192);
  updateCompSnapshot();
  updateTakesSnapshot();
}

TakeFolder::~TakeFolder() {
  activeCompSnapshot_.store(nullptr);
  activeTakesSnapshot_.store(nullptr);
}

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

  // Update takes snapshot for RT-safe access
  updateTakesSnapshot();

  return index;
}

void TakeFolder::removeTake(int index) {
  if (index < 0 || index >= static_cast<int>(takes_.size()))
    return;

  takes_.erase(takes_.begin() + index);

  // Adjust comp regions
  if (currentCompSnapshot_) {
    auto newRegions = currentCompSnapshot_->regions;
    for (auto &region : newRegions) {
      if (region.takeIndex == index) {
        // Point to take 0 if the referenced take was removed
        region.takeIndex = 0;
      } else if (region.takeIndex > index) {
        // Shift down indices above the removed one
        region.takeIndex--;
      }
    }
    currentCompSnapshot_->regions = std::move(newRegions);
  }

  // If active take was removed, switch to comp mode
  int active = activeTakeIndex_.load();
  if (active == index) {
    activeTakeIndex_.store(-1);
  } else if (active > index) {
    activeTakeIndex_.store(active - 1);
  }

  recalculateLength();
  
  // Update takes snapshot for RT-safe access
  updateTakesSnapshot();
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

  // Find and remove/split any overlapping regions
  std::vector<CompRegion> newRegions;
  int64_t newEnd = startSamples + lengthSamples;

  for (const auto &existing : currentCompSnapshot_->regions) {
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

  currentCompSnapshot_->regions = std::move(newRegions);
  normalizeCompRegions();
}

int TakeFolder::getTakeIndexAt(int64_t positionInFolder) const {
  auto* snapshot = activeCompSnapshot_.load(std::memory_order_acquire);
  if (!snapshot) return 0;

  for (const auto &region : snapshot->regions) {
    if (positionInFolder >= region.startSamples &&
        positionInFolder < region.getEndSamples()) {
      return region.takeIndex;
    }
  }

  // Default to first take if no region covers this position
  return 0;
}

void TakeFolder::clearCompRegions() {
  if (!currentCompSnapshot_) {
    currentCompSnapshot_ = std::make_shared<CompSnapshot>();
  }
  currentCompSnapshot_->regions.clear();

  // Create single region covering entire folder with take 0
  if (length_.load() > 0) {
    currentCompSnapshot_->regions.emplace_back(0, length_.load(), 0);
  }
  updateCompSnapshot();
}


//==============================================================================
// Audio Rendering (RT-SAFE)
//==============================================================================

void TakeFolder::getNextAudioBlock(juce::AudioBuffer<float> &buffer,
                                   int64_t startSample, int numSamples,
                                   double sampleRate) {
  // Note: sampleRate parameter reserved for future time-stretch support.
  // Currently clips are assumed to match project sample rate.
  juce::ignoreUnused(sampleRate);

  // Load RT-safe takes snapshot (acquired via RCU)
  auto* takesSnap = activeTakesSnapshot_.load(std::memory_order_acquire);
  if (!takesSnap || takesSnap->takes.empty()) {
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
  if (activeIdx >= 0 && activeIdx < static_cast<int>(takesSnap->takes.size())) {
    // Audition mode: play single take
    Clip *take = takesSnap->takes[activeIdx];
    if (take) {
      take->getAudioSamples(buffer, startSample, numSamples);
    }
    return;
  }

  // Comp mode: render from different takes based on regions
  buffer.clear();

  // Process each region that overlaps with the requested range
  auto* compSnap = activeCompSnapshot_.load(std::memory_order_acquire);
  if (!compSnap) return;

  const int numTakes = static_cast<int>(takesSnap->takes.size());

  for (const auto &region : compSnap->regions) {
    int64_t regionStart = folderStart + region.startSamples;
    int64_t regionEnd = regionStart + region.lengthSamples;

    // Check overlap with requested range
    int64_t overlapStart = juce::jmax(startSample, regionStart);
    int64_t overlapEnd = juce::jmin(startSample + numSamples, regionEnd);

    if (overlapStart >= overlapEnd)
      continue; // No overlap

    int bufferOffset = static_cast<int>(overlapStart - startSample);
    int samplesToRender = static_cast<int>(overlapEnd - overlapStart);

    // Get audio from the appropriate take (using snapshot)
    if (region.takeIndex >= 0 && region.takeIndex < numTakes) {
      Clip *take = takesSnap->takes[region.takeIndex];
      if (take) {
        // regionBuffer_ is pre-allocated to 8192 samples.
        // Only copy what we need - no reallocation needed for typical block sizes.
        jassert(samplesToRender <= regionBuffer_.getNumSamples());
        
        take->getAudioSamples(regionBuffer_, overlapStart, samplesToRender);

        // Copy to output buffer at correct offset
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
          buffer.copyFrom(ch, bufferOffset, regionBuffer_, ch, 0,
                          samplesToRender);
        }
      }
    }
  }
}

//==============================================================================
// Flatten
//==============================================================================

std::unique_ptr<Clip> TakeFolder::flatten(double sampleRate, const juce::File& outputDirectory) {
  int64_t totalLength = length_.load();
  if (totalLength <= 0 || takes_.empty())
    return nullptr;

  // 1. Prepare output file
  outputDirectory.createDirectory();
  juce::File outputFile = outputDirectory.getChildFile(name_ + "_flattened.wav")
                                         .getNonexistentSibling();
  
  // 2. Setup format writer
  juce::WavAudioFormat wavFormat;
  // JUCE 8 API: createWriterFor takes unique_ptr<OutputStream>& by lvalue ref 
  // It will steal ownership internally, leaving our variable null on success
  std::unique_ptr<juce::OutputStream> fileStream = std::make_unique<juce::FileOutputStream>(outputFile);
  
  // Check if file opened successfully (cast needed to access failedToOpen)
  if (auto* fos = dynamic_cast<juce::FileOutputStream*>(fileStream.get())) {
    if (fos->failedToOpen()) {
      DBG("TakeFolder: Failed to open output file for flattening: " + outputFile.getFullPathName());
      return nullptr;
    }
  }

  // Writer takes ownership of stream via lvalue ref (JUCE 8 API)
  juce::AudioFormatWriterOptions options;
  options = options.withSampleRate(sampleRate)
                   .withNumChannels(2)
                   .withBitsPerSample(24);
  
  std::unique_ptr<juce::AudioFormatWriter> writer(wavFormat.createWriterFor(fileStream, options));

  if (!writer) {
     DBG("TakeFolder: Failed to create WAV writer");
     return nullptr;
  }

  // 3. Render and write in chunks
  int64_t folderStart = startPosition_.load();
  constexpr int chunkSize = 8192; // Manageable chunk size
  int64_t position = 0; // Relative to folder start
  int64_t remaining = totalLength;

  juce::AudioBuffer<float> chunkBuffer(2, chunkSize);

  while (remaining > 0) {
    int samplesToProcess = static_cast<int>(juce::jmin((int64_t)chunkSize, remaining));
    
    // getNextAudioBlock expects absolute project time
    getNextAudioBlock(chunkBuffer, folderStart + position, samplesToProcess, sampleRate);

    if (writer->writeFromAudioSampleBuffer(chunkBuffer, 0, samplesToProcess)) {
      position += samplesToProcess;
      remaining -= samplesToProcess;
    } else {
      DBG("TakeFolder: Write failed");
      return nullptr;
    }
  }

  // Writer destructor finalizes file
  writer.reset();

  // 4. Create Clip referencing the new file
  auto flatClip = std::make_unique<Clip>();
  flatClip->setType(Clip::Type::Audio);
  flatClip->setName(name_ + " (Flattened)");
  flatClip->setStartPosition(folderStart);
  flatClip->setLength(totalLength);
  
  // Important: set the audio file so calls to prepareToPlay load it safely from pool
  flatClip->setAudioFile(outputFile);

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
  if (!currentCompSnapshot_ || currentCompSnapshot_->regions.empty())
    return;

  // Merge adjacent regions with same take index
  std::vector<CompRegion> merged;
  merged.reserve(currentCompSnapshot_->regions.size());

  for (const auto &region : currentCompSnapshot_->regions) {
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

  currentCompSnapshot_->regions = std::move(merged);
  updateCompSnapshot();
}

void TakeFolder::updateCompSnapshot() {
  auto newSnapshot = std::make_shared<CompSnapshot>();
  if (currentCompSnapshot_) {
    newSnapshot->regions = currentCompSnapshot_->regions;
  }
  
  activeCompSnapshot_.store(newSnapshot.get(), std::memory_order_release);
  
  // Move old snapshot to trash with current epoch
  if (currentCompSnapshot_) {
    RealTimeGarbageCollector::getInstance().deferDelete(currentCompSnapshot_);
  }
  currentCompSnapshot_ = newSnapshot;
}

void TakeFolder::updateTakesSnapshot() {
  auto newSnapshot = std::make_shared<TakesSnapshot>();
  
  // Copy raw pointers from shared_ptr vector
  newSnapshot->takes.reserve(takes_.size());
  for (const auto& take : takes_) {
    newSnapshot->takes.push_back(take.get());
  }
  
  // Store atomically for RT access
  activeTakesSnapshot_.store(newSnapshot.get(), std::memory_order_release);
  
  // Bump epoch to signal new generation
  takesSnapshotEpoch_.fetch_add(1, std::memory_order_release);
  
  // Move old snapshot to trash
  if (currentTakesSnapshot_) {
    RealTimeGarbageCollector::getInstance().deferDelete(currentTakesSnapshot_);
  }
  currentTakesSnapshot_ = newSnapshot;
}

} // namespace zenith
