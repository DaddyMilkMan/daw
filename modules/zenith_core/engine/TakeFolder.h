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

#pragma once

#include "Clip.h"
#include <atomic>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <memory>
#include <vector>

namespace zenith {

//==============================================================================
/**
 * @brief A region within a TakeFolder that specifies which take to use.
 *
 * Comp regions define sections of the timeline where a specific take
 * should be heard. They are non-overlapping and cover the TakeFolder range.
 */
struct CompRegion {
  juce::String id;       ///< Unique identifier
  int64_t startSamples;  ///< Start position relative to TakeFolder start
  int64_t lengthSamples; ///< Length of this region
  int takeIndex;         ///< Which take (0-based) to use for this region

  CompRegion() : startSamples(0), lengthSamples(0), takeIndex(0) {
    id = juce::Uuid().toString();
  }

  CompRegion(int64_t start, int64_t length, int take)
      : startSamples(start), lengthSamples(length), takeIndex(take) {
    id = juce::Uuid().toString();
  }

  int64_t getEndSamples() const { return startSamples + lengthSamples; }
};

//==============================================================================
/**
 * @brief Container for multiple takes and their comp regions.
 *
 * A TakeFolder represents a "stack" of clips recorded at the same position.
 * It manages:
 * - Multiple takes (clips)
 * - Comp regions (which take to play where)
 * - Active take for audition
 * - Flatten operation (bake comp to single clip)
 */
class TakeFolder {
public:
  //==========================================================================
  // Construction
  //==========================================================================

  TakeFolder();
  ~TakeFolder();

  //==========================================================================
  // Identification
  //==========================================================================

  const juce::String &getId() const { return id_; }
  void setId(const juce::String &id) { id_ = id; }

  const juce::String &getName() const { return name_; }
  void setName(const juce::String &name) { name_ = name; }

  //==========================================================================
  // Position (relative to track timeline)
  //==========================================================================

  int64_t getStartPosition() const { return startPosition_.load(); }
  void setStartPosition(int64_t pos) { startPosition_.store(pos); }

  int64_t getLength() const { return length_.load(); }
  void setLength(int64_t len) { length_.store(len); }

  int64_t getEndPosition() const { return getStartPosition() + getLength(); }

  //==========================================================================
  // Take Management
  //==========================================================================

  /**
   * @brief Add a new take (clip) to the folder.
   * @param clip The clip to add as a take.
   * @return Index of the new take.
   */
  int addTake(std::shared_ptr<Clip> clip);

  /**
   * @brief Remove a take by index.
   * @note Adjusts comp regions that reference removed or higher-indexed takes.
   */
  void removeTake(int index);

  /**
   * @brief Get a take by index.
   */
  Clip *getTake(int index) const;

  /**
   * @brief Get shared pointer to take.
   */
  std::shared_ptr<Clip> getTakeShared(int index) const;

  /**
   * @brief Get number of takes.
   */
  int getNumTakes() const { return static_cast<int>(takes_.size()); }

  /**
   * @brief Get all takes (for UI enumeration).
   */
  const std::vector<std::shared_ptr<Clip>> &getTakes() const { return takes_; }

  //==========================================================================
  // Active Take (for audition, RT-safe)
  //==========================================================================

  /**
   * @brief Set which take to audition (plays entire take, ignoring comp).
   * @param index -1 to use comp regions, 0+ to audition specific take.
   */
  void setActiveTakeIndex(int index) { activeTakeIndex_.store(index); }

  /**
   * @brief Get current audition take index.
   * @return -1 if using comp regions, otherwise the take index.
   */
  int getActiveTakeIndex() const { return activeTakeIndex_.load(); }

  /**
   * @brief Check if using comp mode (not auditioning single take).
   */
  bool isCompMode() const { return activeTakeIndex_.load() < 0; }

  //==========================================================================
  // Comp Region Management
  //==========================================================================

  /**
   * @brief Add or update a comp region.
   * @note Regions are automatically merged/split to avoid overlaps.
   */
  void setCompRegion(int64_t startSamples, int64_t lengthSamples,
                     int takeIndex);

  /**
   * @brief Get all comp regions (for UI display).
   */
  const std::vector<CompRegion>& getCompRegions() const {
    if (currentCompSnapshot_) return currentCompSnapshot_->regions;
    static const std::vector<CompRegion> empty;
    return empty;
  }

  /**
   * @brief Find which take index should play at a given position.
   * @param positionInFolder Position relative to TakeFolder start.
   * @return Take index to use, or 0 if no regions cover this position.
   */
  int getTakeIndexAt(int64_t positionInFolder) const;

  /**
   * @brief Clear all comp regions (reset to take 0 for entire range).
   */
  void clearCompRegions();

  //==========================================================================
  // Audio Rendering (RT-safe)
  //==========================================================================

  /**
   * @brief Render audio from the appropriate take(s) based on comp regions.
   * @param buffer Output buffer to fill.
   * @param startSample Start position in transport samples.
   * @param numSamples Number of samples to render.
   * @param sampleRate Current sample rate.
   * @note This method is RT-safe and lock-free.
   */
  void getNextAudioBlock(juce::AudioBuffer<float> &buffer, int64_t startSample,
                         int numSamples, double sampleRate);

  //==========================================================================
  // Flatten (Consolidate comp to single clip)
  //==========================================================================

  /**
   * @brief Create a new clip that renders the entire comp as audio.
   * @param sampleRate Sample rate for the rendered clip.
   * @return New clip containing the flattened audio.
   */
  std::unique_ptr<Clip> flatten(double sampleRate, const juce::File& outputDirectory);

  //==========================================================================
  // Expansion State (for UI)
  //==========================================================================

  bool isExpanded() const { return expanded_.load(); }
  void setExpanded(bool expanded) { expanded_.store(expanded); }

  //==========================================================================
  // Color
  //==========================================================================

  juce::Colour getColor() const { return color_; }
  void setColor(juce::Colour color) { color_ = color; }

private:
  juce::String id_;
  juce::String name_{"Take Folder"};

  std::atomic<int64_t> startPosition_{0};
  std::atomic<int64_t> length_{0};

  std::vector<std::shared_ptr<Clip>> takes_;
  std::atomic<int> activeTakeIndex_{-1}; ///< -1 = comp mode

  std::atomic<bool> expanded_{true};
  juce::Colour color_{juce::Colours::orange};

  // RCU Snapshot for comp regions
  struct CompSnapshot {
    std::vector<CompRegion> regions;
  };

  std::atomic<const CompSnapshot*> activeCompSnapshot_{nullptr};
  std::shared_ptr<CompSnapshot> currentCompSnapshot_;

  // RCU Snapshot for takes (RT-safe access to clips)
  struct TakesSnapshot {
    std::vector<Clip*> takes;  // Raw pointers for RT access
  };
  std::atomic<const TakesSnapshot*> activeTakesSnapshot_{nullptr};
  std::shared_ptr<TakesSnapshot> currentTakesSnapshot_;
  std::atomic<uint64_t> takesSnapshotEpoch_{0};  // Grace period epoch

  // Pre-allocated buffer for RT-safe rendering (sized for max expected block)
  juce::AudioBuffer<float> regionBuffer_;

  // Helper: Recalculate length from takes
  void recalculateLength();

  // Helper: Normalize comp regions (merge overlaps, fill gaps)
  void normalizeCompRegions();

  // Helper: Update RCU snapshots
  void updateCompSnapshot();
  void updateTakesSnapshot();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TakeFolder)
};

} // namespace zenith
