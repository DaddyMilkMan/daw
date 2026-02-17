/*
  KeyboardShortcutManager.h

  Global keyboard shortcut manager for Zenith DAW

  Provides a centralized system for registering, managing, and triggering
  keyboard shortcuts across the application with context awareness.
*/

#pragma once

#include "../framework/SkiaComponent.h"
#include "../design-system/ZenithDesignSystem.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_core/juce_core.h>
#include <functional>
#include <map>
#include <vector>
#include <memory>
#include <set>

namespace zenith::UI {

/**
 * Keyboard shortcut category for organization
 */
enum class ShortcutCategory {
    None = 0,
    File,           // File operations
    Edit,          // Edit operations
    View,          // View operations
    Transport,     // Transport controls
    Mixer,         // Mixer controls
    PianoRoll,    // Piano roll operations
    Arranger,      // Arranger operations
    Window,        // Window management
    Help           // Help and documentation
};

/**
 * Modifier keys mask
 */
using ModifierKeys = juce::ModifierKeys;

/**
 * Keyboard shortcut definition
 */
class ShortcutContext {
public:
    ShortcutContext(const juce::String& name);
    ~ShortcutContext();

    /**
     * Get context name
     */
    juce::String getName() const;

    /**
     * Add shortcut to context
     */
    void addShortcut(const KeyboardShortcut& shortcut);

    /**
     * Remove shortcut from context
     */
    void removeShortcut(const juce::KeyPress& key);
    bool hasShortcut(const juce::KeyPress& key) const;

    /**
     * Get all shortcuts in context
     */
    std::vector<KeyboardShortcut> getShortcuts() const;

    /**
     * Check if context is active
     */
    bool isActive() const;

    /**
     * Set active state
     */
    void setActive(bool active);

    /**
     * Check if context is enabled
     */
    bool isEnabled() const;

    /**
     * Set enabled state
     */
    void setEnabled(bool enabled);

private:
    juce::String name_;
    std::vector<KeyboardShortcut> shortcuts_;
    bool active_;
    bool enabled_;
    std::set<juce::KeyPress> shortcutKeys_;

    friend class KeyboardShortcutManager;
};

/**
 * Keyboard Shortcut Manager Singleton

 * Manages global keyboard shortcuts with:
 * - Context-aware shortcut scoping
 * - Priority-based shortcut resolution
 * - Dynamic shortcut registration
 * - Conflict detection
 * - Visual shortcut hints
 * - Customizable key bindings
 */

} // namespace
