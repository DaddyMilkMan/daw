/**
 * @file MenuBar.h
 * @brief Custom Skia-based Menu Bar for Zenith DAW - Neon Noir Edition
 * 
 * Premium glassmorphic menu bar with animated hover states, vector icons,
 * and neon glow effects matching the Zenith design system.
 */

#pragma once

#include "../design-system/InteractionHelper.h"
#include "../design-system/ZenithIcons.h"
#include "../framework/GlassmorphicPanel.h"
#include "../framework/NeonGlow.h"
#include "../framework/SkiaComponent.h"
#include <JuceHeader.h>

namespace zenith {

class ZenithMenuBar : public SkiaComponent {
public:
  ZenithMenuBar();
  ~ZenithMenuBar() override = default;

  // Configuration Callbacks
  std::function<void()> onNewProject;
  std::function<void()> onOpenProject;
  std::function<void()> onSaveProject;
  std::function<void()> onSaveProjectAs;
  std::function<void()> onImportAudio;
  std::function<void()> onExportAudio;
  std::function<void()> onUndo;
  std::function<void()> onRedo;
  std::function<void()> onToggleMixer;
  std::function<void()> onToggleBrowser;

  void paint(juce::Graphics &g) override;
  void drawSkia(SkCanvas *canvas) override;
  void resized() override;

  void mouseDown(const juce::MouseEvent &e) override;
  void mouseMove(const juce::MouseEvent &e) override;
  void mouseExit(const juce::MouseEvent &e) override;
  void mouseUp(const juce::MouseEvent &e) override;

  void timerCallback() override;
  void visibilityChanged() override;

private:
  struct MenuItem {
    juce::String name;
    juce::Rectangle<int> bounds;
    InteractionState state;  // Smooth hover/press animations
    SkPath icon;             // Menu item icon
  };

  std::vector<MenuItem> items_;
  int hoveredItemIndex_ = -1;

  // Collab button
  juce::Rectangle<int> collabButtonBounds_;
  InteractionState collabState_;
  std::unique_ptr<juce::CallOutBox> collabCallout_;

  // Cached rendering resources
  SkFont menuFont_;
  SkFont smallFont_;
  
  void showFileMenu();
  void showEditMenu();
  void showViewMenu();
  void showHelpMenu();

  void updateLayout();
  void updateCachedPaints();
  
  void drawMenuItem(SkCanvas *canvas, const MenuItem &item, bool isHovered);
  void drawCollabButton(SkCanvas *canvas);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithMenuBar)
};

} // namespace zenith
