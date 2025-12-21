/*
  ==============================================================================

    SkiaFileChooser.h
    Created: 2025-12-07
    Updated: 2025-12-19
    Author:  Zenith DAW Team

    Premium Skia-based file chooser component with Neon Noir design.
    Features:
    - Glassmorphic panel design
    - Breadcrumb navigation
    - Quick access sidebar
    - File type icons
    - Search/filter
    - Keyboard navigation

  ==============================================================================
*/

#pragma once

#include "SkiaButton.h"
#include "SkiaComponent.h"
#include "SkiaLabel.h"
#include "SkiaListBox.h"
#include "SkiaTextEditor.h"
#include "../framework/GlassmorphicPanel.h"
#include "../design-system/ZenithDesignSystem.h"
#include <juce_core/juce_core.h>
#include <functional>
#include <vector>

namespace zenith {

/**
 * @brief Premium file chooser component with Zenith Neon Noir design
 *
 * A fully-featured file browser that replaces the basic Windows file dialog
 * with a beautiful, consistent in-app experience.
 */
class SkiaFileChooser : public SkiaComponent {
public:
  enum class Mode { 
    OpenFile,     // Select existing file
    SaveFile,     // Save to new/existing file
    OpenDirectory // Select directory
  };

  enum class Result { 
    Cancelled, 
    Approved 
  };

  using Callback = std::function<void(Result result, const juce::File &file)>;

  /**
   * @brief Construct a file chooser
   * @param dialogTitle Title shown at top of dialog
   * @param initialFileOrDirectory Starting location
   * @param filePatternsAllowed File patterns like "*.wav;*.mp3"
   * @param mode Open, Save, or Directory mode
   */
  SkiaFileChooser(const juce::String &dialogTitle,
                  const juce::File &initialFileOrDirectory,
                  const juce::String &filePatternsAllowed,
                  Mode mode = Mode::OpenFile);
  ~SkiaFileChooser() override;

  // Show the dialog (async)
  void showAsync(Callback callback);

  // Get current directory
  juce::File getCurrentDirectory() const { return currentDirectory_; }

  // Set file patterns (e.g., "*.wav;*.mp3;*.flac")
  void setFilePatterns(const juce::String &patterns);

  // Get selected file
  juce::File getSelectedFile() const;

  // Set default filename for save mode
  void setDefaultFilename(const juce::String &filename);

private:
  // Quick access locations
  struct QuickAccessItem {
    juce::String name;
    SkPath icon;        // SVG icon path
    juce::File path;
    SkRect bounds;
    bool isHovered = false;
  };

  // Breadcrumb segment
  struct BreadcrumbSegment {
    juce::String name;
    juce::File path;
    SkRect bounds;
    bool isHovered = false;
  };

  // File entry with metadata
  struct FileEntry {
    juce::File file;
    juce::String name;
    juce::String sizeString;
    juce::String dateString;
    bool isDirectory = false;
    SkRect bounds;
    bool isHovered = false;
    bool isSelected = false;
  };

  // File system model for list box
  class FileSystemModel : public SkiaListBox::Model {
  public:
    FileSystemModel(SkiaFileChooser *chooser) : chooser_(chooser) {}

    int getNumRows() override;
    void paintListBoxItem(int rowNumber, SkCanvas &canvas, int width,
                          int height, bool rowIsSelected) override;
    void listBoxItemClicked(int rowNumber, const juce::MouseEvent &e) override;
    void listBoxItemDoubleClicked(int rowNumber,
                                  const juce::MouseEvent &e) override;

  private:
    SkiaFileChooser *chooser_;
  };

  // Dialog components
  std::unique_ptr<SkiaLabel> titleLabel_;
  std::unique_ptr<SkiaListBox> fileList_;
  std::unique_ptr<SkiaTextEditor> fileNameEditor_;
  std::unique_ptr<SkiaTextEditor> searchEditor_;
  std::unique_ptr<SkiaButton> okButton_;
  std::unique_ptr<SkiaButton> cancelButton_;
  std::unique_ptr<SkiaButton> upButton_;
  std::unique_ptr<SkiaButton> newFolderButton_;

  // List model
  std::unique_ptr<FileSystemModel> listModel_;

  // Quick access sidebar items
  std::vector<QuickAccessItem> quickAccessItems_;

  // Breadcrumb path segments
  std::vector<BreadcrumbSegment> breadcrumbs_;

  // Current state
  juce::File currentDirectory_;
  juce::String filePatterns_;
  juce::String searchFilter_;
  Mode mode_;
  juce::String dialogTitle_;
  juce::String defaultFilename_;

  // File entries (combined dirs + files)
  std::vector<FileEntry> fileEntries_;
  int selectedIndex_ = -1;

  Callback callback_;

  // Layout bounds
  SkRect sidebarBounds_;
  SkRect breadcrumbBounds_;
  SkRect searchBounds_;
  SkRect fileListBounds_;
  SkRect footerBounds_;

  // Appearance
  SkColor backgroundColour_;
  SkColor sidebarColour_;
  SkColor textColour_;
  SkFont font_;
  SkFont headerFont_;
  SkFont smallFont_;

  // Animation
  AnimatedValue fadeAlpha_{0.0f};  // Use new single-arg constructor
  float hoverTransition_ = 0.0f;

  // Internal methods
  void initializeQuickAccess();
  void updateBreadcrumbs();
  void refreshDirectory();
  void applySearchFilter();
  void navigateUp();
  void navigateToDirectory(const juce::File &directory);
  void handleFileSelected(int index);
  void handleFileDoubleClick(int index);
  void handleQuickAccessClick(int index);
  void handleBreadcrumbClick(int index);
  void handleOkPressed();
  void handleCancelPressed();
  void handleSearchChanged();
  void updateFileNameEditor();
  void createNewFolder();

  // Drawing methods
  void drawSkia(SkCanvas *canvas) override;
  void drawSidebar(SkCanvas *canvas);
  void drawBreadcrumbs(SkCanvas *canvas);
  void drawFooter(SkCanvas *canvas);

  // Event handling (override methods from base class)
  void resized() override;
  void mouseMove(const juce::MouseEvent &e) override;
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseUp(const juce::MouseEvent &e) override;
  void timerCallback() override;

public:
  // Public keyboard handling so SkiaFileChooserDialog can delegate key events
  bool keyPressed(const juce::KeyPress &key) override;

private:

  // Utility
  static juce::String formatFileSize(juce::int64 bytes);
  static juce::String formatDate(juce::Time time);
  static SkPath getFileTypeIcon(const juce::File &file);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaFileChooser)
};

} // namespace zenith