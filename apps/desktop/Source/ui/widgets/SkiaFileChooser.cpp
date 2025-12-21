/*
  ==============================================================================

    SkiaFileChooser.cpp
    Created: 2025-12-07
    Updated: 2025-12-19
    Author:  Zenith DAW Team

    Premium Skia-based file chooser implementation with Neon Noir design.

  ==============================================================================
*/

#include "SkiaFileChooser.h"
#include "../framework/GlassmorphicPanel.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../design-system/ZenithIcons.h"
#include <core/SkTextBlob.h>
#include <effects/SkGradientShader.h>

namespace zenith {

// Layout constants
namespace {
constexpr float SIDEBAR_WIDTH = 200.0f;
constexpr float HEADER_HEIGHT = 50.0f;
constexpr float BREADCRUMB_HEIGHT = 40.0f;
constexpr float SEARCH_HEIGHT = 40.0f;
constexpr float FOOTER_HEIGHT = 60.0f;
constexpr float ROW_HEIGHT = 36.0f;
constexpr float ICON_SIZE = 20.0f;
constexpr float PADDING = design::spacing::MD;
constexpr float SMALL_PADDING = design::spacing::SM;
} // namespace

SkiaFileChooser::SkiaFileChooser(const juce::String &dialogTitle,
                                 const juce::File &initialFileOrDirectory,
                                 const juce::String &filePatternsAllowed,
                                 Mode mode)
    : dialogTitle_(dialogTitle), filePatterns_(filePatternsAllowed),
      mode_(mode) {

  // Set initial directory
  if (initialFileOrDirectory.isDirectory()) {
    currentDirectory_ = initialFileOrDirectory;
  } else if (initialFileOrDirectory.exists()) {
    currentDirectory_ = initialFileOrDirectory.getParentDirectory();
    if (mode == Mode::SaveFile) {
      defaultFilename_ = initialFileOrDirectory.getFileName();
    }
  } else {
    currentDirectory_ = juce::File::getSpecialLocation(
        juce::File::userDocumentsDirectory);
  }

  // Initialize colors
  backgroundColour_ = design::colors::BG_DARKER;
  sidebarColour_ = design::colors::BG_DARK;
  textColour_ = design::colors::TEXT_PRIMARY;

  // Initialize fonts
  font_ = design::typography::getSkFont(design::typography::FONT_MD);
  headerFont_ = design::typography::getSkFont(design::typography::FONT_LG, 
                                               design::FontWeight::SemiBold);
  smallFont_ = design::typography::getSkFont(design::typography::FONT_SM);

  // Create UI components
  titleLabel_ = std::make_unique<SkiaLabel>();
  titleLabel_->setText(dialogTitle, juce::dontSendNotification);
  titleLabel_->setFont(headerFont_);
  titleLabel_->setJustification(SkiaLabel::Justification::Left);
  titleLabel_->setTextColour(design::colors::TEXT_PRIMARY);
  addAndMakeVisible(titleLabel_.get());

  // Search editor
  searchEditor_ = std::make_unique<SkiaTextEditor>();
  searchEditor_->setMultiLine(false);
  searchEditor_->setTextToShowWhenEmpty(
      "Search files...",
      design::withAlpha(design::colors::TEXT_SECONDARY, 0.6f));
  searchEditor_->onTextChange = [this]() { handleSearchChanged(); };
  addAndMakeVisible(searchEditor_.get());

  // File list
  fileList_ = std::make_unique<SkiaListBox>();
  listModel_ = std::make_unique<FileSystemModel>(this);
  fileList_->setModel(listModel_.get());
  fileList_->setRowHeight(static_cast<int>(ROW_HEIGHT));
  fileList_->onRowClicked = [this](int row) { handleFileSelected(row); };
  fileList_->onRowDoubleClicked = [this](int row) { handleFileDoubleClick(row); };
  addAndMakeVisible(fileList_.get());

  // File name editor (for save mode)
  fileNameEditor_ = std::make_unique<SkiaTextEditor>();
  fileNameEditor_->setMultiLine(false);
  fileNameEditor_->setTextToShowWhenEmpty(
      "Enter filename...",
      design::withAlpha(design::colors::TEXT_SECONDARY, 0.6f));
  if (mode_ == Mode::SaveFile) {
    addAndMakeVisible(fileNameEditor_.get());
  }

  // OK button
  okButton_ = std::make_unique<SkiaButton>();
  okButton_->setText(mode == Mode::SaveFile ? "Save" : "Open");
  okButton_->setStyle(SkiaButton::Style::Primary);
  okButton_->onClick = [this]() { handleOkPressed(); };
  addAndMakeVisible(okButton_.get());

  // Cancel button
  cancelButton_ = std::make_unique<SkiaButton>();
  cancelButton_->setText("Cancel");
  cancelButton_->setStyle(SkiaButton::Style::Secondary);
  cancelButton_->onClick = [this]() { handleCancelPressed(); };
  addAndMakeVisible(cancelButton_.get());

  // Up button
  upButton_ = std::make_unique<SkiaButton>();
  upButton_->setText("↑");
  upButton_->setStyle(SkiaButton::Style::Ghost);
  upButton_->onClick = [this]() { navigateUp(); };
  addAndMakeVisible(upButton_.get());

  // New folder button
  newFolderButton_ = std::make_unique<SkiaButton>();
  newFolderButton_->setText("+ Folder");
  newFolderButton_->setStyle(SkiaButton::Style::Ghost);
  newFolderButton_->onClick = [this]() { createNewFolder(); };
  if (mode_ == Mode::SaveFile) {
    addAndMakeVisible(newFolderButton_.get());
  }

  // Initialize quick access sidebar
  initializeQuickAccess();

  // Refresh directory and breadcrumbs
  updateBreadcrumbs();
  refreshDirectory();

  // Start animation timer
  startTimerHz(60);
}

SkiaFileChooser::~SkiaFileChooser() {
  stopTimer();
}

void SkiaFileChooser::showAsync(Callback callback) {
  callback_ = callback;
  fadeAlpha_.setTarget(1.0f, 200);  // Animate to fully visible over 200ms
  setVisible(true);
}

void SkiaFileChooser::setFilePatterns(const juce::String &patterns) {
  filePatterns_ = patterns;
  refreshDirectory();
}

juce::File SkiaFileChooser::getSelectedFile() const {
  if (mode_ == Mode::SaveFile && fileNameEditor_) {
    juce::String fileName = fileNameEditor_->getText().trim();
    if (fileName.isNotEmpty()) {
      return currentDirectory_.getChildFile(fileName);
    }
  }

  if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(fileEntries_.size())) {
    return fileEntries_[selectedIndex_].file;
  }

  return juce::File();
}

void SkiaFileChooser::setDefaultFilename(const juce::String &filename) {
  defaultFilename_ = filename;
  if (fileNameEditor_) {
    fileNameEditor_->setText(filename);
  }
}

void SkiaFileChooser::initializeQuickAccess() {
  quickAccessItems_.clear();

  // Desktop
  quickAccessItems_.push_back({
      "Desktop", icons::Desktop(),
      juce::File::getSpecialLocation(juce::File::userDesktopDirectory),
      SkRect::MakeEmpty(), false});

  // Documents
  quickAccessItems_.push_back({
      "Documents", icons::File(),
      juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
      SkRect::MakeEmpty(), false});

  // Music
  quickAccessItems_.push_back({
      "Music", icons::MusicNote(),
      juce::File::getSpecialLocation(juce::File::userMusicDirectory),
      SkRect::MakeEmpty(), false});

  // Home
  quickAccessItems_.push_back({
      "Home", icons::Home(),
      juce::File::getSpecialLocation(juce::File::userHomeDirectory),
      SkRect::MakeEmpty(), false});

  // Drives (Windows specific)
#if JUCE_WINDOWS
  for (int i = 0; i < 26; ++i) {
    juce::String driveLetter = juce::String::charToString('A' + i) + ":";
    juce::File drive(driveLetter + "\\");
    if (drive.isDirectory()) {
      quickAccessItems_.push_back({
          driveLetter, icons::HardDrive(), drive, SkRect::MakeEmpty(), false});
    }
  }
#endif
}

void SkiaFileChooser::updateBreadcrumbs() {
  breadcrumbs_.clear();

  juce::File path = currentDirectory_;
  std::vector<BreadcrumbSegment> segments;

  while (path.exists() && path.getFullPathName().isNotEmpty()) {
    juce::String name = path.getFileName();
    if (name.isEmpty()) {
      name = path.getFullPathName(); // Root like "C:\"
    }

    segments.push_back({name, path, SkRect::MakeEmpty(), false});

    juce::File parent = path.getParentDirectory();
    if (parent == path) break; // Reached root
    path = parent;
  }

  // Reverse to get root first
  for (auto it = segments.rbegin(); it != segments.rend(); ++it) {
    breadcrumbs_.push_back(*it);
  }
}

void SkiaFileChooser::refreshDirectory() {
  if (!currentDirectory_.isDirectory()) {
    return;
  }

  fileEntries_.clear();
  selectedIndex_ = -1;

  // Get directories
  juce::Array<juce::File> directories;
  currentDirectory_.findChildFiles(directories, juce::File::findDirectories, false);
  directories.sort();

  for (const auto &dir : directories) {
    FileEntry entry;
    entry.file = dir;
    entry.name = dir.getFileName();
    entry.isDirectory = true;
    entry.dateString = formatDate(dir.getLastModificationTime());
    entry.sizeString = ""; // Don't show size for directories
    fileEntries_.push_back(entry);
  }

  // Get files matching patterns
  juce::Array<juce::File> files;
  if (filePatterns_.isEmpty() || mode_ == Mode::OpenDirectory) {
    // Only if not in directory mode, show all files
    if (mode_ != Mode::OpenDirectory) {
      currentDirectory_.findChildFiles(files, juce::File::findFiles, false);
    }
  } else {
    juce::StringArray patterns;
    patterns.addTokens(filePatterns_, ";,", "\"'");
    patterns.trim();
    patterns.removeEmptyStrings();

    for (const auto &pattern : patterns) {
      juce::Array<juce::File> matchedFiles;
      currentDirectory_.findChildFiles(matchedFiles, juce::File::findFiles,
                                       false, pattern);
      files.addArray(matchedFiles);
    }
  }

  files.sort();

  for (const auto &file : files) {
    FileEntry entry;
    entry.file = file;
    entry.name = file.getFileName();
    entry.isDirectory = false;
    entry.sizeString = formatFileSize(file.getSize());
    entry.dateString = formatDate(file.getLastModificationTime());
    fileEntries_.push_back(entry);
  }

  // Apply search filter if active
  applySearchFilter();

  // Update file list
  if (fileList_) {
    fileList_->updateContent();
  }

  // Update breadcrumbs
  updateBreadcrumbs();

  markDirty();
}

void SkiaFileChooser::applySearchFilter() {
  if (searchFilter_.isEmpty()) {
    return;
  }

  juce::String filterLower = searchFilter_.toLowerCase();
  
  fileEntries_.erase(
      std::remove_if(fileEntries_.begin(), fileEntries_.end(),
                     [&filterLower](const FileEntry &entry) {
                       return !entry.name.toLowerCase().contains(filterLower);
                     }),
      fileEntries_.end());
}

void SkiaFileChooser::navigateUp() {
  juce::File parent = currentDirectory_.getParentDirectory();
  if (parent.exists() && parent != currentDirectory_) {
    navigateToDirectory(parent);
  }
}

void SkiaFileChooser::navigateToDirectory(const juce::File &directory) {
  if (directory.isDirectory() && directory != currentDirectory_) {
    currentDirectory_ = directory;
    refreshDirectory();
    markDirty();
  }
}

void SkiaFileChooser::handleFileSelected(int index) {
  if (index < 0 || index >= static_cast<int>(fileEntries_.size())) {
    return;
  }

  // Update selection
  for (size_t i = 0; i < fileEntries_.size(); ++i) {
    fileEntries_[i].isSelected = (static_cast<int>(i) == index);
  }
  selectedIndex_ = index;

  const auto &entry = fileEntries_[index];

  // Update filename editor for non-directory selections
  if (!entry.isDirectory && mode_ == Mode::SaveFile && fileNameEditor_) {
    fileNameEditor_->setText(entry.name);
  }

  markDirty();
}

void SkiaFileChooser::handleFileDoubleClick(int index) {
  if (index < 0 || index >= static_cast<int>(fileEntries_.size())) {
    return;
  }

  const auto &entry = fileEntries_[index];

  if (entry.isDirectory) {
    navigateToDirectory(entry.file);
  } else {
    // Double-click on file = select and confirm
    handleFileSelected(index);
    handleOkPressed();
  }
}

void SkiaFileChooser::handleQuickAccessClick(int index) {
  if (index < 0 || index >= static_cast<int>(quickAccessItems_.size())) {
    return;
  }

  navigateToDirectory(quickAccessItems_[index].path);
}

void SkiaFileChooser::handleBreadcrumbClick(int index) {
  if (index < 0 || index >= static_cast<int>(breadcrumbs_.size())) {
    return;
  }

  navigateToDirectory(breadcrumbs_[index].path);
}

void SkiaFileChooser::handleOkPressed() {
  juce::File selectedFile = getSelectedFile();

  if (mode_ == Mode::SaveFile) {
    // For save mode, ensure we have a filename
    juce::String fileName = fileNameEditor_ ? fileNameEditor_->getText().trim() : "";
    if (fileName.isEmpty()) {
      // TODO: Visual feedback for empty filename
      return;
    }

    // Enforce extension if patterns are simple (e.g. *.zth)
    // If multiple patterns or complex, we might skip, but for now specific to project
    // we assume the first extension in the pattern list is default.
    // patterns: "*.zth;*.wav" -> use .zth
    juce::StringArray patterns;
    patterns.addTokens(filePatterns_, ";,", "\"'");
    if (patterns.size() > 0) {
      juce::String defaultExt = patterns[0].replace("*", "");
      if (!fileName.endsWithIgnoreCase(defaultExt)) {
        // Check if it ends with any valid extension from list
        bool hasValidExt = false;
        for (auto& pat : patterns) {
           if (fileName.endsWithIgnoreCase(pat.replace("*", ""))) {
             hasValidExt = true;
             break;
           }
        }
        if (!hasValidExt) {
          fileName += defaultExt;
          selectedFile = currentDirectory_.getChildFile(fileName);
        }
      }
    } else {
        selectedFile = currentDirectory_.getChildFile(fileName);
    }

    // Overwrite Warning
    if (selectedFile.existsAsFile()) {
        // Use native alert for data safety (fastest robust solution)
        // In fully polished app, this would be a custom glassmorphic overlay.
        int result = juce::NativeMessageBox::showYesNoBox(
            juce::AlertWindow::WarningIcon,
            "Confirm Overwrite",
            "The file '" + fileName + "' already exists.\nDo you want to replace it?",
            this->getTopLevelComponent(), nullptr);
        
        if (result == 0) { // No
            return;
        }
        // Yes -> Proceed
    }
  } else if (mode_ == Mode::OpenDirectory) {
    selectedFile = currentDirectory_;
  } else {
    // For open mode, ensure a file is selected
    if (!selectedFile.exists()) {
      return;
    }
  }

  if (callback_) {
    callback_(Result::Approved, selectedFile);
  }
  fadeAlpha_.setTarget(0.0f, 200);  // Animate to invisible over 200ms
}

void SkiaFileChooser::handleCancelPressed() {
  if (callback_) {
    callback_(Result::Cancelled, juce::File());
  }
  fadeAlpha_.setTarget(0.0f, 200);  // Animate to invisible over 200ms
}

void SkiaFileChooser::handleSearchChanged() {
  searchFilter_ = searchEditor_ ? searchEditor_->getText() : "";
  refreshDirectory();
}

void SkiaFileChooser::updateFileNameEditor() {
  if (!fileNameEditor_ || mode_ != Mode::SaveFile) {
    return;
  }

  if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(fileEntries_.size())) {
    if (!fileEntries_[selectedIndex_].isDirectory) {
      fileNameEditor_->setText(fileEntries_[selectedIndex_].name);
    }
  }
}

void SkiaFileChooser::createNewFolder() {
  // Simple implementation - create "New Folder" and navigate to it
  juce::File newFolder = currentDirectory_.getChildFile("New Folder");
  int suffix = 1;
  while (newFolder.exists()) {
    newFolder = currentDirectory_.getChildFile("New Folder " + juce::String(suffix++));
  }

  if (newFolder.createDirectory()) {
    refreshDirectory();
  }
}

void SkiaFileChooser::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();
  float width = bounds.getWidth();
  float height = bounds.getHeight();

  // Draw glassmorphic background
  GlassmorphicPanel::Options opts;
  opts.style = GlassmorphicPanel::Style::Floating;
  opts.cornerRadius = design::dimensions::RADIUS_XL;
  opts.drawShadow = true;
  GlassmorphicPanel::drawWithOptions(
      canvas, SkRect::MakeWH(width, height), opts);

  // Draw sidebar
  drawSidebar(canvas);

  // Draw main content area
  float contentLeft = SIDEBAR_WIDTH + PADDING;
  float contentWidth = width - contentLeft - PADDING;

  // Draw breadcrumbs
  drawBreadcrumbs(canvas);

  // File list background (subtle panel)
  GlassmorphicPanel::draw(
      canvas,
      SkRect::MakeLTRB(contentLeft, HEADER_HEIGHT + BREADCRUMB_HEIGHT + SEARCH_HEIGHT + PADDING,
                       width - PADDING, height - FOOTER_HEIGHT - PADDING),
      GlassmorphicPanel::Style::Subtle);

  // Draw footer
  drawFooter(canvas);
}

void SkiaFileChooser::drawSidebar(SkCanvas *canvas) {
  using namespace design;

  // Sidebar background
  SkRect sidebarRect = SkRect::MakeLTRB(0, 0, SIDEBAR_WIDTH, getHeight());

  SkPaint sidebarPaint;
  sidebarPaint.setAntiAlias(true);
  sidebarPaint.setColor(withAlpha(colors::BG_DARKER, 0.6f));
  canvas->drawRoundRect(sidebarRect, dimensions::RADIUS_LG, dimensions::RADIUS_LG, sidebarPaint);

  // Quick Access header
  SkPaint headerPaint;
  headerPaint.setAntiAlias(true);
  headerPaint.setColor(colors::TEXT_SECONDARY);

  SkFont headerFont = typography::getSkFont(typography::FONT_XS, FontWeight::Medium);
  canvas->drawString("QUICK ACCESS", PADDING, HEADER_HEIGHT + 20, headerFont, headerPaint);

  // Draw quick access items
  float y = HEADER_HEIGHT + 40;
  for (size_t i = 0; i < quickAccessItems_.size(); ++i) {
    auto &item = quickAccessItems_[i];
    item.bounds = SkRect::MakeLTRB(SMALL_PADDING, y, SIDEBAR_WIDTH - SMALL_PADDING, y + ROW_HEIGHT);

    // Hover background
    if (item.isHovered) {
      SkPaint hoverPaint;
      hoverPaint.setAntiAlias(true);
      hoverPaint.setColor(colors::GLASS_HOVER);
      canvas->drawRoundRect(item.bounds, dimensions::RADIUS_SM, dimensions::RADIUS_SM, hoverPaint);
    }

    // Icon
    icons::IconStyle style;
    style.color = item.isHovered ? colors::CYAN : colors::TEXT_SECONDARY;
    style.strokeWidth = icons::STROKE_REGULAR;
    style.filled = false;
    
    // Draw icon (centered vertically, 18px)
    icons::drawIcon(canvas, item.icon, PADDING + 4, y + 9.0f, 18.0f, style);

    // Name
    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(item.isHovered ? colors::TEXT_PRIMARY : colors::TEXT_SECONDARY);

    canvas->drawString(item.name.toRawUTF8(), PADDING + 32, y + ROW_HEIGHT / 2 + 5, font_, textPaint);

    y += ROW_HEIGHT;
  }
}

void SkiaFileChooser::drawBreadcrumbs(SkCanvas *canvas) {
  using namespace design;

  float x = SIDEBAR_WIDTH + PADDING;
  float y = HEADER_HEIGHT + SMALL_PADDING;
  float height = BREADCRUMB_HEIGHT - SMALL_PADDING * 2;

  SkPaint textPaint;
  textPaint.setAntiAlias(true);

  SkPaint separatorPaint;
  separatorPaint.setAntiAlias(true);
  separatorPaint.setColor(colors::TEXT_TERTIARY);

  for (size_t i = 0; i < breadcrumbs_.size(); ++i) {
    auto &crumb = breadcrumbs_[i];

    // Measure text width
    SkRect textBounds;
    font_.measureText(crumb.name.toRawUTF8(), crumb.name.getNumBytesAsUTF8(),
                      SkTextEncoding::kUTF8, &textBounds);
    float textWidth = textBounds.width();

    crumb.bounds = SkRect::MakeLTRB(x, y, x + textWidth + SMALL_PADDING * 2, y + height);

    // Hover background
    if (crumb.isHovered) {
      SkPaint hoverPaint;
      hoverPaint.setAntiAlias(true);
      hoverPaint.setColor(colors::GLASS_HOVER);
      canvas->drawRoundRect(crumb.bounds, dimensions::RADIUS_SM, dimensions::RADIUS_SM, hoverPaint);
    }

    // Text
    textPaint.setColor(crumb.isHovered ? colors::CYAN : colors::TEXT_SECONDARY);
    canvas->drawString(crumb.name.toRawUTF8(), x + SMALL_PADDING, y + height / 2 + 5, font_, textPaint);

    x += textWidth + SMALL_PADDING * 2;

    // Separator (except for last)
    if (i < breadcrumbs_.size() - 1) {
      icons::IconStyle sepStyle;
      sepStyle.color = colors::TEXT_TERTIARY;
      sepStyle.strokeWidth = 1.5f;
      
      icons::drawIcon(canvas, icons::ChevronRight(), x, y + height/2 - 6, 12.0f, sepStyle);
      x += 16;
    }
  }
}

void SkiaFileChooser::drawFooter(SkCanvas *canvas) {
  // Footer is handled by JUCE components (buttons, text editor)
  // Just draw a subtle divider line
  float y = getHeight() - FOOTER_HEIGHT;
  GlassmorphicPanel::drawDivider(canvas, SIDEBAR_WIDTH + PADDING, y, getWidth() - PADDING);
}

void SkiaFileChooser::resized() {
  auto bounds = getLocalBounds();
  float width = static_cast<float>(bounds.getWidth());
  float height = static_cast<float>(bounds.getHeight());

  float contentLeft = SIDEBAR_WIDTH + PADDING;
  float contentWidth = width - contentLeft - PADDING;

  // Title
  titleLabel_->setBounds(static_cast<int>(contentLeft), 0,
                         static_cast<int>(contentWidth - 50), static_cast<int>(HEADER_HEIGHT));

  // Up button (top right of content area)
  upButton_->setBounds(static_cast<int>(width - PADDING - 40),
                       static_cast<int>(PADDING),
                       40, 32);

  // Search editor
  searchEditor_->setBounds(static_cast<int>(contentLeft),
                           static_cast<int>(HEADER_HEIGHT + BREADCRUMB_HEIGHT),
                           static_cast<int>(contentWidth),
                           static_cast<int>(SEARCH_HEIGHT - SMALL_PADDING));

  // File list
  fileList_->setBounds(static_cast<int>(contentLeft),
                       static_cast<int>(HEADER_HEIGHT + BREADCRUMB_HEIGHT + SEARCH_HEIGHT + PADDING),
                       static_cast<int>(contentWidth),
                       static_cast<int>(height - HEADER_HEIGHT - BREADCRUMB_HEIGHT - SEARCH_HEIGHT - FOOTER_HEIGHT - PADDING * 2));

  // Footer area
  float footerY = height - FOOTER_HEIGHT + PADDING;

  // For save mode, show filename editor
  if (mode_ == Mode::SaveFile && fileNameEditor_) {
    fileNameEditor_->setBounds(static_cast<int>(contentLeft),
                               static_cast<int>(footerY),
                               static_cast<int>(contentWidth - 230),
                               32);

    newFolderButton_->setBounds(static_cast<int>(width - PADDING - 220),
                                static_cast<int>(footerY), 80, 32);
  }

  // Buttons at bottom right
  cancelButton_->setBounds(static_cast<int>(width - PADDING - 110),
                           static_cast<int>(footerY), 100, 32);
  okButton_->setBounds(static_cast<int>(width - PADDING - 220),
                       static_cast<int>(footerY), 100, 32);

  // Adjust for save mode
  if (mode_ == Mode::SaveFile) {
    cancelButton_->setBounds(static_cast<int>(width - PADDING - 110),
                             static_cast<int>(footerY), 100, 32);
    okButton_->setBounds(static_cast<int>(width - PADDING - 220),
                         static_cast<int>(footerY), 100, 32);
  }
}

void SkiaFileChooser::mouseMove(const juce::MouseEvent &e) {
  SkiaComponent::mouseMove(e);

  auto pos = e.getPosition();
  bool needsRepaint = false;

  // Check quick access hover
  for (auto &item : quickAccessItems_) {
    bool wasHovered = item.isHovered;
    item.isHovered = item.bounds.contains(static_cast<float>(pos.x),
                                          static_cast<float>(pos.y));
    if (wasHovered != item.isHovered) needsRepaint = true;
  }

  // Check breadcrumb hover
  for (auto &crumb : breadcrumbs_) {
    bool wasHovered = crumb.isHovered;
    crumb.isHovered = crumb.bounds.contains(static_cast<float>(pos.x),
                                            static_cast<float>(pos.y));
    if (wasHovered != crumb.isHovered) needsRepaint = true;
  }

  if (needsRepaint) {
    markDirty();
  }
}

void SkiaFileChooser::mouseDown(const juce::MouseEvent &e) {
  SkiaComponent::mouseDown(e);

  auto pos = e.getPosition();

  // Check quick access click
  for (size_t i = 0; i < quickAccessItems_.size(); ++i) {
    if (quickAccessItems_[i].bounds.contains(static_cast<float>(pos.x),
                                             static_cast<float>(pos.y))) {
      handleQuickAccessClick(static_cast<int>(i));
      return;
    }
  }

  // Check breadcrumb click
  for (size_t i = 0; i < breadcrumbs_.size(); ++i) {
    if (breadcrumbs_[i].bounds.contains(static_cast<float>(pos.x),
                                        static_cast<float>(pos.y))) {
      handleBreadcrumbClick(static_cast<int>(i));
      return;
    }
  }
}

void SkiaFileChooser::mouseUp(const juce::MouseEvent &e) {
  SkiaComponent::mouseUp(e);
}

bool SkiaFileChooser::keyPressed(const juce::KeyPress &key) {
  // ESC to cancel
  if (key == juce::KeyPress::escapeKey) {
    handleCancelPressed();
    return true;
  }

  // Enter to confirm
  if (key == juce::KeyPress::returnKey) {
    handleOkPressed();
    return true;
  }

  // Arrow keys for navigation
  if (key == juce::KeyPress::upKey) {
    if (selectedIndex_ > 0) {
      handleFileSelected(selectedIndex_ - 1);
      fileList_->scrollToEnsureRowIsOnscreen(selectedIndex_);
    }
    return true;
  }

  if (key == juce::KeyPress::downKey) {
    if (selectedIndex_ < static_cast<int>(fileEntries_.size()) - 1) {
      handleFileSelected(selectedIndex_ + 1);
      fileList_->scrollToEnsureRowIsOnscreen(selectedIndex_);
    }
    return true;
  }

  // Backspace to go up
  if (key == juce::KeyPress::backspaceKey) {
    navigateUp();
    return true;
  }

  return false;
}

void SkiaFileChooser::timerCallback() {
  fadeAlpha_.update(16.67f);  // Update with approximate 60fps frame time

  // Hide when fade complete
  float currentAlpha = fadeAlpha_.getCurrentValue();
  if (currentAlpha < 0.01f && !fadeAlpha_.isAnimating()) {
    setVisible(false);
    stopTimer();
  }

  markDirty();
}

// Utility functions
juce::String SkiaFileChooser::formatFileSize(juce::int64 bytes) {
  if (bytes < 1024) {
    return juce::String(bytes) + " B";
  } else if (bytes < 1024 * 1024) {
    return juce::String(bytes / 1024) + " KB";
  } else if (bytes < 1024 * 1024 * 1024) {
    return juce::String(bytes / (1024 * 1024)) + " MB";
  } else {
    return juce::String(bytes / (1024 * 1024 * 1024)) + " GB";
  }
}

juce::String SkiaFileChooser::formatDate(juce::Time time) {
  return time.formatted("%Y-%m-%d %H:%M");
}

SkPath SkiaFileChooser::getFileTypeIcon(const juce::File &file) {
  if (file.isDirectory()) {
    return icons::Folder();
  }

  juce::String ext = file.getFileExtension().toLowerCase();

  // Audio files
  if (ext == ".wav" || ext == ".aiff" || ext == ".aif" || ext == ".mp3" ||
      ext == ".flac" || ext == ".ogg" || ext == ".m4a") {
    return icons::Audio();
  }

  // MIDI files
  if (ext == ".mid" || ext == ".midi") {
    return icons::MIDI();
  }

  // Project files
  if (ext == ".zth" || ext == ".als" || ext == ".flp" || ext == ".ptx") {
    return icons::Project();
  }

  // Image files
  if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".gif" ||
      ext == ".bmp" || ext == ".svg") {
    return icons::Image();
  }

  // Video files
  if (ext == ".mp4" || ext == ".mov" || ext == ".avi" || ext == ".mkv") {
    return icons::Video();
  }

  // Text/Document files
  if (ext == ".txt" || ext == ".md" || ext == ".json" || ext == ".xml") {
    return icons::File();
  }

  return icons::File();
}

// FileSystemModel implementation
int SkiaFileChooser::FileSystemModel::getNumRows() {
  return static_cast<int>(chooser_->fileEntries_.size());
}

void SkiaFileChooser::FileSystemModel::paintListBoxItem(int rowNumber,
                                                        SkCanvas &canvas,
                                                        int width, int height,
                                                        bool rowIsSelected) {
  using namespace design;

  if (rowNumber < 0 || rowNumber >= static_cast<int>(chooser_->fileEntries_.size())) {
    return;
  }

  const auto &entry = chooser_->fileEntries_[rowNumber];

  // Selection/hover background
  if (rowIsSelected) {
    SkPaint selectedBg;
    selectedBg.setAntiAlias(true);
    selectedBg.setColor(withAlpha(colors::CYAN, 0.3f));
    canvas.drawRoundRect(SkRect::MakeWH(static_cast<float>(width), static_cast<float>(height)),
                         dimensions::RADIUS_SM, dimensions::RADIUS_SM, selectedBg);
  }

  // Icon
  icons::IconStyle style;
  style.color = entry.isDirectory ? colors::AMBER : colors::TEXT_SECONDARY;
  if (rowIsSelected) style.color = colors::CYAN;
  style.strokeWidth = icons::STROKE_REGULAR;
  style.filled = false;
  
  // Custom glowing effect for selected items
  if (rowIsSelected) {
    style.glowRadius = 4.0f;
    style.glowColor = colors::CYAN;
  }
  
  icons::drawIcon(&canvas, getFileTypeIcon(entry.file), 
                  12.0f, static_cast<float>(height) / 2.0f - 9.0f, 
                  18.0f, style);

  // Filename
  SkPaint textPaint;
  textPaint.setAntiAlias(true);
  textPaint.setColor(rowIsSelected ? colors::TEXT_PRIMARY : 
                     (entry.isDirectory ? colors::TEXT_PRIMARY : colors::TEXT_SECONDARY));

  SkFont font = typography::getSkFont(typography::FONT_MD);
  canvas.drawString(entry.name.toRawUTF8(), 40.0f, static_cast<float>(height) / 2.0f + 5.0f,
                    font, textPaint);

  // Size (right side, only for files)
  if (!entry.isDirectory && !entry.sizeString.isEmpty()) {
    SkPaint sizePaint;
    sizePaint.setAntiAlias(true);
    sizePaint.setColor(colors::TEXT_TERTIARY);

    SkFont smallFont = typography::getSkFont(typography::FONT_SM);
    canvas.drawString(entry.sizeString.toRawUTF8(),
                      static_cast<float>(width) - 150.0f,
                      static_cast<float>(height) / 2.0f + 4.0f,
                      smallFont, sizePaint);
  }

  // Date (far right)
  if (!entry.dateString.isEmpty()) {
    SkPaint datePaint;
    datePaint.setAntiAlias(true);
    datePaint.setColor(colors::TEXT_TERTIARY);

    SkFont smallFont = typography::getSkFont(typography::FONT_XS);
    canvas.drawString(entry.dateString.toRawUTF8(),
                      static_cast<float>(width) - 80.0f,
                      static_cast<float>(height) / 2.0f + 4.0f,
                      smallFont, datePaint);
  }
}

void SkiaFileChooser::FileSystemModel::listBoxItemClicked(
    int rowNumber, const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  chooser_->handleFileSelected(rowNumber);
}

void SkiaFileChooser::FileSystemModel::listBoxItemDoubleClicked(
    int rowNumber, const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  chooser_->handleFileDoubleClick(rowNumber);
}

} // namespace zenith