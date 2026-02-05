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

    ProjectRecoveryModal.h
    Created: 2025-12-29
    Author:  Zenith DAW Team

    Premium Skia-based Project Recovery Modal with glassmorphism effects.
    Replaces ugly native JUCE dialog with a modern, glowy UI.


  ==============================================================================
*/

#pragma once

#include "../framework/SkiaComponent.h"
#include "../framework/GlassmorphicPanel.h"
#include "../utils/PhysicsSpring.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../../engine/ProjectFileIO.h"
#include <functional>
#include <memory>
#include <vector>

namespace zenith {

/**
 * @brief Premium glassmorphic modal for project recovery
 *
 * Features:
 * - Floating glassmorphic panel with backdrop blur
 * - Animated gradient border (cyan ↔ magenta)
 * - Recovery file list with selection
 * - Glowing action buttons
 * - Fade in/out animations
 */
class ProjectRecoveryModal : public SkiaComponent {
public:
  using RecoverCallback = std::function<void(const RecoveryInfo &selected)>;
  using DismissCallback = std::function<void()>;

  /**
   * @brief Construct the recovery modal
   * @param recoveries List of available recovery files
   * @param onRecover Callback when user selects to recover
   * @param onDiscard Callback when user discards all recoveries
   */
  ProjectRecoveryModal(const std::vector<RecoveryInfo> &recoveries,
                       RecoverCallback onRecover, DismissCallback onDiscard);

  ~ProjectRecoveryModal() override;

  // SkiaComponent interface
  void drawSkia(SkCanvas *canvas) override;
  void resized() override;

  // Mouse interaction
  void mouseMove(const juce::MouseEvent &e) override;
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseUp(const juce::MouseEvent &e) override;
  void mouseExit(const juce::MouseEvent &e) override;

  // Keyboard
  bool keyPressed(const juce::KeyPress &key) override;

  // Animation timer
  void timerCallback() override;

  // Show/hide with animation
  void show();
  void dismiss();

  // Hit test - only the card area intercepts clicks
  bool hitTest(int x, int y) override;

private:
  // Recovery item for the list
  struct RecoveryItem {
    RecoveryInfo info;
    juce::String displayName;    // Formatted project name
    juce::String timeAgo;        // "2 hours ago" etc
    juce::String fileSize;       // "1.2 MB"
    SkRect bounds;
    bool isHovered = false;
    bool isSelected = false;
    PhysicsSpring scaleSpring{1.0f};
  };

  // Callbacks
  RecoverCallback onRecover_;
  DismissCallback onDiscard_;

  // State
  std::vector<RecoveryItem> items_;
  int selectedIndex_ = -1;
  bool isVisible_ = false;

  // Animation
  AnimatedValue alpha_;
  float animationTime_ = 0.0f;
  float gradientAngle_ = 0.0f;     // Border gradient rotation
  float iconPulse_ = 0.0f;         // Icon glow pulse

  // Layout rects
  SkRect cardBounds_;
  SkRect titleBounds_;
  SkRect subtitleBounds_;
  SkRect listBounds_;
  SkRect recoverButtonBounds_;
  SkRect discardButtonBounds_;
  SkRect closeButtonBounds_;

  // Button states
  bool isRecoverHovered_ = false;
  bool isDiscardHovered_ = false;
  bool isCloseHovered_ = false;

  // Cached fonts
  SkFont titleFont_;
  SkFont subtitleFont_;
  SkFont itemNameFont_;
  SkFont itemDetailFont_;
  SkFont buttonFont_;

  // Drawing helpers
  void drawBackground(SkCanvas *canvas);
  void drawCard(SkCanvas *canvas);
  void drawAnimatedBorder(SkCanvas *canvas);
  void drawRecoveryIcon(SkCanvas *canvas);
  void drawTitle(SkCanvas *canvas);
  void drawRecoveryList(SkCanvas *canvas);
  void drawRecoveryItem(SkCanvas *canvas, RecoveryItem &item, int index);
  void drawButtons(SkCanvas *canvas);
  void drawCloseButton(SkCanvas *canvas);

  // Helpers
  void updateLayout();
  juce::String formatTimeAgo(juce::int64 timestamp) const;
  juce::String formatFileSize(juce::int64 bytes) const;
  void selectItem(int index);

  // Constants
  static constexpr float CARD_WIDTH = 520.0f;
  static constexpr float CARD_HEIGHT = 420.0f;
  static constexpr float ITEM_HEIGHT = 64.0f;
  static constexpr float BUTTON_HEIGHT = 40.0f;
  static constexpr float PADDING = 24.0f;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProjectRecoveryModal)
};

} // namespace zenith
