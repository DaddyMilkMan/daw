/*
  ==============================================================================

    WingmanPanel.h
    Created: 2025-11-29 (Rewritten: 2025-12-30)
    Author:  Marcus Williams (Original) / AI Assistant (Redesign)

    Modern Wingman AI Assistant Panel with Glassmorphism
    - Pure Skia rendering via SkiaComponent
    - Sharp rectangle container, hairline borders
    - Brain icon (pink when reasoning ON)
    - Send icon (blue on click)

  ==============================================================================
*/

#pragma once

#include "../framework/SkiaComponent.h"
#include "../controls/ZenithButton.h"
#include "../controls/SkiaTextInput.h"
#include "../network/GrokDAWController.h"
#include "../../commands/CommandAPI.h"
#include "Engine.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace zenith {

/**
    Chat message data for pure Skia rendering
*/
struct ChatMessage {
  juce::String speaker;
  juce::String text;
  bool isUser = false;
  float cachedHeight = 0.0f; // Cached bubble height for layout
};

/**
    Wingman AI Assistant Panel - Pure Skia Rendering

    Premium glassmorphic chat interface for DAW AI control.
    Features:
    - Sharp rectangular panel (pop-out look)
    - Hairline 0.5px borders
    - Brain toggle for reasoning mode (pink glow)
    - Send button with blue feedback
    - Pure Skia chat rendering with manual scroll
    - Thread-safe async Grok integration
*/
class WingmanPanel : public SkiaComponent {
public:
  //==========================================================================
  WingmanPanel(CommandAPI &api, Engine &engine);
  ~WingmanPanel() override;

  //==========================================================================
  // SkiaComponent override
  void drawSkia(SkCanvas *canvas) override;
  void onShow() override;

  // Component overrides
  void resized() override;
  void visibilityChanged() override;
  void mouseWheelMove(const juce::MouseEvent &e, const juce::MouseWheelDetails &wheel) override;

  //==========================================================================
  /**
      Initialize Grok integration

      @param apiKey Grok API key (optional, retrieves from SecureKeyStore if
     empty)
      @return true if initialized successfully
  */
  bool initializeGrok(const juce::String &apiKey = juce::String());

  /**
      Check if Grok is ready
  */
  bool isGrokReady() const;

private:
  //==========================================================================
  // UI Components (Skia-rendered buttons + input field)

  std::unique_ptr<SkiaTextInput> inputField_;
  std::unique_ptr<ZenithButton> brainToggle_;  // Reasoning mode toggle
  std::unique_ptr<ZenithButton> sendButton_;   // Send message
  std::unique_ptr<ZenithButton> settingsButton_; // Settings (header)

  //==========================================================================
  // Chat Data (Pure Skia - no JUCE components)

  std::vector<ChatMessage> messages_;
  float scrollOffset_ = 0.0f;
  float contentHeight_ = 0.0f;
  juce::Rectangle<float> chatAreaBounds_;

  //==========================================================================
  // Backend

  CommandAPI &commandAPI_;
  Engine &engine_;
  std::unique_ptr<GrokDAWController> grokController_;

  //==========================================================================
  // State

  bool isProcessing_ = false;
  bool reasoningMode_ = false;  // true = Thinking mode (grok-4.1), false = Fast mode

  //==========================================================================
  // Layout Constants
  
  static constexpr int HEADER_HEIGHT = 48;
  static constexpr int INPUT_ROW_HEIGHT = 56;
  static constexpr int BUTTON_SIZE = 40;
  static constexpr int PADDING = 12;
  static constexpr float BUBBLE_MAX_WIDTH_RATIO = 0.8f;
  static constexpr float BUBBLE_PADDING = 10.0f;
  static constexpr float BUBBLE_RADIUS = 12.0f;
  static constexpr float LINE_HEIGHT = 18.0f;

  //==========================================================================
  // Methods

  void sendMessage();
  void appendMessage(const juce::String &speaker, const juce::String &message);
  void recalculateLayout();
  void scrollToBottom();
  
  void drawChatArea(SkCanvas *canvas);
  void drawMessage(SkCanvas *canvas, const ChatMessage &msg, float y, float maxWidth);
  float calculateMessageHeight(const ChatMessage &msg, float maxWidth);

  void setupBrainToggle();
  void setupSendButton();
  void setupSettingsButton();

  //==========================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WingmanPanel)
};

} // namespace zenith
