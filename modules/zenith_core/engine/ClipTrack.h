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

#pragma once

#include "Clip.h"
#include "TakeFolder.h"
#include "Track.h"
#include <algorithm>

namespace zenith {

/**

 * @brief Track that can contain clips (Audio, MIDI, or Instrument tracks)
 *
 * ## Ownership Model (to prevent shared_ptr cycles):
 *
 * **ClipTrack -> Clips:** Owned via std::unique_ptr in clipsOwned_ vector
 * **ClipSnapshot:** Uses shared_ptr for RCU pattern, but only for internal management
 * **Clips -> Track:** No back-reference stored in Clip (uses parameter passing)
 *
 * @note Clips should NEVER hold std::shared_ptr<ClipTrack> or std::shared_ptr<Track>
 */
class ClipTrack : public Track {
public:
  ClipTrack(const juce::String &name, Type type) : Track(name, type) {
    currentClipSnapshot_ = std::make_shared<ClipSnapshot>();
    activeClipSnapshot_.store(currentClipSnapshot_.get());
  }

  virtual ~ClipTrack() {
    clipsOwned_.clear();
    takeFoldersOwned_.clear();
    currentClipSnapshot_ = std::make_shared<ClipSnapshot>();
    activeClipSnapshot_.store(currentClipSnapshot_.get());
  }

  void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override {
    Track::prepareToPlay(samplesPerBlockExpected, sampleRate);
    clipBuffer_.setSize(2, samplesPerBlockExpected, false, true, true);

    for (auto &clip : clipsOwned_) {
      if (clip != nullptr) {
        clip->prepareToPlay(samplesPerBlockExpected, sampleRate);
      }
    }

    // Prepare take folders and their clips
    for (auto &folder : takeFoldersOwned_) {
      if (folder != nullptr) {
        for (int i = 0; i < folder->getNumTakes(); ++i) {
          if (auto *take = folder->getTake(i)) {
            take->prepareToPlay(samplesPerBlockExpected, sampleRate);
          }
        }
      }
    }
  }

  void releaseResources() override {
    Track::releaseResources();
    for (auto &clip : clipsOwned_) {
      if (clip != nullptr) {
        clip->releaseResources();
      }
    }
    for (auto &folder : takeFoldersOwned_) {
      if (folder != nullptr) {
        for (int i = 0; i < folder->getNumTakes(); ++i) {
          if (auto *take = folder->getTake(i)) {
            take->releaseResources();
          }
        }
      }
    }
  }

  // Clip management
  void addClip(std::unique_ptr<Clip> clip);
  void removeClip(int clipIndex);
  void removeClip(Clip *clip);
  void clearClips();
  int getNumClips() const { return static_cast<int>(clipsOwned_.size()); }
  Clip *getClip(int index) const {
    if (index >= 0 && index < static_cast<int>(clipsOwned_.size()))
      return clipsOwned_[index].get();
    return nullptr;
  }
  const std::vector<std::unique_ptr<Clip>> &getClips() const {
    return clipsOwned_;
  }

  //==========================================================================
  // Take Folder Management (override from Track)
  //==========================================================================

  int getNumTakeFolders() const {
    return static_cast<int>(takeFoldersOwned_.size());
  }

  TakeFolder *getTakeFolder(int index) const {
    if (index >= 0 && index < static_cast<int>(takeFoldersOwned_.size()))
      return takeFoldersOwned_[index].get();
    return nullptr;
  }

  TakeFolder *getTakeFolderAt(int64_t position) const {
    for (const auto &folder : takeFoldersOwned_) {
      if (folder && position >= folder->getStartPosition() &&
          position < folder->getEndPosition()) {
        return folder.get();
      }
    }
    return nullptr;
  }

  void addTakeFolder(std::shared_ptr<TakeFolder> folder) {
    if (folder) {
      takeFoldersOwned_.push_back(folder);
    }
  }

  void removeTakeFolder(TakeFolder *folder) {
    takeFoldersOwned_.erase(
        std::remove_if(takeFoldersOwned_.begin(), takeFoldersOwned_.end(),
                       [folder](const std::shared_ptr<TakeFolder> &f) {
                         return f.get() == folder;
                       }),
        takeFoldersOwned_.end());
  }

  /**
   * @brief Create a new take folder from an existing clip.
   * @param clip The clip to convert (will become first take).
   * @return The newly created take folder.
   */
  std::shared_ptr<TakeFolder> createTakeFolderFromClip(Clip *clip);

  /**
   * @brief Flatten a take folder to a single clip.
   * @param folder The folder to flatten.
   * @param sampleRate Sample rate for rendering.
   * @return The flattened clip (also added to track).
   */
  Clip *flattenTakeFolder(TakeFolder *folder, double sampleRate);

protected:
  struct ClipSnapshot {
    std::vector<Clip *> clips;
    std::vector<TakeFolder *> takeFolders;
    ClipSnapshot() = default;
    explicit ClipSnapshot(
        const std::vector<std::unique_ptr<Clip>> &ownedClips,
        const std::vector<std::shared_ptr<TakeFolder>> &ownedFolders) {
      clips.reserve(ownedClips.size());
      for (const auto &clip : ownedClips)
        clips.push_back(clip.get());
      
      // Sort clips by start position for optimized audio thread lookup
      std::sort(clips.begin(), clips.end(), [](Clip* a, Clip* b) {
          if (!a || !b) return a < b;
          return a->getStartPosition() < b->getStartPosition();
      });

      takeFolders.reserve(ownedFolders.size());
      for (const auto &folder : ownedFolders)
        takeFolders.push_back(folder.get());
    }
  };

  std::vector<std::unique_ptr<Clip>> clipsOwned_;
  std::vector<std::shared_ptr<TakeFolder>> takeFoldersOwned_;
  std::atomic<const ClipSnapshot *> activeClipSnapshot_{nullptr};
  std::shared_ptr<ClipSnapshot> currentClipSnapshot_;

  void updateClipSnapshot();
  juce::AudioBuffer<float> clipBuffer_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClipTrack)
};

} // namespace zenith
