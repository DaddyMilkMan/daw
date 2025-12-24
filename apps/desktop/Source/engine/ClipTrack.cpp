#include "ClipTrack.h"
#include "RealTimeGarbageCollector.h"

namespace zenith {

void ClipTrack::addClip(std::unique_ptr<Clip> clip) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
  if (clip != nullptr) {
    if (currentSampleRate > 0)
      clip->prepareToPlay(currentBlockSize, currentSampleRate);

    clipsOwned_.push_back(std::move(clip));
    updateClipSnapshot();
  }
}

void ClipTrack::removeClip(int clipIndex) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
  if (clipIndex >= 0 && clipIndex < static_cast<int>(clipsOwned_.size())) {
    clipsOwned_.erase(clipsOwned_.begin() + clipIndex);
    updateClipSnapshot();
  }
}

void ClipTrack::removeClip(Clip *clip) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
  auto it = std::find_if(
      clipsOwned_.begin(), clipsOwned_.end(),
      [clip](const std::unique_ptr<Clip> &c) { return c.get() == clip; });
  if (it != clipsOwned_.end()) {
    clipsOwned_.erase(it);
    updateClipSnapshot();
  }
}

void ClipTrack::clearClips() {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
  clipsOwned_.clear();
  updateClipSnapshot();
}

void ClipTrack::updateClipSnapshot() {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  // Create new snapshot
  auto newSnapshot =
      std::make_shared<ClipSnapshot>(clipsOwned_, takeFoldersOwned_);

  // Swap atomically
  activeClipSnapshot_.store(newSnapshot.get(), std::memory_order_release);

  // Defer deletion of the old snapshot to be safe for audio thread
  if (currentClipSnapshot_) {
    RealTimeGarbageCollector::getInstance().push(currentClipSnapshot_);
  }

  currentClipSnapshot_ = newSnapshot;
}

std::shared_ptr<TakeFolder> ClipTrack::createTakeFolderFromClip(Clip *clip) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  if (clip == nullptr)
    return nullptr;

  // Find and remove the clip from our owned list
  std::unique_ptr<Clip> ownedClip;
  auto it = std::find_if(
      clipsOwned_.begin(), clipsOwned_.end(),
      [clip](const std::unique_ptr<Clip> &c) { return c.get() == clip; });

  if (it == clipsOwned_.end())
    return nullptr; // Clip not owned by this track

  // Take ownership
  ownedClip = std::move(*it);
  clipsOwned_.erase(it);

  // Create the TakeFolder
  auto folder = std::make_shared<TakeFolder>();
  folder->setName(ownedClip->getName() + " Takes");
  folder->setStartPosition(ownedClip->getStartPosition());
  folder->setColor(ownedClip->getColor());

  // Add the clip as first take (transfer ownership)
  std::shared_ptr<Clip> sharedClip = std::move(ownedClip);
  folder->addTake(sharedClip);

  // Add folder to track
  takeFoldersOwned_.push_back(folder);
  updateClipSnapshot();

  return folder;
}

Clip *ClipTrack::flattenTakeFolder(TakeFolder *folder, double sampleRate) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  if (folder == nullptr)
    return nullptr;

  // Find the folder in our list
  auto it = std::find_if(takeFoldersOwned_.begin(), takeFoldersOwned_.end(),
                         [folder](const std::shared_ptr<TakeFolder> &f) {
                           return f.get() == folder;
                         });

  if (it == takeFoldersOwned_.end())
    return nullptr; // Folder not owned by this track

  // Flatten to a new clip
  auto flattenedClip = folder->flatten(sampleRate);
  if (!flattenedClip)
    return nullptr;

  // Add the flattened clip
  auto clipPtr = flattenedClip.get();

  // Transfer ownership from unique_ptr returned by flatten
  clipsOwned_.push_back(std::move(flattenedClip));

  // Remove the take folder
  takeFoldersOwned_.erase(it);

  updateClipSnapshot();

  return clipPtr;
}

} // namespace zenith
