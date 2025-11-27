/**
 * @file FileMenuComponent.cpp
 * @brief File menu implementation with smooth animations and modern design
 */

#include "FileMenuComponent.h"
#include "../../include/Engine.h"
#include "../../include/ProjectState.h"
#include "ZenithLookAndFeel.h"
#include <juce_data_structures/juce_data_structures.h>

namespace zenith {

// Forward declarations for helpers
static std::unique_ptr<juce::PropertiesFile> createPropertiesFile();
static void addToRecentFilesList(juce::StringArray &recentFiles,
                                 const juce::File &file);

// ... (lines 20-304)

// Helper to get properties file
static std::unique_ptr<juce::PropertiesFile> createPropertiesFile() {
  juce::PropertiesFile::Options options;
  options.applicationName = "ZenithDAW";
  options.filenameSuffix = ".settings";
  options.folderName = "ZenithDAW";
  options.osxLibrarySubFolder = "Application Support";
  options.commonToAllUsers = false;
  options.ignoreCaseOfKeyNames = true;
  options.storageFormat = juce::PropertiesFile::storeAsXML;

  return std::make_unique<juce::PropertiesFile>(options);
}

void FileMenuComponent::refreshRecentFiles() {
  auto props = createPropertiesFile();
  recentFiles_ =
      juce::StringArray::fromTokens(props->getValue("recentFiles"), "|", "");

  // Ensure valid files
  for (int i = recentFiles_.size(); --i >= 0;) {
    if (!juce::File(recentFiles_[i]).exists())
      recentFiles_.remove(i);
  }
}

// ... (lines 330-355)

// Helper to add to recent files
static void addToRecentFilesList(juce::StringArray &recentFiles,
                                 const juce::File &file) {
  recentFiles.removeString(file.getFullPathName());
  recentFiles.insert(0, file.getFullPathName());
  while (recentFiles.size() > 10)
    recentFiles.remove(recentFiles.size() - 1);

  auto props = createPropertiesFile();
  props->setValue("recentFiles", recentFiles.joinIntoString("|"));
}

} // namespace zenith
