/*
  ==============================================================================

    SkiaTextInput.h
    Created: 2026-01-10
    Author:  Zenith DAW

    Pure Skia text input component - QUALITY implementation.
    Features:
    - Pill-shaped or rectangular styling
    - Blinking cursor
    - Full text selection (click-drag, Shift+Arrow, Ctrl+A, double-click word)
    - Clipboard support (Ctrl+C, Ctrl+V, Ctrl+X)
    - Horizontal scrolling for long text
    - Clean glassmorphic appearance

  ==============================================================================
*/

#pragma once

#include "../framework/SkiaComponent.h"
#include "../design-system/ZenithDesignSystem.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace zenith {

/**
    Pure Skia text input component - QUALITY IMPLEMENTATION.
    
    Full-featured text input with:
    - Blinking cursor
    - Text selection (mouse + keyboard)
    - Clipboard operations
    - Horizontal scroll for overflow
    - Placeholder text
    - Pill-shaped border
*/
class SkiaTextInput : public SkiaComponent {
public:
  // Callbacks
  std::function<void()> onReturnKey;
  std::function<void(const juce::String&)> onTextChanged;

  std::function<void()> onEscapeKey;
  std::function<void()> onFocusLostCallback;

  SkiaTextInput();
  ~SkiaTextInput() override;

  // Text access
  juce::String getText() const { return text_; }
  void setText(const juce::String& newText);
  void clear();
  void selectAll();

  void setPlaceholder(const juce::String& placeholder) { placeholder_ = placeholder; markDirty(); }
  void setMaxLength(int maxLen) { maxLength_ = maxLen; }

  // Style options
  void setPillShape(bool usePill) { pillShape_ = usePill; markDirty(); }
  void setFontSize(float size) { fontSize_ = size; recalculateTextMetrics(); markDirty(); }

  // SkiaComponent
  void drawSkia(SkCanvas* canvas) override;
  void resized() override;

  // Input handling
  void mouseDown(const juce::MouseEvent& e) override;
  void mouseUp(const juce::MouseEvent& e) override;
  void mouseDrag(const juce::MouseEvent& e) override;
  void mouseDoubleClick(const juce::MouseEvent& e) override;
  bool keyPressed(const juce::KeyPress& key) override;
  void focusGained(FocusChangeType cause) override;
  void focusLost(FocusChangeType cause) override;

  void timerCallback() override;

private:
  juce::String text_;
  juce::String placeholder_ = "Type here...";
  int cursorPos_ = 0;
  int selectionAnchor_ = -1; // Anchor point for selection (start of drag)
  bool isFocused_ = false;
  bool cursorVisible_ = true;
  float fontSize_ = 14.0f;
  int maxLength_ = 1000;
  bool pillShape_ = true;

  // Scrolling
  float scrollOffset_ = 0.0f;
  float textAreaWidth_ = 0.0f;
  float totalTextWidth_ = 0.0f;

  // Cursor blink
  int blinkCounter_ = 0;
  static constexpr int BLINK_RATE = 30; // frames (at 60fps = ~500ms)

  // Layout constants
  float getPadding() const { return pillShape_ ? getHeight() * 0.4f : 12.0f; }

  // Text metrics (cached)
  SkFont cachedFont_;
  void recalculateTextMetrics();

  // Text layout helpers
  float getTextWidth(const juce::String& str) const;
  float getTextWidthUpTo(int charIndex) const;
  int getCharIndexAt(float localX) const;
  float getCursorXPosition() const;
  void ensureCursorVisible();

  // Selection helpers
  bool hasSelection() const { return selectionAnchor_ >= 0 && selectionAnchor_ != cursorPos_; }
  int getSelectionStart() const { return hasSelection() ? std::min(selectionAnchor_, cursorPos_) : cursorPos_; }
  int getSelectionEnd() const { return hasSelection() ? std::max(selectionAnchor_, cursorPos_) : cursorPos_; }
  juce::String getSelectedText() const;
  void deleteSelection();

  // Text manipulation
  void insertText(const juce::String& textToInsert);
  void moveCursor(int delta, bool extendSelection);
  void moveCursorToWordBoundary(int direction, bool extendSelection);

  // Clipboard
  void copyToClipboard();
  void pasteFromClipboard();
  void cutToClipboard();

  // Word boundary detection
  int findWordStart(int fromPos) const;
  int findWordEnd(int fromPos) const;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaTextInput)
};

} // namespace zenith
