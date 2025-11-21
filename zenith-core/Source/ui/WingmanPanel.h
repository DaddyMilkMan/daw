/*
  ==============================================================================

    WingmanPanel.h
    Created: 2025-11-14
    Author:  Zenith DAW - Phase 7: Wingman AI Integration

    In-DAW command console for AI-driven workflow

    Responsibilities:
    - Display command history (input + output)
    - Text input field for JSON commands or shorthand or natural language
    - Parse shorthand commands to JSON
    - Execute commands via CommandAPI (Command mode)
    - Send natural language to AI bridge server (AI mode)
    - Handle AI responses with yes/no confirmation workflow
    - Show results in message history

    Modes:
    - Command mode: Manual JSON or shorthand execution
    - AI mode: Natural language → AI → command batch → confirmation

    Shorthand examples:
    - "tracks" → list_tracks
    - "create audio Guitar" → create_track type:audio name:Guitar
    - "volume track_0 -6" → set_track_volume trackId:track_0 volumeDb:-6
    - "graph" → get_session_graph

    AI mode examples:
    - "make a 4 bar drum loop"
    - "sidechain the bass to the kick"
    - "create vocals track and add reverb"

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <memory>

// Conditional Skia components
#ifdef ZENITH_USE_SKIA
    #include "skia/SkiaButtonComponent.h"
#endif

namespace zenith {
    class CommandAPI;
    class AIBridgeClient;
}

//==============================================================================
/**
    In-DAW Wingman command console component with AI integration
*/
class WingmanPanel : public juce::Component,
                     public juce::Timer,
                     private juce::TextEditor::Listener,
                     private juce::ChangeListener
{
public:
    //==============================================================================
    WingmanPanel(zenith::CommandAPI& api, zenith::AIBridgeClient& aiClient);
    ~WingmanPanel() override;

    //==============================================================================
    // Component interface
    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    //==============================================================================
    // Command execution
    void executeCommand(const juce::String& input);
    void clearHistory();

private:
    //==============================================================================
    // TextEditor::Listener
    void textEditorReturnKeyPressed(juce::TextEditor& editor) override;

    //==============================================================================
    // ChangeListener (for AI responses)
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    //==============================================================================
    // Mode management
    enum class Mode { Command, AI };

    void setMode(Mode newMode) [[maybe_unused]];
    Mode getCurrentMode() const { return currentMode; }

    //==============================================================================
    // AI mode handlers
    void executeAIRequest(const juce::String& naturalLanguage);
    void handleAIResponse();
    void showAIPlan(const juce::String& thought, const juce::Array<juce::var>& commands);
    void executePendingBatch();
    void cancelPendingBatch();

    //==============================================================================
    // Shorthand parser
    juce::String parseShorthand(const juce::String& input);

    //==============================================================================
    // UI helpers
    void addMessage(const juce::String& message, bool isUserInput) [[maybe_unused]];
    void scrollHistoryToBottom();

    //==============================================================================
    // Member variables

    zenith::CommandAPI& commandAPI;
    zenith::AIBridgeClient& aiBridgeClient;

    // UI components
    std::unique_ptr<juce::TextEditor> commandInput;
    std::unique_ptr<juce::TextEditor> historyDisplay;

#ifdef ZENITH_USE_SKIA
    // Skia GPU-rendered buttons with spring physics
    std::unique_ptr<zenith::SkiaButtonComponent> clearButton;
    std::unique_ptr<zenith::SkiaButtonComponent> commandModeButton;
    std::unique_ptr<zenith::SkiaButtonComponent> aiModeButton;
#else
    // Fallback JUCE buttons
    std::unique_ptr<juce::TextButton> clearButton;
    std::unique_ptr<juce::TextButton> commandModeButton;
    std::unique_ptr<juce::TextButton> aiModeButton;
#endif

    // Message history
    juce::StringArray messageHistory;

    // Mode state
    Mode currentMode{Mode::Command};

    // Pending batch state
    bool hasPendingBatch{false};
    juce::Array<juce::var> pendingCommands;
    juce::String pendingRequestId;

    // Animation state
    float inputFocusAnim{0.0f};
    bool inputHasFocus{false};
    bool isWaitingForAI{false};
    float typingIndicatorPhase{0.0f};

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WingmanPanel)
};

