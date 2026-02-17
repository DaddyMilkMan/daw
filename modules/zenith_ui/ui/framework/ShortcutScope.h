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
class ShortcutScope {
public:
    /**
     * Constructor
     */
    ShortcutScope(KeyboardShortcutManager& manager, ShortcutContext* context);

    /**
     * Destructor
     */
    ~ShortcutScope();

    /**
     * Add shortcut in scope
     */
    void addShortcut(const KeyboardShortcut& shortcut);

    /**
     * Remove shortcut in scope
     */
    void removeShortcut(const juce::KeyPress& key);

private:
    KeyboardShortcutManager& manager_;
    ShortcutContext* context_;
    std::vector<KeyboardShortcut> addedShortcuts_;
    std::vector<juce::KeyPress> removedShortcuts_;
};

} // namespace Zenith::UI