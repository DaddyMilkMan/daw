/*
  ==============================================================================

    WingmanPanel.h
    Created: 2025-11-29 (Updated for Pure Skia)
    Author:  Marcus Williams (UX Team)

    Wingman AI Assistant Panel with Grok Integration
    Pure Skia rendering - no JUCE UI components

  ==============================================================================
*/

#pragma once

#include "../../commands/CommandAPI.h"
#include "../ai/SampleHunterAgent.h"
#include "../controls/SkiaButton.h"
#include "../controls/SkiaComboBox.h"
#include "../controls/SkiaLabel.h"
#include "../controls/SkiaTextEditor.h"
#include "../framework/SkiaComponent.h"
#include "../network/GrokDAWController.h"
#include "Engine.h"
#include <memory>
#include <vector>

namespace zenith {

/**
    Wingman AI Assistant Panel (Pure Skia)

    Provides a chat-like interface for controlling the DAW with natural
   language. Now includes Sample Hunter integration for finding sounds via chat.
*/
class WingmanPanel : public SkiaComponent,
                     public ai::SampleHunterAgent::Listener {
public:
  //==========================================================================
  WingmanPanel(CommandAPI &api, Engine &engine);
  ~WingmanPanel() override;

  //==========================================================================
  // SkiaComponent overrides
  void drawSkia(SkCanvas *canvas) override;
  void resized() override;

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
  // SampleHunterAgent::Listener
  void sampleDownloaded(const ai::FoundSample &sample) override;
  void sampleAnalyzed(const ai::FoundSample &sample) override;
  void sampleImported(const juce::File &file) override;
  void huntingProgressChanged(float progress,
                              const juce::String &status) override;
  void huntingComplete(const ai::HuntingStats &stats, bool success) override;

  // UI Components (Pure Skia)

  std::unique_ptr<SkiaTextEditor> inputField_;
  std::unique_ptr<SkiaButton> sendButton_;
  std::unique_ptr<SkiaButton> fastModeButton_;
  std::unique_ptr<SkiaButton> thinkingModeButton_;
  std::unique_ptr<SkiaLabel> reasoningLabel_;
  std::unique_ptr<SkiaLabel> statusLabel_;
  std::unique_ptr<SkiaButton> clearButton_;
  std::unique_ptr<SkiaButton> settingsButton_;

  // Conversation messages (simple text storage for now)
  struct ChatMessage {
    juce::String speaker;
    juce::String content;
    SkColor speakerColor;
  };
  std::vector<ChatMessage> conversationHistory_;
  float conversationScrollY_ = 0.0f;

  //==========================================================================
  // Backend

  CommandAPI &commandAPI_;
  Engine &engine_;
  std::unique_ptr<GrokDAWController> grokController_;

  //==========================================================================
  // State

  bool isProcessing_ = false;
  GrokMode currentMode_ = GrokMode::Fast;

  // Sample Hunter state
  bool isSampleSearchActive_ = false;
  std::vector<ai::FoundSample> lastSearchResults_;
  juce::String lastSearchQuery_;

  //==========================================================================
  // Methods

  void sendCommand();
  void appendToConversation(const juce::String &speaker,
                            const juce::String &message);
  void setStatus(const juce::String &status, SkColor colour = SK_ColorWHITE);
  void showSettings();

  // Rendering helpers
  void drawHeader(SkCanvas *canvas, const SkRect &bounds);
  void drawConversation(SkCanvas *canvas, const SkRect &bounds);
  void mouseWheelMove(const juce::MouseEvent &e,
                      const juce::MouseWheelDetails &wheel) override;

  // Sample Hunter methods (Stateless logic)
  static bool detectSampleSearchIntent(const juce::String &message,
                                       juce::String &outQuery);
  static juce::String cleanQueryFiller(const juce::String &rawQuery);

  // Sample Hunter methods (Stateful)
  bool handleImportCommand(const juce::String &message);
  void displaySearchResults(const std::vector<ai::FoundSample> &results);

  //==========================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WingmanPanel)
};

} // namespace zenith
