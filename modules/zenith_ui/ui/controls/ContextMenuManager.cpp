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

    ContextMenuManager.cpp
    Created: 2025-12-25
    Author:  Zenith DAW Team

    Implementation of centralized context menu management

  ==============================================================================

*/

#include "ContextMenuManager.h"

namespace zenith {

//==============================================================================
// MenuHostWindow - Private helper class
//==============================================================================

class ContextMenuManager::MenuHostWindow : public juce::Component {
public:
  MenuHostWindow() {
    setOpaque(false);
    setAlwaysOnTop(true);
  }

  void paint(juce::Graphics& g) override {
    // Transparent - menu does its own drawing
    juce::ignoreUnused(g);
  }

  void addMenu(SkiaPopupMenu* menu) {
    if (menu) {
      addAndMakeVisible(menu);
    }
  }

  void mouseDown(const juce::MouseEvent& e) override {
    // Check if click was outside the menu
    auto* menu = dynamic_cast<SkiaPopupMenu*>(getChildComponent(0));
    if (menu) {
      auto localBounds = menu->getBoundsInParent();
      if (!localBounds.contains(e.getPosition())) {
        ContextMenuManager::getInstance().dismissActiveMenu();
      }
    }
  }
};

//==============================================================================
// ContextMenuManager Implementation
//==============================================================================

ContextMenuManager::ContextMenuManager() {
  juce::Desktop::getInstance().addGlobalMouseListener(this);
}

ContextMenuManager::~ContextMenuManager() {
  juce::Desktop::getInstance().removeGlobalMouseListener(this);
  dismissActiveMenu();
}

void ContextMenuManager::showMenu(std::unique_ptr<SkiaPopupMenu> menu,
                                  juce::Point<int> screenPosition) {
  if (!menu)
    return;

  // Dismiss any existing menu
  dismissActiveMenu();

  activeMenu_ = std::move(menu);
  activeMenu_->addComponentListener(this);

  // Create host window if needed
  if (!hostWindow_) {
    hostWindow_ = std::make_unique<MenuHostWindow>();
  }

  // Get display bounds
  auto displays = juce::Desktop::getInstance().getDisplays();
  auto displayBounds = displays.getPrimaryDisplay()->userArea;

  // Set up host window to cover entire display
  hostWindow_->setBounds(displayBounds);
  hostWindow_->setVisible(true);
  hostWindow_->addToDesktop(juce::ComponentPeer::windowIsTemporary |
                            juce::ComponentPeer::windowHasDropShadow);
  hostWindow_->toFront(true);

  // Add menu to host and position it
  hostWindow_->addMenu(activeMenu_.get());
  
  // Convert screen position to host-relative
  auto hostOffset = hostWindow_->getScreenPosition();
  auto localPos = screenPosition - hostOffset;
  
  activeMenu_->showAt(hostWindow_.get(), localPos.x, localPos.y);
}

void ContextMenuManager::showMenuAtMouse(std::unique_ptr<SkiaPopupMenu> menu) {
  auto mousePos = juce::Desktop::getInstance().getMousePosition();
  showMenu(std::move(menu), mousePos);
}

void ContextMenuManager::showMenuAt(std::unique_ptr<SkiaPopupMenu> menu,
                                    juce::Component* component, int x, int y) {
  if (!component)
    return;

  auto screenPos = component->localPointToGlobal(juce::Point<int>(x, y));
  showMenu(std::move(menu), screenPos);
}

void ContextMenuManager::dismissActiveMenu() {
  if (activeMenu_) {
    activeMenu_->removeComponentListener(this);
    activeMenu_->hideMenu();
    activeMenu_.reset();
  }

  if (hostWindow_) {
    hostWindow_->setVisible(false);
    hostWindow_->removeFromDesktop();
  }
}

std::unique_ptr<SkiaPopupMenu> ContextMenuManager::createMenu() {
  return std::make_unique<SkiaPopupMenu>();
}

//==============================================================================
// MouseListener
//==============================================================================

void ContextMenuManager::mouseDown(const juce::MouseEvent& e) {
  if (!activeMenu_ || !activeMenu_->isVisible())
    return;

  // Check if click was inside the active menu or any submenu
  auto* source = e.originalComponent;
  
  // Walk up parent chain to see if click was in menu hierarchy
  auto* comp = source;
  while (comp) {
    if (dynamic_cast<SkiaPopupMenu*>(comp) != nullptr) {
      // Click was inside a menu - let it handle it
      return;
    }
    comp = comp->getParentComponent();
  }

  // Click was outside menu - dismiss
  dismissActiveMenu();
}

//==============================================================================
// ComponentListener
//==============================================================================

void ContextMenuManager::componentBeingDeleted(juce::Component& component) {
  if (&component == activeMenu_.get()) {
    // BUG FIX #4: Component is being deleted externally - release ownership
    // to avoid double-delete. Explicit void cast documents intent.
    (void)activeMenu_.release();
    if (hostWindow_) {
      hostWindow_->setVisible(false);
    }
  }
}

} // namespace zenith
