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

// RecentProjectManager.cpp

#include <juce_core/juce_core.h>
#include "RecentProjectManager.h"
#include <algorithm>

namespace zenith {

// RecentProjectEntry Implementation
//==============================================================================

juce::String RecentProjectEntry::getRelativeTimeString() const {
  const juce::Time now = juce::Time::getCurrentTime();
  const juce::RelativeTime diff = now - lastOpened;

  const double seconds = diff.inSeconds();
  const double minutes = diff.inMinutes();
  const double hours = diff.inHours();
  const double days = diff.inDays();
  const double weeks = diff.inWeeks();

  if (seconds < 60) {
    return "Just now";
  } else if (minutes < 60) {
    int m = static_cast<int>(minutes);
    return juce::String(m) + (m == 1 ? " minute ago" : " minutes ago");
  } else if (hours < 24) {
    int h = static_cast<int>(hours);
    return juce::String(h) + (h == 1 ? " hour ago" : " hours ago");
  } else if (days < 7) {
    int d = static_cast<int>(days);
    if (d == 1)
      return "Yesterday";
    return juce::String(d) + " days ago";
  } else if (weeks < 4) {
    int w = static_cast<int>(weeks);
    return juce::String(w) + (w == 1 ? " week ago" : " weeks ago");
  } else {
    // More than a month - show the actual date
    return lastOpened.formatted("%b %d, %Y");
  }
}

juce::var RecentProjectEntry::toJson() const {
  // Bug 28: Use DynamicObject::Ptr to prevent leaks if not immediately wrapped
  juce::DynamicObject::Ptr obj = new juce::DynamicObject();

  obj->setProperty("name", name);
  obj->setProperty("path", path.getFullPathName());
  obj->setProperty("lastOpened", lastOpened.toMilliseconds());
  obj->setProperty("genre", genre);
  obj->setProperty("accentColor", accentColorHex);

  return juce::var(obj);
}

RecentProjectEntry RecentProjectEntry::fromJson(const juce::var &json) {
  RecentProjectEntry entry;

  if (auto *obj = json.getDynamicObject()) {
    entry.name = obj->getProperty("name").toString();
    entry.path = juce::File(obj->getProperty("path").toString());
    entry.lastOpened =
        juce::Time(static_cast<juce::int64>(obj->getProperty("lastOpened")));
    entry.genre = obj->getProperty("genre").toString();
    entry.accentColorHex = obj->getProperty("accentColor").toString();

    // Default accent color if not specified
    if (entry.accentColorHex.isEmpty()) {
      entry.accentColorHex = "#00FFFF";
    }
  }

  return entry;
}

//==============================================================================
// RecentProjectManager Implementation
//==============================================================================

RecentProjectManager::RecentProjectManager(int maxProjects)
    : maxProjects_(maxProjects) {
  load();
}

RecentProjectManager::~RecentProjectManager() {
  if (isDirty_) {
    save();
  }
}

void RecentProjectManager::addProject(const juce::File &path,
                                      const juce::String &name,
                                      const juce::String &genre) {
  if (!path.existsAsFile()) {
    DBG("RecentProjectManager: Cannot add non-existent file: " +
        path.getFullPathName());
    return;
  }

  const juce::String displayName =
      name.isEmpty() ? path.getFileNameWithoutExtension() : name;

  // Check if project already exists and remove it
  auto it = std::find_if(
      projects_.begin(), projects_.end(),
      [&path](const RecentProjectEntry &entry) { return entry.path == path; });

  juce::String existingGenre = genre;
  juce::String existingAccent = "#00FFFF";

  if (it != projects_.end()) {
    // Preserve existing metadata if not overridden
    if (existingGenre.isEmpty())
      existingGenre = it->genre;
    existingAccent = it->accentColorHex;
    projects_.erase(it);
  }

  // Create new entry with current timestamp
  RecentProjectEntry entry(displayName, path, juce::Time::getCurrentTime(),
                           existingGenre, existingAccent);

  // Insert at the front (most recent)
  projects_.insert(projects_.begin(), entry);

  // Enforce limits
  enforceMaxSize();

  isDirty_ = true;
  notifyListeners();

  DBG("RecentProjectManager: Added project '" + displayName + "' from " +
      path.getFullPathName());
}

bool RecentProjectManager::removeProject(const juce::File &path) {
  auto it = std::find_if(
      projects_.begin(), projects_.end(),
      [&path](const RecentProjectEntry &entry) { return entry.path == path; });

  if (it != projects_.end()) {
    DBG("RecentProjectManager: Removed project: " + it->name);
    projects_.erase(it);
    isDirty_ = true;
    notifyListeners();
    return true;
  }

  return false;
}

void RecentProjectManager::clearAll() {
  if (!projects_.empty()) {
    projects_.clear();
    isDirty_ = true;
    notifyListeners();
    DBG("RecentProjectManager: Cleared all recent projects");
  }
}

std::vector<RecentProjectEntry>
RecentProjectManager::getRecentProjects(bool pruneInvalid) {
  if (pruneInvalid) {
    pruneInvalidEntries();
  }

  return projects_;
}

std::vector<RecentProjectEntry> RecentProjectManager::getRecentProjectsFast() const {
  return projects_;
}

const RecentProjectEntry *RecentProjectManager::getProject(int index) const {
  if (index >= 0 && index < static_cast<int>(projects_.size())) {
    return &projects_[static_cast<size_t>(index)];
  }
  return nullptr;
}

int RecentProjectManager::getNumProjects() const {
  return static_cast<int>(projects_.size());
}

bool RecentProjectManager::isEmpty() const { return projects_.empty(); }

bool RecentProjectManager::save() {
  juce::File file = getStorageFile();

  // Ensure parent directory exists
  if (!file.getParentDirectory().exists()) {
    if (!file.getParentDirectory().createDirectory()) {
      DBG("RecentProjectManager: Failed to create storage directory");
      return false;
    }
  }

  // Build JSON array
  juce::Array<juce::var> jsonArray;
  for (const auto &entry : projects_) {
    jsonArray.add(entry.toJson());
  }

  juce::var root(jsonArray);
  juce::String jsonString = juce::JSON::toString(root, true);

  // Write to file
  if (file.replaceWithText(jsonString)) {
    isDirty_ = false;
    DBG("RecentProjectManager: Saved " + juce::String(projects_.size()) +
        " projects to " + file.getFullPathName());
    return true;
  }

  DBG("RecentProjectManager: Failed to save to " + file.getFullPathName());
  return false;
}

bool RecentProjectManager::load() {
  juce::File file = getStorageFile();

  if (!file.existsAsFile()) {
    DBG("RecentProjectManager: No existing file at " + file.getFullPathName());
    return true; // Not an error - just no saved data yet
  }

  juce::String jsonString = file.loadFileAsString();
  if (jsonString.isEmpty()) {
    DBG("RecentProjectManager: Empty file at " + file.getFullPathName());
    return true;
  }

  juce::var parsed = juce::JSON::parse(jsonString);
  if (!parsed.isArray()) {
    DBG("RecentProjectManager: Invalid JSON format in " +
        file.getFullPathName());
    return false;
  }

  projects_.clear();

  for (const auto &item : *parsed.getArray()) {
    RecentProjectEntry entry = RecentProjectEntry::fromJson(item);
    if (!entry.path.getFullPathName().isEmpty()) {
      projects_.push_back(entry);
    }
  }

  // Sort by time (most recent first)
  sortByTime();

  // Prune invalid entries
  pruneInvalidEntries();

  // Enforce max size
  enforceMaxSize();

  isDirty_ = false;
  DBG("RecentProjectManager: Loaded " + juce::String(projects_.size()) +
      " projects from " + file.getFullPathName());

  return true;
}

juce::File RecentProjectManager::getStorageFile() const {
  // Store in user app data directory
  // Windows: C:\Users\<user>\AppData\Roaming\ZenithDAW\recent_projects.json
  // macOS: ~/Library/Application Support/ZenithDAW/recent_projects.json
  // Linux: ~/.config/ZenithDAW/recent_projects.json

  juce::File appDataDir =
      juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
          .getChildFile("ZenithDAW");

  return appDataDir.getChildFile("recent_projects.json");
}

void RecentProjectManager::addListener(Listener *listener) {
  listeners_.add(listener);
}

void RecentProjectManager::removeListener(Listener *listener) {
  listeners_.remove(listener);
}

void RecentProjectManager::pruneInvalidEntries() {
  size_t originalSize = projects_.size();

  projects_.erase(std::remove_if(projects_.begin(), projects_.end(),
                                 [](const RecentProjectEntry &entry) {
                                   return !entry.isValid();
                                 }),
                  projects_.end());

  if (projects_.size() < originalSize) {
    isDirty_ = true;
    DBG("RecentProjectManager: Pruned " +
        juce::String(originalSize - projects_.size()) + " invalid entries");
  }
}

void RecentProjectManager::enforceMaxSize() {
  if (static_cast<int>(projects_.size()) > maxProjects_) {
    projects_.resize(static_cast<size_t>(maxProjects_));
    isDirty_ = true;
  }
}

void RecentProjectManager::notifyListeners() {
  listeners_.call(&Listener::recentProjectsChanged);
}

void RecentProjectManager::sortByTime() {
  std::sort(projects_.begin(), projects_.end(),
            [](const RecentProjectEntry &a, const RecentProjectEntry &b) {
              return a.lastOpened > b.lastOpened; // Descending order
            });
}

} // namespace zenith
