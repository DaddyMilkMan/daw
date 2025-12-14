/*
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

#include "../engine/RecentProjectManager.h"
#include "skia/AuroraBackground.h"
#include "skia/GlassmorphicPanel.h"
#include "skia/SkiaComponent.h"
#include "skia/ZenithDesignSystem.h"
#include <functional>
#include <memory>

namespace zenith {

class ZenithHubComponent : public SkiaComponent,
                           public RecentProjectManager::Listener {
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

  // Animation hook
  void timerCallback() override;

  // RecentProjectManager::Listener
  void recentProjectsChanged() override;

  void show();
  void dismiss();

  /**
   * @brief Refresh the recent projects list from the manager
   */
  void refreshProjects();

private:
  RecentProjectManager &recentProjectManager_;
  LoadProjectCallback onLoadProject_;
  NewProjectCallback onNewProject_;
  std::function<void()> onDismiss_;

  // Animation states
  AnimatedValue alpha_;
  float animationTime_ = 0.0f;

  // Parallax / 3D Tilt
  struct Spring {
    float current = 0.0f;
    float target = 0.0f;
    float velocity = 0.0f;
    float stiffness = 0.1f;
    float damping = 0.82f;

    void update() {
      float force = (target - current) * stiffness;
      velocity += force;
      velocity *= damping;
      current += velocity;
    }
  };
  Spring tiltX_;
  Spring tiltY_;

  // Layout
  SkRect mainCardBounds_;
  SkRect recentArea_;
  SkRect templatesArea_;
  SkRect accountArea_;

  struct RecentProject {
    juce::String name;
    juce::String date;
    juce::String genre;
    juce::File path; // Actual file path for loading
    SkColor accent;
    SkRect bounds;
    bool isHovered = false;
    std::vector<float> waveform;
    Spring scaleSpring{1.0f, 1.0f}; // Start at 1.0
  };
  std::vector<RecentProject> recentProjects_;

  struct TemplateItem {
    juce::String name;
    juce::String icon; // Unicode or ID
    SkColor color;
    SkRect bounds;
    bool isHovered = false;
    Spring scaleSpring{1.0f, 1.0f};
  };
  std::vector<TemplateItem> templates_;

  // Profile
  SkRect profileBounds_;
  bool isProfileHovered_ = false;

  // New Project Button
  SkRect newProjectButtonBounds_;
  bool isNewProjectHovered_ = false;
  float buttonGradientAngle_ = 0.0f;

  struct Ripple {
    float x, y;
    float radius = 0.0f;
    float opacity = 1.0f;
    bool active = true;
  };
  std::vector<Ripple> buttonRipples_;

  // Helpers
  void drawBackground(SkCanvas *canvas);
  void drawRecentProjects(SkCanvas *canvas);
  void drawTemplates(SkCanvas *canvas);
  void drawAccount(SkCanvas *canvas);
  void drawNewProjectButton(SkCanvas *canvas);

  /** @brief Convert RecentProjectEntry to internal format */
  void loadFromManager();

  /** @brief Assign accent colors based on genre */
  static SkColor getAccentColorForGenre(const juce::String &genre);

  void updateLayout();

  // Aurora living background
  std::unique_ptr<AuroraBackground> auroraBackground_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithHubComponent)
};

} // namespace zenith
