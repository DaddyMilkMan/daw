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

 * @file MenuBar.h
 * @brief Custom Skia-based Menu Bar for Zenith DAW - Neon Noir Edition
 * 
 * Premium glassmorphic menu bar with animated hover states, vector icons,
 * and neon glow effects matching the Zenith design system.
 */


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
  std::function<void()> onToggleView;
  std::function<void()> onToggleWingman;
  std::function<void()> onToggleSettings;
  std::function<void()> onZoomIn;
  std::function<void()> onZoomOut;
  std::function<void()> onZoomToFit;

  void setUpdateAvailable(bool available) { 
    if (updateAvailable_ != available) {
        updateAvailable_ = available; 
        repaint(); 
    }
  }

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
  bool updateAvailable_ = false;
  
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
