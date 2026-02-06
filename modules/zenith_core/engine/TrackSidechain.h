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
