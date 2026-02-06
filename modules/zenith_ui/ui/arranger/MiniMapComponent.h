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

#include "SkiaComponent.h"
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>


// Forward declarations for Skia types
class SkCanvas;

class SkImage;

namespace zenith {

/**
 * @class MiniMapComponent
 * @brief A high-performance mini-map for navigating the arrangement view.
 *
 * Renders a simplified LOD (Level of Detail) representation of the entire
 * arrangement as colored rectangles. Provides click-jump navigation and
 * displays a viewport overlay showing the current visible area.
 *
 * Performance Strategy:
 * - Clips are cached to an off-screen SkImage when arrangement changes
 * - Only the viewport overlay is drawn live each frame
 * - Uses fast rect drawing without anti-aliasing for 60fps performance
 */
class MiniMapComponent : public SkiaComponent {
public:
  MiniMapComponent();
  ~MiniMapComponent() override;

  /**
   * @struct MiniMapClip
   * @brief LOD representation of a clip for mini-map rendering.
   */
  struct MiniMapClip {
    double startBeats = 0.0;
    double lengthBeats = 1.0;
    int trackIndex = 0;
    bool isMidi = false;
    bool isSelected = false;
  };

  /**
   * @brief Update the full arrangement structure.
   *
   * This triggers a cache rebuild. Should be called when clips are
   * added, removed, or modified.
   *
   * @param totalDurationBeats Total duration of the arrangement in beats
   * @param totalTracks Total number of tracks
   * @param clips Vector of clip data for rendering
   */
  void setArrangementData(double totalDurationBeats, int totalTracks,
                          const std::vector<MiniMapClip> &clips);

  /**
   * @brief Update the viewport visualization.
   *
   * This is a fast operation that only updates the viewport overlay
   * without rebuilding the clip cache.
   *
   * @param startBeat First visible beat position
   * @param durationBeats Number of beats visible
   * @param firstTrackIndex Index of first visible track
   * @param visibleTracks Number of visible tracks
   */
  void setVisibleRange(double startBeat, double durationBeats,
                       int firstTrackIndex, int visibleTracks);

  /**
   * @brief Callback fired when user clicks to navigate.
   *
   * The parameter is the beat position that was clicked.
   */
  std::function<void(double beat)> onNavigate;

  void resized() override;
  void mouseUp(const juce::MouseEvent &e) override;
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseDrag(const juce::MouseEvent &e) override;

  void drawSkia(SkCanvas *canvas) override;

private:
  double totalDurationBeats_ = 100.0;
  int totalTracks_ = 8;

  // Viewport state
  double visibleStart_ = 0.0;
  double visibleDuration_ = 16.0;
  int firstTrackIndex_ = 0;
  int visibleTrackCount_ = 8;

  // Clip data for rendering
  std::vector<MiniMapClip> clips_;

  // Cached rendering
  sk_sp<SkImage> cachedImage_;
  bool needsCacheUpdate_ = true;

  void updateCachedImage();

  // Coordinate conversion
  float beatsToX(double beats) const;
  double xToBeats(float x) const;
  float trackToY(int trackIndex) const;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MiniMapComponent)
};

} // namespace zenith
