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
#include "../controls/SkiaComboBox.h"
#include "../controls/SkiaLabel.h"
#include "../controls/SkiaTextEditor.h"
#include "../controls/ZenithButton.h"
#include "../framework/SkiaComponent.h"
#include <juce_core/juce_core.h>

namespace zenith {
class SettingsComponent;

/**
    Wingman AI Assistant Panel

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
  // Component overrides
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

  //==========================================================================
  // UI Components

  // ...
  std::unique_ptr<SkiaTextEditor> inputField;
  std::unique_ptr<SkiaTextEditor> conversationDisplay;
  std::unique_ptr<ZenithButton> sendButton;
  std::unique_ptr<ZenithButton> acceptButton;
  std::unique_ptr<ZenithButton> denyButton;
  std::unique_ptr<SkiaComboBox> modeSelector;
  std::unique_ptr<SkiaLabel> modeLabel;
  std::unique_ptr<SkiaLabel> statusLabel;
  std::unique_ptr<ZenithButton> clearButton;
  std::unique_ptr<ZenithButton> settingsButton;
  std::unique_ptr<SettingsComponent> settingsOverlay_;
  std::unique_ptr<ZenithButton> closeSettingsButton_;

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
  void setStatus(const juce::String &status, SkColor colour);
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
