#pragma once

#include "Track.h"
#include "Clip.h"

namespace zenith {

class ClipTrack : public Track {
public:
    ClipTrack(const juce::String& name, Type type) : Track(name, type) {
        currentClipSnapshot_ = std::make_shared<ClipSnapshot>();
        activeClipSnapshot_.store(currentClipSnapshot_.get());
    }
    
    virtual ~ClipTrack() {
        clipsOwned_.clear();
        currentClipSnapshot_ = std::make_shared<ClipSnapshot>();
        activeClipSnapshot_.store(currentClipSnapshot_.get());
    }

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override {
        Track::prepareToPlay(samplesPerBlockExpected, sampleRate);
        clipBuffer_.setSize(2, samplesPerBlockExpected, false, true, true);
        
        for (auto& clip : clipsOwned_) {
            if (clip != nullptr) {
                clip->prepareToPlay(samplesPerBlockExpected, sampleRate);
            }
        }
    }

    void releaseResources() override {
        Track::releaseResources();
        for (auto& clip : clipsOwned_) {
            if (clip != nullptr) {
                clip->releaseResources();
            }
        }
    }

    // Clip management
    void addClip(std::unique_ptr<Clip> clip);
    void removeClip(int clipIndex);
    void removeClip(Clip* clip);
    void clearClips();
    int getNumClips() const { return static_cast<int>(clipsOwned_.size()); }
    Clip* getClip(int index) const {
        if (index >= 0 && index < static_cast<int>(clipsOwned_.size()))
            return clipsOwned_[index].get();
        return nullptr;
    }
    const std::vector<std::unique_ptr<Clip>>& getClips() const { return clipsOwned_; }
    
    // State management
    juce::ValueTree getState() const override;
    void loadState(const juce::ValueTree& state) override;

protected:
    struct ClipSnapshot {
        std::vector<Clip*> clips;
        ClipSnapshot() = default;
        explicit ClipSnapshot(const std::vector<std::unique_ptr<Clip>>& ownedClips) {
            clips.reserve(ownedClips.size());
            for (const auto& clip : ownedClips)
                clips.push_back(clip.get());
        }
    };

    std::vector<std::unique_ptr<Clip>> clipsOwned_;
    std::atomic<const ClipSnapshot*> activeClipSnapshot_{nullptr};
    std::shared_ptr<ClipSnapshot> currentClipSnapshot_;
    std::vector<std::shared_ptr<ClipSnapshot>> clipSnapshotTrash_;

    void updateClipSnapshot();
    juce::AudioBuffer<float> clipBuffer_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClipTrack)
};

} // namespace zenith
