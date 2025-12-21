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

#include <atomic>
#include <memory>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../framework/SkiaComponent.h"
#include "../widgets/SkiaButton.h"
#include "../widgets/SkiaLabel.h"
#include "../widgets/SkiaTextEditor.h"
#include "../widgets/SkiaComboBox.h"
#include "../widgets/MarkdownComponent.h"
#include "../../ai/SampleHunterAgent.h"
#include "../../network/AIBridgeClient.h"

namespace zenith {

class Engine;
class CommandAPI;
// GrokDAWController removed

/**
    Wingman AI Assistant Panel

    Provides a chat-like interface for controlling the DAW with natural
   language. Now includes Sample Hunter integration for finding sounds via chat.
*/
class WingmanPanel : public SkiaComponent,
                     public ai::SampleHunterAgent::Listener,
                     public AIBridgeClient::Listener { // Added listener for AIBridge
public:
  //==========================================================================
  WingmanPanel(CommandAPI &api, Engine &engine);
  ~WingmanPanel() override;

  //==========================================================================
  // SkiaComponent overrides
  void drawSkia(SkCanvas *canvas) override;
  void paint(juce::Graphics& g) override { SkiaComponent::paint(g); }
  void resized() override;

  //==========================================================================
  // AIBridgeClient::Listener overrides
  void responseReceived(const juce::String& response) override;
  void errorReceived(const juce::String& error) override;
  void statusChanged(const juce::String& status) override;

  // SampleHunterAgent::Listener overrides
  void sampleDownloaded(const ai::FoundSample &sample) override;
  void sampleAnalyzed(const ai::FoundSample &sample) override;
  void sampleImported(const juce::File &file) override;
  void huntingProgressChanged(float progress, const juce::String &status) override;
  void huntingComplete(const ai::HuntingStats &stats, bool success) override;

private:
  // ... existing UI components ...
  std::unique_ptr<SkiaTextEditor> inputField;
  std::unique_ptr<widgets::MarkdownComponent> conversationDisplay;
  std::unique_ptr<SkiaButton> sendButton;
  std::unique_ptr<SkiaComboBox> modeSelector;
  std::unique_ptr<SkiaLabel> modeLabel;
  std::unique_ptr<SkiaLabel> statusLabel;
  std::unique_ptr<SkiaButton> clearButton;
  std::unique_ptr<SkiaButton> settingsButton;

  //==========================================================================
  // Backend

  CommandAPI &commandAPI;
  Engine &engine_;
  std::unique_ptr<AIBridgeClient> aiClient_; // Replaced GrokDAWController

  // ... state ...

  //==========================================================================
  // State

  bool isProcessing = false;
  GrokMode currentMode = GrokMode::Fast;

  // Sample Hunter state
  bool isSampleSearchActive_ = false;
  std::vector<ai::FoundSample> lastSearchResults_;
  juce::String lastSearchQuery_;

  // Thread safety: shutdown flag for async callbacks
  std::shared_ptr<std::atomic<bool>> isShuttingDown_ =
      std::make_shared<std::atomic<bool>>(false);

  //==========================================================================
  // Methods

  void sendCommand();
  void appendToConversation(const juce::String &speaker,
                            const juce::String &message);
  void setStatus(const juce::String &status,
                 SkColor colour = 0xFFFFFFFF);
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
