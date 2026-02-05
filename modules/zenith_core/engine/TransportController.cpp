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

    TransportController.cpp
    Created: 2025-12-09
    Author:  Zenith DAW

    Transport controller implementation.

  ==============================================================================

*/

#include "TransportController.h"
#include "TempoMap.h"

namespace zenith {

//==============================================================================
double TransportController::getPlayheadBeats() const {
    double sr = sampleRate_.load();
    if (sr <= 0.0) {
        return 0.0;
    }

    juce::int64 samples = playheadSamples_.load();

    // Use tempo map if available for accurate beat position
    if (tempoMap_ != nullptr) {
        return tempoMap_->samplesToBeats(samples, sr);
    }

    // Fallback: simple calculation using current tempo
    double seconds = static_cast<double>(samples) / sr;
    double bpm = tempo_.load();
    return seconds * bpm / 60.0;
}

} // namespace zenith
