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

/*
    ==============================================================================
    Original file header:
*/

  ==============================================================================

    SkiaFileChooser.h
    Created: 2025-12-07
    Author:  AI Assistant

    Pure Skia-based file chooser component to replace juce::FileChooser

  ==============================================================================

*/

#pragma once

#include "../controls/SkiaButton.h"
#include "../framework/SkiaComponent.h"
#include "../controls/SkiaLabel.h"
#include "../controls/SkiaListBox.h"
#include "../controls/SkiaTextEditor.h"
#include <juce_core/juce_core.h>

namespace zenith {

class SkiaFileChooser : public SkiaComponent {
public:
  enum class Mode { OpenFile, SaveFile, OpenDirectory };

  enum class Result { Cancelled, Approved };

  using Callback = std::function<void(Result result, const juce::File &file)>;

  SkiaFileChooser(const juce::String &dialogTitle,
                  const juce::File &initialFileOrDirectory,
                  const juce::String &filePatternsAllowed,
                  Mode mode = Mode::OpenFile);
  ~SkiaFileChooser() override;

  // Show the dialog
  void showAsync(Callback callback);

  // Get current directory
  juce::File getCurrentDirectory() const { return currentDirectory_; }

  // Set file patterns (e.g., "*.wav;*.mp3;*.flac")
  void setFilePatterns(const juce::String &patterns);

private:
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
  std::unique_ptr<SkiaButton> okButton_;
  std::unique_ptr<SkiaButton> cancelButton_;
  std::unique_ptr<SkiaButton> upButton_;

  // Current state
  juce::File currentDirectory_;
  juce::String filePatterns_;
  Mode mode_;
  juce::String dialogTitle_;

  juce::Array<juce::File> currentFiles_;
  juce::Array<juce::File> currentDirectories_;

  Callback callback_;

  // Appearance
  SkColor backgroundColour_;
  SkColor textColour_;
  SkFont font_;

  // Internal methods
  void refreshDirectory();
  void navigateUp();
  void navigateToDirectory(const juce::File &directory);
  void handleFileSelected(const juce::File &file);
  void handleOkPressed();
  void handleCancelPressed();
  void updateFileNameEditor();

  // Component interface
  void drawSkia(SkCanvas *canvas) override;
  void resized() override;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaFileChooser)
};

} // namespace zenith