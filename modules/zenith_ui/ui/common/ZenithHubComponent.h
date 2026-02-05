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

    ZenithHubComponent.h
    Created: 2025-12-13
    Author:  Zenith DAW Team

    The premium "Welcome Screen" / Dashboard for Zenith.
    Displays recent projects, templates, and user profile.


    Pinocchio Protocol: Removed mock data, now uses RecentProjectManager
    for real persistent project data.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "../../network/AuthenticationService.h"
#include "../../engine/RecentProjectManager.h"
#include "../utils/PhysicsSpring.h"
#include "../framework/AuroraBackground.h"
#include "../framework/GlassmorphicPanel.h"
#include "../framework/SkiaComponent.h"
#include "../controls/SkiaTextEditor.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../framework/AnimationCoordinator.h"
#include <functional>
#include <memory>
#include <vector>
#include <mutex>
#include <atomic>

class SkSurface;

namespace zenith {

class ZenithHubComponent : public SkiaComponent,
                           public RecentProjectManager::Listener,
                           public AuthenticationService::Listener {
public:
  /**
   * @brief Callback type for project loading
   * @param projectPath Path to the project file to load
   */
  using LoadProjectCallback =
      std::function<void(const juce::File &projectPath)>;

  /**
   * @brief Callback type for new project creation
   */
  using NewProjectCallback = std::function<void()>;

  /**
   * @brief Constructor with dependency injection
   * @param recentProjectManager Reference to the RecentProjectManager for
   * persistence
   * @param onLoadProject Callback when a project is selected
   * @param onNewProject Callback when "New Project" is clicked
   * @param onDismiss Callback when the hub is dismissed (for cleanup/hiding)
   */
  explicit ZenithHubComponent(RecentProjectManager &recentProjectManager,
                              LoadProjectCallback onLoadProject,
                              NewProjectCallback onNewProject,
                              std::function<void()> onDismiss);
  ~ZenithHubComponent() override;

  void drawSkia(SkCanvas *canvas) override;
  void resized() override;

  void mouseMove(const juce::MouseEvent &e) override;

  void mouseDown(const juce::MouseEvent &e) override;
  void mouseUp(const juce::MouseEvent &e) override;
  void mouseExit(const juce::MouseEvent &e) override;
  // Keyboard Navigation
  bool keyPressed(const juce::KeyPress &key) override;

  // New: Restrict hits to card only
  bool hitTest(int x, int y) override;

  // AnimationListener interface (replaces timerCallback)
  void onAnimationTick(float deltaMs) override;
  bool isAnimating() const override;
  bool requiresVisibility() const override { return true; }

  // RecentProjectManager::Listener
  void recentProjectsChanged() override;

  void show();
  void dismiss();
  void refreshProjects();

  float getAlpha() const { return alpha_.get(); }

  /**
   * @brief Refresh the recent projects list from the manager
   */

private:
  RecentProjectManager &recentProjectManager_;
  LoadProjectCallback onLoadProject_;
  NewProjectCallback onNewProject_;
  std::function<void()> onDismiss_;

  // Animation states
  AnimatedValue alpha_;
  float animationTime_ = 0.0f;

  // Parallax / 3D Tilt
  zenith::PhysicsSpring tiltX_;
  zenith::PhysicsSpring tiltY_;

  // Curtain Lift Animation
  zenith::PhysicsSpring curtainY_;
  zenith::PhysicsSpring curtainAlpha_{1.0f};
  std::unique_ptr<juce::VBlankAttachment> vBlankAttachment_;

  // Cached Fonts & Paints - Optimization for A+ Grade
  SkFont titleFont_;
  SkFont subFont_;
  SkFont headerFont_;
  SkFont cardTitleFont_;
  SkFont cardDateFont_;
  SkFont cardGenreFont_;
  SkFont buttonFont_;
  SkFont statusFont_;
  SkFont templateFont_;
  SkFont profileFont_;
  SkFont bodyFont_;

  SkPaint textPaint_;
  SkPaint subPaint_;

  // Layout
  SkRect mainCardBounds_;
  SkRect recentArea_; // Total area for recent projects section
  SkRect recentHeaderBounds_;
  SkRect recentGridBounds_;

  SkRect accountArea_; // Total area for account section
  SkRect accountHeaderBounds_;
  SkRect accountContentBounds_;

  SkRect templatesArea_; // Total area for templates section
  SkRect quickStartHeaderBounds_;
  SkRect templatesContentBounds_;

  struct RecentProject {
    juce::String name;
    juce::String date;
    juce::String genre;
    juce::File path; // Actual file path for loading
    SkColor accent;
    SkRect bounds;
    bool isHovered = false;  // Changed from atomic - only accessed from message thread
    std::vector<float> waveform;
    zenith::PhysicsSpring scaleSpring{1.0f}; // Start at 1.0
  };
  std::vector<RecentProject> recentProjects_;
  std::mutex projectsMutex_;

  struct TemplateItem {
    juce::String name;
    juce::String icon; // Unicode or ID
    SkColor color;
    SkRect bounds;
    bool isHovered = false;  // Changed from atomic - only accessed from message thread
    zenith::PhysicsSpring scaleSpring{1.0f};
  };
  std::vector<TemplateItem> templates_;

  // AuthenticationService::Listener
  void authStateChanged(bool isLoggedIn, const AuthUser& user) override;

  // Profile Menu
  // bool isLoggedIn_ = true; // REMOVED: Using AuthenticationService state
  zenith::PhysicsSpring menuSpring_{0.0f};
  void drawProfileMenu(SkCanvas* canvas);

  // New Project Button
  SkRect newProjectButtonBounds_;
  std::atomic<bool> isNewProjectHovered_{false};
  float buttonGradientAngle_ = 0.0f;

  // Profile Icon (top-right of hub)
  SkRect profileIconBounds_;
  SkRect friendsMenuItemBounds_;
  std::atomic<bool> isProfileIconHovered_{false};
  std::atomic<bool> isProfileMenuOpen_{false};

  // Greeting Customization
  juce::String greetingText_ = "Welcome back, User";
  SkRect greetingTextBounds_;
  SkRect greetingEditIconBounds_;
  std::unique_ptr<SkiaTextEditor> greetingEditor_;
  std::atomic<bool> isGreetingHovered_{false};

  void showGreetingEditor();
  void hideGreetingEditor(bool save);

  struct Ripple {
    float x, y;
    float radius = 0.0f;
    float opacity = 1.0f;
    bool active = true;
  };
  std::vector<Ripple> buttonRipples_;

  // Keyboard Navigation State
  enum class SelectionSection { None, Recent, New, Templates };
  SelectionSection selectedSection_ = SelectionSection::None;
  int selectedIndex_ = -1;

  void moveSelection(int dx, int dy);
  void triggerSelection();

  // Helpers
  void drawText(SkCanvas *canvas, const juce::String &text,
                const SkRect &bounds, const SkFont &font, const SkPaint &paint,
                bool centerVertical = true);
  void drawBackground(SkCanvas *canvas);
  void drawProjectList(SkCanvas *canvas);
  void drawRecentProjects(SkCanvas *canvas);
  void drawTemplates(SkCanvas *canvas);
  void drawNewProjectButton(SkCanvas *canvas);
  void drawProfileIcon(SkCanvas *canvas);

  /** @brief Convert RecentProjectEntry to internal format */
  void loadFromManager();

  /** @brief Assign accent colors based on genre */
  static SkColor getAccentColorForGenre(const juce::String &genre);

  void updateLayout();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithHubComponent)
};

} // namespace zenith
