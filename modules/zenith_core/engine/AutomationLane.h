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
    AutomationLane.h
    Author: Zenith DAW

    A lock-free compatible container for automation data.
    Designed to be swapped atomically (RCU style).
  ==============================================================================
*/


#pragma once
#include <algorithm>
#include <cmath>
#include <juce_core/juce_core.h>
#include <vector>

namespace zenith {

struct AutomationPoint {
  double timeBeats;
  float value;
  float curve; // 0.5 = linear, >0.5 log, <0.5 exp
};

class AutomationLane {
public:
  AutomationLane() = default;

  // Build from a vector of points (must be sorted)
  explicit AutomationLane(const std::vector<AutomationPoint> &sortedPoints)
      : points(sortedPoints) {}

  float getValueAt(double timeBeats) const {
    if (points.empty())
      return 0.0f; // Default value

    // Boundary checks
    if (timeBeats <= points.front().timeBeats)
      return points.front().value;
    if (timeBeats >= points.back().timeBeats)
      return points.back().value;

    // Binary search for the point BEFORE timeBeats
    // std::lower_bound returns first element >= val
    auto it = std::lower_bound(
        points.begin(), points.end(), timeBeats,
        [](const AutomationPoint &p, double t) { return p.timeBeats < t; });

    // "it" is the point AFTER or AT timeBeats
    // If it == begin, we are before or at the first point (handled by boundary
    // check above) So we can safely decrement.
    if (it == points.begin())
      return points.front().value;

    const auto &p2 = *it;
    const auto &p1 = *(it - 1);

    // Interpolate
    double range = p2.timeBeats - p1.timeBeats;
    if (range <= 0.000001)
      return p2.value;

    // Apply curve warping
    // curve: 0.5 = linear, <0.5 = ease-in (exponential start), >0.5 = ease-out
    // (log start)
    double t = (timeBeats - p1.timeBeats) / range;
    float curve = p1.curve;
    float curvedT = static_cast<float>(t);

    if (std::abs(curve - 0.5f) > 0.001f) {
      // Map curve 0-1 to exponential power 0.1-10
      // curve=0 -> power=4 (strong ease-in)
      // curve=0.5 -> power=1 (linear)
      // curve=1 -> power=0.25 (strong ease-out)
      float power = std::pow(2.0f, (0.5f - curve) * 4.0f);
      curvedT = std::pow(curvedT, power);
    }

    return p1.value + (p2.value - p1.value) * curvedT;
  }

  bool isEmpty() const { return points.empty(); }
  const std::vector<AutomationPoint> &getPoints() const { return points; }

private:
  std::vector<AutomationPoint> points;
};

} // namespace zenith
