/*
  ==============================================================================

    SkiaFileChooserDialog.cpp
    Created: 2025-12-19
    Author:  Zenith DAW Team

    Modal file chooser dialog implementation with backdrop blur and animations.

  ==============================================================================
*/

#include "SkiaFileChooserDialog.h"
#include "../framework/GlassmorphicPanel.h"
#include "../design-system/ZenithDesignSystem.h"

namespace zenith {

// Static factory methods
void SkiaFileChooserDialog::showOpenDialog(juce::Component* parent,
                                           const juce::String& title,
                                           const juce::File& initialDirectory,
                                           const juce::String& filePatterns,
                                           Callback callback) {
  // Create dialog (self-destructs when closed)
  auto* dialog = new SkiaFileChooserDialog(
      parent, title, initialDirectory, filePatterns, "",
      SkiaFileChooser::Mode::OpenFile, std::move(callback));
  dialog->show();
}

void SkiaFileChooserDialog::showSaveDialog(juce::Component* parent,
                                           const juce::String& title,
                                           const juce::File& initialDirectory,
                                           const juce::String& defaultFilename,
                                           Callback callback) {
  // Extract file extension for filter if provided
  juce::String patterns;
  if (defaultFilename.contains(".")) {
    juce::String ext = defaultFilename.fromLastOccurrenceOf(".", true, false);
    patterns = "*" + ext;
  }

  auto* dialog = new SkiaFileChooserDialog(
      parent, title, initialDirectory, patterns, defaultFilename,
      SkiaFileChooser::Mode::SaveFile, std::move(callback));
  dialog->show();
}

void SkiaFileChooserDialog::showDirectoryDialog(juce::Component* parent,
                                                const juce::String& title,
                                                const juce::File& initialDirectory,
                                                Callback callback) {
  auto* dialog = new SkiaFileChooserDialog(
      parent, title, initialDirectory, "", "",
      SkiaFileChooser::Mode::OpenDirectory, std::move(callback));
  dialog->show();
}

// Constructor
SkiaFileChooserDialog::SkiaFileChooserDialog(juce::Component* parent,
                                             const juce::String& title,
                                             const juce::File& initialDirectory,
                                             const juce::String& filePatterns,
                                             const juce::String& defaultFilename,
                                             SkiaFileChooser::Mode mode,
                                             Callback callback)
    : parentComponent_(parent), callback_(std::move(callback)) {

  // Create file chooser
  fileChooser_ = std::make_unique<SkiaFileChooser>(
      title, initialDirectory, filePatterns, mode);

  if (!defaultFilename.isEmpty()) {
    fileChooser_->setDefaultFilename(defaultFilename);
  }

  // Set up result callback
  fileChooser_->showAsync([this](SkiaFileChooser::Result result, const juce::File& file) {
    onChooserResult(result, file);
  });

  addAndMakeVisible(fileChooser_.get());

  // Set up as overlay
  setAlwaysOnTop(true);
  setWantsKeyboardFocus(true);
}

SkiaFileChooserDialog::~SkiaFileChooserDialog() {
  isShuttingDown_->store(true);
  stopTimer();
}

void SkiaFileChooserDialog::show() {
  if (parentComponent_ == nullptr) {
    return;
  }

  // Add to parent and cover it
  parentComponent_->addAndMakeVisible(this);
  setBounds(parentComponent_->getLocalBounds());
  toFront(true);
  grabKeyboardFocus();

  // Start animation
  backdropAlpha_.setTarget(1.0f, 200);
  dialogScale_.setTarget(1.0f, 200);
  startTimerHz(60);
}

void SkiaFileChooserDialog::dismiss(bool accepted) {
  isClosing_ = true;

  // Trigger callback
  if (callback_) {
    callback_(accepted, resultFile_);
  }

  // Animate out
  backdropAlpha_.setTarget(0.0f, 200);
  dialogScale_.setTarget(0.95f, 200);
}

void SkiaFileChooserDialog::onChooserResult(SkiaFileChooser::Result result,
                                            const juce::File& file) {
  resultFile_ = file;
  dismiss(result == SkiaFileChooser::Result::Approved);
}

void SkiaFileChooserDialog::drawSkia(SkCanvas* canvas) {
  using namespace design;

  auto bounds = getLocalBounds().toFloat();
  float width = bounds.getWidth();
  float height = bounds.getHeight();

  // Backdrop (semi-transparent with optional blur effect)
  float alpha = backdropAlpha_.getCurrentValue();

  SkPaint backdropPaint;
  backdropPaint.setAntiAlias(true);
  backdropPaint.setColor(SkColorSetARGB(
      static_cast<U8CPU>(alpha * 180), 0, 0, 0)); // 70% black at full alpha

  canvas->drawRect(SkRect::MakeWH(width, height), backdropPaint);

  // Dialog panel (centered, scaled)
  SkRect dialogBounds = getDialogBounds();

  // Apply scale transform for animation
  float scale = dialogScale_.getCurrentValue();
  if (scale != 1.0f) {
    float cx = dialogBounds.centerX();
    float cy = dialogBounds.centerY();
    canvas->save();
    canvas->translate(cx, cy);
    canvas->scale(scale, scale);
    canvas->translate(-cx, -cy);
  }

  // Draw glassmorphic dialog background
  GlassmorphicPanel::Options opts;
  opts.style = GlassmorphicPanel::Style::Floating;
  opts.cornerRadius = dimensions::RADIUS_XL;
  opts.drawShadow = true;
  opts.accentColor = colors::CYAN;
  opts.glowIntensity = 0.3f;

  GlassmorphicPanel::drawWithOptions(canvas, dialogBounds, opts);

  if (scale != 1.0f) {
    canvas->restore();
  }
}

void SkiaFileChooserDialog::resized() {
  // Match parent bounds
  if (parentComponent_) {
    setBounds(parentComponent_->getLocalBounds());
  }

  // Position file chooser within dialog bounds
  SkRect dialogBounds = getDialogBounds();

  int x = static_cast<int>(dialogBounds.left());
  int y = static_cast<int>(dialogBounds.top());
  int w = static_cast<int>(dialogBounds.width());
  int h = static_cast<int>(dialogBounds.height());

  if (fileChooser_) {
    fileChooser_->setBounds(x, y, w, h);
  }
}

bool SkiaFileChooserDialog::keyPressed(const juce::KeyPress& key) {
  // ESC to cancel
  if (key == juce::KeyPress::escapeKey) {
    dismiss(false);
    return true;
  }

  // Pass to file chooser
  if (fileChooser_) {
    return fileChooser_->keyPressed(key);
  }

  return false;
}

void SkiaFileChooserDialog::mouseDown(const juce::MouseEvent& e) {
  // Click outside dialog to cancel
  SkRect dialogBounds = getDialogBounds();
  auto pos = e.getPosition();

  if (!dialogBounds.contains(static_cast<float>(pos.x),
                             static_cast<float>(pos.y))) {
    dismiss(false);
  }
}

void SkiaFileChooserDialog::timerCallback() {
  backdropAlpha_.update(16.67f);
  dialogScale_.update(16.67f);

  markDirty();

  // Self-destruct when fade-out complete
  if (isClosing_ && backdropAlpha_.getCurrentValue() < 0.01f) {
    stopTimer();
    // Remove from parent and delete
    if (auto* parent = getParentComponent()) {
      parent->removeChildComponent(this);
    }
    delete this;
    return;
  }
}

SkRect SkiaFileChooserDialog::getDialogBounds() const {
  float parentWidth = static_cast<float>(getWidth());
  float parentHeight = static_cast<float>(getHeight());

  // Calculate dialog size (responsive)
  float dialogWidth = parentWidth * DIALOG_WIDTH_RATIO;
  float dialogHeight = parentHeight * DIALOG_HEIGHT_RATIO;

  // Apply min/max constraints
  dialogWidth = std::clamp(dialogWidth, DIALOG_MIN_WIDTH, DIALOG_MAX_WIDTH);
  dialogHeight = std::clamp(dialogHeight, DIALOG_MIN_HEIGHT, DIALOG_MAX_HEIGHT);

  // Center in parent
  float x = (parentWidth - dialogWidth) / 2.0f;
  float y = (parentHeight - dialogHeight) / 2.0f;

  return SkRect::MakeXYWH(x, y, dialogWidth, dialogHeight);
}

} // namespace zenith
