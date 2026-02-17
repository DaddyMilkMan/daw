/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#pragma once

#include "SkiaComponent.h"
#include "SkiaLayout.h"
#include "ZenithDesignSystem.h"
#include <juce_core/juce_core.h>

// Forward declarations for UI components
namespace zenith {
namespace layout {
class UserPreferences {
public:
  static UserPreferences &getInstance();

  // Auto-save
  void setAutoSaveEnabled(bool enabled);
  bool isAutoSaveEnabled() const;

  void setAutoSaveInterval(int minutes);
  int getAutoSaveInterval() const;

  // Tooltips
  void setTooltipsEnabled(bool enabled);
  bool isTooltipsEnabled() const;

  void setTooltipDelay(int milliseconds);
  int getTooltipDelay() const;

  // Welcome screen
  void setWelcomeScreenEnabled(bool enabled);
  bool isWelcomeScreenEnabled() const;

  // Recent files
  void setRecentFilesMaxCount(int count);
  int getRecentFilesMaxCount() const;

  juce::StringArray getRecentFiles() const;
  void addRecentFile(const juce::File &file);
  void clearRecentFiles();

  // Undo/Redo
  void setUndoLimit(int limit);
  int getUndoLimit() const;

  // Keyboard shortcuts
  juce::DynamicObject::Ptr getKeyboardShortcuts() const;
  void setKeyboardShortcuts(const juce::DynamicObject::Ptr &shortcuts);

private:
  UserPreferences() = default;
  ~UserPreferences() = default;

  juce::CriticalSection lock_;
};

// ============================================================================
// Settings UI Component
// ============================================================================

} // namespace
