/**
 * @file MenuBar.h
 * @brief Custom Skia-based Menu Bar for Zenith DAW
 *
 * Premium matte-black menu bar with animated hover states, vector icons,
 * and restrained blue accent lighting that matches the Zenith design system.
 */

#pragma once

#include "../design-system/InteractionHelper.h"
#include "../design-system/ZenithIcons.h"
#include "../framework/NeonGlow.h"
#include "../framework/SkiaComponent.h"
#include "../controls/SkiaPopupMenu.h"
#include <juce_gui_basics/juce_gui_basics.h>

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
  std::function<void()> onCut;
  std::function<void()> onCopy;
  std::function<void()> onPaste;
  std::function<void()> onDelete;
  std::function<void()> onSelectAll;
  std::function<void()> onToggleMixer;
  std::function<void()> onToggleBrowser;
  std::function<void()> onToggleView;
  std::function<void()> onZoomIn;
  std::function<void()> onZoomOut;
  std::function<void()> onFitToWindow;
  std::function<void()> onGettingStarted;
  std::function<void()> onKeyboardShortcuts;
  std::function<void()> onDocumentation;
  std::function<void()> onReportBug;
  std::function<void()> onAbout;

  void setUpdateAvailable(bool available) { 
    if (updateAvailable_ != available) {
        updateAvailable_ = available; 
        repaint(); 
    }
  }

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

  // Cached rendering resources
  SkFont menuFont_;
  SkFont smallFont_;
  bool updateAvailable_ = false;

  // Skia popup menus
  std::unique_ptr<SkiaPopupMenu> fileMenu_;
  std::unique_ptr<SkiaPopupMenu> editMenu_;
  std::unique_ptr<SkiaPopupMenu> viewMenu_;
  std::unique_ptr<SkiaPopupMenu> helpMenu_;

  void showFileMenu();
  void showEditMenu();
  void showViewMenu();
  void showHelpMenu();

  void updateLayout();
  void updateCachedPaints();
  
  void drawMenuItem(SkCanvas *canvas, const MenuItem &item, bool isHovered);
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithMenuBar)
};

} // namespace zenith
