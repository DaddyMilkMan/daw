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

#include <juce_gui_basics/juce_gui_basics.h>
#include "CommandAPI.h"
#include "../network/AIBridgeClient.h"
#include "../network/GrokDAWController.h"
#include "../../include/Engine.h"

namespace zenith {

/**
    Wingman AI Assistant Panel
    
    Provides a chat-like interface for controlling the DAW with natural language
*/
class WingmanPanel : public juce::Component,
                     private juce::TextEditor::Listener,
                     private juce::Button::Listener
{
public:
    //==========================================================================
    WingmanPanel(CommandAPI& api, AIBridgeClient& client, Engine& engine);
    ~WingmanPanel() override;
    
    //==========================================================================
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    //==========================================================================
    /**
        Initialize Grok integration
        
        @param apiKey Grok API key (optional, retrieves from SecureKeyStore if empty)
        @return true if initialized successfully
    */
    bool initializeGrok(const juce::String& apiKey = juce::String());
    
    /**
        Check if Grok is ready
    */
    bool isGrokReady() const;
    
private:
    //==========================================================================
    // TextEditor::Listener
    void textEditorReturnKeyPressed(juce::TextEditor& editor) override;
    
    // Button::Listener
    void buttonClicked(juce::Button* button) override;
    
    //==========================================================================
    // UI Components
    
    std::unique_ptr<juce::TextEditor> inputField;
    std::unique_ptr<juce::TextEditor> conversationDisplay;
    std::unique_ptr<juce::TextButton> sendButton;
    std::unique_ptr<juce::ComboBox> modeSelector;
    std::unique_ptr<juce::Label> modeLabel;
    std::unique_ptr<juce::Label> statusLabel;
    std::unique_ptr<juce::TextButton> clearButton;
    std::unique_ptr<juce::TextButton> settingsButton;
    
    //==========================================================================
    // Backend
    
    CommandAPI& commandAPI;
    AIBridgeClient& aiBridgeClient;
    Engine& engine_;
    std::unique_ptr<GrokDAWController> grokController;
    
    //==========================================================================
    // State
    
    bool isProcessing = false;
    GrokMode currentMode = GrokMode::Fast;
    
    //==========================================================================
    // Methods
    
    void sendCommand();
    void appendToConversation(const juce::String& speaker, const juce::String& message);
    void setStatus(const juce::String& status, juce::Colour colour = juce::Colours::white);
    void updateModeFromSelector();
    void showSettings();
    
    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WingmanPanel)
};

} // namespace zenith
