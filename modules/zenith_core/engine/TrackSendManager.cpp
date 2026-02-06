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
