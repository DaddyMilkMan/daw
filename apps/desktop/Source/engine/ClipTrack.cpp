#include "ClipTrack.h"
#include "RealTimeGarbageCollector.h"

namespace zenith {

void ClipTrack::addClip(std::shared_ptr<Clip> clip) {
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    if (clip != nullptr) {
        if (currentSampleRate > 0)
            clip->prepareToPlay(currentBlockSize, currentSampleRate);
            
        clipsOwned_.push_back(clip);
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

void ClipTrack::removeClip(Clip* clip) {
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    auto it = std::find_if(clipsOwned_.begin(), clipsOwned_.end(),
                           [clip](const std::shared_ptr<Clip>& c) { return c.get() == clip; });
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
    auto newSnapshot = std::make_shared<ClipSnapshot>(clipsOwned_, takeFoldersOwned_);

    
    // Swap atomically
    activeClipSnapshot_.store(newSnapshot.get(), std::memory_order_release);
    
    // Keep reference alive
    if (currentClipSnapshot_)
        RealTimeGarbageCollector::getInstance().deferDelete(currentClipSnapshot_);
        
    currentClipSnapshot_ = newSnapshot;
}

} // namespace zenith
