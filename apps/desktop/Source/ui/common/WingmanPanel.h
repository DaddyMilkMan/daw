/*
  ==============================================================================

    WingmanPanel.h
    Created: 2025-11-29 (Updated for Premium UI)
    Author:  Marcus Williams (UX Team)

    Wingman AI Assistant Panel - Premium Glassmorphic Implementation
    - Natural language command input
    - Context-aware suggestions
    - Premium animations & glassmorphism
    - Sample Hunter & Grok Integration

  ==============================================================================
*/

#pragma once

#include "../ai/SampleHunterAgent.h"
#include "../network/GrokDAWController.h"
#include "Engine.h"
#include "../../commands/CommandAPI.h"
#include "../framework/SkiaComponent.h"
#include "../framework/Animation.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <functional>
#include <deque>
#include "../components/AIChatMessage.h"

namespace zenith {

/**
    Wingman AI Assistant Panel - Premium Implementation

    Provides a premium, glassmorphic chat interface for controlling the DAW.
*/
class WingmanPanel : public SkiaComponent,
                     private juce::TextEditor::Listener,
                     public ai::SampleHunterAgent::Listener {
public:
  //==========================================================================
  WingmanPanel(CommandAPI &api, Engine &engine);
  ~WingmanPanel() override;

  //==========================================================================
  // Component overrides
  void paint(juce::Graphics &g) override;
  void drawSkia(SkCanvas* canvas) override;
  void resized() override;
  void timerCallback() override;
  void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& d) override; // Correct signature for JUCE 8
  void mouseMove(const juce::MouseEvent& e) override;
  void mouseDown(const juce::MouseEvent& e) override;
  void mouseUp(const juce::MouseEvent& e) override;
  void mouseExit(const juce::MouseEvent& e) override;

  //==========================================================================
  // Logic
  bool initializeGrok(const juce::String &apiKey = juce::String());
  bool isGrokReady() const;
  
  void setContext(const juce::String& contextName);
  void toggleMinimize();

  //==========================================================================
  // Callbacks
  std::function<void(const juce::String&)> onSendMessage;
  std::function<void(const juce::String&)> onSuggestionClicked;
  std::function<void()> onVoiceInputStart;
  std::function<void()> onVoiceInputEnd;

private:
  //==========================================================================
  // Data Structures
  struct Suggestion {
    Suggestion(const juce::String& t, const juce::String& i) : text(t), id(i) {}
    Suggestion() = default;

    juce::String text;
    juce::String id;
    
    // Animation/Interaction state
    animation::AnimatedValue<float> hoverProgress{0.0f};
    juce::Rectangle<float> bounds;
    bool isHovered = false;
  };

  //==========================================================================
  // Methods
  void sendCommand();
  void appendToConversation(const juce::String &speaker, const juce::String &message, bool isUser);
  void updateSuggestions();
  void layoutMessages();
  void drawHeader(SkCanvas* canvas);
  void drawContextIndicator(SkCanvas* canvas);
  void drawChatArea(SkCanvas* canvas);
  void drawSuggestions(SkCanvas* canvas);
  void drawInputArea(SkCanvas* canvas);
  
  // AIChatMessage Integration
  std::deque<std::unique_ptr<AIChatMessage>> messages_; 
  // No longer need drawMessage as components draw themselves
  
  // SampleHunter Integration
  void sampleDownloaded(const ai::FoundSample &sample) override;
  void sampleAnalyzed(const ai::FoundSample &sample) override;
  void sampleImported(const juce::File &file) override;
  void huntingProgressChanged(float progress, const juce::String &status) override;
  void huntingComplete(const ai::HuntingStats &stats, bool success) override;

  // TextEditor Listener
  void textEditorReturnKeyPressed(juce::TextEditor &editor) override;

  //==========================================================================
  // Dependencies
  CommandAPI &commandAPI;
  Engine &engine_;
  std::unique_ptr<GrokDAWController> grokController;

  //==========================================================================
  // UI Components
  std::unique_ptr<juce::TextEditor> inputField_;
  // Custom buttons managed via mouse events/Skia
  juce::Rectangle<float> minimizeBtnBounds_;
  juce::Rectangle<float> sendBtnBounds_;
  juce::Rectangle<float> voiceBtnBounds_;
  juce::Rectangle<float> brainBtnBounds_;
  
  //==========================================================================
  // State
  std::vector<Suggestion> suggestions_;
  
  juce::String currentContext_ = "Arrangement";
  juce::String inputPlaceholder_ = "Ask Wingman...";
  
  // Animation State
  animation::AnimatedValue<float> panelWidth_{380.0f};
  animation::AnimatedValue<float> scrollY_{0.0f};
  float targetScrollY_ = 0.0f;
  float maxScrollY_ = 0.0f;
  
  animation::AnimatedValue<float> typingIndicatorOpacity_{0.0f};
  float processingDotPhase_ = 0.0f;
  
  // Interaction State
  bool isMinimized_ = false;
  bool isProcessing_ = false;
  bool isTyping_ = false;
  
  // Interaction State
  bool isMinimizeHovered_ = false;
  bool isSendHovered_ = false;
  bool isVoiceHovered_ = false;
  bool isBrainHovered_ = false;
  bool isBrainActive_ = false;
  bool isDraggingScroll_ = false;
  float contentHeight_ = 0.0f;
  
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WingmanPanel)
};

} // namespace zenith
