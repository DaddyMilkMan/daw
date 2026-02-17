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

} // namespace
