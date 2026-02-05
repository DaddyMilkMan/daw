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

 * @file TrackGroupHeader.h
 * @brief Track group header component with VU meter for mixer view
 *
 * Features:
 * - Collapsible track group with expand/collapse
 * - Integrated VU meter with non-blocking setLevel() API
 * - Mute/Solo controls for entire group
 * - Glassmorphic styling with Neon Noir design system
 * - AnimationCoordinator integration for smooth 60Hz updates

 */

#pragma once

#include <atomic>
#include <functional>
#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "../controls/SkiaButton.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../framework/AnimationCoordinator.h"
#include "../framework/SkiaComponent.h"

namespace zenith {

// Forward declarations
class Track;
class ProjectState;

/**
 * @struct TrackGroupHeaderProps
 * @brief Configuration properties for TrackGroupHeader
 *
 * All fields have sensible defaults that won't conflict with existing models.
 */
struct TrackGroupHeaderProps {
  juce::String groupName{"Group"};
  juce::String groupId;
  juce::Colour color{static_cast<uint32_t>(design::colors::ACCENT_PRIMARY)};
  bool isExpanded{true};
  bool isMuted{false};
  bool isSolo{false};
  int trackCount{0};

  // Callbacks (all optional)
  std::function<void(bool)> onExpandToggle;
  std::function<void()> onMuteClick;
  std::function<void()> onSoloClick;
  std::function<void()> onClick;
};

/**
 * @class TrackGroupHeader
 * @brief A collapsible header for track groups in the mixer
 *
 * Displays group name, track count, mute/solo controls, and an integrated
 * VU meter showing the group's summed output level. Uses Skia for GPU-accelerated
 * rendering with the Neon Noir glassmorphism design system.
 *
 * Performance characteristics:
 * - VU meter uses atomic operations for thread-safe level updates
 * - Registers with AnimationCoordinator for centralized 60Hz updates
 * - Dirty-rect optimization for minimal repaint area
 */
class TrackGroupHeader : public SkiaComponent {
public:
  //==========================================================================
  // Construction
  //==========================================================================

  TrackGroupHeader();
  ~TrackGroupHeader() override;

  //==========================================================================
  // Props/State Management
  //==========================================================================

  /**
   * @brief Set all properties at once
   * @param props The new property values
   */
  void setProps(const TrackGroupHeaderProps &props);

  /**
   * @brief Get current properties
   */
  const TrackGroupHeaderProps &getProps() const { return props_; }

  /**
   * @brief Update individual properties
   */
  void setGroupName(const juce::String &name);
  void setColor(const juce::Colour &color);
  void setExpanded(bool expanded);
  void setMuted(bool muted);
  void setSolo(bool solo);
  void setTrackCount(int count);

  //==========================================================================
  // VU Meter Subcomponent (Nested Class)
  //==========================================================================

  /**
   * @class VUMeter
   * @brief Non-blocking VU meter with thread-safe level updates
   *
   * The setLevel() API is designed to be called from any thread (including
   * the audio thread) without blocking. Updates are rendered at 60Hz via
   * the AnimationCoordinator.
   *
   * Non-blocking mechanism:
   * - Audio thread calls setLevel() which writes to std::atomic<float>
   * - No mutex, no blocking, no memory allocation
   * - UI thread reads atomic value during AnimationCoordinator tick
   * - Gravity-based ballistics provide smooth visual decay
   *
   * Frame drop protection:
   * - Threshold-based repaint: skip if change < 0.001
   * - Dirty rect optimization: only repaint changed region
   * - Visibility culling via AnimationCoordinator
   */
  class VUMeter : public SkiaComponent {
  public:
    VUMeter();
    ~VUMeter() override;

    //========================================================================
    // Thread-Safe Level Updates (Non-Blocking)
    //========================================================================

    /**
     * @brief Set the meter level (mono)
     * @param level Normalized level 0.0-1.0
     * @note Thread-safe, lock-free. Safe to call from audio thread.
     */
    void setLevel(float level);

    /**
     * @brief Set stereo meter levels
     * @param left Left channel level 0.0-1.0
     * @param right Right channel level 0.0-1.0
     * @note Thread-safe, lock-free. Safe to call from audio thread.
     */
    void setLevels(float left, float right);

    /**
     * @brief Enable/disable stereo mode
     */
    void setStereo(bool stereo) { stereo_ = stereo; }
    bool isStereo() const { return stereo_; }

    //========================================================================
    // SkiaComponent Interface
    //========================================================================

    void drawSkia(SkCanvas *canvas) override;

    //========================================================================
    // AnimationListener Interface
    //========================================================================

    void onAnimationTick(float deltaMs) override;
    bool isAnimating() const override;
    bool requiresVisibility() const override { return true; }

  private:
    void drawMeterBar(SkCanvas *canvas, const SkRect &bounds, float level,
                      float peak);

    // Thread-safe target levels (written by audio thread)
    std::atomic<float> targetLevel_{0.0f};
    std::atomic<float> targetLevelL_{0.0f};
    std::atomic<float> targetLevelR_{0.0f};

    // Smoothed display values (read/written only on UI thread)
    float currentLevel_{0.0f};
    float currentLevelL_{0.0f};
    float currentLevelR_{0.0f};

    // Peak hold values
    float peakLevel_{0.0f};
    float peakLevelL_{0.0f};
    float peakLevelR_{0.0f};
    int peakHoldCounter_{0};
    int peakHoldCounterL_{0};
    int peakHoldCounterR_{0};

    // Velocity for gravity-based decay
    float velocity_{0.0f};
    float velocityL_{0.0f};
    float velocityR_{0.0f};

    // Previous frame values for dirty rect optimization
    float previousLevel_{0.0f};
    float previousLevelL_{0.0f};
    float previousLevelR_{0.0f};

    bool stereo_{false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VUMeter)
  };

  /**
   * @brief Get the VU meter for external level updates
   */
  VUMeter *getVUMeter() { return &vuMeter_; }

  //==========================================================================
  // Component Interface
  //==========================================================================

  void drawSkia(SkCanvas *canvas) override;
  void resized() override;
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseEnter(const juce::MouseEvent &e) override;
  void mouseExit(const juce::MouseEvent &e) override;

  //==========================================================================
  // AnimationListener Interface
  //==========================================================================

  void onAnimationTick(float deltaMs) override;
  bool isAnimating() const override;
  bool requiresVisibility() const override { return true; }

private:
  //==========================================================================
  // Internal Methods
  //==========================================================================

  void updateButtonStates();
  void drawExpandChevron(SkCanvas *canvas, const SkRect &bounds, bool expanded);

  //==========================================================================
  // Member Variables
  //==========================================================================

  TrackGroupHeaderProps props_;

  // Child components
  VUMeter vuMeter_;
  SkiaButton muteButton_{"M"};
  SkiaButton soloButton_{"S"};

  // Animation state
  float hoverAnim_{0.0f};
  float expandAnim_{1.0f}; // 1.0 = expanded, 0.0 = collapsed
  bool isHovered_{false};

  // Layout constants
  static constexpr float kHeight = 36.0f;
  static constexpr float kMeterWidth = 24.0f;
  static constexpr float kButtonSize = 24.0f;
  static constexpr float kChevronSize = 16.0f;
  static constexpr float kPadding = 8.0f;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackGroupHeader)
};

} // namespace zenith
