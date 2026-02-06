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

namespace zenith {

/**
 * @brief Grid resolution options for snapping
 */
enum class GridResolution {
  Bar_1 = 0, ///< 4 beats (in 4/4)
  Beat_1,    ///< 1 beat (quarter note)
  Beat_1_2,  ///< 1/2 beat (eighth note)
  Beat_1_4,  ///< 1/4 beat (sixteenth note)
  Beat_1_8,  ///< 1/8 beat (thirty-second)
  Beat_1_3,  ///< 1/3 beat (triplet eighth)
  Beat_1_6,  ///< 1/6 beat (triplet sixteenth)
  Off        ///< No snap
};

/**
 * @brief Convert grid resolution to beat value
 * @param res Grid resolution enum value
 * @return Beat value (e.g., 4.0 for Bar_1, 1.0 for Beat_1)
 */
inline double gridResolutionToBeats(GridResolution res) {
  switch (res) {
  case GridResolution::Bar_1:
    return 4.0;
  case GridResolution::Beat_1:
    return 1.0;
  case GridResolution::Beat_1_2:
    return 0.5;
  case GridResolution::Beat_1_4:
    return 0.25;
  case GridResolution::Beat_1_8:
    return 0.125;
  case GridResolution::Beat_1_3:
    return 1.0 / 3.0;
  case GridResolution::Beat_1_6:
    return 1.0 / 6.0;
  case GridResolution::Off:
    return 0.0;
  default:
    return 1.0;
  }
}

/**
 * @brief Tools available in the Arranger
 */
enum class ArrangerTool {
  Select = 0, ///< Standard selection and move
  Split,      ///< Split clips (Razor)
  Eraser,     ///< Delete clips
  Pencil      ///< Draw clips/notes
};

} // namespace zenith
