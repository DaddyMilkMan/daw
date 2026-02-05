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

/*
    ==============================================================================
    Original file header:
*/

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
