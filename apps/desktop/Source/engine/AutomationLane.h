/*
  ==============================================================================
    AutomationLane.h
    Author: Zenith DAW
<<<<<<< HEAD
    
=======

>>>>>>> origin/master
    A lock-free compatible container for automation data.
    Designed to be swapped atomically (RCU style).
  ==============================================================================
*/

#pragma once
<<<<<<< HEAD
#include <vector>
#include <algorithm>
#include <cmath>
#include <juce_core/juce_core.h>
=======
#include <algorithm>
#include <cmath>
#include <juce_core/juce_core.h>
#include <vector>
>>>>>>> origin/master

namespace zenith {

struct AutomationPoint {
<<<<<<< HEAD
    double timeBeats;
    float value;
    float curve; // 0.5 = linear, >0.5 log, <0.5 exp
=======
  double timeBeats;
  float value;
  float curve; // 0.5 = linear, >0.5 log, <0.5 exp
>>>>>>> origin/master
};

class AutomationLane {
public:
<<<<<<< HEAD
    AutomationLane() = default;
    
    // Build from a vector of points (must be sorted)
    explicit AutomationLane(const std::vector<AutomationPoint>& sortedPoints) 
        : points(sortedPoints) {
        jassert(std::is_sorted(points.begin(), points.end(), 
                               [](const AutomationPoint& a, const AutomationPoint& b) {
                                   return a.timeBeats < b.timeBeats;
                               }));
    }

    float getValueAt(double timeBeats) const {
        if (points.empty()) return 0.0f; // Default value

        // Boundary checks
        if (timeBeats <= points.front().timeBeats) return points.front().value;
        if (timeBeats >= points.back().timeBeats) return points.back().value;

        // Binary search for the point BEFORE timeBeats
        // std::lower_bound returns first element >= val
        auto it = std::lower_bound(points.begin(), points.end(), timeBeats, 
            [](const AutomationPoint& p, double t) { return p.timeBeats < t; });

        // "it" is the point AFTER or AT timeBeats
        // If it == begin, we are before or at the first point (handled by boundary check above)
        // So we can safely decrement.
        if (it == points.begin()) return points.front().value;

        const auto& p2 = *it;
        const auto& p1 = *(it - 1);

        // Interpolate
        double range = p2.timeBeats - p1.timeBeats;
        if (range <= 0.000001) return p2.value;

        double t = (timeBeats - p1.timeBeats) / range;
        
        // Apply curve (Basic implementation: Linear for now to ensure stability)
        // TODO: Implement bezier/curve math based on p1.curve
        // Simple curve implementation:
        // float curve = p1.curve;
        // if (std::abs(curve - 0.5f) > 0.001f) {
        //    // Warp t
        // }
        
        return p1.value + (p2.value - p1.value) * static_cast<float>(t);
    }
    
    bool isEmpty() const { return points.empty(); }
    const std::vector<AutomationPoint>& getPoints() const { return points; }

private:
    std::vector<AutomationPoint> points;
};

}
=======
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
>>>>>>> origin/master
