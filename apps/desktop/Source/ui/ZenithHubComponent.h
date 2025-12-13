/*
  ==============================================================================

    ZenithHubComponent.h
    Created: 2025-12-13
    Author:  Zenith DAW Team

    The premium "Welcome Screen" / Dashboard for Zenith.
    Displays recent projects, templates, and user profile.

  ==============================================================================
*/

#pragma once

#include "skia/GlassmorphicPanel.h"
#include "skia/SkiaComponent.h"
#include "skia/ZenithDesignSystem.h"
#include <functional>

namespace zenith {

class ZenithHubComponent : public SkiaComponent {
public:
  explicit ZenithHubComponent(std::function<void()> onDismiss);
  ~ZenithHubComponent() override;

  void drawSkia(SkCanvas *canvas) override;
  void resized() override;

  void mouseMove(const juce::MouseEvent &e) override;
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseUp(const juce::MouseEvent &e) override;

  // Animation hook
  void timerCallback() override;

  void show();
  void dismiss();

private:
  std::function<void()> onDismiss_;

  // Animation states
  AnimatedValue alpha_;
  float animationTime_ = 0.0f;

  // Layout
  SkRect mainCardBounds_;
  SkRect recentArea_;
  SkRect templatesArea_;
  SkRect accountArea_;

  struct RecentProject {
    juce::String name;
    juce::String date;
    juce::String genre;
    SkColor accent;
    SkRect bounds;
    bool isHovered = false;
  };
  std::vector<RecentProject> recentProjects_;

  struct TemplateItem {
    juce::String name;
    juce::String icon; // Unicode or ID
    SkColor color;
    SkRect bounds;
    bool isHovered = false;
  };
  std::vector<TemplateItem> templates_;

  // Profile
  SkRect profileBounds_;
  bool isProfileHovered_ = false;

  // New Project Button
  SkRect newProjectButtonBounds_;
  bool isNewProjectHovered_ = false;

  // Helpers
  void drawBackground(SkCanvas *canvas);
  void drawRecentProjects(SkCanvas *canvas);
  void drawTemplates(SkCanvas *canvas);
  void drawAccount(SkCanvas *canvas);
  void drawNewProjectButton(SkCanvas *canvas);

  void createMockData();
  void updateLayout();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithHubComponent)
};

} // namespace zenith
