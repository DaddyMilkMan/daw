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

    DebugConsoleComponent.h
    Created: 2025-12-07
    Author:  Zenith DAW AI Team

    Debug Console UI Component

    A sleek, minimal debug console that displays session health information

    in the bottom bar. Shows a summary of issues fixed rather than verbose logs.

  ==============================================================================
*/

#pragma once

#include "../../ai/SessionDebuggerAgent.h"
#include "SkiaComponent.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_core/juce_core.h>
#include "ZenithSkia.h"

namespace zenith {

#ifdef ZENITH_USE_SKIA

/**
    Debug Console Component - Displays session health in the bottom bar

    Shows:
    - Session health score (color-coded)
    - Number of active issues
    - Summary of fixes applied
    - Quick status indicators for CPU, Clipping, Latency
*/
class DebugConsoleComponent : public SkiaComponent,
                              public ai::SessionDebuggerAgent::Listener {
public:
  //==========================================================================
  explicit DebugConsoleComponent(ai::SessionDebuggerAgent &debugger);
  ~DebugConsoleComponent() override;

  //==========================================================================
  // SkiaComponent
  void drawSkia(SkCanvas *canvas) override;
  void resized() override;

  //==========================================================================
  // Mouse interaction
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseEnter(const juce::MouseEvent &e) override;
  void mouseExit(const juce::MouseEvent &e) override;

  //==========================================================================
  // SessionDebuggerAgent::Listener
  void issueDetected(const ai::SessionIssue &issue) override;
  void issueResolved(const ai::SessionIssue &issue,
                     const ai::FixAction &fix) override;
  void sessionHealthChanged(float newHealthScore) override;

  //==========================================================================
  // Timer callback for animation
  void timerCallback() override;

  //==========================================================================
  // Configuration
  void setExpanded(bool expanded);
  bool isExpanded() const { return isExpanded_; }

private:
  //==========================================================================
  ai::SessionDebuggerAgent &debugger_;

  // State
  bool isExpanded_ = false;
  bool isHovered_ = false;
  float healthScore_ = 100.0f;
  float displayedHealthScore_ = 100.0f; // Animated value

  // Animation
  float animationProgress_ = 0.0f;
  bool showNewFixNotification_ = false;
  juce::String latestFixMessage_;
  juce::Time lastFixTime_;

  // Cached paints
  ::SkPaint bgPaint_;
  ::SkPaint borderPaint_;
  ::SkPaint healthGoodPaint_;
  ::SkPaint healthWarningPaint_;
  ::SkPaint healthCriticalPaint_;
  ::SkPaint textPaint_;
  ::SkPaint iconPaint_;
  ::SkPaint notificationPaint_;
  ::SkFont font_;
  ::SkFont boldFont_;
  ::SkFont smallFont_;

  // Layout constants
  static constexpr float kCollapsedHeight = 32.0f;
  static constexpr float kExpandedHeight = 120.0f;
  static constexpr float kCornerRadius = 8.0f;
  static constexpr float kPadding = 12.0f;

  //==========================================================================
  void updateCachedPaints();
  void drawCollapsedView(SkCanvas *canvas, const SkRect &bounds);
  void drawExpandedView(SkCanvas *canvas, const SkRect &bounds);
  void drawHealthIndicator(SkCanvas *canvas, float x, float y, float size);
  void drawStatusIcons(SkCanvas *canvas, float x, float y);
  void drawNotificationBadge(SkCanvas *canvas, float x, float y);

  SkColor getHealthColor(float score) const;
  juce::String getHealthStatusText(float score) const;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DebugConsoleComponent)
};

#endif // ZENITH_USE_SKIA

} // namespace zenith
