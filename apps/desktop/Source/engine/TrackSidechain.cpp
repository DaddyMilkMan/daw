/*
  ==============================================================================

    TrackSidechain.cpp
    Created: 2026-01-31
    Author:  Zenith DAW

  ==============================================================================
*/

#include "TrackSidechain.h"
#include "TrackProcessor.h"

namespace zenith {

TrackSidechain::TrackSidechain(TrackProcessor &processor)
    : processor_(processor) {}

void TrackSidechain::setPluginSidechainSource(
    int pluginIndex, std::shared_ptr<Track> sourceTrack) {
  sidechainSource_.store(std::move(sourceTrack), std::memory_order_release);
  processor_.setSidechainSource(pluginIndex, sidechainSource_.load(std::memory_order_acquire));
}

std::shared_ptr<Track> TrackSidechain::getSidechainSource() const {
  return sidechainSource_.load(std::memory_order_acquire);
}

} // namespace zenith
