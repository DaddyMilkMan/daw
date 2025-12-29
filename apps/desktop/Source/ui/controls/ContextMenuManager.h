/*
  ==============================================================================

    ContextMenuManager.h
    Created: 2025-12-25
    Author:  Zenith DAW Team

    Singleton manager for centralized context menu handling.
    Ensures only one context menu is visible at a time and handles
    global dismissal on click-outside.

  ==============================================================================
*/

#pragma once

#include "SkiaPopupMenu.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

namespace zenith {

/**
 * @class ContextMenuManager
 * @brief Singleton that manages context menu display
 *
 * Features:
 * - Single active menu at a time (auto-dismiss previous)
 * - Global mouse listener for click-outside dismissal
 * - Screen boundary collision detection
 */
class ContextMenuManager : private juce::MouseListener,
                           private juce::ComponentListener {
public:
  //============================================================================
  // Singleton Access
  //============================================================================
  
  static ContextMenuManager& getInstance() {
    static ContextMenuManager instance;
    return instance;
  }

  //============================================================================
  // Menu Management
  //============================================================================
  
  /**
   * @brief Show a context menu at the given position
   *
   * If another menu is currently visible, it will be dismissed first.
   *
   * @param menu The menu to show (ownership transferred)
   * @param screenPosition Position on screen to show menu
   */
  void showMenu(std::unique_ptr<SkiaPopupMenu> menu,
                juce::Point<int> screenPosition);
  
  /**
   * @brief Show a context menu at the current mouse position
   */
  void showMenuAtMouse(std::unique_ptr<SkiaPopupMenu> menu);
  
  /**
   * @brief Show a context menu relative to a component
   */
  void showMenuAt(std::unique_ptr<SkiaPopupMenu> menu,
                  juce::Component* component, int x, int y);
  
  /**
   * @brief Dismiss the currently active menu (if any)
   */
  void dismissActiveMenu();
  
  /**
   * @brief Check if a menu is currently visible
   */
  bool isMenuVisible() const { return activeMenu_ != nullptr && activeMenu_->isVisible(); }
  
  /**
   * @brief Get the currently active menu (may be nullptr)
   */
  SkiaPopupMenu* getActiveMenu() const { return activeMenu_.get(); }

  //============================================================================
  // Factory Methods
  //============================================================================
  
  /**
   * @brief Create a new empty popup menu
   */
  static std::unique_ptr<SkiaPopupMenu> createMenu();

private:
  ContextMenuManager();
  ~ContextMenuManager() override;

  // Delete copy/move constructors
  ContextMenuManager(const ContextMenuManager&) = delete;
  ContextMenuManager& operator=(const ContextMenuManager&) = delete;
  ContextMenuManager(ContextMenuManager&&) = delete;
  ContextMenuManager& operator=(ContextMenuManager&&) = delete;

  //============================================================================
  // MouseListener Interface
  //============================================================================
  
  void mouseDown(const juce::MouseEvent& e) override;

  //============================================================================
  // ComponentListener Interface
  //============================================================================
  
  void componentBeingDeleted(juce::Component& component) override;

  //============================================================================
  // Private Members
  //============================================================================
  
  std::unique_ptr<SkiaPopupMenu> activeMenu_;
  
  // Desktop window to host the menu
  std::unique_ptr<juce::Component> menuHost_;
  
  // Private class for hosting menu on desktop
  class MenuHostWindow;
  std::unique_ptr<MenuHostWindow> hostWindow_;
};

} // namespace zenith
