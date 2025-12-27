/*
  ==============================================================================

    SkiaFileChooser.cpp
    Created: 2025-12-07
    Author:  AI Assistant

    Pure Skia-based file chooser implementation

  ==============================================================================
*/

#include "SkiaFileChooser.h"
#include "ZenithDesignSystem.h"

namespace zenith {

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
  } else {
    currentDirectory_ = juce::File::getCurrentWorkingDirectory();
  }

  // Create UI components
  titleLabel_ = std::make_unique<SkiaLabel>();
  titleLabel_->setText(dialogTitle, juce::dontSendNotification);
  titleLabel_->setFont(20.0f);
  titleLabel_->setJustification(SkiaLabel::Justification::Left);
  addAndMakeVisible(titleLabel_.get());

  fileList_ = std::make_unique<SkiaListBox>();
  fileList_->setModel(new FileSystemModel(this));
  fileList_->setRowHeight(28);
  addAndMakeVisible(fileList_.get());

  fileNameEditor_ = std::make_unique<SkiaTextEditor>();
  fileNameEditor_->setMultiLine(false);
  fileNameEditor_->setTextToShowWhenEmpty(
      "Enter file name...",
      design::withAlpha(design::colors::TEXT_PRIMARY, 0.5f));
  addAndMakeVisible(fileNameEditor_.get());

  okButton_ = std::make_unique<SkiaButton>();
  okButton_->setText(mode == Mode::SaveFile ? "Save" : "Open");
  okButton_->setStyle(SkiaButton::Style::Primary);
  okButton_->onClick = [this]() { handleOkPressed(); };
  addAndMakeVisible(okButton_.get());

  cancelButton_ = std::make_unique<SkiaButton>();
  cancelButton_->setText("Cancel");
  cancelButton_->setStyle(SkiaButton::Style::Secondary);
  cancelButton_->onClick = [this]() { handleCancelPressed(); };
  addAndMakeVisible(cancelButton_.get());

  upButton_ = std::make_unique<SkiaButton>();
  upButton_->setText("↑");
  upButton_->setStyle(SkiaButton::Style::Ghost);
  upButton_->onClick = [this]() { navigateUp(); };
  addAndMakeVisible(upButton_.get());

  // Set default appearance
  backgroundColour_ = design::colors::BG_DARKER;
  textColour_ = design::colors::TEXT_PRIMARY;
  font_.setSize(design::typography::FONT_MD);

  // Refresh directory contents
  refreshDirectory();
}

SkiaFileChooser::~SkiaFileChooser() {}

void SkiaFileChooser::showAsync(Callback callback) {
  callback_ = callback;
  // In a real implementation, this would show as a modal dialog
  // For now, we'll just make the component visible
  setVisible(true);
}

void SkiaFileChooser::setFilePatterns(const juce::String &patterns) {
  filePatterns_ = patterns;
  refreshDirectory();
}

void SkiaFileChooser::refreshDirectory() {
  if (!currentDirectory_.isDirectory()) {
    return;
  }

  currentFiles_.clear();
  currentDirectories_.clear();

  // Get directories
  currentDirectory_.findChildFiles(currentDirectories_,
                                   juce::File::findDirectories, false);
  currentDirectories_.sort();

  // Get files matching patterns
  if (filePatterns_.isEmpty()) {
    currentDirectory_.findChildFiles(currentFiles_, juce::File::findFiles,
                                     false);
  } else {
    juce::StringArray patterns;
    patterns.addTokens(filePatterns_, ";,", "\"'");
    patterns.trim();
    patterns.removeEmptyStrings();

    for (const auto &pattern : patterns) {
      juce::Array<juce::File> matchedFiles;
      currentDirectory_.findChildFiles(matchedFiles, juce::File::findFiles,
                                       false, pattern);
      currentFiles_.addArray(matchedFiles);
    }
  }

  currentFiles_.sort();

  // Update file list
  if (fileList_) {
    fileList_->updateContent();
  }

  // Update file name editor for save mode
  if (mode_ == Mode::SaveFile && fileNameEditor_) {
    fileNameEditor_->setText("");
  }
}

void SkiaFileChooser::navigateUp() {
  auto parent = currentDirectory_.getParentDirectory();
  if (parent.exists()) {
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

void SkiaFileChooser::handleFileSelected(const juce::File &file) {
  if (file.isDirectory()) {
    navigateToDirectory(file);
  } else {
    if (mode_ == Mode::SaveFile && fileNameEditor_) {
      fileNameEditor_->setText(file.getFileName());
    }
  }
}

void SkiaFileChooser::handleOkPressed() {
  juce::File selectedFile;

  if (mode_ == Mode::SaveFile && fileNameEditor_) {
    juce::String fileName = fileNameEditor_->getText();
    if (fileName.isNotEmpty()) {
      selectedFile = currentDirectory_.getChildFile(fileName);
    }
  } else if (fileList_) {
    int selectedRow = fileList_->getSelectedRow();
    if (selectedRow >= 0) {
      int dirCount = currentDirectories_.size();
      if (selectedRow < dirCount) {
        selectedFile = currentDirectories_[selectedRow];
      } else {
        selectedFile = currentFiles_[selectedRow - dirCount];
      }
    }
  }

  if (selectedFile.exists() || mode_ == Mode::SaveFile) {
    if (callback_) {
      callback_(Result::Approved, selectedFile);
    }
    setVisible(false);
  }
}

void SkiaFileChooser::handleCancelPressed() {
  if (callback_) {
    callback_(Result::Cancelled, juce::File());
  }
  setVisible(false);
}

void SkiaFileChooser::updateFileNameEditor() {
  if (!fileNameEditor_ || mode_ != Mode::SaveFile) {
    return;
  }

  int selectedRow = fileList_->getSelectedRow();
  if (selectedRow >= 0) {
    int dirCount = currentDirectories_.size();
    if (selectedRow >= dirCount) {
      juce::String fileName =
          currentFiles_[selectedRow - dirCount].getFileName();
      fileNameEditor_->setText(fileName);
    }
  }
}

void SkiaFileChooser::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();

  // Draw background
  SkPaint bgPaint;
  bgPaint.setColor(backgroundColour_);
  canvas->drawRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()),
                   bgPaint);

  // Draw border
  SkPaint borderPaint;
  borderPaint.setColor(design::colors::BORDER_DEFAULT);
  borderPaint.setStyle(SkPaint::kStroke_Style);
  borderPaint.setStrokeWidth(1.0f);
  borderPaint.setAntiAlias(true);
  canvas->drawRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()),
                   borderPaint);
}

void SkiaFileChooser::resized() {
  auto bounds = getLocalBounds();

  // Title at top
  titleLabel_->setBounds(bounds.removeFromTop(40).reduced(10, 5));

  // Up button at top right
  upButton_->setBounds(bounds.getWidth() - 50, 5, 40, 30);

  // File list takes most of space
  auto listBounds = bounds.removeFromTop(bounds.getHeight() - 80);
  fileList_->setBounds(listBounds.reduced(10));

  // File name editor (for save mode)
  if (mode_ == Mode::SaveFile) {
    auto fileNameBounds = bounds.removeFromTop(40).reduced(10, 5);
    fileNameEditor_->setBounds(fileNameBounds);
  }

  // Buttons at bottom
  auto buttonBounds = bounds.removeFromBottom(50).reduced(10, 10);
  cancelButton_->setBounds(buttonBounds.removeFromRight(100).reduced(5));
  buttonBounds.removeFromRight(10);
  okButton_->setBounds(buttonBounds.removeFromRight(100).reduced(5));
}

// FileSystemModel implementation

int SkiaFileChooser::FileSystemModel::getNumRows() {
  return chooser_->currentDirectories_.size() + chooser_->currentFiles_.size();
}

void SkiaFileChooser::FileSystemModel::paintListBoxItem(int rowNumber,
                                                        SkCanvas &canvas,
                                                        int width, int height,
                                                        bool rowIsSelected) {
  if (rowNumber < 0 || rowNumber >= getNumRows()) {
    return;
  }

  int dirCount = chooser_->currentDirectories_.size();
  juce::File file;
  bool isDirectory = false;

  if (rowNumber < dirCount) {
    file = chooser_->currentDirectories_[rowNumber];
    isDirectory = true;
  } else {
    file = chooser_->currentFiles_[rowNumber - dirCount];
  }

  // Draw selection background
  if (rowIsSelected) {
    SkPaint selectedBgPaint;
    selectedBgPaint.setColor(design::colors::CYAN);
    canvas.drawRect(SkRect::MakeXYWH(0, 0, width, height), selectedBgPaint);
  }

  // Draw icon
  SkPaint iconPaint;
  iconPaint.setColor(isDirectory ? design::colors::AMBER
                                 : design::colors::TEXT_PRIMARY);
  iconPaint.setAntiAlias(true);

  SkFont iconFont;
  iconFont.setSize(16.0f);

  const char *icon = isDirectory ? "📁" : "📄";
  canvas.drawString(icon, 8.0f, height * 0.5f + 6.0f, iconFont, iconPaint);

  // Draw filename
  SkPaint textPaint;
  textPaint.setColor(rowIsSelected ? design::colors::BG_DARKEST
                                   : design::colors::TEXT_PRIMARY);
  textPaint.setAntiAlias(true);

  SkFont font;
  font.setSize(design::typography::FONT_MD);

  juce::String fileName = file.getFileName();
  if (fileName.isEmpty()) {
    fileName = file.getFullPathName();
  }

  float textY = height * 0.5f + font.getSize() * 0.3f;
  canvas.drawString(fileName.toRawUTF8(), 32.0f, textY, font, textPaint);
}

void SkiaFileChooser::FileSystemModel::listBoxItemClicked(
    int rowNumber, const juce::MouseEvent &e) {
  juce::ignoreUnused(e);

  int dirCount = chooser_->currentDirectories_.size();
  juce::File file;

  if (rowNumber < dirCount) {
    file = chooser_->currentDirectories_[rowNumber];
  } else {
    file = chooser_->currentFiles_[rowNumber - dirCount];
  }

  chooser_->handleFileSelected(file);
}

void SkiaFileChooser::FileSystemModel::listBoxItemDoubleClicked(
    int rowNumber, const juce::MouseEvent &e) {
  juce::ignoreUnused(e);

  int dirCount = chooser_->currentDirectories_.size();

  if (rowNumber < dirCount) {
    juce::File dir = chooser_->currentDirectories_[rowNumber];
    chooser_->navigateToDirectory(dir);
  }
}

} // namespace zenith