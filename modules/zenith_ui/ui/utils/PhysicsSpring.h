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

#include <algorithm>
#include <cmath>

namespace zenith {

class PhysicsSpring {
public:
  PhysicsSpring(float initialValue = 0.0f)
      : current(initialValue), target(initialValue), velocity(0.0f) {}

  void setTarget(float newTarget) { target = newTarget; }

  void setCurrent(float value) {
    current = value;
    velocity = 0.0f; // Reset velocity on hard set
  }

  void snapToTarget() {
    current = target;
    velocity = 0.0f;
  }

  float getCurrent() const { return current; }
  float getTarget() const { return target; }
  float getVelocity() const { return velocity; }

  bool isResting(float threshold = 0.001f) const {
    return std::abs(target - current) < threshold &&
           std::abs(velocity) < threshold;
  }

  /**
   * @brief Update the spring physics
   * @param stiffness Controls how fast it moves to target (0.01 - 1.0)
   * @param damping Controls how much it oscillates (0.0 - 1.0, 1.0 = no
   * oscillation)
   * @return true if still animating, false if resting
   */
  bool update(float stiffness = 0.15f, float damping = 0.85f) {
    if (isResting()) {
      current = target;
      velocity = 0.0f;
      return false;
    }

    float force = (target - current) * stiffness;
    velocity += force;
    velocity *= damping;
    current += velocity;

    return true;
  }

private:
  float current;
  float target;
  float velocity;
};

} // namespace zenith
