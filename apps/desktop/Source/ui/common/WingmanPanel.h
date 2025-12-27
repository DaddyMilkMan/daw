/*
  ==============================================================================

    WingmanPanel.h
    Created: 2025-11-29 (Updated for Grok Integration)
    Author:  Marcus Williams (UX Team)

    Wingman AI Assistant Panel with Grok Integration
    - Natural language command input
    - Mode selector (Fast/Thinking)
    - Conversation history
    - Preset generation interface

  ==============================================================================
*/

#pragma once

#include "../ai/SampleHunterAgent.h"
#include "../network/GrokDAWController.h"
#include "Engine.h"
// #include "../network/AIBridgeClient.h" // File missing - disabled temporarily
#include "../../commands/CommandAPI.h"
#include "../controls/MarkdownComponent.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace zenith {

/**
    Wingman AI Assistant Panel

    Provides a chat-like interface for controlling the DAW with natural
   language. Now includes Sample Hunter integration for finding sounds via chat.
*/
class WingmanPanel : public juce::Component,
                     private juce::TextEditor::Listener,
                     private juce::Button::Listener,
                     public ai::SampleHunterAgent::Listener {
public:
  //==========================================================================
  WingmanPanel(CommandAPI &api, Engine &engine);
  ~WingmanPanel() override;

  //==========================================================================
  // Component overrides
  void paint(juce::Graphics &g) override;
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
  // TextEditor::Listener
  void textEditorReturnKeyPressed(juce::TextEditor &editor) override;

  // Button::Listener
  void buttonClicked(juce::Button *button) override;

  //==========================================================================
  // SampleHunterAgent::Listener
  void sampleDownloaded(const ai::FoundSample &sample) override;
  void sampleAnalyzed(const ai::FoundSample &sample) override;
  void sampleImported(const juce::File &file) override;
  void huntingProgressChanged(float progress,
                              const juce::String &status) override;
  void huntingComplete(const ai::HuntingStats &stats, bool success) override;

  //==========================================================================
  // UI Components

  // ...
  std::unique_ptr<juce::TextEditor> inputField;
  std::unique_ptr<widgets::MarkdownComponent> conversationDisplay;
  std::unique_ptr<juce::TextButton> sendButton;
  std::unique_ptr<juce::ComboBox> modeSelector;
  std::unique_ptr<juce::Label> modeLabel;
  std::unique_ptr<juce::Label> statusLabel;
  std::unique_ptr<juce::TextButton> clearButton;
  std::unique_ptr<juce::TextButton> settingsButton;

  //==========================================================================
  // Backend

  CommandAPI &commandAPI;
  Engine &engine_;
  std::unique_ptr<GrokDAWController> grokController;

  //==========================================================================
  // State

  bool isProcessing = false;
  GrokMode currentMode = GrokMode::Fast;

  // Sample Hunter state
  bool isSampleSearchActive_ = false;
  std::vector<ai::FoundSample> lastSearchResults_;
  juce::String lastSearchQuery_;

  //==========================================================================
  // Methods

  void sendCommand();
  void appendToConversation(const juce::String &speaker,
                            const juce::String &message);
  void setStatus(const juce::String &status,
                 juce::Colour colour = juce::Colours::white);
  void updateModeFromSelector();
  void showSettings();

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
