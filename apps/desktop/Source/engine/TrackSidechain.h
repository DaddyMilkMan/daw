/*
  ==============================================================================

    TrackSidechain.h
    Created: 2026-01-31
    Author:  Zenith DAW

    Sidechain routing management for Track.

  ==============================================================================
*/

#pragma once

#include <atomic>
#include <memory>
#include <juce_core/juce_core.h>

namespace zenith {

class Track;
class TrackProcessor;

/**
 * @brief Owns sidechain routing for a Track (RT-safe, lock-free).
 */
class TrackSidechain {
public:
  explicit TrackSidechain(TrackProcessor &processor);

  void setPluginSidechainSource(int pluginIndex,
                                std::shared_ptr<Track> sourceTrack);
  std::shared_ptr<Track> getSidechainSource() const;

private:
  TrackProcessor &processor_;
  std::atomic<std::shared_ptr<Track>> sidechainSource_{nullptr};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackSidechain)
};

} // namespace zenith
