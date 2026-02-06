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
class KeyboardShortcutManager : public juce::Component {
public:
    /**
     * Get the singleton instance
     */
    static KeyboardShortcutManager& getInstance();

    /**
     * Register a global shortcut
     */
    void registerShortcut(const KeyboardShortcut& shortcut);

    /**
     * Register a context-specific shortcut
     */
    void registerShortcut(ShortcutContext* context, const KeyboardShortcut& shortcut);

    /**
     * Unregister a shortcut
     */
    void unregisterShortcut(const juce::KeyPress& key);

    /**
     * Unregister all shortcuts for a context
     */
    void unregisterContextShortcuts(ShortcutContext* context);

    /**
     * Create a new shortcut context
     */
    ShortcutContext* createContext(const juce::String& name);

    /**
     * Remove a context
     */
    void removeContext(ShortcutContext* context);

    /**
     * Set active context
     */
    void setActiveContext(ShortcutContext* context);

    /**
     * Get active context
     */
    ShortcutContext* getActiveContext() const;

    /**
     * Find shortcut for key press
     */
    KeyboardShortcut* findShortcut(const juce::KeyPress& key);

    /**
     * Execute shortcut action
     */
    bool executeShortcut(const juce::KeyPress& key);

    /**
     * Check if shortcut can execute
     */
    bool canExecuteShortcut(const KeyboardShortcut& shortcut) const;

    /**
     * Show shortcut hints
     */
    void showShortcutHints();

    /**
     * Hide shortcut hints
     */
    void hideShortcutHints();

    /**
     * Get shortcuts by category
     */
    std::vector<KeyboardShortcut> getShortcutsByCategory(ShortcutCategory category) const;

    /**
     * Get all conflicts
     */
    std::vector<std::pair<juce::KeyPress, std::vector<juce::String>>> getConflicts() const;

    /**
     * Check for conflicts
     */
    bool hasConflicts() const;

    /**
     * Resolve conflicts automatically
     */
    void resolveConflicts();

    /**
     * Set visual shortcut hint provider
     */
    void setShortcutHintProvider(std::function<juce::String(const juce::KeyPress&)> provider);

    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    KeyboardShortcutManager();
    ~KeyboardShortcutManager() override;


    // Private implementation
    struct Impl;
    std::unique_ptr<Impl> pimpl_;

    // Private methods
    void showShortcutHintsInternal();
    void hideShortcutHintsInternal();
    void drawShortcutOverlay(juce::Graphics& g);
    juce::String getShortcutDescription(const KeyboardShortcut& shortcut) const;
    juce::Colour getShortcutColor(ShortcutCategory category) const;

    // Event handling
    bool keyPressed(const juce::KeyPress& key) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KeyboardShortcutManager)
};

/**
 * Built-in shortcut definitions
 */
namespace BuiltInShortcuts {
    // File operations
    extern const juce::KeyPress NEW_PROJECT;
    extern const juce::KeyPress OPEN_PROJECT;
    extern const juce::KeyPress SAVE_PROJECT;
    extern const juce::KeyPress SAVE_AS_PROJECT;
    extern const juce::KeyPress EXPORT_AUDIO;

    // Edit operations
    extern const juce::KeyPress UNDO;
    extern const juce::KeyPress REDO;
    extern const juce::KeyPress CUT;
    extern const juce::KeyPress COPY;
    extern const juce::KeyPress PASTE;
    extern const juce::KeyPress DELETE;

    // Transport operations
    extern const juce::KeyPress PLAY;
    extern const juce::KeyPress STOP;
    extern const juce::KeyPress RECORD;
    extern const juce::KeyPress LOOP;
    extern const juce::KeyPress METRONOME;

    // Navigation
    extern const juce::KeyPress NAVIGATE_LEFT;
    extern const juce::KeyPress NAVIGATE_RIGHT;
    extern const juce::KeyPress NAVIGATE_UP;
    extern const juce::KeyPress NAVIGATE_DOWN;
    extern const juce::KeyPress PAGE_UP;
    extern const juce::KeyPress PAGE_DOWN;
    extern const juce::KeyPress HOME;
    extern const juce::KeyPress END;

    // Zoom
    extern const juce::KeyPress ZOOM_IN;
    extern const juce::KeyPress ZOOM_OUT;
    extern const juce::KeyPress ZOOM_RESET;
}

/**
 * Shortcut utility functions
 */
namespace ShortcutUtils {
    /**
     * Create a shortcut from key string
     */
    juce::KeyPress createShortcut(const juce::String& keyString);

    /**
     * Format key for display
     */
    juce::String formatKeyForDisplay(const juce::KeyPress& key);

    /**
     * Parse key string
     */
    std::vector<juce::KeyPress> parseKeyString(const juce::String& keyString);

    /**
     * Check if key is modifier key
     */
    bool isModifierKey(const juce::KeyPress& key);

    /**
     * Check if key is printable character
     */
    bool isPrintableKey(const juce::KeyPress& key);
}

/**
 * RAII shortcut scope
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