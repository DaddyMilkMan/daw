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
