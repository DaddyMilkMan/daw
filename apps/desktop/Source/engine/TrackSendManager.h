/*
  ==============================================================================

    TrackSendManager.h
    Created: 2026-01-31
    Author:  Zenith DAW

    Send routing management for Track.

  ==============================================================================
*/

#pragma once

#include "MixerChannel.h"
#include <atomic>
#include <juce_core/juce_core.h>

namespace zenith {

/**
 * @brief Owns send destinations and levels for a Track.
 */
class TrackSendManager {
public:
  explicit TrackSendManager(MixerChannel &mixerChannel);

  void setSendDestination(int sendIndex, int auxBusIndex);
  int getSendDestination(int sendIndex) const;
  void setSendLevel(int sendIndex, float level);
  float getSendLevel(int sendIndex) const;
  void setSendPreFader(int sendIndex, bool preFader);
  bool isSendPreFader(int sendIndex) const;

private:
  MixerChannel &mixerChannel_;
  static constexpr int numSends = ::zenith::constants::kNumSends;
  std::atomic<int> sendDestinations_[numSends];

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackSendManager)
};

} // namespace zenith
