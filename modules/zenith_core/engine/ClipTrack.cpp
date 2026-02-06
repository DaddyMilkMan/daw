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
    
    // Create new snapshot using the correct constructor
    std::vector<std::shared_ptr<TakeFolder>> takeFolders;
    takeFolders.reserve(takeFoldersOwned_.size());
    for (const auto &folder : takeFoldersOwned_) {
        takeFolders.push_back(folder);
    }
    
    auto newSnapshot = std::make_shared<ClipSnapshot>(clipsOwned_, takeFolders);
    
    // Swap atomically
    activeClipSnapshot_.store(newSnapshot.get(), std::memory_order_release);
    
    // Keep reference alive
    if (currentClipSnapshot_)
        RealTimeGarbageCollector::getInstance().deferDelete(currentClipSnapshot_);
        
    currentClipSnapshot_ = newSnapshot;
}

} // namespace zenith
