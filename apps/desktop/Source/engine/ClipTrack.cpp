#include "ClipTrack.h"

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

void ClipTrack::removeClip(Clip* clip) {
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    auto it = std::find_if(clipsOwned_.begin(), clipsOwned_.end(),
                           [clip](const std::unique_ptr<Clip>& c) { return c.get() == clip; });
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
    auto newSnapshot = std::make_shared<ClipSnapshot>(clipsOwned_);
    
    // Swap atomically
    activeClipSnapshot_.store(newSnapshot.get(), std::memory_order_release);
    
    // Keep reference alive
    if (currentClipSnapshot_)
        clipSnapshotTrash_.push_back(currentClipSnapshot_);
        
    currentClipSnapshot_ = newSnapshot;
    
    // Cleanup trash (limit size or clean based on some heuristic)
    if (clipSnapshotTrash_.size() > 5)
        clipSnapshotTrash_.erase(clipSnapshotTrash_.begin());
}

juce::ValueTree ClipTrack::getState() const {
    juce::ValueTree state = Track::getState();
    
    juce::ValueTree clipsTree("Clips");
    for (const auto& clip : clipsOwned_) {
        if (clip != nullptr) {
            clipsTree.appendChild(clip->getState(), nullptr);
        }
    }
    state.appendChild(clipsTree, nullptr);
    
    return state;
}

void ClipTrack::loadState(const juce::ValueTree& state) {
    Track::loadState(state);
    
    juce::ValueTree clipsTree = state.getChildWithName("Clips");
    clearClips();
    
    for (int i = 0; i < clipsTree.getNumChildren(); ++i) {
        auto clipState = clipsTree.getChild(i);
        auto clip = Clip::createFromState(clipState);
        if (clip != nullptr) {
            addClip(std::move(clip));
        }
    }
}

} // namespace zenith
