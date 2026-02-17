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
struct KeyboardShortcut {
    juce::KeyPress key;
    juce::String name;
    juce::String description;
    ShortcutCategory category;
    bool enabled;
    bool visible;
    int priority;
    std::function<void()> action;
    std::function<bool()> canExecute;
    juce::Component* context;

    KeyboardShortcut()
        : category(ShortcutCategory::None)
        , enabled(true)
        , visible(true)
        , priority(0)
        , context(nullptr) {}

    KeyboardShortcut(const juce::KeyPress& k,
                     const juce::String& n,
                     const juce::String& desc,
                     ShortcutCategory cat = ShortcutCategory::None)
        : key(k)
        , name(n)
        , description(desc)
        , category(cat)
        , enabled(true)
        , visible(true)
        , priority(0)
        , context(nullptr) {}

    // Comparison operators for sorting
    bool operator<(const KeyboardShortcut& other) const {
        if (category != other.category)
            return static_cast<int>(category) < static_cast<int>(other.category);
        return priority < other.priority;
    }
};

/**
 * Shortcut context for scope-based shortcuts
 */

} // namespace
