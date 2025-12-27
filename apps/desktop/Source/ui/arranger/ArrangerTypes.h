/*
  ==============================================================================

    ArrangerTypes.h
    Created: 2025-12-26
    Author:  Zenith DAW

    Common types and enums for the Arranger module.

  ==============================================================================
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
