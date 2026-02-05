/*
  ==============================================================================

    TrackSendManager.cpp
    Created: 2026-01-31
    Author:  Zenith DAW

  ==============================================================================
*/

#include "TrackSendManager.h"

namespace zenith {

TrackSendManager::TrackSendManager(MixerChannel &mixerChannel)
    : mixerChannel_(mixerChannel) {
  for (int i = 0; i < numSends; ++i) {
    sendDestinations_[i].store(-1, std::memory_order_relaxed);
  }
}

void TrackSendManager::setSendDestination(int sendIndex, int auxBusIndex) {
  if (juce::isPositiveAndBelow(sendIndex, numSends)) {
    sendDestinations_[sendIndex].store(auxBusIndex, std::memory_order_relaxed);
  }
}

int TrackSendManager::getSendDestination(int sendIndex) const {
  if (juce::isPositiveAndBelow(sendIndex, numSends)) {
    return sendDestinations_[sendIndex].load(std::memory_order_relaxed);
  }
  return -1;
}

void TrackSendManager::setSendLevel(int sendIndex, float level) {
  mixerChannel_.setSendLevel(sendIndex, level);
}

float TrackSendManager::getSendLevel(int sendIndex) const {
  return mixerChannel_.getSendLevel(sendIndex);
}

void TrackSendManager::setSendPreFader(int sendIndex, bool preFader) {
  mixerChannel_.setSendPreFader(sendIndex, preFader);
}

bool TrackSendManager::isSendPreFader(int sendIndex) const {
  return mixerChannel_.isSendPreFader(sendIndex);
}

} // namespace zenith
