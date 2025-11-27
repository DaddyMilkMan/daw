/*
  ==============================================================================
    RightSidePanel.h
    The container for the right-hand AI assistant panel (Wingman)
    Full Skia rendering - professional DAW inspector/AI panel
  ==============================================================================
*/
#pragma once
#include "SkiaComponent.h"
#include "SkiaTheme.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

#ifdef ZENITH_USE_SKIA
#include <include/core/SkCanvas.h>
#endif

namespace zenith {

class RightSidePanel : public SkiaComponent, public juce::Timer {
public:
  RightSidePanel();
  ~RightSidePanel() override;

  void resized() override;
  
#ifdef ZENITH_USE_SKIA
  void drawSkia(SkCanvas *canvas) override;
#endif

  void mouseDown(const juce::MouseEvent &e) override;
  void mouseMove(const juce::MouseEvent &e) override;
  void mouseExit(const juce::MouseEvent &e) override;
  
  void timerCallback() override;

  // Chat message structure
  struct ChatMessage {
    juce::String text;
    bool isUser;
    juce::int64 timestamp;
  };

  // Add a message to the chat
  void addMessage(const juce::String& text, bool isUser);

  // Deprecated - kept for compatibility
  void setWingmanPanel(juce::Component *panel) { juce::ignoreUnused(panel); }

private:
  // Drawing methods
  void drawBackground(SkCanvas *canvas);
  void drawHeader(SkCanvas *canvas);
  void drawChatArea(SkCanvas *canvas);
  void drawInputArea(SkCanvas *canvas);
  void drawConnectionStatus(SkCanvas *canvas);

  // Hit testing
  enum class HitZone { None, SendButton, InputField, ChatArea };
  HitZone hitTest(juce::Point<int> pos) const;

  // State
  std::vector<ChatMessage> messages_;
  juce::String inputText_;
  float scrollOffset_ = 0.0f;
  HitZone hoveredZone_ = HitZone::None;
  bool isConnected_ = false;
  float connectionPulse_ = 0.0f;

  // Layout constants
  static constexpr float HEADER_HEIGHT = 48.0f;
  static constexpr float INPUT_HEIGHT = 56.0f;
  static constexpr float PADDING = 12.0f;
  static constexpr float MESSAGE_GAP = 8.0f;
  static constexpr float CORNER_RADIUS = 8.0f;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RightSidePanel)
};

} // namespace zenith
