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

#pragma once

#include <functional>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <vector>


namespace zenith {

/**
 * @struct RecentProjectEntry
 // Brief: Represents a single recent project entry
 */
struct RecentProjectEntry {
  juce::String name;           ///< Display name of the project
  juce::File path;             ///< Full path to the .zth project file
  juce::Time lastOpened;       ///< When the project was last opened
  juce::String genre;          ///< Optional genre/category tag
  juce::String accentColorHex; ///< Hex color for UI display (e.g., "#00FFFF")

  /** Default constructor */
  RecentProjectEntry() = default;

  /** Full constructor */
  RecentProjectEntry(const juce::String &name, const juce::File &path,
                     const juce::Time &lastOpened,
                     const juce::String &genre = "",
                     const juce::String &accentColor = "#00FFFF")
      : name(name), path(path), lastOpened(lastOpened), genre(genre),
        accentColorHex(accentColor) {}

  /** Check if the project file still exists on disk */
  bool isValid() const { return path.existsAsFile(); }

  /** Get a human-readable relative time string (e.g., "2 hours ago") */
  juce::String getRelativeTimeString() const;

  /** Convert to JSON object for serialization */
  juce::var toJson() const;

  /** Create from JSON object */
  static RecentProjectEntry fromJson(const juce::var &json);
};

/**
 * @class RecentProjectManager
 // Brief: Manages persistent storage of recently opened projects
 *
 * This class:
 * - Loads/saves recent projects from/to a JSON file
 * - Automatically prunes entries for deleted project files
 * - Limits the list to a configurable maximum size
 * - Provides thread-safe access to recent projects
 */
class RecentProjectManager {
public:
  /** Maximum number of recent projects to retain */
  static constexpr int MAX_RECENT_PROJECTS = 20;

  /**
   * Constructor
   * @param maxProjects Maximum number of projects to store (default: 20)
   */
  explicit RecentProjectManager(int maxProjects = MAX_RECENT_PROJECTS);

  /** Destructor - saves any pending changes */
  ~RecentProjectManager();

  //==========================================================================
  // Core API
  //==========================================================================

  /**
   * Add or update a project in the recent list
   * If the project already exists, it is moved to the front and updated.
   *
   * @param path Path to the project file (.zth)
   * @param name Display name (if empty, uses filename without extension)
   * @param genre Optional genre/category
   */
  void addProject(const juce::File &path, const juce::String &name = "",
                  const juce::String &genre = "");

  /**
   * Remove a project from the recent list
   * @param path Path to the project file
   * @return true if the project was found and removed
   */
  bool removeProject(const juce::File &path);

  /**
   * Clear all recent projects
   */
  void clearAll();

  /**
   * Get the list of recent projects
   * Projects are ordered by last opened time (most recent first)
   * @param pruneInvalid If true, removes entries for non-existent files
   * @return Vector of recent project entries
   */
  std::vector<RecentProjectEntry> getRecentProjects(bool pruneInvalid = true);

  /**
   * Get the list of recent projects WITHOUT pruning.
   * Safe for calling from the Message Thread without blocking on I/O.
   */
  std::vector<RecentProjectEntry> getRecentProjectsFast() const;

  /**
   * Get a specific project by index
   * @param index Zero-based index (0 = most recent)
   * @return Pointer to entry, or nullptr if index is out of range
   */
  const RecentProjectEntry *getProject(int index) const;

  /**
   * Get the number of recent projects
   */
  int getNumProjects() const;

  /**
   * Check if the list is empty
   */
  bool isEmpty() const;

  //==========================================================================
  // Persistence
  //==========================================================================

  /**
   * Force save to disk
   * @return true if save was successful
   */
  bool save();

  /**
   * Reload from disk
   * @return true if load was successful
   */
  bool load();

  /**
   * Get the path to the recent projects JSON file
   */
  juce::File getStorageFile() const;

  //==========================================================================
  // Listeners
  //==========================================================================

  /**
   * Listener interface for changes to the recent projects list
   */
  class Listener {
  public:
    virtual ~Listener() = default;

    /** Called when the recent projects list changes */
    virtual void recentProjectsChanged() = 0;
  };

  void addListener(Listener *listener);
  void removeListener(Listener *listener);

private:
  //==========================================================================
  // Internal Methods
  //==========================================================================

  /** Prune entries for non-existent files */
  void pruneInvalidEntries();

  /** Enforce maximum list size */
  void enforceMaxSize();

  /** Notify listeners of changes */
  void notifyListeners();

  /** Sort entries by last opened time (descending) */
  void sortByTime();

  //==========================================================================
  // Member Variables
  //==========================================================================

  std::vector<RecentProjectEntry> projects_;
  int maxProjects_;
  bool isDirty_ = false;
  juce::ListenerList<Listener> listeners_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RecentProjectManager)
};

} // namespace zenith
