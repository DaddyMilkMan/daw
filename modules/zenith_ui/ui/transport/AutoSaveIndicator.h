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

#include "zenith_core/engine/ProjectState.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../framework/SkiaComponent.h"
#include <JuceHeader.h>

namespace zenith {

class AutoSaveIndicator : public SkiaComponent {
public:
  AutoSaveIndicator(ProjectState &state) : projectState(state) {
    if (juce::MessageManager::getInstanceWithoutCreating() != nullptr) startTimer(500); // Check state every 500ms
    setWantsKeyboardFocus(false);
  }

  ~AutoSaveIndicator() override { stopTimer(); }

  void timerCallback() override {
    SkiaComponent::timerCallback(); // Call base implementation first

    bool dirty = projectState.hasUnsavedChanges();

    // DESIGN NOTE: We show dirty/clean indicated with pulse animation.
    // A real-time "Saving..." state would require ProjectFileIO to expose
    // an isSaving atomic flag. The current UX with pulse animation provides
    // sufficient feedback for the "unsaved changes" state without the
    // complexity of tracking save-in-progress across threads.

    if (dirty != wasDirty) {
      wasDirty = dirty;
      repaint();
    }

    // Pulse animation if dirty
    if (dirty) {
      pulsePhase += 0.1f;
      if (pulsePhase > juce::MathConstants<float>::twoPi) {
        pulsePhase -= juce::MathConstants<float>::twoPi;
      }
      repaint();
    }
  }

  void drawSkia(SkCanvas *canvas) override {
    // Dot indicator
    float radius = 4.0f;
    float cx = 10.0f;
    float cy = getHeight() / 2.0f;

    SkPaint paint;
    paint.setAntiAlias(true);

    if (wasDirty) {
      // Unsaved: Amber pulse
      float alpha = 0.6f + 0.4f * std::sin(pulsePhase);
      paint.setColor(
          SkColorSetA(SkColorSetRGB(255, 191, 0), (int)(alpha * 255)));
      canvas->drawCircle(cx, cy, radius, paint);

      // Text: "Unsaved"
      paint.setColor(design::colors::TEXT_SECONDARY);
      auto font = design::getSkFont(10.0f);
      canvas->drawString("Unsaved", cx + 10, cy + 3, font, paint);
    } else {
      // Saved: Green static
      paint.setColor(SkColorSetRGB(0, 255, 0));
      canvas->drawCircle(cx, cy, radius, paint);

      // Text: "Saved"
      paint.setColor(design::colors::TEXT_TERTIARY);
      auto font = design::getSkFont(10.0f);
      canvas->drawString("Saved", cx + 10, cy + 3, font, paint);
    }
  }

private:
  ProjectState &projectState;
  bool wasDirty = false;
  float pulsePhase = 0.0f;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutoSaveIndicator)
};

} // namespace zenith
